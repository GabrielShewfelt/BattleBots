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

#define LISTEN_PORT 5006
#define HOTSPOT_BCAST "10.83.189.255"   // broadcast address you used originally
#define BUFSZ 2048
#define INET_STRLEN 64

// stored player info (strings for debug, and sockaddr_in for sends)
char player1_ip[INET_STRLEN] = "X";
char player2_ip[INET_STRLEN] = "X";
int  player1_port = 0;
int  player2_port = 0;

struct sockaddr_in p1addr, p2addr;
int sockfd = -1;

/* Minimal JSON parsing helpers:
   Expect input like: {"ip":"10.83.189.250","port":60211,"data":"finding_rpis"}
   We do simple substring searches and copy the values. */

static int extract_json_str(const char *json, const char *key, char *out, size_t outlen)
{
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

static int extract_json_int(const char *json, const char *key, int *out)
{
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    char *p = strstr(json, pattern);
    if (!p) return -1;
    p += strlen(pattern);
    // parse integer
    char *end;
    long val = strtol(p, &end, 10);
    if (p == end) return -1;
    *out = (int)val;
    return 0;
}

/* Helper: compare an IP string to stored player IPs.
   Returns 1 for player1, 2 for player2, 0 for no match. */
static int which_player_by_ip(const char *ipstr)
{
    if (ipstr == NULL) return 0;
    if (player1_ip[0] != 'X' && strcmp(player1_ip, ipstr) == 0) return 1;
    if (player2_ip[0] != 'X' && strcmp(player2_ip, ipstr) == 0) return 2;

    /* numeric fallback in case formatting differs */
    struct in_addr a_recv, a_p1, a_p2;
    if (inet_pton(AF_INET, ipstr, &a_recv) == 1) 
    {
        if (player1_ip[0] != 'X' && inet_pton(AF_INET, player1_ip, &a_p1) == 1 &&
            a_recv.s_addr == a_p1.s_addr) return 1;
        if (player2_ip[0] != 'X' && inet_pton(AF_INET, player2_ip, &a_p2) == 1 &&
            a_recv.s_addr == a_p2.s_addr) return 2;
    }
    return 0;
}

/* Thread function: listens for forwarded JSON and prints when data == "hit" */
static void *hit_listener_thread(void *arg)
{
    (void)arg;
    char buf[BUFSZ];
    struct sockaddr_in recv_addr;
    socklen_t addr_len = sizeof(recv_addr);

    while (1) 
    {
        ssize_t len = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                               (struct sockaddr *)&recv_addr, &addr_len);
        if (len < 0) 
        {
            if (errno == EINTR) continue;
            // If socket becomes non-blocking elsewhere, handle EAGAIN/EWOULDBLOCK here
            perror("hit_listener recvfrom");
            // small sleep to avoid spinning on persistent errors
            sleep(1);
            continue;
        }
        buf[len] = '\0';

        // parse data field
        char datastr[BUFSZ] = {0};
        if (extract_json_str(buf, "data", datastr, sizeof(datastr)) < 0) 
        {
            // not JSON or no data field; ignore
            continue;
        }

        if (strcmp(datastr, "hit") == 0) 
        {
            /* Extract ip from JSON if present; otherwise fall back to UDP src address */
            char json_ip[INET_STRLEN] = {0};
            int got_ip = (extract_json_str(buf, "ip", json_ip, sizeof(json_ip)) == 0);

            char src_ip[INET_STRLEN] = {0};
            if (got_ip) 
            {
                strncpy(src_ip, json_ip, INET_STRLEN - 1);
            } 
            else 
            {
                inet_ntop(AF_INET, &recv_addr.sin_addr, src_ip, sizeof(src_ip));
            }

            int player = which_player_by_ip(src_ip);
            if (player == 1) 
            {
                printf(">>> HIT from PLAYER 1 (ip=%s)\n", src_ip);
                //Call function here
            } 
            else if (player == 2) 
            {
                printf(">>> HIT from PLAYER 2 (ip=%s)\n", src_ip);
                //Call function here
            } 
            else 
            {
                printf(">>> HIT from UNKNOWN IP %s\n", src_ip);
            }

            // continue listening for more hits
        }
        // otherwise ignore other messages in this thread
    }
    return NULL;
}

