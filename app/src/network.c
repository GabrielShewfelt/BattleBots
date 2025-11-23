#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>      // Needed for non-blocking flags
#include <sys/time.h>   // Needed for struct timeval

#include "network.h"

// Define broadcast IP from your findRPIs.c
#define BROADCAST_IP "10.83.189.255" 
#define PORT 5006

static int sockfd = -1;
static struct sockaddr_in p1_addr;
static struct sockaddr_in p2_addr;
static int p1_connected = 0;
static int p2_connected = 0;

// Helper to set address structure
static void set_addr(struct sockaddr_in *addr, const char *ip) {
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(PORT);
    addr->sin_addr.s_addr = inet_addr(ip);
}

int network_init() {
    struct sockaddr_in broadcast_addr;
    int broadcast_enable = 1;
    char send_msg[] = "finding_rpis";
    char recv_buf[1024];
    struct sockaddr_in recv_addr;
    socklen_t addr_len;

    // 1. Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return -1;
    }

    // 2. Enable broadcast
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Error enabling broadcast");
        return -1;
    }

    // 3. Set Receive Timeout (2 seconds)
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error setting timeout");
    }

    // 4. Send Discovery Packet
    set_addr(&broadcast_addr, BROADCAST_IP);
    
    printf("[Network] Broadcasting discovery packet to %s...\n", BROADCAST_IP);
    ssize_t sent = sendto(sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
    if (sent < 0) {
        perror("Discovery send failed");
        return -1;
    }

    // 5. Wait for both Pis to respond
    printf("[Network] Waiting for Raspberry Pis...\n");
    
    while (!p1_connected || !p2_connected) {
        addr_len = sizeof(recv_addr);
        ssize_t recv_len = recvfrom(sockfd, recv_buf, sizeof(recv_buf) - 1, 0, (struct sockaddr *)&recv_addr, &addr_len);

        if (recv_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("[Network] Timeout waiting for Pis. Retrying broadcast...\n");
                sendto(sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
                continue;
            } else {
                perror("recvfrom error");
                continue; 
            }
        }

        recv_buf[recv_len] = '\0'; // Null terminate

        // Check if it's a valid handshake response
        if (strncmp(recv_buf, "rpi", 3) == 0) {
            char *ip = inet_ntoa(recv_addr.sin_addr);
            
            if (!p1_connected) {
                printf("[Network] Found Player 1 at %s\n", ip);
                p1_addr = recv_addr;
                p1_connected = 1;
            } else if (!p2_connected) {
                // Ensure we found a DIFFERENT IP for player 2
                if (recv_addr.sin_addr.s_addr != p1_addr.sin_addr.s_addr) {
                    printf("[Network] Found Player 2 at %s\n", ip);
                    p2_addr = recv_addr;
                    p2_connected = 1;
                }
            }
        }
    }

    printf("[Network] Both carts connected!\n");

    // 6. Set socket to Non-Blocking for the main game loop
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    return 0;
}

void network_send_command_to_bot(int index, int drive, int swing) {
    struct sockaddr_in target;
    if (index == 1) target = p1_addr;
    else if (index == 2) target = p2_addr;
    else return;

    // --- DRIVE COMMANDS ---
    const char* drive_cmd = "stop"; 
    if (drive == 1) drive_cmd = "forward";
    else if (drive == -1) drive_cmd = "back";

    sendto(sockfd, drive_cmd, strlen(drive_cmd), 0, (struct sockaddr *)&target, sizeof(target));

    // --- SWING COMMANDS ---
    const char* swing_cmd = "armstop";
    if (swing == 1) swing_cmd = "armup";
    else if (swing == -1) swing_cmd = "armdown";

    sendto(sockfd, swing_cmd, strlen(swing_cmd), 0, (struct sockaddr *)&target, sizeof(target));
}

int network_poll_hit_event(HitEvent *out) {
    char buf[128];
    struct sockaddr_in sender;
    socklen_t len = sizeof(sender);

    // Try to receive data (Non-blocking)
    ssize_t n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&sender, &len);

    if (n > 0) {
        buf[n] = '\0';
        
        // Check for "hit" message
        if (strncmp(buf, "hit", 3) == 0) {
            // Identify which bot sent the message
            // Since the bot reports its OWN hit, the sender IS the victim.
            if (sender.sin_addr.s_addr == p1_addr.sin_addr.s_addr) {
                out->bot_id_hit = 1; 
                return 1;
            } else if (sender.sin_addr.s_addr == p2_addr.sin_addr.s_addr) {
                out->bot_id_hit = 2;
                return 1;
            }
        }
    }

    return 0; // No event
}