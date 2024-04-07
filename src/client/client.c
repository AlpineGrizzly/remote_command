/** Rcmd client 
 * 
 *  Remote command client that will connect to/issue command execution requests
 *  to a predefined server. 
 *
 *  @author Dalton Kinney
 *  @date   April 7th, 2024
 */
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
#include <netdb.h>

// Arg parsing
#define NUM_ARGS 6
// s_name - name of the server machine, 
// s_port - port number to which it should connect, 
// count  - execution_count, 
// delay  - time_delay (in seconds) and the command
// cmd    - Command to execute remotely on server
enum Args{None, S_name, S_port, Count, Delay, Cmd};

// Network defines
// TODO TCP concurrent currently implemented - need to add define toggle for udp iterative client

#define BUFSIZE 256 // Buffer size limiit for communicating with server

// Command messages
#define RCEND "rcend"

/**
 * usage
 * 
 * Prints the usage of the program
*/
void usage() { 
	char* usage_string = "Usage: client [s_name] [s_port] [count] [delay]\n" 
						 "Rcmd client \n\n"
                         "s_name  Name of the server machine\n" 
                         "s_port  Port number to which it should connect\n" 
                         "count   Execution_count\n" 
                         "delay   Time_delay (in seconds)\n" 
                         "cmd     Command to run\n" 
                         "-h      Show this information\n";
    
	printf("%s", usage_string);
    exit(0);
}

int host2ipv6(char *hostname, char *dst) { 
    struct addrinfo hints, *res, *p;
    int status;

    // Set up hints to get IPv6 addresses
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    // Get address info
    if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 0;
    }

    //printf("IPv6 addresses for %s:\n\n", hostname);

    // Loop through all the results and print the IP address
    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // Get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET6) { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // Convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, dst, sizeof dst);
        //printf("  %s: %s\n", ipver, dst);
    }
    freeaddrinfo(res); // free the linked list

    return 1;
}

int main(int argc, char* argv[]) { 
    char s_ip[INET6_ADDRSTRLEN]; 
    char *cmd;  
    int s_port, count, delay;
    struct sockaddr_in6 server, client;
    int sd; 
    struct hostent *lh;
    size_t mlen; // Contains size of message in bytes
    char sbuf[BUFSIZE], cbuf[BUFSIZE]; // Server and Client message buffers respectively

    // Parse arguments 
    if (argc != NUM_ARGS) { 
        usage();
    }
    
    // Do hostname lookup for corresponding ipv6 address
    if (!host2ipv6(argv[S_name], s_ip)) { 
        printf("Failed to find ipv6 for %s\n", argv[S_name]);
        exit(0);
    }
    printf("Found %s for %s\n", s_ip, argv[S_name]);
    s_port = atoi(argv[S_port]);
    count  = atoi(argv[Count]);
    delay  = atoi(argv[Delay]);
    cmd    = argv[Cmd];

    // Initiate connection to server
    sd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (sd < 0) { 
        printf("Unable to initialize socket!\n");
        exit(0);
    }

    // Network assignments
    server.sin6_family = AF_INET6;
    server.sin6_port = htons(s_port);
    inet_pton(AF_INET6, s_ip, &server.sin6_addr);
    server.sin6_scope_id = if_nametoindex("enp7s0");
    
    // Connect cient to server socket
    if (connect(sd, (struct sockaddr*)&server, sizeof(server)) != 0) { 
        printf("Failed to connect to server!\n");
        exit(0);
    }
    printf("Successfully connected to %s! --> %d\n", s_ip, sd);

    // Send the command request to the server to execute
    sprintf(cbuf, "%s", cmd);

    printf("Executing '%s' %d times %d seconds apart\n", cbuf, count, delay);
    // Delay and count logic will be here in a for loop for number of request exceution times
    for (int i = 0; i < count; i++) { 
        // send command to server 
        printf("Requesting command: [%ld]'%s'\n", strlen(cbuf), cbuf);
        if (send(sd, cbuf, strlen(cbuf), 0) != strlen(cbuf)) { 
            printf("Unable to send command to server\n");
            break;
        }
        
        // TODO Listen for reply from server 

        sleep(delay); // Sleep for delay in seconds
    }
    
    // send kill message
    if (send(sd, RCEND, sizeof RCEND, 0) != sizeof RCEND) { 
        printf("Unable to send client_done to server\n");
        close(sd);
        exit(0);
    }
    printf("Client done!\n");

    // Close connection to server
    close(sd);

    return 0;
}