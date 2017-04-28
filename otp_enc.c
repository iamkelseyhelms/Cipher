/***********************************************************
 * Author:          Kelsey Helms
 * Date Created:    March 17, 2017
 * Filename:        otp_enc.c
 *
 * Overview:
 * This is the encrypting client.
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


/***********************************************************
 * error: prints correct error statement and exits.
 *
 * parameters: error message.
 * returns: none.
 ***********************************************************/

void error(const char *msg) {
    perror(msg);
    exit(1);
}


/***********************************************************
 * getLength: gets the length of the file.
 *
 * parameters: pointer to file.
 * returns: length.
 ***********************************************************/

long getLength(char *filename) {
    FILE *file = fopen(filename, "r");
    fpos_t position;
    long length;

    fgetpos(file, &position);    //save previous position in file

    if (fseek(file, 0, SEEK_END) || (length = ftell(file)) == -1) {    //seek to end or determine offset of end
        fprintf(stderr, "Encrypt Client: ERROR Finding file length");
        exit(1);
    }
    fsetpos(file, &position);    //restore position

    return length;
}


/***********************************************************
 * charToInt: turns a char into an int.
 *
 * parameters: char.
 * returns: int.
 ***********************************************************/

int charToInt(char c) {
    if (c == ' ') {
        return 26;
    } else {
        return (c - 'A');
    }
}


/***********************************************************
 * intToChar: turns an int into a char.
 *
 * parameters: int.
 * returns: char.
 ***********************************************************/

char intToChar(int i) {
    if (i == 26) {
        return ' ';
    } else {
        return (i + 'A');
    }
}


/***********************************************************
 * sendFile: sends contents of the file to the server.
 *
 * parameters: pointer to file, file descriptor, file length.
 * returns: none.
 ***********************************************************/

void sendFile(char *filename, int sockfd, int filelength) {
    int fd = open(filename, 'r');    //open for read-only file from command line
    char buffer[100000];
    memset(buffer, '\0', sizeof(buffer));
    int charsRead, charsWritten;

    while (filelength > 0) {    //read in the file in chunks until the whole file is read
        charsRead = read(fd, buffer, sizeof(buffer));
        if (charsRead == 0) {    //we're done reading from the file
            break;
        }
        if (charsRead < 0) {    //handle errors
            perror("Encrypt Client: ERROR reading file");
            exit(1);
        }
        filelength -= charsRead;
    }
    char *p;
    p = buffer;    //keep track of where in buffer we are
    while (charsRead > 0) {
        charsWritten = write(sockfd, p, charsRead);
        if (charsWritten < 0) {   //handle errors
            perror("Encrypt Client: ERROR writing to socket");
            exit(1);
        }
        charsRead -= charsWritten;
        p += charsWritten;
    }
    return;
}


/***********************************************************
 * main: creates key based on number of chars.
 *
 * parameters: number of arguments, argument array.
 * returns: none.
 ***********************************************************/

int main(int argc, char *argv[]) {
    int socketFD, portNumber, n, optimumValue, plaintextfd;
    struct sockaddr_in serverAddress;
    struct hostent *serverHostInfo;
    FILE *fp;
    const char hostname[] = "localhost";
    char buffer[100000];
    memset(buffer, '\0', sizeof(buffer));

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <inputfile> <key> <port>\n", argv[0]);    //check usage & args
        exit(1);
    }

    memset((char*)&serverAddress, '\0', sizeof(serverAddress));    //clear out the address struct
    portNumber = atoi(argv[3]);    //get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET;    //create a network-capable socket
    serverAddress.sin_port = htons(portNumber);    //store the port number
    serverHostInfo = gethostbyname(hostname);    //sonvert the machine name into a special form of address

    if (serverHostInfo == NULL) {
        fprintf(stderr, "Encrypt CLIENT: ERROR, no such host\n");
        exit(0);
    }

    memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);    //copy in the address


    socketFD = socket(AF_INET, SOCK_STREAM, 0);    //create the socket
    if (socketFD < 0)
        error("Encrypt Client: ERROR opening socket");

    optimumValue = 1;
    setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &optimumValue, sizeof(int));    //allow reuse of port


    if (connect(socketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {    //connect to server socket
        perror("Encrypt Client: ERROR connecting");
        exit(1);
    }

    char auth[] = "enc_bs";
    write(socketFD, auth, sizeof(auth));    //send authority
    read(socketFD, buffer, sizeof(buffer));    //read response
    if (strcmp(buffer, "enc_d_bs") != 0) {    //make sure it's the correct server
        fprintf(stderr, "Unable to contact otp_enc_d on given port\n");
        exit(2);
    }

    long fileLength = getLength(argv[1]);
    long keylength = getLength(argv[2]);
    if (fileLength > keylength) {    //check that key is at least as long as message
        fprintf(stderr, "Key is too short");
        exit(1);
    }

    int plainfd = open(argv[1], 'r');
    while (read(plainfd, buffer, 1) != 0) {
        if (buffer[0] != ' ' && (buffer[0] < 'A' || buffer[0] > 'Z')) {    //check that plaintext contains only valid characters
            if (buffer[0] != '\n') {
                fprintf(stderr, "%s contains invalid characters\n", argv[1]);
                exit(1);
            }
        }
    }
    memset(buffer, '\0', sizeof(buffer));    //clear buffer

    sendFile(argv[1], socketFD, fileLength);    //send plaintextfile
    sendFile(argv[2], socketFD, keylength);    //send key
    n = recv(socketFD, buffer, sizeof(buffer) - 1, 0);    //read data from the socket, leaving \0 at end

    if (n < 0) {
        perror("Encrypt Client: ERROR reading from socket");
        exit(1);
    }
    printf("%s\n", buffer);
    close(socketFD);    //close the socket
    return 0;
}