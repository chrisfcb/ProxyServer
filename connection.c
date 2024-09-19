/*
 * This is a file that implements the operation on TCP sockets that are used by
 * all of the programs used in this assignment.
 *
 * *** YOU MUST IMPLEMENT THESE FUNCTIONS ***
 *
 * The parameters and return values of the existing functions must not be changed.
 * You can add function, definition etc. as required.
 */
#include "connection.h"

void check_error(int res, char *msg) {
    if (res == -1) {
        perror(msg);
        exit(-1);
    }
}

int tcp_connect( char* hostname, int port )
{
    int fd, wc;
    struct in_addr ip_adress;
    struct sockaddr_in dest_addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    check_error(fd, "socket");

    wc = inet_pton(AF_INET, hostname, &ip_adress);
    if (!wc) {
        fprintf(stderr, "Invalid network adress\n");
        return -1;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr = ip_adress;

    wc = connect(fd, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr_in));
    check_error(wc, "connect");

    return fd;
}

int tcp_read( int sock, char* buffer, int n )
{
    int rc;

    rc = recv(sock, buffer, n, 0);
    check_error(rc, "read");
    if (!rc) {
        fprintf(stderr, "Socket is closed by the other side\n");
        return 0;
    }

    return rc;
}

int tcp_write( int sock, char* buffer, int bytes )
{
    int wc;

    wc = send(sock, buffer, bytes, 0);
    check_error(wc, "send");

    return wc;
}

int tcp_write_loop( int sock, char* buffer, int bytes )
{
    int bytes_written, wc;
    bytes_written = 0;

    while(bytes_written < bytes) {
        wc = tcp_write(sock, buffer + bytes_written, bytes - bytes_written);
        bytes_written += wc;
    }

    return bytes_written;
}

void tcp_close( int sock )
{
    int wc;

    wc = close(sock);
    check_error(wc, "close");
}

int tcp_create_and_listen( int port )
{
    int moteplass_fd, rc;
    struct sockaddr_in my_addr;

    moteplass_fd = socket(AF_INET, SOCK_STREAM, 0);

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    rc = bind(moteplass_fd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr_in));
    check_error(rc, "bind");

    rc = listen(moteplass_fd, 3);
    check_error(rc, "listen");

    return moteplass_fd;
}

int tcp_accept( int server_sock )
{
    int com_fd;

    com_fd = accept(server_sock, NULL, NULL);
    check_error(com_fd, "accept");

    return com_fd;
}

int tcp_wait( fd_set* waiting_set, int wait_end )
{
    int result;
    fd_set read_set;

    while(1) {
        memcpy(&read_set, waiting_set, sizeof(fd_set));

        result = select(wait_end, &read_set, NULL, NULL, NULL);
        check_error(result, "select");

        for (int i = 0; i < wait_end; i++) {
            if (FD_ISSET(i, &read_set)) {
                return i;
            }
        }
    }
}

int tcp_wait_timeout( fd_set* waiting_set, int wait_end, int seconds )
{

    struct timeval tv;

    tv.tv_sec = seconds;
    tv.tv_usec = 0;

    int result;
    fd_set read_set;

    memcpy(&read_set, waiting_set, sizeof(fd_set));

    result = select(wait_end, &read_set, NULL, NULL, &tv);
    check_error(result, "select");

    for (int i = 0; i < wait_end; i++) {
        if (FD_ISSET(i, &read_set)) {
            return i;
        }
    }

    return 0;
}

