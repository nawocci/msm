/*
 * block2mtd.c - create an mtd from a block device
 *
 * Copyright (C) 2001,2002	Simon Evans <spse@secret.org.uk>
 * Copyright (C) 2004-2006	Joern Engel <joern@wh.fh-wedel.de>
 *
 * Licence: GPL
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/*
 * When the first attempt at device initialization fails, we may need to
 * wait a little bit and retry. This timeout, by default 3 seconds, gives
 * device time to start up. Required on BCM2708 and a few other chipsets.
 */
#define MTD_DEFAULT_TIMEOUT	5

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/backing-dev.h>
#include <linux/bio.h>
#include <linux/pagemap.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/mtd/mtd.h>
#include <linux/mutex.h>
#include <linux/mount.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <linux/atomic.h>
#include <linux/ctype.h>
#include <linux/export.h>
#include <linux/kexec.h>
#include <linux/kmod.h>
#include <linux/kmsg_dump.h>
#include <linux/reboot.h>
#include <linux/suspend.h>
#include <linux/syscalls.h>
#include <linux/syscore_ops.h>
#include <linux/uaccess.h>

/* Maximum number of comma-separated items in the 'block2mtd=' parameter */
#define BLOCK2MTD_PARAM_MAX_COUNT 3

/* Info for the block device */
struct block2mtd_dev {
	struct list_head list;
	struct block_device *blkdev;
	struct mtd_info mtd;
	struct mutex write_mutex;
};


/* Static info about the MTD, used in cleanup_module */
static LIST_HEAD(blkmtd_device_list);

// Mi ADD
static int is_first_init = 1;
struct device_init_info {
    char* devname;
    int erase_size;
    size_t label;
    int timeout;
};

static struct device_init_info* last_init_info = NULL;
static struct task_struct *retry_thread = NULL;
static char* block2mtd_device_name = NULL;

static struct page *page_read(struct address_space *mapping, pgoff_t index)
{
	return read_mapping_page(mapping, index, NULL);
}

/* erase a specified part of the device */
static int _block2mtd_erase(struct block2mtd_dev *dev, loff_t to, size_t len)
{
	struct address_space *mapping = dev->blkdev->bd_inode->i_mapping;
	struct page *page;
	pgoff_t index = to >> PAGE_SHIFT;	// page index
	int pages = len >> PAGE_SHIFT;
	u_long *p;
	u_long *max;

	while (pages) {
		page = page_read(mapping, index);
		if (IS_ERR(page))
			return PTR_ERR(page);

		max = page_address(page) + PAGE_SIZE;
		for (p=page_address(page); p<max; p++)
			if (*p != -1UL) {
				lock_page(page);
				memset(page_address(page), 0xff, PAGE_SIZE);
				set_page_dirty(page);
				unlock_page(page);
				balance_dirty_pages_ratelimited(mapping);
				break;
			}

		put_page(page);
		pages--;
		index++;
	}
	return 0;
}
static int block2mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct block2mtd_dev *dev = mtd->priv;
	size_t from = instr->addr;
	size_t len = instr->len;
	int err;

	mutex_lock(&dev->write_mutex);
	err = _block2mtd_erase(dev, from, len);
	mutex_unlock(&dev->write_mutex);
	if (err)
		pr_err("erase failed err = %d\n", err);

	return err;
}


static int block2mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u_char *buf)
{
	struct block2mtd_dev *dev = mtd->priv;
	struct page *page;
	pgoff_t index = from >> PAGE_SHIFT;
	int offset = from & (PAGE_SIZE-1);
	int cpylen;

	while (len) {
		if ((offset + len) > PAGE_SIZE)
			cpylen = PAGE_SIZE - offset;	// multiple pages
		else
			cpylen = len;	// this page
		len = len - cpylen;

		page = page_read(dev->blkdev->bd_inode->i_mapping, index);
		if (IS_ERR(page))
			return PTR_ERR(page);

		memcpy(buf, page_address(page) + offset, cpylen);
		put_page(page);

		if (retlen)
			*retlen += cpylen;
		buf += cpylen;
		offset = 0;
		index++;
	}
	return 0;
}


