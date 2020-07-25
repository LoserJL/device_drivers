/********************** 说明 ***************************
 * 内核中的i2c_dev.c文件可以被看作是一个i2c设备驱动，不过它实现
 * 的i2c_client是虚拟、临时的，主要是为了便于从用户空间操作i2c
 * 外设。i2c_dev.c针对每个i2c适配器生成一个主设备号为89的设备文
 * 件，实现了i2c_driver的成员函数及文件操作接口，因此i2c_dev.c
 * 的主体是“i2c_driver成员函数+字符设备驱动”。
 * 
 * 以下代码就是利用O_RDWR IOCTL读写i2c设备
******************************************************/

#include <stdio.h>
#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

int main(int argc, char **argv)
{
	struct i2c_rdwr_ioctl_data work_queue;
	unsigned int idx;
	unsigned int fd;
	unsigned int slave_address, reg_address;
	unsigned char val;
	int i;
	int ret;

	if (argc < 4) {
		printf("Use:\n%s /dev/i2c-x start_addr reg_addr\n", argv[0]);
		return 0;
	}

	fd = open(argv[1], O_RDWR);

	if (!fd) {
		printf("Error on opening the device file\n");
		return 0;
	}
	sscanf(argv[2], "%x", &slave_address);
	sscanf(argv[3], "%x", &reg_address);

	work_queue.nmsgs = 2; /* 设置超时 */
	work_queue.msgs = (struct i2c_msg *)malloc(work_queue.nmsgs *sizeof(struct i2c_msg));
	if (!work_queue.msgs) {
		printf("Memory alloc error\n");
		close(fd);
		return 0;
	}

	ioctl(fd, I2C_TIMEOUT, 2); /* 设置超时 */
	ioctl(fd, I2C_RETRIES, 1); /* 设置重试次数 */

	for (i = reg_address; i < reg_address + 16; i++) {
		val = i;
		(work_queue.msgs[0]).len = 1;
		(work_queue.msgs[0]).addr = slave_address;
		(work_queue.msgs[0]).buf = &val;

		(work_queue.msgs[1]).len = 1;
		(work_queue.msgs[1]).flags = I2C_M_RD;
		(work_queue.msgs[1]).addr = slave_address;
		(work_queue.msgs[1]).buf = &val;

		ret = ioctl(fd, I2C_RDWR, (unsigned long)&work_queue);
		if (ret < 0)
			printf("Error during I2C_RDWR ioctl with error code: %d\n", ret);
		printf("reg:%02x val:%02x\n", i, val);
	}

	close(fd);
	return 0;
}