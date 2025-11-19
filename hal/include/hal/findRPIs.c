// discover_client.c
// Compile with: gcc discover_client.c -o discover_client
// Run: ./discover_client

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <time.h>

char HOTSPOTIP[] = "10.83.189.255";

char *player1address = "X";
char *player2address = "X";

struct sockaddr_in p1addr, p2addr;

int sockfd;


int setup(void) 
{
    struct sockaddr_in broadcast_addr, recv_addr;
    socklen_t addr_len;
    int broadcast_enable = 1;
    char send_msg[] = "finding_rpis";
    char recv_buf[1024];

    // 1. Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }
    
    // 2. Enable broadcast
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
        &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("setsockopt(SO_BROADCAST)");
        close(sockfd);
        return 1;
    }
    
    // 3. Set a receive timeout (e.g. 2 seconds)
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   &tv, sizeof(tv)) < 0) {
        perror("setsockopt(SO_RCVTIMEO)");
        close(sockfd);
        return 1;
    }
    
    // 4. Setup broadcast address (255.255.255.255:5006)
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(5006);
    broadcast_addr.sin_addr.s_addr = inet_addr(HOTSPOTIP);
    
    // 5. Send discovery message
    ssize_t sent = sendto(sockfd,
                          send_msg,
                          strlen(send_msg),
                          0,
                          (struct sockaddr *)&broadcast_addr,
                          sizeof(broadcast_addr));

    if (sent < 0) {
        perror("sendto");
        close(sockfd);
        return 1;
    }

    printf("Sent discovery message: \"%s\"\n", send_msg);
    
    for(;;)
    {
        // 6. Receive response from server
        addr_len = sizeof(recv_addr);
        ssize_t recv_len = recvfrom(sockfd,
            recv_buf,
            sizeof(recv_buf) - 1,
            0,
            (struct sockaddr *)&recv_addr,
            &addr_len);
            
            if (recv_len < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            } else {
                perror("recvfrom");
                close(sockfd);
                return 1;
            }
        }
        if (recv_len != 3 || memcmp(recv_buf, "rpi", 3) != 0)
        {
            continue;
        }
    
        // IP of the server is the source address of this packet
        char *server_ip = inet_ntoa(recv_addr.sin_addr);
        printf("Got response from %s\n", server_ip);
        if (player1address == "X")
        {
            player1address = server_ip;
            break; //remove this
        }
        else
        {
            player2address = server_ip;
            break;
        }
    }

    // Set addresses for commands
    memset(&p1addr, 0, sizeof(p1addr));
    p1addr.sin_family = AF_INET;
    p1addr.sin_port = htons(5006);
    p1addr.sin_addr.s_addr = inet_addr(player1address);
    
    // memset(&p2addr, 0, sizeof(p2addr));
    // p2addr.sin_family = AF_INET;
    // p2addr.sin_port = htons(5006);
    // p2addr.sin_addr.s_addr = inet_addr(player2address);

    return 0;
}

struct sockaddr_in getAddr(int player)
{
    if (player == 1)
    {
        return p1addr;
    }
    else
    {
        return p2addr;
    }
}

void moveForward(int player)
{
    struct sockaddr_in ad = getAddr(player);
    ssize_t sent2 = sendto(sockfd,
        "forward",
        7,
        0,
        (struct sockaddr *)&ad,
        sizeof(ad));
}

void moveBack(int player)
{
    struct sockaddr_in ad = getAddr(player);
    ssize_t sent2 = sendto(sockfd,
        "back",
        4,
        0,
        (struct sockaddr *)&ad,
        sizeof(ad));
}

void moveStop(int player)
{
    struct sockaddr_in ad = getAddr(player);
    ssize_t sent2 = sendto(sockfd,
        "stop",
        4,
        0,
        (struct sockaddr *)&ad,
        sizeof(ad));
}

void armUp(int player)
{
    struct sockaddr_in ad = getAddr(player);
    ssize_t sent2 = sendto(sockfd,
        "armup",
        5,
        0,
        (struct sockaddr *)&ad,
        sizeof(ad));
}

void armDown(int player)
{
    struct sockaddr_in ad = getAddr(player);
    ssize_t sent2 = sendto(sockfd,
        "armdown",
        7,
        0,
        (struct sockaddr *)&ad,
        sizeof(ad));
}

void armStop(int player)
{
    struct sockaddr_in ad = getAddr(player);
    ssize_t sent2 = sendto(sockfd,
        "armstop",
        7,
        0,
        (struct sockaddr *)&ad,
        sizeof(ad));
}

void sleep_ms(int ms)
{
    struct timespec request = {.tv_sec = ms/1000, .tv_nsec = (ms % 1000) * 1000000L};
    struct timespec remaining;
    nanosleep(&request, &remaining);
}

int main(void)
{
    setup();
    sleep_ms(1000);
    printf("Testing wheels...\n");
    sleep_ms(1000);
    moveForward(1);
    sleep_ms(1000);
    moveBack(1);
    sleep_ms(1000);
    moveStop(1);
    sleep_ms(1000);
    printf("Testing arm...\n");
    sleep_ms(1000);
    armUp(1);
    sleep_ms(500);
    armStop(1);
    sleep_ms(500);
    armDown(1);
    sleep_ms(500);
    armStop(1);

    close(sockfd);
    return 0;
}