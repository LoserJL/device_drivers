/********************** 说明 ***************************
 * 内核中的i2c_dev.c文件可以被看作是一个i2c设备驱动，不过它实现
 * 的i2c_client是虚拟、临时的，主要是为了便于从用户空间操作i2c
 * 外设。i2c_dev.c针对每个i2c适配器生成一个主设备号为89的设备文
 * 件，实现了i2c_driver的成员函数及文件操作接口，因此i2c_dev.c
 * 的主体是“i2c_driver成员函数+字符设备驱动”。
 * 
 * 以下代码就是利用该文件中实现的read()和write()读写i2c设备
******************************************************/

#include <stdio.h>
#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define BUFF_SIZE	32

int main(int argc, char **argv)
{
	unsigned int fd;
	unsigned short mem_addr;
	unsigned short size;
	unsigned short idx;
	char buf[BUFF_SIZE];
	char cswap;

	union {
		unsigned short addr;
		char bytes[2];
	} tmp;

	if (argc < 3) {
		printf("Use:\n%s /dev/i2c-x mem_addr size\n", argv[0]);
		return 0;
	}
	sscanf(argv[2], "%d", &mem_addr);
	sscanf(argv[3], "%d", &size);

	if (size > BUFF_SIZE)
		size = BUFF_SIZE;

	fd = open(argv[1], O_RDWR);

	if (!fd) {
		printf("Error on opening the device file\n");
		return 0;
	}

	ioctl(fd, I2C_SLAVE, 0x50); /* 设置EEPROM地址 */
	ioctl(fd, I2C_TIMEOUT, 1); /* 设置超时 */
	ioctl(fd, I2C_RETRIES, 1); /* 设置重试次数 */

	for (idx = 0; idx < size; ++idx, ++mem_addr) {
		tmp.addr = mem_addr;
		cswap = tmp.bytes[0];
		tmp.bytes[0] = tmp.bytes[1];
		tmp.bytes[1] = cswap;
		write(fd, &tmp.addr, 2);
		read(fd, &buf[idx], 1);
	}
	buf[size] = 0;
	close(fd);
	printf("Read %d char: %s\n", size, buf);
	return 0;
}