int setup_and_discover(void)
{
    struct sockaddr_in send_bcast_addr, listen_addr, recv_addr;
    socklen_t addr_len = sizeof(recv_addr);
    char recvbuf[BUFSZ];
    int broadcast_enable = 1;

    // create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        perror("socket");
        return -1;
    }

    // allow immediate reuse
    int on = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) 
    {
        perror("setsockopt(SO_REUSEADDR)");
        // not fatal
    }

    // enable broadcast for sending discovery
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable,
                   sizeof(broadcast_enable)) < 0) 
    {
        perror("setsockopt(SO_BROADCAST)");
        close(sockfd);
        return -1;
    }

    // bind socket so we can receive replies on LISTEN_PORT
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY; // listen on all interfaces
    listen_addr.sin_port = htons(LISTEN_PORT);

    if (bind(sockfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) 
    {
        perror("bind");
        close(sockfd);
        return -1;
    }

    // prepare broadcast address to send discovery to
    memset(&send_bcast_addr, 0, sizeof(send_bcast_addr));
    send_bcast_addr.sin_family = AF_INET;
    send_bcast_addr.sin_port = htons(LISTEN_PORT);
    if (inet_pton(AF_INET, HOTSPOT_BCAST, &send_bcast_addr.sin_addr) != 1) 
    {
        fprintf(stderr, "invalid HOTSPOT_BCAST address\n");
        close(sockfd);
        return -1;
    }

    // send discovery packet (single shot)
    const char *send_msg = "finding_rpis";
    ssize_t sent = sendto(sockfd, send_msg, strlen(send_msg), 0,
                          (struct sockaddr *)&send_bcast_addr, sizeof(send_bcast_addr));
    if (sent < 0) 
    {
        perror("sendto discovery");
        // continue anyway; maybe host forwarder will see other traffic
    } 
    else 
    {
        printf("Sent discovery message: \"%s\" to %s:%d\n", send_msg,
               HOTSPOT_BCAST, LISTEN_PORT);
    }

    // optionally set a total discovery timeout (e.g., 15 seconds)
    const time_t DISCOVER_TIMEOUT = 15;
    time_t start = time(NULL);

    int players_found = 0;
    while (players_found < 2) 
    {
        // optional overall timeout
        if (DISCOVER_TIMEOUT > 0 && (time(NULL) - start) > DISCOVER_TIMEOUT) 
        {
            printf("Discovery timed out after %ld seconds; players found: %d\n",
                   (long)DISCOVER_TIMEOUT, players_found);
            break;
        }

        // receive forwarded JSON from host
        ssize_t len = recvfrom(sockfd, recvbuf, sizeof(recvbuf) - 1, 0,
                               (struct sockaddr *)&recv_addr, &addr_len);
        if (len < 0) 
        {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            perror("recvfrom");
            continue;
        }
        recvbuf[len] = '\0';

        // parse JSON: ip, port, data
        char ipstr[INET_STRLEN] = {0};
        char datastr[BUFSZ] = {0};
        int port = 0;
        int got_ip = (extract_json_str(recvbuf, "ip", ipstr, sizeof(ipstr)) == 0);
        int got_port = (extract_json_int(recvbuf, "port", &port) == 0);
        int got_data = (extract_json_str(recvbuf, "data", datastr, sizeof(datastr)) == 0);

        if (!got_ip || !got_port) 
        {
            fprintf(stderr, "received invalid JSON (missing ip/port): %s\n", recvbuf);
            continue;
        }

        if (!got_data) datastr[0] = '\0';

        printf("Received forwarded JSON: ip=%s port=%d data=\"%s\"\n",
               ipstr, port, datastr);

        // only consider discovery messages for assignment
        if (strcmp(datastr, "rpi") != 0) 
        {
            printf("Ignoring non-discovery data: \"%s\"\n", datastr);
            continue;
        }

        // assign players in order, avoid duplicates
        if (players_found == 0) 
        {
            strncpy(player1_ip, ipstr, INET_STRLEN - 1);
            player1_port = port;
            memset(&p1addr, 0, sizeof(p1addr));
            p1addr.sin_family = AF_INET;
            p1addr.sin_port = htons(player1_port);
            if (inet_pton(AF_INET, player1_ip, &p1addr.sin_addr) != 1) 
            {
                fprintf(stderr, "inet_pton failed for %s\n", player1_ip);
                player1_ip[0] = 'X'; player1_port = 0;
                continue;
            }
            players_found++;
            printf("Assigned PLAYER 1 -> %s:%d\n", player1_ip, player1_port);
            continue;
        }

        if (players_found == 1) {
            if (strcmp(player1_ip, ipstr) == 0 && player1_port == port) 
            {
                printf("Duplicate discovery from same client %s:%d, ignoring\n", ipstr, port);
                continue;
            }
            strncpy(player2_ip, ipstr, INET_STRLEN - 1);
            player2_port = port;
            memset(&p2addr, 0, sizeof(p2addr));
            p2addr.sin_family = AF_INET;
            p2addr.sin_port = htons(player2_port);
            if (inet_pton(AF_INET, player2_ip, &p2addr.sin_addr) != 1) 
            {
                fprintf(stderr, "inet_pton failed for %s\n", player2_ip);
                player2_ip[0] = 'X'; player2_port = 0;
                continue;
            }
            players_found++;
            printf("Assigned PLAYER 2 -> %s:%d\n", player2_ip, player2_port);
            break;
        }
    }

    // start hit-listener thread (detached)
    pthread_t thr;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thr, &attr, hit_listener_thread, NULL) != 0) 
    {
        perror("pthread_create");
        // continue without thread
    } 
    else 
    {
        printf("Hit listener thread started (waiting for forwarded 'hit')\n");
    }
    pthread_attr_destroy(&attr);

    return 0;
}

