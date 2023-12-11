#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define MAX_SIZE 256

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

int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 4) {
        char usage[MAX_SIZE];
        snprintf(usage, sizeof(usage), "ERROR: Usage: %s <host> <port> <file>\n", argv[0]);
        print_message(usage);
        exit(EXIT_FAILURE);
    }

    // Call the download function with provided arguments
    gettftp(argv[1], argv[2], argv[3]);

    return 0;
}
    
