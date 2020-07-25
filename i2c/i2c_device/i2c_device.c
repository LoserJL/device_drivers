/**********************结构体及函数分析***************************/

/***** i2c核心中的函数 *****/
/** 增加/删除i2c_driver **/
int i2c_register_driver(struct module *owner, struct i2c_driver *driver);
void i2c_del_driver(struct i2c_driver *driver);
#define i2c_add_driver(driver) \
		i2c_register_driver(THIS_MODULE, driver);

/** i2c传输 **/
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

/**
 * i2c设备驱动结构体
 *
 * driver.owner设为THIS_MODULE
 * driver.name是该驱动的名字
 * 
 * 对于自动检测设备，必须同时定义@detect和@address_list，还应设置@class，
 * 否则将仅创建强制带有模块参数的设备。检测功能必须至少填充成功检测到的
 * i2c_board_info结构的名称字段，以及标志字段。
 * 
 * 如果缺少@detect，驱动程序对于枚举设备仍然可以正常工作。检测到的设备将完全不受支持。
 * 对于许多无法可靠检测到的I2C/SMBus设备，以及在实践中始终可以枚举的设备，这是可以预期的。
 *
 * 传递给@detect回调的i2c_client结构不是真正的i2c_client。它已经初始化，因此您可以在其上
 * 调用i2c_smbus_read_byte_data系列函数。不要做任何其他事情。特别是，不允许在其上调用dev_dbg系列函数。
 */
struct i2c_driver {
	/* i2c设备的类型（用于检测） */
	unsigned int class;

	/* 有新总线添加的回调，通知驱动有新总线出现，不建议使用了，为了会被删除 */
	int (*attach_adapter)(struct i2c_adapter *) __deprecated;

	/* 应该使用这些标准接口 */
	int (*probe)(struct i2c_client *, const struct i2c_device_id *);
	int (*remove)(struct i2c_client *);

	/* driver model interfaces that don't relate to enumeration  */
	void (*shutdown)(struct i2c_client *);

	/* 警报的回调，如SMBus的警报协议 */
	void (*alert)(struct i2c_client *, unsigned int data);

	/* 类似于ioctl的命令，可用于执行特定功能 */
	int (*command)(struct i2c_client *client, unsigned int cmd, void *arg);

	/* 设备驱动 */
	struct device_driver driver;
	/* 该驱动支持的i2c设备列表 */
	const struct i2c_device_id *id_table;

	/* 设备检测回调，用于自动创建设备 */
	int (*detect)(struct i2c_client *, struct i2c_board_info *);
	/* 要探测的i2c地址（用于检测） */
	const unsigned short *address_list;
	/* 创建的检测到的客户端列表（仅适用于i2c-core） */
	struct list_head clients;
};
#define to_i2c_driver(d) container_of(d, struct i2c_driver, driver)

/**
 * I2C slave设备结构体
 * 
 * 一个i2c_client标识连接到i2c总线的一个个设备（即芯片）。
 */
struct i2c_client {
	/* I2C_CLIENT_TEN表示设备使用十位芯片地址； 
	 * I2C_CLIENT_PEC表示它使用SMBus数据包错误检查*/
	unsigned short flags;		/* div., see below		*/
	/* 连接到父适配器的I2C总线上使用的地址。7位地址为低7位 */
	unsigned short addr;		/* chip address - NOTE: 7bit	*/
					/* addresses are stored in the	*/
					/* _LOWER_ 7 bits		*/
	/* 名称 */
	char name[I2C_NAME_SIZE];
	/* 依附的i2c_adapter */
	struct i2c_adapter *adapter;	/* the adapter we sit on	*/
	/* slave设备 */
	struct device dev;		/* the device structure		*/
	/* 中断 */
	int irq;			/* irq issued by device		*/
	/* i2c_driver.clients列表或i2c-core的userspace_devices列表的成员 */
	struct list_head detected;
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	/* 使用适配器的I2C从模式时的回调。适配器调用它以将从属事件传递给从属驱动程序。 */
	i2c_slave_cb_t slave_cb;	/* callback for slave mode	*/
#endif
};
#define to_i2c_client(d) container_of(d, struct i2c_client, dev)

/**********************驱动模板***************************/

struct xxx_data {
	struct xxx_platform_data chip;
	...
	struct bin_attribute bin;
	...
};

static const struct i2c_device_id xxx_ids[] = {
	{"xxx", XXX_DEVICE_MAGIC(128 / 8, XXX_FLAG_TAKE8ADDR)},
	{}
};
MODULE_DEVICE_TABLE(i2c, xxx_ids);

staitc ssize_t xxx_eeprom_read(struct xxx_data *xxx, char *buf,
					unsigned offset, size_t count)
{
	struct i2c_msg msg[2];
	...
	i2c_transfer(client->adapter, msg, 2);
	...
}

static ssizr_t xxx_read(struct xxx_data *xxx,
					char *buf, loff_t off, size_t count)
{
	...
	status = xxx_eeprom_read(xxx, buf, off, count);
	...

	return retval;
}

static ssize_t xxx_bin_read(struct file *filp, struct kobject *kobj,
				struct bin_attribute *attr,
				char *buf, loff_t off, size_t count)
{
	struct xxx_data *xxx;

	xxx = dev_get_drvdata(container_of(kobj, struct device, kobj));
	return xxx_read(xxx, buf, off, count);
}

static int xxx_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	...
	sysfs_bin_attr_init(&xxx->bin); //以bin_attribute二进制sysfs节点形式呈现
	xxx->bin.attr.name = "eeprom";
	xxx->bin.attr.mode = chip.flags & XXX_FLAG_IRUGO ? S_IRUGO : S_IRUSR;
	xxx->bin.read = xxx_bin_read;
	xxx->bin.size = chip.byte_len;
	...

	return err;
}

static int xxx_remove(struct i2c_client *client)
{
	...
	sysfs_remove_bin_file(&client->dev.kobj, &xxx->bin);
	...

	return 0;
}

static struct i2c_driver xxx_driver = {
	.driver = {
		.name = "xxx",
		.owner = THIS_MODULE,
	},
	.probe = xxx_probe;
	.remove = xxx_remove;
	.id_table = xxx_ids,
};

static int __init xxx_init(void)
{
	...
	return i2c_add_driver(&xxx_driver);
}
module_init(xxx_init);

static void __exit xxx_exit(void)
{
	i2c_del_driver(&xxx_driver);
}
module_exit(xxx_exit);

MODULE_LICENSE("GPL v2");