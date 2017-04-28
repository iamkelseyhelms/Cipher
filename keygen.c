/***********************************************************
 * Author:          Kelsey Helms
 * Date Created:    March 17, 2017
 * Filename:        keygen.c
 *
 * Overview:
 * This program creates a key for use in a one-time pad.
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/***********************************************************
 * main: creates key based on number of chars.
 *
 * parameters: number of arguments, argument array.
 * returns: none.
 ***********************************************************/

int main (int argc, char *argv[]){

    int i, length;
    char key[length + 1];
    char randomLetter;

    if (argc != 2){
        printf("Usage: %s length\n", argv[0]);    //check usage & args
        exit (0);
    }
    srand(time(NULL));    //randomize with time
    length = atoi(argv[1]);    //cast length from char to int

    for (i=0; i<length; i++){    //for each char
        randomLetter = " ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rand() % 27];    //get random letter
        key[i] = randomLetter;    //store in key
    }
    key[length] = '\0';    //end with null terminator
    printf("%s\n", key);

    return 0;
}