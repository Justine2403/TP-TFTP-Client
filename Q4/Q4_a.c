#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>  
#include <errno.h>  
#define MAX_SIZE 256
#define RRQ_MODE "octet"

#define ARGUMENTS_ERROR "Error, you must specify 3 arguments."
#define SERVER_ADDR_ERROR "Error trying to get the server address."
#define SOCKET_ERROR "Socket creation error"
#define SEND_ERROR "Error sending data"
#define RECEIVE_ERROR "Error receiving data"

struct addrinfo hints, *res;

//define a function to print a message
int print_message(char *output) {
	write(1, output, strlen(output)); 	
}

void print_success(char* host, char* port, char* file) {
    char message[MAX_SIZE];
    snprintf(message, sizeof(message), "Uploading file '%s' to TFTP server at '%s' on port '%s'\n", file, host, port); 
    print_message(message);
}

void gettftp(char* host, char* port, char* file){
    //initializing hints with zeros
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_DGRAM;// UDP datagram 
    hints.ai_protocol=IPPROTO_UDP;// UDP protocol
    int status = getaddrinfo(host, port, &hints, &res); 

    // check for errors
    if (status != 0) {
        char error[MAX_SIZE];
        snprintf(error, sizeof(error), "getaddrinfo: %s\n", gai_strerror(status));
        print_message(error);
        exit(EXIT_FAILURE);
    }  

    char ipstr[INET6_ADDRSTRLEN]; // space to hold the IP string
    void *addr; 

    // get the pointer to the address itself,
    if (res->ai_family == AF_INET) { 
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
        addr = &(ipv4->sin_addr);
    } 
    // different fields in IPv6 and IPv4:
    else {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
        addr = &(ipv6->sin6_addr);
    }

    inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr)); // convert the IP to a string

    char message[MAX_SIZE];

    int ret = snprintf(message, sizeof(message), "Server IP: %s\n", ipstr);
    
    
    print_message(message); //  print the IP address
   

    //reserve socket connexion for the server
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    char sck[MAX_SIZE];
    snprintf(sck,sizeof(sck),"%d",sockfd);
    print_message(sck);

    if (sockfd == -1) {
        char error[MAX_SIZE];
        snprintf(error, sizeof(error), "ERROR: socket: %s\n", strerror(errno));
        print_message(error);
        exit(EXIT_FAILURE);
    }
 
    // Build RRQ packet
    char rrq_buffer[MAX_SIZE];
    rrq_buffer[0] = 0x00;  // Opcode (2 bytes)
    rrq_buffer[1] = 0x01;
    strcpy(rrq_buffer + 2, file);  // Filename
    strcpy(rrq_buffer + 2 + strlen(file) + 1, RRQ_MODE);  // 1 byte of zeros + the mode
    int rrq_length = 2 + strlen(rrq_buffer) + 1 + strlen(file) + 1;

    // Send the RRQ packet
    if (sendto(sockfd, rrq_buffer, rrq_length, 0, res->ai_addr, res->ai_addrlen) == -1) {
        perror(SEND_ERROR);
        close(sockfd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
        }
    sendto(sockfd, rrq_buffer, rrq_length, 0, res->ai_addr, res->ai_addrlen);
    
    freeaddrinfo(res); // free the linked list
}


int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 4) {
        char usage[MAX_SIZE];
        snprintf(usage, sizeof(usage), "ERROR: Usage: %s <host> <port> <file>\n", argv[0]);
        print_message(usage);
        exit(EXIT_FAILURE);
    }
    gettftp(argv[1], argv[2], argv[3]);
    print_success(argv[1], argv[2], argv[3]);
    return 0;
}