/* write data to the underlying device */
static int _block2mtd_write(struct block2mtd_dev *dev, const u_char *buf,
		loff_t to, size_t len, size_t *retlen)
{
	struct page *page;
	struct address_space *mapping = dev->blkdev->bd_inode->i_mapping;
	pgoff_t index = to >> PAGE_SHIFT;	// page index
	int offset = to & ~PAGE_MASK;	// page offset
	int cpylen;

	while (len) {
		if ((offset+len) > PAGE_SIZE)
			cpylen = PAGE_SIZE - offset;	// multiple pages
		else
			cpylen = len;			// this page
		len = len - cpylen;

		page = page_read(mapping, index);
		if (IS_ERR(page))
			return PTR_ERR(page);

		if (memcmp(page_address(page)+offset, buf, cpylen)) {
			lock_page(page);
			memcpy(page_address(page) + offset, buf, cpylen);
			set_page_dirty(page);
			unlock_page(page);
			balance_dirty_pages_ratelimited(mapping);
		}
		put_page(page);

		if (retlen)
			*retlen += cpylen;

		buf += cpylen;
		offset = 0;
		index++;
	}
	return 0;
}


static int block2mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	struct block2mtd_dev *dev = mtd->priv;
	int err;

	mutex_lock(&dev->write_mutex);
	err = _block2mtd_write(dev, buf, to, len, retlen);
	mutex_unlock(&dev->write_mutex);
	if (err > 0)
		err = 0;

	dev->mtd._sync(mtd);
	return err;
}


/* sync the device - wait until the write queue is empty */
static void block2mtd_sync(struct mtd_info *mtd)
{
	struct block2mtd_dev *dev = mtd->priv;
	sync_blockdev(dev->blkdev);
	return;
}


static void block2mtd_free_device(struct block2mtd_dev *dev)
{
	if (!dev)
		return;

	kfree(dev->mtd.name);

	if (dev->blkdev) {
		invalidate_mapping_pages(dev->blkdev->bd_inode->i_mapping,
					0, -1);
		blkdev_put(dev->blkdev, NULL);
	}

	kfree(dev);
}


/*
 * This function is marked __ref because it calls the __init marked
 * early_lookup_bdev when called from the early boot code.
 */
static struct block_device __ref *mdtblock_early_get_bdev(const char *devname,
		blk_mode_t mode, int timeout, struct block2mtd_dev *dev)
{
	struct block_device *bdev = ERR_PTR(-ENODEV);
#ifndef MODULE
	int i;

	/*
	 * We can't use early_lookup_bdev from a running system.
	 */
	if (system_state >= SYSTEM_RUNNING)
		return bdev;

	/*
	 * We might not have the root device mounted at this point.
	 * Try to resolve the device name by other means.
	 */
	for (i = 0; i <= timeout; i++) {
		dev_t devt;

		if (i)
			/*
			 * Calling wait_for_device_probe in the first loop
			 * was not enough, sleep for a bit in subsequent
			 * go-arounds.
			 */
			msleep(1000);
		wait_for_device_probe();

		if (!early_lookup_bdev(devname, &devt)) {
			bdev = blkdev_get_by_dev(devt, mode, dev, NULL);
			if (!IS_ERR(bdev))
				break;
		}
	}
#endif
	return bdev;
}

static int retry_add_device_thread_func(void* data);

