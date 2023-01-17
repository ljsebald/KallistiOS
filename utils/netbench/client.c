/* KallistiOS ##version##

   utils/netbench/client.c
   Copyright (C) 2023 Lawrence Sebald
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <kos.h>

#define SERVER "127.0.0.1"
#define PORT 1337
#define BLOCKSZ 1024

uint8_t buffer[BLOCKSZ];

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

int main(int argc, char *argv[]) {
    int s = -1;
    size_t total = 0;
    ssize_t len;
    struct timeval start, stop;
    uint64_t tt;
    float rate;
    char pfx = ' ';
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(SERVER);

    if((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Start timing. The loop will likely exit on the first pass... */
    printf("Opening connection...\n");
    memset(&stop, 0, sizeof(struct timeval));
    gettimeofday(&start, NULL);

    if(connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
        perror("connect");
        close(s);
        exit(EXIT_FAILURE);
    }

    if(s < 0) {
        printf("Could not connect to server. Is the server running?\n");
        exit(EXIT_FAILURE);
    }

    for(;;) {
        len = recv(s, buffer, BLOCKSZ, 0);
        if(len < 0) {
            if(errno == ECONNRESET) {
                break;
            }

            perror("recv");
            break;
        }
        else if(len == 0) {
            gettimeofday(&stop, NULL);
            printf("Connection closed\n");
            break;
        }

        total += (size_t)len;
    }

    if(stop.tv_sec != 0 || stop.tv_usec != 0) {
        tt = (stop.tv_sec - start.tv_sec) * 1000;
        tt += (stop.tv_usec - start.tv_usec) / 1000;
        printf("%zu bytes received in %" PRIu64 " milliseconds\n", total, tt);

        rate = total / (((float)tt) / 1000);
        if(rate > 1000) {
            rate /= 1000.0f;
            pfx = 'k';
        }
        if(rate > 1000) {
            rate /= 1000.0f;
            pfx = 'M';
        }
        if(rate > 1000) {
            rate /= 1000.0f;
            pfx = 'G';
        }

        printf("Rate: %.5f %cBPS\n", rate, pfx);
    }

    close(s);
    return 0;
}
