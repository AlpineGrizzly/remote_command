/** Rcmd server 
 * 
 *  Remote command server that serves clients over Ipv6 for remote command
 *  executions 
 *
 *  @author Dalton Kinney
 *  @date   April 7th, 2024
 */

// Standard libraries
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <stdint.h>

// Socket libraries
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>

// Network defines
#define QUEUELEN 5

// Arg parsing
#define NUM_ARGS 2
enum Args{None, Port};

/**
 * usage
 * 
 * Prints the usage of the program
*/
void usage() { 
	char* usage_string = "Usage: server [port]\n" 
						 "Rcmd server  \n\n"
                         "Serves remote execution of commands from clients on serverside\n" 
                         "port    Port to listening for incoming command requests from clients\ns"
                         "-h      Show this information\n";
    
	printf("%s", usage_string);
    exit(0);
}

/** Pass off function to handle incoming clients requests */
void serve_client(char* client_id, int client_sd) { 
    // TODO 
    printf("Serving %s:%d\n", client_id, client_sd);
}

int main(int argc, char* argv[]) {
    int sd, client_sd, s_port; // Socket descriptors for server and clients + listening port
    char addr6_str[INET6_ADDRSTRLEN];
    struct sockaddr_in6 server, client; // Ipv6 socket structs
    
    // Parse port number to listen for incoming commands
    if (argc != NUM_ARGS) 
        usage(); 

    s_port = atoi(argv[Port]);

    // TODO get server hostname and print
    printf("Listening for messages on %d\n", s_port);

    // Initialize socket descriptor for server and set network parameters
    sd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (sd < 0) { 
        printf("Unable to create socket!\n");
        exit(0);
    }
    printf("Socket initialized: %d\n", sd);

    // set socket options
    int enable = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

    server.sin6_family = AF_INET6;
    server.sin6_addr = in6addr_any;
    server.sin6_port = htons(s_port);
    server.sin6_scope_id = if_nametoindex("enp7s0");

    // Bind port 
    if ((bind(sd, (struct sockaddr*)&server, sizeof(server)) != 0)) { 
        printf("Unable to bind to socket!\n");
        close(sd);
        exit(0);
    }
    printf("Bind success %d\n", s_port);

    // Listen for connections from clients
    if (listen(sd, QUEUELEN) != 0) { 
        printf("Unable to listen\n");
        exit(0);
    }
    int client_len = sizeof(client);

    // Main listen and server loop
    while(1) {
        printf("Listening for clients...\n");

        // Receive client request for file transfer
        client_sd = accept(sd, (struct sockaddr*)&client, (socklen_t *)&client_len);

        // Accept file from the client
        if (client_sd < 0) { 
            printf("Failed client request:: %d\n", client_sd);
            continue;
        }

        inet_ntop(AF_INET6, &(client.sin6_addr), addr6_str, sizeof(addr6_str));
        //printf("Serving %s:%i\n", addr6_str, ntohs(client.sin6_port));
 
        serve_client(addr6_str, client_sd); // Handle client request
        close(client_sd); // parent closes connection with served client 
    }

    close(sd);
    return 0;
}