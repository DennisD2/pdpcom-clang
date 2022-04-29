#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>

#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/signal.h>

#include "tty.h"

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyUSB1"
#define FALSE 0
#define TRUE 1

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

void signal_handler_IO (int status);   /* definition of signal handler */
//int wait_flag=TRUE;                    /* TRUE while no signal received */

/***************************************************************************
* signal handler. sets wait_flag to FALSE, to indicate above loop that     *
* characters have been received.                                           *
***************************************************************************/
void signal_handler_IO (int status)
{
    //printf("received SIGIO signal.\n");
    //wait_flag = FALSE;
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
    /* open the device to be non-blocking (read will return immediately) */
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

void setKbdMode(int dir) {
    static struct termios oldt, newt;

    if ( dir == 1 ) {
        tcgetattr( STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_iflag = IGNPAR;
        newt.c_lflag &= ~( ICANON | ECHO );
        newt.c_cc[VMIN]=1;
        newt.c_cc[VTIME]=0;
        tcsetattr( STDIN_FILENO, TCSANOW, &newt);
    }
    else
        tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}

int handle_kbd_input(int ttyfd){
    int ch = getchar();
    //printf("%c<%x>", ch, ch);
    char c = ch;
    write(ttyfd,&c,1);
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
    if (ttyfd<0) exit(0);

    setKbdMode(1);

    int kbdfd = STDIN_FILENO ; // STDIN
    maxfd = max (ttyfd, kbdfd)+1;  /* maximum bit entry (fd) to test */

    /* loop for input */
    struct timeval timeout;
    while (loop) {
        FD_ZERO(&readfs);
        FD_SET(ttyfd, &readfs);  /* set testing for source 1 */
        /* set timeout value within input loop */
        timeout.tv_usec = 0;  /* microseconds */
        timeout.tv_sec  = 0;  /* seconds */
        select(maxfd, &readfs, NULL, NULL, &timeout);
        if (FD_ISSET(ttyfd,&readfs)) {
            /* input from TTY */
            handle_tty_input(ttyfd);
        }

        timeout.tv_usec = 0;
        timeout.tv_sec  = 0;
        FD_ZERO(&readfs);
        FD_SET(kbdfd, &readfs);
        select(STDIN_FILENO+1, &readfs, NULL, NULL, &timeout);
        if (FD_ISSET(kbdfd,&readfs)) {
            /* input from KBD */
            handle_kbd_input(ttyfd);
        }
    }
}
