#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>

#include "lcd.h"

#define baudrate	B500000

static int verbose = 0;

#define LOG(_x) \
         if (verbose) { \
            printf(_x); \
         }

void LCD_SetVerbose(int v)
{
   verbose = v;
}


int LCD_SerialSetup(const char *devPath)
{
   struct termios options;
   int res;
   int fd;

   fd = open(devPath, O_RDWR | O_NOCTTY | O_NDELAY);
   if (fd == -1) {
      printf("failed to open '%s': %s\n", devPath, strerror(errno));
      return -1;
   }

   res = tcgetattr(fd, &options);
   if (res != 0) {
      printf("tcgetattr failed: %s\n", strerror(errno));
      return -1;
   }

   cfsetispeed(&options, baudrate);
   cfsetospeed(&options, baudrate);

   options.c_cflag |= CS8;
   options.c_iflag |= IGNBRK;
   options.c_iflag &= ~( BRKINT | ICRNL | IMAXBEL | IXON);
   options.c_oflag &= ~( OPOST | ONLCR );
   options.c_lflag &= ~( ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK |
                         ECHOCTL | ECHOKE);
   options.c_lflag |= NOFLSH;
   options.c_cflag &= ~CRTSCTS;

   res = tcsetattr(fd, TCSANOW, &options);
   if (res != 0) {
      printf("tcsetattr failed: %s\n", strerror(errno));
      return -1;
   }

   return fd;
}


int LCD_WriteData(int fd, const char *str)
{
   int isreset = strncmp(str, "\ec", 2) == 0;
   ssize_t res;
   char isbusy = 0;
   int length;

   LOG("Writing prelude..\n");
   res = write(fd, "\006", 1);
   if (res != 1) {
      printf("* failed to write: %zd (%s)\n", res, strerror(errno));
      return 1;
   }
   length = strlen(str) + 48;
   res = write(fd, &length, 1);
   if (res != 1) {
      printf("** failed to write: %zd (%s)\n", res, strerror(errno));
      return 1;
   }

   if (isreset) {
      LOG("Expecting reset..\n");
      while (1) {
         res = read(fd, &isbusy, 1);
         if (res != -1) {
            break;
         }
         usleep(10000);
      }
      LOG("Reset almost done.\n");
      while (isbusy != '6') {
         res = read(fd, &isbusy, 1);
         if (res == -1) {
            break;
         } else {
            //printf("reset: %d -- v=: 0x%x -- '%c'\n", res, isbusy, isbusy);
         }
      }
      LOG("Reset done.\n");
   } else {
      LOG("Waiting for non-busy status.\n");
      while (isbusy != '6') {
         res = read(fd, &isbusy, 1);
         if (res == -1) {
            usleep(10000);
         } else {
            //printf("res: %d -- v=: 0x%x -- '%c'\n", res, isbusy, isbusy);
         }
      }
   }

   LOG("Now writing..\n");
   res = write(fd, str, length - 48);
   if (res != length - 48) {
      printf("failed to write: %zd (%s)\n", res, strerror(errno));
      return 1;
   }
   return 0;
}


