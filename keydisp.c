#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define GPIO_PHY_BASEADDR  0x3F200000
#define GPIO_AREA_SIZE	4096	// PAGE_SIZE
#define GPIO_GPFSEL0	0x0000	// for gpio 0..9, MSB 2bits are reserved
// omit GPFSEL1..GPFSEL5
#define GPIO_GPSET0	0x001C	// gpio 0..31
#define GPIO_GPSET1	0x0020	// gpio 32..53
#define GPIO_GPCLR0	0x0028	// gpio 0..31
#define GPIO_GPCLR1	0x002C	// gpio 32..53
#define GPIO_GPLEV0	0x0034	// gpio 0..31
#define GPIO_GPLEV1	0x0038	// gpio 32..53

extern void ledOnOff();
extern unsigned int memread(void *baseaddr, int offset);
extern void memwrite(void *baseaddr, int offset, unsigned int x);

extern int lcd_cmdwrite(int fd, unsigned char dat);
extern int lcd_datawrite(int fd, char dat[]);
extern void initLCD(int fd);
extern int location(int fd, int y);
extern int clear(int fd);

void *gpio_baseaddr;

int check = 0;
int action_red = 0;
int action_tea = 0;

void sig_handler(int signum)
{
	check = 1;
}

int main()
{
	int gpio_fd;
	int i2c_fd;
	int status;
	int status_red;
	int val;
	struct itimerval timval;
	timval.it_interval.tv_sec = 0;
	timval.it_interval.tv_usec = 0;
	timval.it_value.tv_sec = 1;
	timval.it_value.tv_usec = 0;

	struct itimerval timval_red;
	timval_red.it_interval.tv_sec = 0;
	timval_red.it_interval.tv_usec = 0;
	timval_red.it_value.tv_sec = 1;
	timval_red.it_value.tv_usec = 0;


	if (signal(SIGALRM,sig_handler) == SIG_ERR)
	{
		perror("signal Error ");
		exit(1);
	}

	gpio_fd = open("/dev/mem", O_RDWR);
	if (gpio_fd < 0)
	{
		perror("Error gpio open: ");
		exit(1);
	}
	i2c_fd = open("/dev/i2c-1", O_RDWR);
	if (i2c_fd < 0)
	{
		perror("Error i2c open: ");
		exit(1);
	}
	val = ioctl(i2c_fd, I2C_SLAVE,0x3e);
	if (val < 0)
	{
		perror("Error ioctl: ");
		exit(1);
	}
	initLCD(i2c_fd);
	gpio_baseaddr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, gpio_fd, GPIO_PHY_BASEADDR);
	unsigned int x = memread(gpio_baseaddr, GPIO_GPLEV0);
	status = (x >> 22) & 1;
	status_red = (x >> 23) & 1;
	while(1)
	{
		int status1;
		int status_red1;
		x = memread(gpio_baseaddr, GPIO_GPLEV0);
		status1 = (x >> 22) & 1;
		status_red1 = (x >> 23) & 1;
		if (check == 1 && action_tea == 1)
		{
			clear(i2c_fd);
			check = 0;
			action_tea = 0;
		}
		if (status == 1 && status1 == 0)
		{
			location(i2c_fd, 0);
			int val = lcd_datawrite(i2c_fd, "hello");
			action_tea = 1;
			if (val < 0)
			{
				perror("Error lcd_datawrite: ");
				exit(1);
			}
				
		}
		else if (status == 0 && status1 == 1)
		{
			setitimer(ITIMER_REAL, &timval, NULL);
		}
		status = status1;
		if (check == 1 && action_red == 1)
		{
			location(i2c_fd, 1);
			clear(i2c_fd);
			check = 0;
			action_red = 0;
		}
		if (status_red == 1 && status_red1 == 0)
		{
			location(i2c_fd, 1);
			int val_red = lcd_datawrite(i2c_fd, "world");
			action_red = 1;
			if (val_red < 0)
			{
				perror("Error lcd_datawrite: ");
				exit(1);
			}
				
		}
		else if (status_red == 0 && status_red1 == 1)
		{
			setitimer(ITIMER_REAL, &timval_red, NULL);
		}

	}
}	
