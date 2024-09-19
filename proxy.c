/*
 * This is the main program for the proxy, which receives connections for sending and receiving clients
 * both in binary and XML format. Many clients can be connected at the same time. The proxy implements
 * an event loop.
 *
 * *** YOU MUST IMPLEMENT THESE FUNCTIONS ***
 *
 * The parameters and return values of the existing functions must not be changed.
 * You can add function, definition etc. as required.
 */
#include "xmlfile.h"
#include "connection.h"
#include "record.h"
#include "recordToFormat.h"
#include "recordFromFormat.h"

#include <arpa/inet.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define MAX_CLIENTS 26
#define BUFSIZE 800

/* This struct should contain the information that you want
 * keep for one connected client.
 */
struct Client
{
    int socket;
    char ID;
    int isBinaryFormat;  
    Record* internalStruct;
};

typedef struct Client Client;

Client* clients[MAX_CLIENTS]; 

void usage( char* cmd )
{
    fprintf( stderr, "Usage: %s <port>\n"
                     "       This is the proxy server. It takes as imput the port where it accepts connections\n"
                     "       from \"xmlSender\", \"binSender\" and \"anyReceiver\" applications.\n"
                     "       <port> - a 16-bit integer in host byte order identifying the proxy server's port\n"
                     "\n",
                     cmd );
    exit( -1 );
}

/*
 * This function is called when a new connection is noticed on the server
 * socket.
 * The proxy accepts a new connection and creates the relevant data structures.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */

int handleNewClient( int server_sock )
{
    Client* client = (Client*)malloc(sizeof(Client));
    if (!client) {
        perror("Error allocating memory for new client");
        return EXIT_FAILURE;
    }

    int com_fd = tcp_accept(server_sock);
    if (com_fd < 0) {
        perror("Error accepting new client");
        tcp_close(com_fd);
        return EXIT_FAILURE;
    }

    client->socket = com_fd;

    char buf[BUFSIZE];
    tcp_read(com_fd, buf, 1);
    if (buf[0] == 'X') {
        client->isBinaryFormat = 0;
    } else if (buf[0] == 'B') {
        client->isBinaryFormat = 1;
    } else {
        tcp_close(com_fd);
        perror("Couldn't read format type");
        return EXIT_FAILURE;
    }

    memset(buf, 0, sizeof(buf));

    tcp_read(com_fd, buf, 1);
    client->ID = buf[0];

    printf("ID: %d ", client->ID);
    fflush(stdout);

    memset(buf, 0, sizeof(buf));

    client->internalStruct = NULL;

    int emptySlotIndex = -1;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] == NULL)
        {
            emptySlotIndex = i;
            break;
        }
    }

    if (emptySlotIndex == -1)
    {
        perror("Maximum number of clients reached");
        tcp_close(client->socket);
        free(client);
        return EXIT_FAILURE;
    }

    clients[emptySlotIndex] = client;

    return client->socket;
}

/*
 * This function is called when a connection is broken by one of the connecting
 * clients. Data structures are clean up and resources that are no longer needed
 * are released.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */
void removeClient( Client* client )
{
    deleteRecord(client->internalStruct);
    free(client);
    return;
}

/*
 * This function is called when the proxy received enough data from a sending
 * client to create a Record. The 'dest' field of the Record determines the
 * client to which the proxy should send this Record.
 *
 * If no such client is connected to the proxy, the Record is discarded without
 * error. Resources are released as appropriate.
 *
 * If such a client is connected, this functions find the correct socket for
 * sending to that client, and determines if the Record must be converted to
 * XML format or to binary format for sendig to that client.
 *
 * It does then send the converted messages.
 * Finally, this function deletes the Record before returning.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */
void forwardMessage( Record* msg, Client* client )
{

    char dest = msg->dest;
    
    int BufferSize = BUFSIZE;
    char* data;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->ID == dest) {
            printf("\nClient receiving message: %d", clients[i]->ID);
            fflush(stdout);
                if (clients[i]->isBinaryFormat == 0) {
                    data = recordToXML(msg, &BufferSize); 
                } else {
                    data = recordToBinary(msg, &BufferSize);
                };
            tcp_write(clients[i]->socket, data, BufferSize);
        }
    }
    
    clearRecord(msg);
}

/*
 * This function is called whenever activity is noticed on a connected socket,
 * and that socket is associated with a client. This can be sending client
 * or a receiving client.
 *
 * The calling function finds the Client structure for the socket where acticity
 * has occurred and calls this function.
 *
 * If this function receives data that completes a record, it creates an internal
 * Record data structure on the heap and calls forwardMessage() with this Record.
 *
 * If this function notices that a client has disconnected, it calls removeClient()
 * to release the resources associated with it.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */
void handleClient( Client* client )
{
    if (client->internalStruct != NULL) {
        clearRecord(client->internalStruct);
    }

    char buf[BUFSIZE];
    int bytes_read = tcp_read(client->socket, buf, BUFSIZE);

    if (bytes_read == 0) {
        removeClient(client);
        return;
    }

    if (client->isBinaryFormat == 0) {
        client->internalStruct = XMLtoRecord(buf, BUFSIZE, NULL);
    } else {
        client->internalStruct = BinaryToRecord(buf, BUFSIZE, NULL);
    }

    forwardMessage(client->internalStruct, client);
    memset(buf, 0, sizeof(buf));
}

int main( int argc, char* argv[] )
{
    int port;
    int server_sock;

    if( argc != 2 )
    {
        usage( argv[0] );
    }

    port = atoi( argv[1] );

    server_sock = tcp_create_and_listen( port );
    if( server_sock < 0 ) exit( -1 );

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_sock, &read_fds);

    /* add your initialization code */
    
    /*
     * The following part is the event loop of the proxy. It waits for new connections,
     * new data arriving on existing connection, and events that indicate that a client
     * has disconnected.
     *
     * This function uses handleNewClient() when activity is seen on the server socket
     * and handleClient() when activity is seen on the socket of an existing connection.
     *
     * The loops ends when no clients are connected any more.
     */
    do
    { 
        fd_set temp_fds = read_fds;

        if (select(FD_SETSIZE, &temp_fds, NULL, NULL, NULL) == -1)
        {
            perror("Error in select");
            break;
        }

        if (FD_ISSET(server_sock, &temp_fds))
        {
            int newClientSock = handleNewClient(server_sock);
            if (newClientSock != -1)
            {
                FD_SET(newClientSock, &read_fds);
                printf("New client connected\n");
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i] != NULL && FD_ISSET(clients[i]->socket, &temp_fds))
            {
                handleClient(clients[i]);
            }
        }
    }


    while( 1 /* fill in your termination condition */ );

    /* add your cleanup code */

    tcp_close( server_sock );

    return 0;
}   

