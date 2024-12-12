#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/vfs.h>
#include <unistd.h>
#include "ssd1306_i2c.h"

#define startsWith(s, match) (strstr((s), (match)) == (s))

typedef struct MemInfo{
unsigned long long totalMem;
unsigned long long freeMem;
}MemInfo;

int main(void) {
	int fd_temp;
	long int user, nice, sys, idle, iowait, irq, softirq, total_1, total_2, idle_1, idle_2;
	unsigned long totalRam, freeRam;
	unsigned long long totalBlocks, totalSize, freeDisk;
	float usage;
	double temp = 0;
	char buf[32], buffer[128], buf_cpu[128], cpu[8], CpuInfo[32] = {0}, CpuTemp[32], RamInfo[32], DiskInfo[32], IpInfo[32], str_ip[INET_ADDRSTRLEN] = {0};
	size_t mbTotalsize, mbFreedisk;
	FILE *fp_CpuUsage;
	FILE *fp_MemUsage;
	struct sysinfo sys_info;
	struct statfs disk_info;
	struct ifaddrs *ifAddrStruct = NULL;
	void *tmpAddrPtr = NULL;
	
	ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
	sprintf(IpInfo, "eth0:0.0.0.0");

	// char buf_model[64], fd_hardware_num, fd_hardware_model, fd_hardware_model_plus[4], fd_hardware_rev[4], Model[64], rev[4];
	// FILE * fp_Model;
	// fp_Model = fopen("/proc/device-tree/model", "r");
	// fgets(buf_model, 64, fp_Model);
	// fclose(fp_Model);
	// sscanf(buf_model,"Raspberry Pi %c Model %c %s %s %s", &fd_hardware_num, &fd_hardware_model, &fd_hardware_model_plus, &rev, &fd_hardware_rev);
	// if(fd_hardware_model_plus[0] == 'P') sprintf(Model, "Raspberry Pi %c%c+ v%s", fd_hardware_num, fd_hardware_model, fd_hardware_rev);
	// else sprintf(Model, "Raspberry Pi %c%c v%s", fd_hardware_num, fd_hardware_model, rev);
	
	while (1) {
		if (sysinfo(&sys_info) != 0) {
			printf("sysinfo-Error\n");
			ssd1306_clearDisplay();
			char *text = "sysinfo-Error";
			ssd1306_drawString(text);
			ssd1306_display();
			continue;
		}
		else {
			ssd1306_clearDisplay();
			
			fp_CpuUsage = fopen("/proc/stat", "r");
			if (fp_CpuUsage == NULL) printf("failed to open /proc/stat\n");
			else {
				fgets(buf_cpu, sizeof(buf_cpu), fp_CpuUsage);
				sscanf(buf_cpu,"%s%d%d%d%d%d%d%d",cpu,&user,&nice,&sys,&idle,&iowait,&irq,&softirq);
				total_1 = user + nice + sys + idle + iowait + irq + softirq;
				idle_1 = idle;
				rewind(fp_CpuUsage);
				sleep(1);
				memset(buf_cpu, 0, sizeof(buf_cpu));
				cpu[0] = '\0';
				user = nice = sys = idle = iowait = softirq = 0;
				fgets(buf_cpu, sizeof(buf_cpu), fp_CpuUsage);
				sscanf(buf_cpu,"%s%d%d%d%d%d%d%d",cpu,&user,&nice,&sys,&idle,&iowait,&irq,&softirq);
				total_2 = user + nice + sys + idle + iowait + irq + softirq;
				idle_2 = idle;
				usage = (float)(total_2-total_1-(idle_2-idle_1)) / (total_2-total_1)*100;
				sprintf(CpuInfo, "CPU:%.0f%%", usage);
				fclose(fp_CpuUsage);
			}
			
			totalRam = sys_info.totalram >> 20;
			fp_MemUsage = fopen("/proc/meminfo", "r");
			while(fgets(buffer, 128, fp_MemUsage)) sscanf(buffer, "MemAvailable: %llu kB", &freeRam);
			fflush(fp_MemUsage);
			rewind(fp_MemUsage);
			sprintf(RamInfo, "RAM:%ld/%ldMB", totalRam - (freeRam / 1024), totalRam);
			fclose(fp_MemUsage);
			
			fd_temp = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY);
			if (fd_temp < 0) {
				temp = 0;
				printf("failed to open thermal_zone0/temp\n");
			}
			else {
				if (read(fd_temp, buf, 32) < 0) {
					temp = 0;
					printf("failed to read temp\n");
				}
				else {
					temp = atoi(buf) / 1000.0;
					sprintf(CpuTemp, "Temp:%.1fC", temp);
				}
			}
			close(fd_temp);
			
			statfs("/", &disk_info);
			totalBlocks = disk_info.f_bsize;
			totalSize = totalBlocks * disk_info.f_blocks;
			mbTotalsize = totalSize >> 20;
			freeDisk = disk_info.f_bfree * totalBlocks;
			mbFreedisk = freeDisk >> 20;
			sprintf(DiskInfo, "Disk:%ld/%ldMB", mbTotalsize - mbFreedisk, mbTotalsize);
			
			getifaddrs(&ifAddrStruct);
			while (ifAddrStruct != NULL) {
				if (ifAddrStruct->ifa_addr->sa_family == AF_INET) {
					tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
					inet_ntop(AF_INET, tmpAddrPtr, str_ip, INET_ADDRSTRLEN);
					if (strcmp(ifAddrStruct->ifa_name, "eth0") == 0) {
						sprintf(IpInfo, "eth0:%s", str_ip);
						break;
					}
					else if (strcmp(ifAddrStruct->ifa_name, "wlan0") == 0) {
						sprintf(IpInfo, "wlan0:%s", str_ip);
						break;
					}
				}
				ifAddrStruct = ifAddrStruct->ifa_next;
			}
			
			// ssd1306_drawText(0, 0, Model);
			ssd1306_drawText(0, 0, CpuInfo);
			ssd1306_drawText(56, 0, CpuTemp);
			ssd1306_drawText(0, 8, RamInfo);
			ssd1306_drawText(0, 16, DiskInfo);
			ssd1306_drawText(0, 24, IpInfo);
			ssd1306_display();
			delay(10);
		}
	}
	return 0;
}

