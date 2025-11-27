#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include "network.h"

#define LISTEN_PORT 5006
#define HOTSPOT_BCAST "10.83.189.255"
#define BUFSZ 2048
#define INET_STRLEN 64

// --- Globals ---
static char player1_ip[INET_STRLEN] = "X";
static char player2_ip[INET_STRLEN] = "X";
static int  player1_port = 0;
static int  player2_port = 0;
static struct sockaddr_in p1addr, p2addr;
static int sockfd = -1;

// --- Thread-Safe Event Queue for Hits ---
// The listener thread pushes hits here; the main loop polls from here.
#define MAX_EVENTS 64
static HitEvent event_queue[MAX_EVENTS];
static int q_head = 0;
static int q_tail = 0;
static pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;

static void push_hit_event(int bot_id) {
    pthread_mutex_lock(&q_lock);
    int next = (q_head + 1) % MAX_EVENTS;
    if (next != q_tail) {
        event_queue[q_head].bot_id_hit = bot_id;
        q_head = next;
    }
    pthread_mutex_unlock(&q_lock);
}

// --- JSON Helpers ---
static int extract_json_str(const char *json, const char *key, char *out, size_t outlen) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    char *p = strstr(json, pattern);
    if (!p) return -1;
    p += strlen(pattern);
    char *q = strchr(p, '"');
    if (!q) return -1;
    size_t len = q - p;
    if (len >= outlen) len = outlen - 1;
    memcpy(out, p, len);
    out[len] = '\0';
    return 0;
}

static int extract_json_int(const char *json, const char *key, int *out) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    char *p = strstr(json, pattern);
    if (!p) return -1;
    p += strlen(pattern);
    char *end;
    long val = strtol(p, &end, 10);
    if (p == end) return -1;
    *out = (int)val;
    return 0;
}

static int which_player_by_ip(const char *ipstr) {
    if (ipstr == NULL) return 0;
    if (player1_ip[0] != 'X' && strcmp(player1_ip, ipstr) == 0) return 1;
    if (player2_ip[0] != 'X' && strcmp(player2_ip, ipstr) == 0) return 2;
    return 0;
}

// --- Listener Thread ---
// Runs in background to catch incoming UDP packets
static void *hit_listener_thread(void *arg) {
    (void)arg;
    char buf[BUFSZ];
    struct sockaddr_in recv_addr;
    socklen_t addr_len = sizeof(recv_addr);

    while (1) {
        ssize_t len = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                               (struct sockaddr *)&recv_addr, &addr_len);
        if (len < 0) {
            if (errno == EINTR) continue;
            perror("hit_listener recvfrom");
            sleep(1);
            continue;
        }
        buf[len] = '\0';

        char datastr[BUFSZ] = {0};
        if (extract_json_str(buf, "data", datastr, sizeof(datastr)) < 0) continue;

        if (strcmp(datastr, "hit") == 0) {
            char json_ip[INET_STRLEN] = {0};
            int got_ip = (extract_json_str(buf, "ip", json_ip, sizeof(json_ip)) == 0);

            char src_ip[INET_STRLEN] = {0};
            if (got_ip) strncpy(src_ip, json_ip, INET_STRLEN - 1);
            else inet_ntop(AF_INET, &recv_addr.sin_addr, src_ip, sizeof(src_ip));

            int player = which_player_by_ip(src_ip);
            if (player > 0) {
                printf(">>> HIT DETECTED from Player %d (%s)\n", player, src_ip);
                push_hit_event(player);
            }
        }
    }
    return NULL;
}

