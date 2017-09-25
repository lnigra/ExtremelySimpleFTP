/* 
 * File:   xsftp-client.c
 * Author: nigra
 *
 * Created on September 23, 2017, 8:53 AM
 * 
 * Client program for Extremely Simple FTP (xsftp) Arduino/Teensy server library
 */

/*
 * Base code: miniterm.c
 * Author: Sven Goldt (goldt@math.tu-berlin.de)
 * Source: ftp://ftp.er.anl.gov/pub/outgoing/REDHAT/miniterm.c
 */

/*
 *  AUTHOR: Sven Goldt (goldt@math.tu-berlin.de)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
*/
/*
 This is like all programs in the Linux Programmer's Guide meant
 as a simple practical demonstration.
 It can be used as a base for a real terminal program.
*/

#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <string.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ENDMINITERM 2 /* ctrl-b to quit miniterm */

#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE; 


void child_handler(int s)
{
   STOP=TRUE;
}

main(argc,argv)
int argc; char *argv[];
{
  char port[20];
  int c, fd, err;
  int speed;
  struct termios oldstdtio,newstdtio,oldtio,newtio;
  struct sigaction sa;
  if(argc<3)
    {
      fprintf(stderr,"Perform basic terminal I/O with specified port.\n");
      fprintf(stderr,"Example syntax: \"miniterm /dev/ttyS0 19200\"\n");
      fprintf(stderr,"Based on Linux miniterm (tjmartin@anl.gov 10/10/02).\n");
      exit(1);
    }
  strcpy(port,argv[1]);
  sscanf(argv[2],"%ld",(long int *)&speed);
/* 
   Open modem device for reading and writing and not as controlling tty
   because we don't want to get killed if linenoise sends CTRL-C.
*/
  fd = open(port, O_RDWR | O_NOCTTY);
  if (fd <0) {perror(port); exit(-1); }
 
  tcgetattr(fd,&oldtio); /* save current modem settings */
 
/* 
   Set bps rate and hardware flow control and 8n1 (8bit,no parity,1 stopbit).
   Also don't hangup automatically and ignore modem status.
   Finally enable receiving characters.
*/
  switch(speed)
    {
    case 300:
      newtio.c_cflag = B300 | CS8 | CLOCAL | CREAD ;
      break;
    case 1200:
      newtio.c_cflag = B1200 | CS8 | CLOCAL | CREAD ;
      break;
    case 2400:
      newtio.c_cflag = B2400 | CS8 | CLOCAL | CREAD ;
      break;
    case 4800:
      newtio.c_cflag = B4800 | CS8 | CLOCAL | CREAD ;
      break;
    case 9600:
      newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD ;
      break;
    case 19200:
      newtio.c_cflag = B19200 | CS8 | CLOCAL | CREAD ;
      break;
    case 38400:
      newtio.c_cflag = B38400 | CS8 | CLOCAL | CREAD ;
      break;
    default:
      newtio.c_cflag = B19200 | CS8 | CLOCAL | CREAD ;
      break;
    }
 
/*
  Ignore bytes with parity errors and make terminal raw and dumb.
*/
  newtio.c_iflag = IGNPAR;
 
/*
  Raw output.
*/
  newtio.c_oflag = 0;
 
/*
  Don't echo characters because if you connect to a host it or your
  modem will echo characters for you. Don't generate signals.
*/
  newtio.c_lflag = 0;
 
/* blocking read until 1 char arrives */
  newtio.c_cc[VMIN]=1;
  newtio.c_cc[VTIME]=0;
 
/* now clean the modem line and activate the settings for modem */
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);

/*
  Strange, but if you uncomment this command miniterm will not work
  even if you stop canonical mode for stdout. This is a linux bug.
*/
  //tcsetattr(1,TCSANOW,&newtio); /* stdout settings like modem settings */
 
/* next stop echo and buffering for stdin */
  tcgetattr(0,&oldstdtio);
  tcgetattr(0,&newstdtio); /* get working stdtio */
  newstdtio.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0,TCSANOW,&newstdtio);
  printf("Starting miniterm with %s at %ld baud.\r\n",port,(long int)speed);
  printf("Exit with ctrl-B then \"stty sane\".\r\n");
/* terminal settings done, now handle in/ouput */
  switch (fork())
    {
    case 0: /* child */
      /* user input */
      close(1); /* stdout not needed */
      for (c=getchar(); c!= ENDMINITERM ; c=getchar()) err = write(fd,&c,1);
      tcsetattr(fd,TCSANOW,&oldtio); /* restore old modem setings */
      tcsetattr(0,TCSANOW,&oldstdtio); /* restore old tty setings */
      close(fd);
      exit(0); /* will send a SIGCHLD to the parent */
      break;
    case -1:
      perror("fork");
      tcsetattr(fd,TCSANOW,&oldtio);
      close(fd);
      exit(-1);
    default: /* parent */
      close(0); /* stdin not needed */
      sa.sa_handler = child_handler;
      sa.sa_flags = 0;
      sigaction(SIGCHLD,&sa,NULL); /* handle dying child */
      while (STOP==FALSE) /* modem input handler */
	{
	  err = read(fd,&c,1); /* modem */
	  err = write(1,&c,1); /* stdout */
	}
      wait(NULL); /* wait for child to die or it will become a zombie */
      tcsetattr(fd,TCSANOW,&oldtio);
      tcsetattr(0,TCSANOW,&oldstdtio);
      break;
    }
}
