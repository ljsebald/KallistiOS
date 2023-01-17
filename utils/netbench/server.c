/* KallistiOS ##version##

   utils/netbench/server.c
   Copyright (C) 2023 Lawrence Sebald
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>

#define BUFSZ (10 * 1024 * 1024)
#define PORT 1337

int main(int argc, char *argv[]) {
    int sock, s;
    void *buf;
    uint32_t *b32;
    int i;
    struct sockaddr_in addr;
    ssize_t len;

    /* Give ourselves a nice 10MiB buffer to work with... */
    buf = mmap(NULL, BUFSZ, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE,
               -1, 0);
    if(buf == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    /* Initialize it... */
    b32 = (uint32_t *)buf;
    for(i = 0; i < BUFSZ / 4; ++i) {
        *b32++ = (uint32_t)i;
    }

    /* Set up a socket and wait for a connection */
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(sock < 0) {
        perror("socket");
        munmap(buf, BUFSZ);
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) {
        perror("bind");
        close(sock);
        munmap(buf, BUFSZ);
        exit(EXIT_FAILURE);
    }

    if(listen(sock, 10)) {
        perror("listen");
        close(sock);
        munmap(buf, BUFSZ);
        exit(EXIT_FAILURE);
    }

    while(1) {
        printf("Waiting for connection... (CTRL+C to exit)\n");
        if((s = accept(sock, NULL, NULL)) < 0) {
            perror("accept");
            continue;
        }

        /* Send the buffer. */
        i = 0;
        while(i < BUFSZ) {
            len = send(s, buf + i, BUFSZ - i, 0);
            if(len < 0) {
                perror("send");
                close(s);
                continue;
            }

            i += len;
        }

        /* We're done, close it and wait for another connection. */
        close(s);
    }

    close(sock);
    munmap(buf, BUFSZ);
    return 0;
}
