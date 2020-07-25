/**********************结构体及函数分析***************************/

/***** i2c核心中的函数 *****/
/** 增加/删除i2c_adapter **/
int i2c_add_adapter(struct i2c_adapter *adap);
void i2c_del_adapter(struct i2c_adapter *adap);

/** i2c传输、发送和接收 **/
/* i2c_transfer()函数用于进行I2C适配器和I2C设备之间的一组消息交互，
 * 其中第2个参数是一个指向i2c_msg数组的指针，所以i2c_transfer()
 * 一次可以传输多个i2c_msg（考虑到很多外设的读写波形比较复杂，
 * 比如读寄存器可能要先写，所以需要两个以上的消息）。
 * 而对于时序比较简单的外设，i2c_master_send() 函数和i2c_master_recv()
 * 函数内部会调用i2c_transfer()函数分别完成一条写消息和一条读消息 */
/* i2c_transfer()函数本身不具备驱动适配器物理硬件以完成消息交互的能力，
 * 它只是寻找到与i2c_adapter对应的i2c_algorithm，并使用i2c_algorithm的master_xfer()
 * 函数真正驱动硬件流程 */
int i2c_transfer(struct i2c_adapter * adap, struct i2c_msg *msgs, int num);
int i2c_master_send(struct i2c_client *client,const char *buf ,int count);
int i2c_master_recv(struct i2c_client *client, char *buf ,int count);

/* 以START开头的i2c传输段 */
struct i2c_msg {
	/* 从机地址，7位或者10位。当10位时，I2C_M_TEN必须在flags中设置，
	 * 且该适配器必须支持I2C_FUNC_10BIT_ADDR*/
	__u16 addr;     /* slave address                        */
	/* I2C_M_RD由所有适配器处理。不得有其他标志除非适配器通过i2c_check_functionality()
	 * 导出相关的I2C_FUNC_*标志 */
	__u16 flags;
#define I2C_M_TEN               0x0010  /* this is a ten bit chip address */
#define I2C_M_RD                0x0001  /* read data, from slave to master */
#define I2C_M_STOP              0x8000  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NOSTART           0x4000  /* if I2C_FUNC_NOSTART */
#define I2C_M_REV_DIR_ADDR      0x2000  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK        0x1000  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NO_RD_ACK         0x0800  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_RECV_LEN          0x0400  /* length will be first received byte */
	/* buf中读取或写入的字节数 */
	__u16 len;              /* msg length                           */
	/* 读取或写入的buf */
	__u8 *buf;              /* pointer to msg data                  */
};

/**
 * 表示i2c传输方法的结构体
 *
 * 以下结构适用于那些希望实现新总线驱动程序的人员:
 * i2c_algorithm是一类可以使用相同的总线算法进行寻址的硬件解决方案的接口，
 * 如bit-banging或PCF8584是最常见的两个。
 * 
 * @master_xfer字段的返回码应指示传输期间发生的错误代码，
 * 如内核中Documentation/i2c/fault-codes文件中所述。
 */
struct i2c_algorithm {
	/* 1. 向给定的i2c adapter发送一组由msgs数组定义的i2c传输事务，
	 * 通过adap指定的适配器发送num条消息 */
	/* 2. 如果适配器算法无法执行i2c级的访问，设置master_xfer为NULL
	 * 如果适配器算法可以执行SMBus访问，则设置smbus_xfer，如果设为NULL，
	 * 则SMBus协议使用常用的i2c messages模拟 */
	/* 3.master_xfer成功时返回处理的消息数，失败时返回一个负值 */
/* I2C传输函数指针， I2C主机驱动的大部分工作也聚集在这里 */
	int (*master_xfer)(struct i2c_adapter *adap, struct i2c_msg *msgs,
			   int num);
	/* 将SMBus传输事务发送到给定的i2c adatpter.
	 * 如果这个调用不存在，则总线层将SMBus调用转换为i2c传输*/
	int (*smbus_xfer) (struct i2c_adapter *adap, u16 addr,
			   unsigned short flags, char read_write,
			   u8 command, int size, union i2c_smbus_data *data);

	/* 返回此算法/适配器对支持的标志，标志参见I2C_FUNC_* */
	/* 确定适配器支持什么 */
	u32 (*functionality) (struct i2c_adapter *);

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	/* 将给定的客户端注册到此适配器的slave模式 */
	int (*reg_slave)(struct i2c_client *client);
	/* 从此适配器的slave模式注销给定的客户端 */
	int (*unreg_slave)(struct i2c_client *client);
#endif
};

