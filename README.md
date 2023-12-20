# TP-TFTP-Client
Develop a TFTP client using RFC specifications and Wireshark captures.
## 1. Use of the arguments of the command : 
### gettftp.c & puttftp.c :
This program is takes three command-line arguments: the host the port and the file to be downloaded. The program verifies the correct number of arguments, extracts the host the port and file information, and displays this information using the write function to write to standard output.
## 2. getaddrinfo :
We initialise a addrinfo structure 'hints' that we initialize tat first with zeros. This structure will then be filled with the server adress using the getaddrinfo if there is no error.

## 3. Socket :
We establish a socket connection for the server using the information we got from the 'getaddrinfo' in the previous part. It first creates a socket ('sockfd') by using the 'socket' system call. We check if the creation of the socket failed by checking if the value of sockfd is equal to 1.

## 4. For gettftp :
gettftp handles the download logic while puttftp handles the upload. 
### RRQ Packet : 
We build a RRQ paquet and send it to the server using the function 'sendto'. The paquet contains the file's name and its transfer mode.

### Single data (DAT) :
The code receives a DAT packet from the server, verifies its opcode to ensure it is a data packet, extracts the block number and data payload, generates an acknowledgment (ACK) packet, and sends the ACK packet back to the server. It finished by freeing the memory associated with the address information linked list. 

### Multipale data :
The code manages the reception of multiple DAT packets, ensures the correct sequencing of data blocks based on block numbers, and sends acknowledgments for each received packet. It also handles the scenario of receiving duplicate packets by resending the previous ACK. The file being received is opened in binary write mode, and data is written to it as it arrives.

## 5. For puttftp : 
The structure for puttftp is the same as for gettftp. The difference resides in their goals; while in gettftp we buil a RRQ packet to download files, in puttftp we build WRQ packet to upload (it signals to the server that the client wants to upload)
