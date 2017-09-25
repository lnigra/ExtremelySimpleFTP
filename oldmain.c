/* 
 * File:   main.c
 * Author: nigra
 *
 * Created on September 2, 2017, 4:36 AM
 */

/*
 * 
 */

/* Based on code found at:
 * https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
 * 
 * Used alternate answer by 
 * For demo code that conforms to POSIX standard as described in Setting 
 * Terminal Modes Properly and Serial Programming Guide for POSIX Operating 
 * Systems, the following is offered.
 * It's essentially derived from the other answer, but inaccurate and 
 * misleading comments have been corrected.
 * 
 * To make the program treat the received data as ASCII codes, compile the 
 * program with the symbol DISPLAY_STRING, e.g.
 * cc -DDISPLAY_STRING demo.c
 */	

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <termios.h>
#include <ctype.h>

bool getReqFlg, fileXferFlg, cmdFlg;
FILE *outFile;
char outFileName[82];
int numBytes, byteCnt, fd, err;
pthread_t inThread;


void* serialInMonitor( void *arg ) {
    unsigned char inByte;
    ssize_t size;
    while ( true ) {
        
        // Pseudo-blocking. Couldn't get blocking to work with fcntl() using
        // O_NONBLOCK or FNDELAY flags.
        while ( read(fd, &inByte, 1) <= 0 );
        
        // Abort file transfer if first byte is zero (file doesn't exist)
        // otherwise, create the new file and proceed with transfer
        if ( getReqFlg ) {
          if ( inByte == 0 ) {
            fileXferFlg = false;
            } else {
                outFile = fopen( outFileName, "w+" );
                while ( read(fd, &inByte, 1) <= 0 );
                numBytes = 0;
                byteCnt = 0;
                fileXferFlg = true;
            }
            getReqFlg = false;
        }

        if ( fileXferFlg ) {
            if ( byteCnt < 4 ) {
                numBytes = 256 * numBytes + inByte; //First 4 bytes are file size
            } else {
                if ( byteCnt == 4 ) {
                    printf( "\nTransferring %i bytes...", numBytes );
                }
                if ( numBytes > 0 ) {
                    fputc( inByte, outFile );
                }

                //On final byte, terminate transfer
                if ( numBytes == 0 || byteCnt == numBytes + 3 ) {
                  fclose( outFile );
                  fileXferFlg = false;
                  printf( "%s", "done\n" );
                }
            }
            byteCnt++;
        } else {
            err = write( STDOUT_FILENO, &inByte, 1 );
        }
    }
}


int main() {
    
    struct termios portSettings;
    unsigned char inTTY, inTerm;
    int count = 0, err, cmdBufIndex;
    unsigned char cmdBuf[30];
    char* line = NULL;
    size_t size;
    bool lineEndFlg, cmdFlg;
    
    fd = open("/dev/ttyACM0", O_RDWR);

    tcgetattr( fd, &portSettings );
    cfsetispeed( &portSettings, B9600 );
    cfsetospeed( &portSettings, B9600 );
//    portSettings.c_cflag &= ~PARENB; /*CLEAR Parity Bit PARENB*/
//    portSettings.c_cflag &= ~CSTOPB; //Stop bits = 1
//    
//    portSettings.c_cflag &= ~CSIZE; /* Clears the Mask       */
//    portSettings.c_cflag |=  CS8;   /* Set the data bits = 8 */
//    portSettings.c_cflag &= ~CRTSCTS;
//    portSettings.c_cflag |= CREAD | CLOCAL;
//    portSettings.c_iflag &= ~(IXON | IXOFF | IXANY);
//    portSettings.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
//    
//    tcsetattr( fd,TCSANOW, &portSettings );
    
    if (fd == -1) {
        printf("\n  Error opening tty\n");
        return 1;
    } else {
        printf("\n  tty Opened Successfully\n");
    }
    
    //fcntl(fd, F_SETFL, FNDELAY);
    
    err = pthread_create( &inThread, NULL, &serialInMonitor, NULL );
    if ( err != 0 ) printf("\ncan't create thread :[%s]", strerror(err));
    
    int cmdIndex = 0;
    while ( true ) {
//        if ( read( STDOUT_FILENO, &inTerm, 1 ) >= 0 ) {
//            err = write( fd, &inTerm, 1 );
//        }
        
        if ( read( STDOUT_FILENO, &inTerm, 1 ) >= 0 ) {
            err = write( fd, &inTerm, 1 );
            if ( !cmdFlg ) {
                if ( inTerm == '\n' || inTerm == '\r' ) {
                    if ( lineEndFlg ) {
                      lineEndFlg = false;
                    } else {
                      lineEndFlg = true;
                      cmdFlg = true;
                    }
                    cmdBufIndex = 0;
                } else {
                    cmdBuf[cmdBufIndex] =  inTerm;
                    cmdBufIndex++;
                    cmdBuf[cmdBufIndex] = 0;
                }
                //printf( "%s\n", cmdBuf );
            }
        }
        if ( cmdFlg ) {
            //printf( "cmdFlg:%i\n", cmdFlg );
            int tmpIndex = 0;
            // Convert command to upper case
            while( cmdBuf[tmpIndex] != 0 ) {
                cmdBuf[tmpIndex] = toupper( cmdBuf[tmpIndex] );
                tmpIndex++;
            }
            // Filter for "get" command and set up for file transfer
            char * pch = strstr( cmdBuf, "GET" );
            if ( pch != NULL ) {
                printf( "pch:%s\n", pch + 4 );
            }
            // Send command with cr+lf
            int i = 0;
            while ( cmdBuf[i] != 0 ) {
                err = write( fd, &cmdBuf[i], 1 );
                i++;
            }
            cmdBuf[i] = '\r';
            err = write( fd, &cmdBuf[i], 1 );
            i++;
            cmdBuf[i] = '\n';
            err = write( fd, &cmdBuf[i], 1 );
            
            // Reset command flag
            cmdFlg = false;
        }
    }
    
}