static struct block2mtd_dev *add_device(char *devname, int erase_size,
		size_t label, int timeout)
{
	const blk_mode_t mode = BLK_OPEN_READ | BLK_OPEN_WRITE;
	struct block_device *bdev;
	struct block2mtd_dev *dev;
	char *name;
	int i;

	if (!devname)
		return NULL;

	dev = kzalloc(sizeof(struct block2mtd_dev), GFP_KERNEL);
	if (!dev)
		return NULL;

	/* Get a handle on the device */
	bdev = blkdev_get_by_path(devname, mode, dev, NULL);

	if(is_first_init == 0 && label == 1) {
		pr_err("label = %zu: boot mode is recovery\n", label);
		for (i = 0; IS_ERR(bdev) && i <= timeout; i++) {
			if (i)
				/*
				* Calling wait_for_device_probe in the first loop
				* was not enough, sleep for a bit in subsequent
				* go-arounds.
				*/
				msleep(2000);
			wait_for_device_probe();

			pr_err("i %d, devname is %s\n", i, devname);
			bdev = blkdev_get_by_path(devname, mode, dev, NULL);
		}
	}

	if (IS_ERR(bdev))
		bdev = mdtblock_early_get_bdev(devname, mode, timeout, dev);
	if (IS_ERR(bdev)) {
		pr_err("error: cannot open device %s\n", devname);
		goto err_free_block2mtd;
	}
	dev->blkdev = bdev;

	if (MAJOR(bdev->bd_dev) == MTD_BLOCK_MAJOR) {
		pr_err("attempting to use an MTD device as a block device\n");
		goto err_free_block2mtd;
	}

	if ((long)dev->blkdev->bd_inode->i_size % erase_size) {
		pr_err("erasesize must be a divisor of device size\n");
		goto err_free_block2mtd;
	}

	mutex_init(&dev->write_mutex);

	name = kasprintf(GFP_KERNEL, "block2mtd: %s", devname);
	if (!name)
		goto err_destroy_mutex;

	dev->mtd.name = name;

	dev->mtd.size = dev->blkdev->bd_inode->i_size & PAGE_MASK;
	dev->mtd.erasesize = erase_size;
	dev->mtd.writesize = 1;
	dev->mtd.writebufsize = PAGE_SIZE;
	dev->mtd.type = MTD_RAM;
	dev->mtd.flags = MTD_CAP_RAM;
	dev->mtd._erase = block2mtd_erase;
	dev->mtd._write = block2mtd_write;
	dev->mtd._sync = block2mtd_sync;
	dev->mtd._read = block2mtd_read;
	dev->mtd.priv = dev;
	dev->mtd.owner = THIS_MODULE;

	if (mtd_device_register(&dev->mtd, NULL, 0)) {
		/* Device didn't get added, so free the entry */
		goto err_destroy_mutex;
	}

	list_add(&dev->list, &blkmtd_device_list);
	pr_info("mtd%d: [%s] erase_size = %dKiB [%d]\n",
		dev->mtd.index,
		dev->mtd.name + strlen("block2mtd: "),
		dev->mtd.erasesize >> 10, dev->mtd.erasesize);
	return dev;

err_destroy_mutex:
	mutex_destroy(&dev->write_mutex);
err_free_block2mtd:
	block2mtd_free_device(dev);
	if (is_first_init == 1 && label == 1 && block2mtd_device_name != NULL) {
        is_first_init = 0;
        last_init_info = kzalloc(sizeof(struct device_init_info), GFP_KERNEL);
        last_init_info->devname = block2mtd_device_name;
        last_init_info->erase_size = erase_size;
        last_init_info->label = label;
        last_init_info->timeout = timeout;
        pr_err("devname is %s, erase_size is %d, label is %zu, timeout is %d\n", block2mtd_device_name, erase_size, label, timeout);
        retry_thread = kthread_run(retry_add_device_thread_func, NULL, "block2mtd_retry");
    }
	return NULL;
}

static int retry_add_device_thread_func(void* data) {
    int retry_count = 2;
    while (retry_count > 0 && last_init_info != NULL) {
        if (add_device(last_init_info->devname, last_init_info->erase_size,
                    last_init_info->label, last_init_info->timeout) != NULL) {
            pr_err("devname is %s, erase_size is %d, label is %zu, timeout is %d\n", last_init_info->devname, last_init_info->erase_size, last_init_info->label, last_init_info->timeout);
            if (last_init_info != NULL)
                kfree(last_init_info);
            if (block2mtd_device_name != NULL)
                kfree(block2mtd_device_name);
            last_init_info = NULL;
            block2mtd_device_name = NULL;
			break;
        } else {
            pr_err("sleep retry_count is %d", retry_count);
            msleep(2000);
        }
        retry_count--;
    }
    if (last_init_info != NULL)
        kfree(last_init_info);
    if (block2mtd_device_name != NULL)
        kfree(block2mtd_device_name);
    return 0;
}

/* This function works similar to reguler strtoul.  In addition, it
 * allows some suffixes for a more human-readable number format:
 * ki, Ki, kiB, KiB	- multiply result with 1024
 * Mi, MiB		- multiply result with 1024^2
 * Gi, GiB		- multiply result with 1024^3
 */
static int ustrtoul(const char *cp, char **endp, unsigned int base)
{
	unsigned long result = simple_strtoul(cp, endp, base);
	switch (**endp) {
	case 'G' :
		result *= 1024;
		fallthrough;
	case 'M':
		result *= 1024;
		fallthrough;
	case 'K':
	case 'k':
		result *= 1024;
	/* By dwmw2 editorial decree, "ki", "Mi" or "Gi" are to be used. */
		if ((*endp)[1] == 'i') {
			if ((*endp)[2] == 'B')
				(*endp) += 3;
			else
				(*endp) += 2;
		}
	}
	return result;
}


static int parse_num(size_t *num, const char *token)
{
	char *endp;
	size_t n;

	n = (size_t) ustrtoul(token, &endp, 0);
	if (*endp)
		return -EINVAL;

	*num = n;
	return 0;
}


