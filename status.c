#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <math.h>
#include <errno.h>

#include "lcd.h"

#define String_startsWith(s, match) (strstr((s), (match)) == (s))


static const char procstat[] = "/proc/stat";
static char serialPort[128] = "/dev/ttyUSB0";
static const char mountPath[] = "/";

static const int GB = 1024*1024*1024;
static char buffer[64];

struct CPUData_ {
   unsigned long long int totalTime;
   unsigned long long int userTime;

   unsigned long long int totalPeriod;
   unsigned long long int userPeriod;
};

struct diskData_ {
   unsigned long long disk_size;
   unsigned long long used;
   unsigned long long free;
};



static void diskSpace(int usbdev)
{
   struct statvfs sfs;
   struct diskData_ diskData;
   if (!statvfs(mountPath, &sfs)) {
      diskData.disk_size = (long long)sfs.f_blocks * sfs.f_bsize;
      diskData.free = (long long)sfs.f_bfree * sfs.f_bsize;
      diskData.used = (long long)diskData.disk_size - diskData.free;
   }

   sprintf(buffer, "\e[37mMount Path : \e[36m%s\n\r", mountPath);
   LCD_WriteData(usbdev, buffer);

   sprintf(buffer, "\e[33mdisk_size : \e[32m%.2lfGB\n\r",
           (double)diskData.disk_size/GB);
   LCD_WriteData(usbdev, buffer);
   sprintf(buffer, "\e[35mused : \e[32m%.2lfGB\n\r",
           (double)diskData.used/GB);
   LCD_WriteData(usbdev, buffer);
   sprintf(buffer, "\e[36mfree : \e[32m%.2lfGB",
           (double)diskData.free/GB);
   LCD_WriteData(usbdev, buffer);
}

void systemInfo(int fd)
{
   time_t t;
   struct utsname uts;

   time(&t);
   sprintf(buffer, "\e[35m%s\r", ctime(&t));
   LCD_WriteData(fd, buffer);
   uname(&uts);
   sprintf(buffer, "\e[37mOSname:\e[36m%s\n\r", uts.sysname);
   LCD_WriteData(fd, buffer);
   sprintf(buffer, "\e[37mVersion:\e[36m%s\n\r", uts.release);
   LCD_WriteData(fd, buffer);
   sprintf(buffer, "\e[37mMachine:\e[36m%s\n\r", uts.machine);
   LCD_WriteData(fd, buffer);
}

static int cpuCount(int cpus)
{
   FILE *file = fopen(procstat, "r");

   do {
      cpus++;
      fgets(buffer, 255, file);
   } while (String_startsWith(buffer, "cpu"));

   fclose(file);

   return cpus;
}

static struct CPUData_* cpuUsageInit(int cpus)
{
   struct CPUData_ *cpuData;
   int i;

   cpuData = (struct CPUData_ *) malloc(cpus * sizeof(struct CPUData_));

   for (i = 0; i < cpus; i++) {
      cpuData[i].totalTime = 1;
      cpuData[i].userTime = 0;
      cpuData[i].totalPeriod = 1;
      cpuData[i].userPeriod = 0;
   }

   return cpuData;
}

static void cpuUsageDisplay(int usbdev, struct CPUData_ *cpuData, int cpus)
{
   unsigned long long int usertime, nicetime, systemtime, systemalltime,
                 idlealltime, idletime, totaltime, virtalltime;
   unsigned long long int ioWait, irq, softIrq, steal, guest;
   double total = 0;
   FILE *file;
   int cpuid;
   int i;

   file = fopen(procstat, "r");
   ioWait = irq = softIrq = steal = guest = 0;

   for (i = 0; i < cpus; i++) {
      fgets(buffer, 255, file);
      if (i == 0) {
         sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                &usertime, &nicetime, &systemtime, &idletime,
                &ioWait, &irq, &softIrq, &steal, &guest);
      } else {
         sscanf(buffer, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                &cpuid, &usertime, &nicetime, &systemtime, &idletime,
                &ioWait, &irq, &softIrq, &steal, &guest);
         assert(cpuid == i - 1);
      }

      idlealltime = idletime + ioWait;
      systemalltime = systemtime + irq + softIrq;
      virtalltime = steal + guest;
      totaltime = usertime + nicetime + systemalltime +
         idlealltime + virtalltime;

      assert(usertime >= cpuData[i].userTime);
      assert(totaltime >= cpuData[i].totalTime);

      cpuData[i].userPeriod = usertime - cpuData[i].userTime;
      cpuData[i].totalPeriod = totaltime - cpuData[i].totalTime;

      cpuData[i].totalTime = totaltime;
      cpuData[i].userTime = usertime;

      total = (double)cpuData[i].totalPeriod;

      if ((i != 0) && (i%2 == 1)) {
         sprintf(buffer, "\e[33mcpu%d:\e[32m%4.1f%% ",
                 i, cpuData[i].userPeriod/total*100.0);
         LCD_WriteData(usbdev, buffer);
      } else if ((i != 0) && (i%2 == 0)) {
         sprintf(buffer, "\e[33mcpu%d:\e[32m%4.1f%%  \n\r",
                 i, cpuData[i].userPeriod/total*100.0);
         LCD_WriteData(usbdev, buffer);
      }
   }

   fclose(file);
}



int main(int argc, char **argv)
{
   struct CPUData_ *cpuData;
   int usbdev;
   int cpus = -1;

   if (argc == 2) {
      sprintf(serialPort, "%s", argv[1]);
   }

   usbdev = LCD_SerialSetup(serialPort);
   if (usbdev == -1) {
      return 1;
   }

   LCD_WriteData(usbdev, "\ec\e[2s\e[1r");
   cpus = cpuCount(cpus);
   cpuData = cpuUsageInit(cpus);

   while (1) {
      LCD_WriteData(usbdev, "\e[H");
      systemInfo(usbdev);
      cpuUsageDisplay(usbdev, cpuData, cpus);
      diskSpace(usbdev);
   }

   return 0;
}
