#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyUSB1"
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

#include "tty.h"

void signal_handler_IO (int status);   /* definition of signal handler */
int wait_flag=TRUE;                    /* TRUE while no signal received */

/***************************************************************************
* signal handler. sets wait_flag to FALSE, to indicate above loop that     *
* characters have been received.                                           *
***************************************************************************/
void signal_handler_IO (int status) {
    //printf("received SIGIO signal.\n");
    wait_flag = FALSE;
}

int openTTYRaw(struct sigaction *saio, struct termios *oldtio, struct termios *newtio ) {
    /* open the device to be non-blocking (read will return immediately) */
    int fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd <0) { perror(MODEMDEVICE); exit(-1); }

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
    newtio->c_iflag = IGNPAR /*| ICRNL*/;
    newtio->c_oflag = 0;
    newtio->c_lflag = 0/*ICANON*/;
    newtio->c_cc[VMIN]=1;
    newtio->c_cc[VTIME]=0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,newtio);

    return fd;
}

int main() {
    printf("Raw Terminal Dump\n");
    int fd,c, res;
    struct sigaction saio;           /* definition of signal action */
    struct termios oldtio,newtio;
    char buf[255];

    fd = openTTYRaw(&saio, &oldtio, &newtio);

    int n;
    unsigned char key;
    struct termios initial_settings,
            new_settings;
    tcgetattr(0,&initial_settings);
    /*
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 0;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);*/

    /* loop while waiting for input. normally we would do something
       useful here */
    while (STOP==FALSE) {
        /* keyboard input */
        n = getchar();
        if(n != EOF) {
            key = n;
            if (key == 27)  {
                /* Escape key pressed */
                break;
            }
            /* do something useful here with key */
            printf("%c", key);
            write(fd,&key,1);
        }

        /* TTY input */
        /* after receiving SIGIO, wait_flag = FALSE, input is available
           and can be read */
        if (wait_flag==FALSE) {
            res = read(fd,buf,255);
            buf[res]=0;

            char asc[3];
            sprintf(asc,"%x", buf[0]);
            if (buf[0]==LF) sprintf(asc,"LF");
            if (buf[0]==CR) sprintf(asc,"CR");

            printf(":%s:%s:%d\n", buf, asc, res);
            wait_flag = TRUE;      /* wait for new input */
        }
    }
    /* restore old port settings */
    tcsetattr(fd,TCSANOW,&oldtio);
    tcsetattr(0, TCSANOW, &initial_settings);
}