static inline void kill_final_newline(char *str)
{
	char *newline = strrchr(str, '\n');
	if (newline && !newline[1])
		*newline = 0;
}


#ifndef MODULE
static int block2mtd_init_called = 0;
/* 80 for device, 12 for erase size */
static char block2mtd_paramline[80 + 12];
#endif

static int block2mtd_setup2(const char *val)
{
	/* 80 for device, 12 for erase size, 80 for name, 8 for timeout */
	char buf[80 + 12 + 80 + 8];
	char *str = buf;
	char *token[BLOCK2MTD_PARAM_MAX_COUNT];
	char *name;
	size_t label = 0;
	size_t length = 32;
	size_t erase_size = PAGE_SIZE;
	unsigned long timeout = MTD_DEFAULT_TIMEOUT;
	int i, ret;

	if (strnlen(val, sizeof(buf)) >= sizeof(buf)) {
		pr_err("parameter too long\n");
		return 0;
	}

	strcpy(str, val);
	kill_final_newline(str);

	for (i = 0; i < BLOCK2MTD_PARAM_MAX_COUNT; i++)
		token[i] = strsep(&str, ",");

	if (str) {
		pr_err("too many arguments\n");
		return 0;
	}

	if (!token[0]) {
		pr_err("no argument\n");
		return 0;
	}

	name = token[0];
	if (strlen(name) + 1 > 80) {
		pr_err("device name too long\n");
		return 0;
	}

	/* Optional argument when custom label is used */
	if (token[1] && strlen(token[1])) {
		ret = parse_num(&erase_size, token[1]);
		if (ret) {
			pr_err("illegal erase size\n");
			return 0;
		}
	}

	if (token[2] && strlen(token[2])) {
		ret = parse_num(&label, token[2]);
		if (ret) {
			pr_err("illegal label\n");
			return 0;
		}
		pr_err("label is %zu\n", label);
	}

	if (label == 1) {
		pr_err("block2mtd_setup2: mode is recovery\n");
		block2mtd_device_name = (char *)kzalloc(length * sizeof(char), GFP_KERNEL);
		if(block2mtd_device_name == NULL) {
			pr_err("block2mtd_device_name kzalloc error\n");
		}
		strncpy(block2mtd_device_name, name, length - 1);
	}

	add_device(name, erase_size, label, timeout);

	return 0;
}


static int block2mtd_setup(const char *val, const struct kernel_param *kp)
{
#ifdef MODULE
	return block2mtd_setup2(val);
#else
	/* If more parameters are later passed in via
	   /sys/module/block2mtd/parameters/block2mtd
	   and block2mtd_init() has already been called,
	   we can parse the argument now. */

	if (block2mtd_init_called)
		return block2mtd_setup2(val);

	/* During early boot stage, we only save the parameters
	   here. We must parse them later: if the param passed
	   from kernel boot command line, block2mtd_setup() is
	   called so early that it is not possible to resolve
	   the device (even kmalloc() fails). Deter that work to
	   block2mtd_setup2(). */

	strscpy(block2mtd_paramline, val, sizeof(block2mtd_paramline));

	return 0;
#endif
}


module_param_call(block2mtd, block2mtd_setup, NULL, NULL, 0200);
MODULE_PARM_DESC(block2mtd, "Device to use. \"block2mtd=<dev>[,[<erasesize>][,<label>]]\"");

static int __init block2mtd_init(void)
{
	int ret = 0;

#ifndef MODULE
	if (strlen(block2mtd_paramline))
		ret = block2mtd_setup2(block2mtd_paramline);
	block2mtd_init_called = 1;
#endif

	return ret;
}


static void block2mtd_exit(void)
{
	struct list_head *pos, *next;

	/* Remove the MTD devices */
	list_for_each_safe(pos, next, &blkmtd_device_list) {
		struct block2mtd_dev *dev = list_entry(pos, typeof(*dev), list);
		block2mtd_sync(&dev->mtd);
		mtd_device_unregister(&dev->mtd);
		mutex_destroy(&dev->write_mutex);
		pr_info("mtd%d: [%s] removed\n",
			dev->mtd.index,
			dev->mtd.name + strlen("block2mtd: "));
		list_del(&dev->list);
		block2mtd_free_device(dev);
	}
}

late_initcall(block2mtd_init);
module_exit(block2mtd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joern Engel <joern@lazybastard.org>");
MODULE_DESCRIPTION("Emulate an MTD using a block device");
