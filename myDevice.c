#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>				// File operations structure: Allows open,read,write,close file 
#include <linux/cdev.h>				// This is a char driver, makes cdev available
#include <linux/semaphore.h>		// Used to access semaphores, syncronizationb behavors
#include <asm/uaccess.h>			// copy_to_user; copy_from_user
#include <linux/init.h>

// 1. Create a structure for device

struct my_device{
	char data[100];
	struct semaphore sem;
}virtual_device;


// 2. To later register our device, we need a cdev object and som other variables
struct cdev * mycdev;
int major_number; 		// Will store our major number, extracted from dev_t dev_num using macro
int ret;				// will be used for holding return values of functions, this is because the 
						// kernel stack is very small. Declearing variables all over the pass in 
						// our module functions will eat up the stack very fast
dev_t dev_num;			// Will hold the major/minor number that the kernel gives us
						// Name appears in /proc/devices

/* Called on device_file open
** @param  inode reference to the file on disk
**		   contains also information about the file
** @param filep represents an abstract open file
*/
int device_open(struct inode * inode, struct file * filp){
	// Only allow one process to open this device file by using semaphore 
	// as mutal exclusive lock - mutex
	if(down_interruptible(&virtual_device.sem) !=0){
		printk(KERN_ALERT "elise: could not lock device file during open");
		return -1;
	}
	printk(KERN_ALERT "elise: opened device");
	return 0;
}

/* Called when the user want to get information from the device file
** @param filp abstract open file
** @param bufStoreData where to store the information we are getting from the devide file
** @param buffCount number of space available in the structure provided by the user 
** @param curOffset is file ofsett of the current opened file
*/
ssize_t device_read(struct file * filp, char * bufStoreData, size_t buffCount, loff_t * curOffset){
	// Take data from kernel space(device) to the user space (process)
	// copy_to_user(destination,source,sizeToTransfer)
	printk(KERN_INFO "elise: reading from the device file");
	ret = copy_to_user(bufStoreData,virtual_device.data,buffCount);
	return ret;
}

/* Called when the user wants to send information to the device file
** @param filp abstract open file
** @param bufStoreData where to store the information we are getting from the devide file
** @param buffCount number of space available in the structure provided by the user 
** @param curOffset is file ofsett of the current opened file
*/
ssize_t device_write(struct file* filp, const char * bufSourceData,size_t bufCount, loff_t* curOffset){
	// Send data from user to kernel
	// copy_to_user(source, destination, sizeToTransfer)
	printk(KERN_INFO "elise: writing to the device file");
	ret = copy_from_user(virtual_device.data, bufSourceData, bufCount);
	return ret;
} 

/* Called upon user closed the devide file
**
*/
int device_close(struct inode * inode, struct file * filp){
	// By calling up, which is opposite of down fore semaphore, we release the
	// mutex that we obtained at device open. This has the effect of allowing other
	// process to use the device now.
	up(&virtual_device.sem);
	printk(KERN_INFO "elise: closed the device file");
	return 0;  
}

// Tell kernel which functions to call when user operates on the device file
struct file_operations fops = {
	.owner 		= THIS_MODULE,		// Prevent unloading of this module when operations are in use
	.open 		= device_open,		// Points to the method to all when opening the device file
	.release  	= device_close,		// Points to the method to all when closing the device file
	.write		= device_write,		// Points to the method to all when writing to the device file
	.read		= device_read		// Points to the method to all when reading from device file
};


#define DEVICE_NAME "elise"

/* 3. Register our device with the system:
**    A two step process
** 	  ------------------------
** 		3.1. Use dynamic allocation to assign our device
**		A major number: alloc_chrdev_region(dev_t *, unit fminor, uint count, char * name) 
*/

static int driver_entry(void){
	// Step 3.1
	ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);

	if(ret < 0){
		printk(KERN_ALERT "elise: failed to allocate a major number");
		// Propagate error
		return ret;
	}

	major_number = MAJOR(dev_num); 		// Extract the major number and store in our macro
	printk(KERN_INFO "elise: Major number is %d", major_number);
	printk(KERN_INFO "\tuse \"mknod /dev/%s c %d 0\" for device file", DEVICE_NAME,major_number);


	// Step 3.2
	mycdev = cdev_alloc(); 				// Creat cdev structure
	mycdev->ops = &fops;				// Struct file_operations
	mycdev->owner = THIS_MODULE;		// 
	
	/* Add to kernel
	** int cdev_add(struct cdev * dev, dev_t num, unsigned int count) 
	*/
	ret = cdev_add(mycdev, dev_num, 1);

	if(ret <0){
		printk(KERN_ALERT "elise: unable to add cdev to kernel");
		return ret;
	} 

	// step 3.4: Initialize semaphore
	sema_init(&virtual_device.sem, 1);	// Initial value of one 

	return 0;
}

static void driver_exit(void){
	// 3.5: Unregister everything in reverse order
	// a
	cdev_del(mycdev);
	// b
	unregister_chrdev_region(dev_num,1); 
	printk(KERN_ALERT "elise: unloaded module");
}

/* Give descriptive information about the module
 * withe the help of macros
*/

// Who rote the module
//MODULE_AUTHOR("Farhad Atroshi");   
//MODULE_LICENSE("GPL");

// Inform the kernel where to start and stop with our driver/module
module_init(driver_entry);
module_exit(driver_exit);