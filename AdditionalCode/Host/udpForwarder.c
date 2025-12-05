#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define LISTEN_PORT 5006               // host listens here for Pi packets
#define BEAGLE_IP   "192.168.7.2"      // where to forward packets
#define BEAGLE_PORT 5006

int main(void)
{
    int sockfd;
    struct sockaddr_in listen_addr, recv_addr, beagle_addr;
    socklen_t addr_len = sizeof(recv_addr);
    char buf[1024];

    // create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // bind to all interfaces on LISTEN_PORT
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(LISTEN_PORT);

    if (bind(sockfd, (struct sockaddr *)&listen_addr,
             sizeof(listen_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    // prepare Beagle address
    memset(&beagle_addr, 0, sizeof(beagle_addr));
    beagle_addr.sin_family = AF_INET;
    beagle_addr.sin_port = htons(BEAGLE_PORT);
    inet_pton(AF_INET, BEAGLE_IP, &beagle_addr.sin_addr);

    printf("UDP forwarder running...\n");

    for (;;) {
        // receive packet from Pi
        ssize_t len = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                               (struct sockaddr *)&recv_addr, &addr_len);
        if (len < 0) {
            perror("recvfrom");
            continue;
        }

        buf[len] = '\0'; // null-terminate

        // Build JSON
        char json[1500];
        snprintf(json, sizeof(json),
                 "{\"ip\":\"%s\",\"port\":%d,\"data\":\"%s\"}",
                 inet_ntoa(recv_addr.sin_addr),
                 ntohs(recv_addr.sin_port),
                 buf);

        printf("Forwarding: %s\n", json);

        // Send JSON to Beagle
        if (sendto(sockfd, json, strlen(json), 0,
                   (struct sockaddr *)&beagle_addr,
                   sizeof(beagle_addr)) < 0) {
            perror("sendto");
        }
    }

    close(sockfd);
    return 0;
}
