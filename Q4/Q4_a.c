#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define MAX_SIZE 256

struct addrinfo hints, *res;

//define a function to print a message
int print_message(char *output) {
	write(1, output, strlen(output)); 	
}

void gettftp(char* host, char* port, char* file) {
    // Implement TFTP download logic here
    char message[MAX_SIZE];
    snprintf(message, sizeof(message), "Uploading file '%s' to TFTP server at '%s' on port '%s'\n", file, host, port); 
    print_message(message);
}

void getaddr(char* host){
    //initializing hints with zeros
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    int status = getaddrinfo(host, NULL, &hints, &res); 

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

    getaddr(argv[1]);
    gettftp(argv[1], argv[2], argv[3]);
    return 0;
}