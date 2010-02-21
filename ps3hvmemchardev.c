/*
 * based on http://tldp.org/LDP/lkmpg/2.6/html/x710.html
 *  and http://www.ps3news.com/forums/ps3-hacks/ps3-hypervisor-bootloader-dumped-ram-more-109794.html#post284289
 */

/*
 * Contributors :
 *   Darío Clavijo for this minors modifications
 *   Youness Alaoui - For some code
 *   George Hotz (geohot) - For the lv1_peek function
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <asm/uaccess.h>  /* for put_user */

/*  
 *  Prototypes - this would normally go in a .h file
 */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "ps3hvmem"	/* Dev name as it appears in /proc/devices   */
/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open?  
				 * Used to prevent multiple access to device */

/*
struct class * myclass;
*/

char *kbuffer;

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

#define MIN(a, b) (a < b?a:b)


/* lv1_peek function written by geohot */
volatile long lv1_peek(unsigned long real_addr) {
  unsigned long ret;
  asm volatile("mr 3, %1\n"
               "li 11, 16\n"
               "sc 1\n"
               "mr %0, 3\n"
               : "=r" (ret)
               : "r" (real_addr)
               : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12");
  return ret;
}


/*
 * This function is called when the module is loaded
 */

int init_module()
{
	/*
	struct device *err_dev;
	*/
        Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
	  printk(KERN_ALERT "Registering char device failed with %d\n", Major);
	  return Major;
	}

	/*myclass = class_create(THIS_MODULE,DEVICE_NAME);
        err_dev = device_create(myclass, NULL, MKDEV(Major,0),NULL,DEVICE_NAME);
	*/

	
	printk(KERN_INFO "%s: I was assigned major number %d. To talk to\n", DEVICE_NAME  ,Major);
	printk(KERN_INFO "%s: the driver, create a dev file with\n", DEVICE_NAME );
	printk(KERN_INFO "%s: 'mknod /dev/%s c %d 0'.\n", DEVICE_NAME,DEVICE_NAME, Major);
	printk(KERN_INFO "%s: Try various minor numbers. Try to cat and echo to\n", DEVICE_NAME );
	printk(KERN_INFO "%s: the device file.\n", DEVICE_NAME );
	printk(KERN_INFO "%s: Remove the device file and module when done.\n", DEVICE_NAME );
	

	return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */

void cleanup_module()
{
	/* 
	 * Unregister the device 
	 */

	/*
	device_destroy(myclass,MKDEV(Major,0));
        class_unregister(myclass);
        class_destroy(myclass);
	*/
	unregister_chrdev(Major, DEVICE_NAME);
	printk(KERN_ALERT "unregister_chrdev: %s\n", DEVICE_NAME);
}

/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
	if (Device_Open)
		return -EBUSY;

	Device_Open++;
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;		/* We're now ready for our next caller */

	/* 
	 * Decrement the usage count, or else once you opened the file, you'll
	 * never get get rid of the module. 
	 */
	module_put(THIS_MODULE);

	return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */


static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
        long data;

	long i;
	long loffset = (long)*offset;
	void * vend = (offset + length);
	void * voffset = offset;
	
	printk(KERN_INFO "ps3hvmem (/dev/%s) called (offset 0x%p - 0x%p) count %d\n", DEVICE_NAME, voffset, vend , (int) length);
        

	int bytes_read = 0;
	for(i = loffset; i < loffset + length; i += sizeof(long)) {
    			data = lv1_peek(i);
			
    			memcpy(kbuffer + (i - loffset), &data,
        		MIN(sizeof(long), length - (i - loffset)));
			put_user(*(kbuffer++), buffer++);
	
			bytes_read++;
  	}

	return bytes_read;
}

/*  
 * Called when a process writes to dev file: echo "hi" > /dev/hello 
 */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
	return -EINVAL;
}



MODULE_AUTHOR("Dario Clavijo");
MODULE_DESCRIPTION("PS3 hvmem chardev");
MODULE_LICENSE("GPL");
