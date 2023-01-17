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

#define SERVER "localhost"
#define PORT "1337"
#define BLOCKSZ 1024

uint8_t buffer[BLOCKSZ];

int main(int argc, char *argv[]) {
    int s = -1;
    struct addrinfo hints, *server, *it;
    size_t total = 0;
    ssize_t len;
    struct timeval start, stop;
    uint64_t tt;
    float rate;
    char pfx = ' ';

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if(getaddrinfo(SERVER, PORT, &hints, &server)) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    /* Start timing. The loop will likely exit on the first pass... */
    printf("Opening connection...\n");
    memset(&stop, 0, sizeof(struct timeval));
    gettimeofday(&start, NULL);

    for(it = server; it && s < 0; it = it->ai_next) {
        if((s = socket(it->ai_family, it->ai_socktype, it->ai_protocol)) < 0) {
            perror("socket");
            continue;
        }

        if(connect(s, it->ai_addr, it->ai_addrlen) < 0) {
            perror("connect");
            close(s);
            s = -1;
        }
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
