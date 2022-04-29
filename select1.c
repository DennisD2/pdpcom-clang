#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>

#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/signal.h>

#include "tty.h"

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyUSB0"
#define FALSE 0
#define TRUE 1

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

void signal_handler_IO (int status);   /* definition of signal handler */
int wait_flag=TRUE;                    /* TRUE while no signal received */

/***************************************************************************
* signal handler. sets wait_flag to FALSE, to indicate above loop that     *
* characters have been received.                                           *
***************************************************************************/
void signal_handler_IO (int status)
{
    //printf("received SIGIO signal.\n");
    wait_flag = FALSE;
}

int handle_tty_input(int fd){
    char buf[255];
    int res;
    res = read(fd,buf,255);
    buf[res]=0;
    char asc[3];
    sprintf(asc,"%x", buf[0]);
    if (buf[0]==LF) sprintf(asc,"LF");
    if (buf[0]==CR) sprintf(asc,"CR");

    //printf(":%s:%s:%d\n", buf, asc, res);
    putchar(buf[0]);
    fflush(stdin->_fileno);
}

int openTTYRaw(struct sigaction *saio, struct termios *oldtio, struct termios *newtio ) {
    /* open the device to be non-blocking (read will return immediatly) */
    int fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd <0) {perror(MODEMDEVICE); exit(-1); }

    /* install the signal handler before making the device asynchronous */
    saio->sa_handler = signal_handler_IO;

    sigset_t sigset;
    sigemptyset(&sigset);
    saio->sa_mask = sigset;
    saio->sa_flags = 0;
    saio->sa_restorer = NULL;
    sigaction(SIGIO,saio,NULL);

    /* allow the process to receive SIGIO */
    fcntl(fd, F_SETOWN, getpid());
    /* Make the file descriptor asynchronous (the manual page says only
       O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
    fcntl(fd, F_SETFL, FASYNC);

    tcgetattr(fd,oldtio); /* save current port settings */
    /* set new port settings for canonical input processing */
    newtio->c_cflag = BAUDRATE /*| CRTSCTS*/ | CS8 | CLOCAL | CREAD;
    newtio->c_iflag = IGNPAR;
    newtio->c_lflag = 0/*ICANON*/;
    newtio->c_lflag = 0;
    newtio->c_cc[VMIN]=1;
    newtio->c_cc[VTIME]=0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,newtio);

    return fd;
}

main()
{
    int    ttyfd;  /* input sources 1 and 2 */
    fd_set readfs;    /* file descriptor set */
    int    maxfd;     /* maximum file desciptor used */
    int    loop=1;    /* loop while TRUE */

    /*-------------------------------------------*/
    int c, res;
    struct termios oldtio,newtio;
    struct sigaction saio;           /* definition of signal action */

    printf("select() Terminal Dump v1\n");

    ttyfd = openTTYRaw(&saio, &oldtio, &newtio);

    /* open_input_source opens a device, sets the port correctly, and
       returns a file descriptor */
    //ttyfd = open_input_source("/dev/ttyUSB");   /* COM2 */
    if (ttyfd<0) exit(0);
    //fd2 = open_input_source("/dev/ttyS2");   /* COM3 */
    maxfd = max (ttyfd, ttyfd)+1;  /* maximum bit entry (fd) to test */

    /* loop for input */
    struct timeval Timeout;
    while (loop) {
        FD_SET(ttyfd, &readfs);  /* set testing for source 1 */
        /* set timeout value within input loop */
        Timeout.tv_usec = 50000;  /* microseconds */
        Timeout.tv_sec  = 0;  /* seconds */
        select(maxfd, &readfs, NULL, NULL, &Timeout);
        if (FD_ISSET(ttyfd,&readfs)) {
            /* input from TTY */
            handle_tty_input(ttyfd);
        }
    }
}