/* i2c_adapter是用于识别物理i2c总线以及访问它所必需的访问算法的结构。 */
struct i2c_adapter {
	struct module *owner;	/* 所有模块，设为THIS_MODULE即可 */
	unsigned int class;		  /* 该适配器支持的从设备的类型*/
	const struct i2c_algorithm *algo; /* 该适配器与从设备的通信算法结构体指针 */
	void *algo_data;	/* algorithm的私有数据 */

	/* data fields that are valid for all devices	*/
	struct rt_mutex bus_lock;	/* 实时互斥体 */

	int timeout;			/* 超时时间 in jiffies 默认1s */
	int retries;			/* 通讯重试次数 */
	struct device dev;		/* 该适配器对应的device */

	int nr;			/* id号，次设备号 */
	char name[48];	/* 适配器的名字 */
	struct completion dev_released;			/* 用于同步 */

	struct mutex userspace_clients_lock;
	struct list_head userspace_clients;		/* 用来挂接与该适配器匹配成功的从设备i2c_client的一个链表头 */

	struct i2c_bus_recovery_info *bus_recovery_info;
};
#define to_i2c_adapter(d) container_of(d, struct i2c_adapter, dev)

/**********************代码模板***************************/

struct xxx_i2c {
	spinlock_t lock;
	wait_queue_head_t wait;
	struct i2c_msg *msg;
	unsigned int msg_num;
	unsigned int msg_idx;
	unsigned int msg_ptr;
	...
	struct i2c_adapter adap;
};

static int xxx_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
		int num)
{
	...
	for (i = 0; i < num; i++) {
		i2c_adapter_xxx_start(); /* 产生开始位 */
		/*是读消息 */
		if (msgs[i]->flags &I2C_M_RD) {
			i2c_adapter_xxx_setaddr((msg->addr << 1) | 1); /* 发送从设备读地址 */
			i2c_adapter_xxx_wait_ack(); /* 获得从设备的ack */
			i2c_adapter_xxx_readbytes(msgs[i]->buf, msgs[i]->len); /* 读取msgs[i] ->len
																	长的数据到msgs[i]->buf */
		} else { /* 是写消息 */
			i2c_adapter_xxx_setaddr(msg->addr << 1); /* 发送从设备写地址 */
			i2c_adapter_xxx_wait_ack(); /* 获得从设备的ack */
			i2c_adapter_xxx_writebytes(msgs[i]->buf, msgs[i]->len); /* 读取msgs[i]->len
																	长的数据到msgs[i]->buf */
		}
	}
	i2c_adapter_xxx_stop(); /* 产生停止位 */
}

static u32 xxx_i2c_func(struct i2c_adapter *adap)
{
	/* 返回i2c的功能 */
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_NOSTART |
		I2C_FUNC_PROTOCOL_MANGLING;
}

static const struct i2c_algorithm xxx_i2c_algorithm = {
	.master_xfer		= xxx_i2c_xfer,
	.functionality		= xxx_i2c_func,
};

static int xxx_i2c_probe(struct platform_device *pdev)
{
	//struct i2c_adapter *adap;
	struct xxx_i2c *i2c;

	...
	xxx_adapter_hw_init(); /* 初始化i2c适配器硬件 */
	/* 需要为i2c申请空间，此处略去 */
	i2c->adap.owner = THIS_MODULE;
	i2c->adap.algo = &xxx_i2c_algorithm;	/* 算法函数 */
	i2c->adap.dev.parent = &pdev->dev;
	i2c->adap.dev.of_node = pdev->dev.of_node;

	rc = i2c_add_adapter(adap);
	...
}

static int xxx_i2c_remove(struct platform_device *pdev)
{
	struct xxx_i2c *i2c = platform_get_drvdata(pdev);

	...
	xxx_adapter_hw_free(); /* 与xxx_adapter_hw_init()相反的操作 */
	i2c_del_adapter(&i2c->adap);

	return 0;
}

static const struct of_device_id xxx_i2c_of_match[] = {
	{.compatible = "vendor,xxx_i2c",},
	{},
};
MODULE_DEVICE_TABLE(of, xxx_i2c_of_match);

static struct platform_driver xxx_i2c_driver = {
	.driver = {
		.name = "xxx_i2c",
		.owner = THIS_MODULE,
		.of_match_table = xxx_i2c_of_match,
	},
	.probe = xxx_i2c_probe,
	.remove = xxx_i2c_remove,
};
module_platform_driver(xxx_i2c_driver);
