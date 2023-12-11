# TP-TFTP-Client
Develop a TFTP client using RFC specifications and Wireshark captures.
## 1. Use of the arguments of the command : 
### gettftp.c & puttftp.c :
This program is takes three command-line arguments: the host the port and the file to be downloaded. The program verifies the correct number of arguments, extracts the host the port and file information, and displays this information using the write function to write to standard output.