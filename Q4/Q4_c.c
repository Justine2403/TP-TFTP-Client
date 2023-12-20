#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>  
#include <errno.h>  
#define MAX_SIZE 516
#define RRQ_MODE "octet"


#define SEND_ERROR "Error sending data"
#define RECEIVE_ERROR "Error receiving data"

struct addrinfo hints, *res;

//define a function to print a message
int print_message(char *output) {
	write(1, output, strlen(output)); 	
}

void gettftp(char* host, char* port, char* file) {
    char message[MAX_SIZE];
    snprintf(message, sizeof(message), "Uploading file '%s' to TFTP server at '%s' on port '%s'\n", file, host, port); 
    print_message(message);
}

void getaddr(char* host, char* port, char* file){
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
    rrq_buffer[0] = 0;  // Opcode (2 bytes)
    rrq_buffer[1] = 1;
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

    //SINGLE DAT PACKET
    //Create a buffer to hold the received data
    char buffer[MAX_SIZE];
    socklen_t fromlen = sizeof(struct sockaddr_storage);

    // Receive the DAT packet
    ssize_t n = recvfrom(sockfd, buffer, MAX_SIZE, 0, (struct sockaddr *)&from, &fromlen);
    if (n == -1) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
}

    // Check the opcode
    if (buffer[0] != 0 || buffer[1] != 3) {
        fprintf(stderr, "Received packet is not a DAT packet\n");
        exit(EXIT_FAILURE);
    }

    // Extract the block number and data
    uint16_t block_number = ntohs(*(uint16_t *)(buffer + 2));
    char *data = buffer + 4;

    // Create the ACK packet
    char ack[4] = {0, 4, buffer[2], buffer[3]};

    // Send the ACK packet
    if (sendto(sockfd, ack, 4, 0, (struct sockaddr *)&from, fromlen) == -1) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }

    //MULTIPLE DAT PACKETS
    /// Receive the DAT packets and send ACKs
    int block_number = 1; // Initial block number
    FILE *output_file = fopen(file, "wb"); // Open the file in binary write mode

    while (1) {
        // Receive the DAT packet
        ssize_t n = recvfrom(sockfd, buffer, MAX_SIZE, 0, (struct sockaddr *)&from, &fromlen);
        if (n == -1) {
            perror(RECEIVE_ERROR);
            exit(EXIT_FAILURE);
        }

        // Check the opcode
        if (buffer[0] != 0 || buffer[1] != 3) {
            fprintf(stderr, "Received packet is not a DAT packet\n");
            exit(EXIT_FAILURE);
        }

        // Extract the block number and data
        int received_block_number = ntohs(*(uint16_t *)(buffer + 2));
        char *data = buffer + 4;

        // Check if the received block number is as expected
        if (received_block_number == block_number) {
            // Write data to the output file
            fwrite(data, 1, n - 4, output_file);

            // Create the ACK packet
            char ack[4] = {0, 4, buffer[2], buffer[3]};

            // Send the ACK packet
            if (sendto(sockfd, ack, 4, 0, (struct sockaddr *)&from, fromlen) == -1) {
                perror(SEND_ERROR);
                close(sockfd);
                fclose(output_file);
                freeaddrinfo(res);
                exit(EXIT_FAILURE);
            }

            // Increment the block number for the next expected packet
            block_number++;

            // Check if this is the last packet
            if (n < MAX_SIZE) {
                break;
            }
        } else {
            // Ignore duplicate packet, resend previous ACK
            char ack[4] = {0, 4, buffer[2], buffer[3]};
            if (sendto(sockfd, ack, 4, 0, (struct sockaddr *)&from, fromlen) == -1) {
                perror(SEND_ERROR);
                close(sockfd);
                fclose(output_file);
                freeaddrinfo(res);
                exit(EXIT_FAILURE);
            }
        }
    }

    // Close the output file
    fclose(output_file);

    // Free the linked list
    freeaddrinfo(res);
}


int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 4) {
        char usage[MAX_SIZE];
        snprintf(usage, sizeof(usage), "ERROR: Usage: %s <host> <port> <file>\n", argv[0]);
        print_message(usage);
        exit(EXIT_FAILURE);
    }
    getaddr(argv[1], argv[2], argv[3]);
    gettftp(argv[1], argv[2], argv[3]);
    return 0;
}