// helper to get address for send functions
struct sockaddr_in getAddr(int player)
{
    if (player == 1) return p1addr;
    return p2addr;
}

// command senders
void send_command_to_player(int player, const char *cmd)
{
    struct sockaddr_in ad = getAddr(player);
    ssize_t sent = sendto(sockfd, cmd, strlen(cmd), 0,
                          (struct sockaddr *)&ad, sizeof(ad));
    if (sent < 0) perror("sendto");
    else printf("Sent '%s' to player %d\n", cmd, player);
}

void moveForward(int player) { send_command_to_player(player, "forward"); }
void moveBack(int player)    { send_command_to_player(player, "back"); }
void moveStop(int player)    { send_command_to_player(player, "stop"); }
void armUp(int player)       { send_command_to_player(player, "armup"); }
void armDown(int player)     { send_command_to_player(player, "armdown"); }
void armStop(int player)     { send_command_to_player(player, "armstop"); }

void sleep_ms(int ms)
{
    struct timespec request = {.tv_sec = ms/1000, .tv_nsec = (ms % 1000) * 1000000L};
    nanosleep(&request, NULL);
}


//Main to test
int main(void)
{
    if (setup_and_discover() != 0) 
    {
        fprintf(stderr, "Discovery failed\n");
        return 1;
    }

    sleep_ms(500);
    printf("Testing wheels (player1)...\n");
    sleep_ms(500);
    moveForward(1);
    moveForward(2);
    sleep_ms(800);
    moveBack(1);
    moveBack(2);
    sleep_ms(800);
    moveStop(1);
    moveStop(2);

    sleep_ms(500);
    printf("Testing arm (player1)...\n");
    armUp(1);
    armUp(2);
    sleep_ms(500);
    armStop(1);
    armStop(2);
    sleep_ms(500);
    armDown(1);
    armDown(2);
    sleep_ms(500);
    armStop(1);
    armStop(2);

    // Keep running so hit listener can operate
    printf("Main thread sleeping; hit listener still active. Ctrl-C to quit.\n");
    while (1) sleep(60);

    close(sockfd);
    return 0;
}
