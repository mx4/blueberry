#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>

#include "lcd.h"

static const char *defaultSerialPort = "/dev/ttyUSB0";

static void usage(void)
{
   printf("lcdprint: print on a LCD\n"
          "Options:\n"
          "  -d <devicePath>            : /dev/ttyUSB0 by default\n"
          "  -s \"messageToPrint\"      : the string to display\n"
          "  -v                         : enable verbose logging\n");
}


int main(int argc, char **argv)
{
   const char *devPath = defaultSerialPort;
   const char *str = "What's up?\r\n";
   int res;
   int fd;

   while (1) {
      int c = getopt(argc, argv, "vd:s:");

      if (c == EOF) {
         break;
      }
      switch (c) {
      case 'd': devPath = optarg;  break;
      case 's': str = optarg;      break;
      case 'v': LCD_SetVerbose(1); break;
      default:  usage();           break;
      }
   }

   fd = LCD_SerialSetup(devPath);
   if (fd == -1) {
      return 1;
   }

   printf("Resetting LCD\n");
   res = LCD_WriteData(fd, "\ec");
   if (res != 0) {
      goto exit;
   }

   printf("Printing '%s'\n", str);
   res = LCD_WriteData(fd, str);
   if (res != 0) {
      goto exit;
   }

exit:
   close(fd);

   return 0;
}
