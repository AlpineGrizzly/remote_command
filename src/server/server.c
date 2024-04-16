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
#include <time.h>

// Socket libraries
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>

// Network defines
#define QUEUELEN 5

// TODO TCP concurrent currently implemented - need to add define toggle for udp iterative server

// Arg parsing
#define NUM_ARGS 2
enum Args{None, Port};

#define BUFSIZE 1024 

// Command messages
#define RCEND "rcend"
#define ACK "ack"

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
    char sbuf[BUFSIZE], cbuf[BUFSIZE], command[BUFSIZE], sys_command[128], tempfile[128]; // 16 for appending string to command
    size_t mlen; 
    int status; // Holds status of executing requested command from client
    time_t rawtime; 
    struct tm *timeinfo;
    char *time_str; // holds pointer to ascii string of time
    char *temp;
    int ecount, delay;
    
    printf("\nConnected to %s\n", client_id);
    
    // Receive parameter string from client and tokenize
    if ((mlen = read(client_sd, cbuf, BUFSIZE)) > 0) { 
        cbuf[mlen] = 0;

        printf("Received %s\n", cbuf);

        //  otherwise tokenize and load parameters
        temp = strtok(cbuf, ","); // Process execution count
        ecount = atoi(temp);
        
        temp = strtok(NULL, ","); // Process delay 
        delay = atoi(temp);

        temp = strtok(NULL, ","); // Process command 
        strcpy(command, temp); 
        strcpy(sys_command, temp);

    } else { 
        printf("Did not receive parameter string\n");
        return;
    }

    // Set up for loop of executing instructions
    sprintf(tempfile, "client_%s.temp", client_id); // Get unique client temp

    // Add redirect to store command output into file
    strcat(command, ">");
    strcat(command, tempfile);

    int flags = fcntl(client_sd, F_GETFL, 0); // Get flags

    printf("Executing %s %d times for %d seconds\n", command, ecount, delay);
    for (int i = 0; i < ecount; i++) { 
        char c;
        int j = 0;

        // Get the current time
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        time_str = asctime(timeinfo);

        // Check for rcend input from user
        if (!strcmp(cbuf, RCEND)) { 
            printf("Time at server: %sClient IP:%s\nStatus: closed\n", time_str, client_id);
            return; // Stop receving commands when client is done
        }
        printf("Time at server: %sClient IP: %s\nCommand: %s\nStatus: connected\n", time_str, client_id, sys_command);

        // Send server time to client
        if (send(client_sd, time_str, strlen(time_str), 0) != strlen(time_str)) { 
            printf("Unable to send time to client\n");
            break;
        }

        // Receive ack 
        if ((mlen = read(client_sd, sbuf, sizeof sbuf)) > 0) {
            sbuf[mlen] = 0; // Null terminate
            if (!strcmp(sbuf, RCEND)) { 
                printf("Received RCEND from client\n");
                printf("Time at server: %sClient IP:%s\nStatus: closed\n", time_str, client_id);
                return;
            } else if (strcmp(sbuf, ACK)) { 
                printf("Unknown command received from client...terminating --- %s\n", sbuf);
                return;
            }
        }

        // execute the command
        status = system(command); 

        if (status < 0) { // get status of executing command
            printf("Command failed execution!\n");
            return;
        }
        
        // Otherwise, if successful, read output from file
        FILE *f = fopen(tempfile, "r");

        if (!f) { 
            printf("Failed to open temp file %s\n", tempfile);
            fclose(f);
            return;
        }

        // Diplay to server and write into buffer to send to client
        while ((c = (char)fgetc(f)) != EOF && j < sizeof sbuf - 1) {
            sbuf[j++] = c;
        }
        sbuf[j] = 0; // NULL TERMINATE
        printf("Execution Output:\n%s\n\n", sbuf);
        fclose(f);

        // Send time of execution and result to client
        // first 8 bytes are length, rest is message
        char reply[sizeof(sbuf)+sizeof(uint64_t)];
        uint64_t output_size = (uint64_t)strlen(sbuf)+sizeof(uint64_t);
        memcpy(reply, &output_size, sizeof(uint64_t));
        strcpy(reply + sizeof(uint64_t), sbuf);

        // Write data to client
        //printf("reply [%ld] %s\n", output_size, reply+sizeof(uint64_t));
        if (write(client_sd, reply, output_size) < 0) { 
            printf("Error sending result to client\n");
            return;
        }
        
        fcntl(client_sd, F_SETFL, flags | O_NONBLOCK); // Make nonblocking
        for (int i = 0; i < delay; i++) { 
            // Check periodically through sleep for a rcend command 
            if ((mlen = read(client_sd, sbuf, sizeof sbuf)) > 0) {
                sbuf[mlen] = 0; // Null terminate
                if (!strcmp(sbuf, RCEND)) { 
                    printf("Received RCEND from client\n");
                    printf("Time at server: %sClient IP:%s\nStatus: closed\n", time_str, client_id);
                    return;
                }
            }
            sleep(i); // Otherwise night night
        }
        fcntl(client_sd, F_SETFL, flags); // Set back blocking flags
        memset(cbuf, 0, sizeof cbuf); // Clear the buffer
    }
    printf("Time at server: %sClient IP:%s\nStatus: closed\n", time_str, client_id);
    return;
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
    pid_t child; 
    while(1) {
        printf("Listening for clients...\n");

        // Receive client request for file transfer
        client_sd = accept(sd, (struct sockaddr*)&client, (socklen_t *)&client_len);

        // Accept file from the client
        if (client_sd < 0) { 
            printf("Failed client request:: %d\n", client_sd);
            continue;
        }

        if ((child = fork()) == 0) { 
            close(sd); // Close listening socket for child
            inet_ntop(AF_INET6, &(client.sin6_addr), addr6_str, sizeof(addr6_str));
            serve_client(addr6_str, client_sd); // Handle client request
            printf("--------------------------------------------\n");
            exit(0);
        }
        close(client_sd); // parent closes connection with served client 
    }

    close(sd);
    return 0;
}