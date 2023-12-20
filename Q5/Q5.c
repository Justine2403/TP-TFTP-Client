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
#define WRQ_MODE "octet"


#define SEND_ERROR "Error sending data"
#define RECEIVE_ERROR "Error receiving data"

struct addrinfo hints, *res;
//define a function to print a message
int print_message(char *output) {
    write(1, output, strlen(output));   
}

void print_success(char* host, char* port, char* file) {
    // Implement TFTP upload logic here
    char message[MAX_SIZE];
    snprintf(message, sizeof(message), "Uploading file '%s' to TFTP server at '%s' on port '%s'\n", file, host, port); 
    print_message(message);
}

void puttftp(char* host, char* port, char* file){
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
    // Build WRQ request
    char wrq_buffer[MAX_SIZE];
    wrq_buffer[0] = 0;
    wrq_buffer[1] = 2;

    // Add the file name to the WRQ buffer
    strcpy(wrq_buffer + 2, file);
    int file_len = strlen(file);
    
    // Add the transfer mode to the WRQ buffer
    strcpy(wrq_buffer + 2 + file_len + 1, WRQ_MODE);
    int wrq_length = 2 + file_len + 1 + strlen(WRQ_MODE) + 1;

    // Send the WRQ request
    if (sendto(sockfd, wrq_buffer, wrq_length, 0, res->ai_addr, res->ai_addrlen) == -1) {
        perror(SEND_ERROR);
        close(sockfd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }
    //SEND SINGLE DATA PACKET
        // Read the file to be sent
    FILE *input_file = fopen(file, "rb");
    if (input_file == NULL) {
        perror("Error opening file");
        close(sockfd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    // Read the file into a buffer
    char data_buffer[MAX_SIZE - 4];
    size_t bytes_read;

    
    // SEND MULTIPLE DATA PACKETS
    while ((bytes_read = fread(data_buffer, 1, sizeof(data_buffer), input_file)) > 0) {
        // Build DAT packet
        char dat_buffer[MAX_SIZE];
        dat_buffer[0] = 0;
        dat_buffer[1] = 3;
        uint16_t block_number = 1; // Update block number accordingly

        // Copy block number to the buffer
        memcpy(dat_buffer + 2, &block_number, sizeof(uint16_t));

        // Copy data to the buffer
        memcpy(dat_buffer + 4, data_buffer, bytes_read);

        // Send the DAT packet
        if (sendto(sockfd, dat_buffer, 4 + bytes_read, 0, res->ai_addr, res->ai_addrlen) == -1) {
            perror(SEND_ERROR);
            close(sockfd);
            fclose(input_file);
            freeaddrinfo(res);
            exit(EXIT_FAILURE);
        }

        // Receive the ACK packet
        char ack_buffer[4];
        if (recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0, NULL, NULL) == -1) {
            perror(RECEIVE_ERROR);
            close(sockfd);
            fclose(input_file);
            freeaddrinfo(res);
            exit(EXIT_FAILURE);
        }

        // Check the opcode in the ACK packet
        if (ack_buffer[0] != 0 || ack_buffer[1] != 4) {
            fprintf(stderr, "Received packet is not an ACK packet\n");
            close(sockfd);
            fclose(input_file);
            freeaddrinfo(res);
            exit(EXIT_FAILURE);
        }

        // Extract the block number from the ACK packet
        uint16_t received_block_number = ntohs(*(uint16_t *)(ack_buffer + 2));

        // Check if the received block number matches the sent block number
        if (received_block_number != block_number) {
            fprintf(stderr, "Received ACK for incorrect block number\n");
            close(sockfd);
            fclose(input_file);
            freeaddrinfo(res);
            exit(EXIT_FAILURE);
        }

        // Increment the block number for the next packet
        block_number++;
    }

    // Close the input file
    fclose(input_file);

    freeaddrinfo(res); // free the linked-list
}

int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 4) {
        char usage[MAX_SIZE];
        snprintf(usage, sizeof(usage), "ERROR: Usage: %s <host> <port> <file>\n", argv[0]);
        print_message(usage);
        exit(EXIT_FAILURE);
    }

    // Call the upload function with provided arguments
    puttftp(argv[1], argv[2], argv[3]);
    print_success(argv[1], argv[2], argv[3]);

    return 0;
}
