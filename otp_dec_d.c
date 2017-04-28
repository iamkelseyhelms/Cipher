/***********************************************************
 * Author:          Kelsey Helms
 * Date Created:    March 17, 2017
 * Filename:        otp_dec_d.c
 *
 * Overview:
 * This is the daemon decrypting server.
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>


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
 * decrypt: decrypts the message.
 *
 * parameters: message, key.
 * returns: none.
 ***********************************************************/

void decrypt(char message[], char key[]) {
    int i;
    char c;
    for (i = 0; message[i] != '\n'; i++) {    //for each char
        c = charToInt(message[i]) - charToInt(key[i]);    //decrypt using key
        if (c < 0) {
            c += 27;
        }
        message[i] = intToChar(c);    //store char in message
    }
    message[i] = '\0';    //end with null terminator
    return;
}


/***********************************************************
 * main: creates key based on number of chars.
 *
 * parameters: number of arguments, argument array.
 * returns: none.
 ***********************************************************/

int main(int argc, char *argv[]) {

    int listenSocketFD, establishedConnectionFD, portNumber, optimumValue, status;
    socklen_t sizeOfClientInfo;
    char buffer[100000];
    struct sockaddr_in serverAddress, clientAddress;
    pid_t pid;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);    //check usage & args
        exit(1);
    }

    memset((char *) &serverAddress, '\0', sizeof(serverAddress));    //clear out the address struct
    portNumber = atoi(argv[1]);    //get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET;    //create a network-capable socket
    serverAddress.sin_port = htons(portNumber);    //store the port number
    serverAddress.sin_addr.s_addr = INADDR_ANY;    //any address is allowed for connection to this process
    
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);     //create the socket
    if (listenSocketFD < 0)
        perror("Decrypt Server: ERROR opening socket");
    optimumValue = 1;
    setsockopt(listenSocketFD, SOL_SOCKET, SO_REUSEADDR, &optimumValue, sizeof(int));   //allow reuse of port

    if (bind(listenSocketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)    //connect socket to port
        perror("Decrypt Server: ERROR on binding");

    listen(listenSocketFD, 5);    //turn socket on - it can now receive up to 5 connections

    while (1) {
        sizeOfClientInfo = sizeof(clientAddress);    //get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *) &clientAddress, &sizeOfClientInfo);    //accept
        if (establishedConnectionFD < 0) 
            perror("Decrypt Server: ERROR on accept");

        pid = fork();    //fork child process

        if (pid < 0) {
            perror("Decrypt Server: ERROR forking process");
            exit(1);
        }

        if (pid == 0) {    //child will handle connection
            memset(buffer, '\0', sizeof(buffer));
            int charsRemaining = sizeof(buffer);
            int charsRead = 0;
            char *p = buffer;    //keep track of where in buffer we are
            char *keyStart;
            int numNewLines = 0;
            int i;

            read(establishedConnectionFD, buffer, sizeof(buffer) - 1);    //read the client's message from the socket

            if (strcmp(buffer, "dec_bs") != 0) {    //write error back to client
                char response[] = "invalid";
                write(establishedConnectionFD, response, sizeof(response));
                _Exit(2);
            } else {    //write authority confirmation back to client
                char response[] = "dec_d_bs";
                write(establishedConnectionFD, response, sizeof(response));
            }
            memset(buffer, '\0', sizeof(buffer));

            while (1) {
                charsRead = read(establishedConnectionFD, p, charsRemaining);
                if (charsRead == 0) {    //we're done reading
                    break;
                }
                if (charsRead < 0) {
                    perror("Decrypt Server: ERROR reading from socket");
                }
                for (i = 0; i < charsRead; i++) {    //search for newlines in buffer
                    if (p[i] == '\n') {
                        numNewLines++;
                        if (numNewLines == 1) {     //first newline starts key
                            keyStart = p + i + 1;
                        }
                    }
                }
                if (numNewLines == 2) {    //second newline is end of message
                    break;
                }

                p += charsRead;
                charsRemaining -= charsRead;
            }
            char message[100000];
            memset(message, '\0', sizeof(message));

            strncpy(message, buffer, keyStart - buffer);    //separate message and key
            decrypt(message, keyStart);    //decrypt message
            write(establishedConnectionFD, message, sizeof(message));    //send decrypted message
        }
        close(establishedConnectionFD);    //close the existing socket which is connected to the client

        while (pid > 0) {    //parent process. wait for children to finish
            pid = waitpid(-1, &status, WNOHANG);
        }
    }
    close(listenSocketFD);    //close the listening socket

    return 0;
}