// --- Initialization ---
int network_init() {
    struct sockaddr_in send_bcast_addr, listen_addr, recv_addr;
    socklen_t addr_len = sizeof(recv_addr);
    char recvbuf[BUFSZ];
    int broadcast_enable = 1;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); return -1; }

    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("setsockopt(SO_BROADCAST)");
        close(sockfd);
        return -1;
    }

    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(LISTEN_PORT);

    if (bind(sockfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    // Prepare broadcast
    memset(&send_bcast_addr, 0, sizeof(send_bcast_addr));
    send_bcast_addr.sin_family = AF_INET;
    send_bcast_addr.sin_port = htons(LISTEN_PORT);
    inet_pton(AF_INET, HOTSPOT_BCAST, &send_bcast_addr.sin_addr);

    // Send discovery
    const char *send_msg = "finding_rpis";
    sendto(sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&send_bcast_addr, sizeof(send_bcast_addr));
    printf("[Network] Sent discovery to %s. Waiting for players...\n", HOTSPOT_BCAST);

    int players_found = 0;
    
    // Blocking loop until we find both bots
    while (players_found < 2) {
        ssize_t len = recvfrom(sockfd, recvbuf, sizeof(recvbuf) - 1, 0, (struct sockaddr *)&recv_addr, &addr_len);
        if (len < 0) continue;
        recvbuf[len] = '\0';

        char ipstr[INET_STRLEN] = {0};
        char datastr[BUFSZ] = {0};
        int port = 0;

        extract_json_str(recvbuf, "ip", ipstr, sizeof(ipstr));
        extract_json_int(recvbuf, "port", &port);
        extract_json_str(recvbuf, "data", datastr, sizeof(datastr));

        if (strcmp(datastr, "rpi") != 0) continue;

        if (players_found == 0) {
            strncpy(player1_ip, ipstr, INET_STRLEN - 1);
            player1_port = port;
            memset(&p1addr, 0, sizeof(p1addr));
            p1addr.sin_family = AF_INET;
            p1addr.sin_port = htons(player1_port);
            inet_pton(AF_INET, player1_ip, &p1addr.sin_addr);
            players_found++;
            printf("[Network] Player 1 found: %s:%d\n", player1_ip, player1_port);
        } else if (players_found == 1) {
            if (strcmp(player1_ip, ipstr) == 0) continue; // Ignore duplicate
            strncpy(player2_ip, ipstr, INET_STRLEN - 1);
            player2_port = port;
            memset(&p2addr, 0, sizeof(p2addr));
            p2addr.sin_family = AF_INET;
            p2addr.sin_port = htons(player2_port);
            inet_pton(AF_INET, player2_ip, &p2addr.sin_addr);
            players_found++;
            printf("[Network] Player 2 found: %s:%d\n", player2_ip, player2_port);
        }
    }

    // Start background listener thread for HIT events
    pthread_t thr;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thr, &attr, hit_listener_thread, NULL) == 0) {
        printf("[Network] Hit listener thread started.\n");
    }
    pthread_attr_destroy(&attr);

    return 0;
}

// --- Public API ---

void network_send_command_to_bot(int index, int drive, int swing) {
    struct sockaddr_in *target = (index == 1) ? &p1addr : &p2addr;
    if (index != 1 && index != 2) return;

    // Map Integers to Protocol Strings
    const char *cmd_move = (drive == 1) ? "forward" : (drive == -1) ? "back" : "stop";
    const char *cmd_arm = (swing == 1) ? "armup" : (swing == -1) ? "armdown" : "armstop";

    sendto(sockfd, cmd_move, strlen(cmd_move), 0, (struct sockaddr *)target, sizeof(*target));
    sendto(sockfd, cmd_arm, strlen(cmd_arm), 0, (struct sockaddr *)target, sizeof(*target));
}

int network_poll_hit_event(HitEvent *out) {
    pthread_mutex_lock(&q_lock);
    if (q_head == q_tail) {
        pthread_mutex_unlock(&q_lock);
        return 0; // Queue empty
    }
    *out = event_queue[q_tail];
    q_tail = (q_tail + 1) % MAX_EVENTS;
    pthread_mutex_unlock(&q_lock);
    return 1; // Event retrieved
}