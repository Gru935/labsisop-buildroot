/**
 * @brief   An introductory character driver. This module maps to /dev/simple_driver and
 * comes with a helper C program that can be run in Linux user space to communicate with
 * this the LKM.
 *
 * Modified from Derek Molloy (http://www.derekmolloy.ie/)
 */

#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>

#define  DEVICE_NAME "xtea_driver" ///< The device will appear at /dev/simple_driver using this value
#define  CLASS_NAME  "xtea_class"        ///< The device class -- this is a character device driver

MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Author Name");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A generic Linux char driver.");  ///< The description -- see modinfo
MODULE_VERSION("0.2");            ///< A version number to inform users

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[256] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class *charClass  = NULL; ///< The device-driver class struct pointer
static struct device *charDevice = NULL; ///< The device-driver device struct pointer

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static int num_rounds = 32;


/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

static void encipher(uint32_t num_rounds, uint32_t v[2], const uint32_t key[4]) {
	uint32_t i, v0 = v[0], v1 = v[1], sum = 0, delta = 0x9E3779B9;
	for (i = 0; i < num_rounds; i++) {
		v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
		sum += delta;
		v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
	}
	v[0] = v0;
	v[1] = v1;
}

static void decipher(uint32_t num_rounds, uint32_t v[2], const uint32_t key[4]) {
	uint32_t v0 = v[0], v1 = v[1];
	uint32_t delta = 0x9E3779B9;
	uint32_t sum = delta * num_rounds;
	uint32_t i;
	for (i = 0; i < num_rounds; i++) {
		v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
		sum -= delta;
		v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
	}
	v[0] = v0;
	v[1] = v1;
}



/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init simple_init(void){
	printk(KERN_INFO "Simple Driver: Initializing the LKM\n");

	// Try to dynamically allocate a major number for the device -- more difficult but worth it
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber<0){
		printk(KERN_ALERT "Simple Driver failed to register a major number\n");
		return majorNumber;
	}
	
	printk(KERN_INFO "Simple Driver: registered correctly with major number %d\n", majorNumber);

	// Register the device class
	charClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(charClass)){                // Check for error and clean up if there is
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Simple Driver: failed to register device class\n");
		return PTR_ERR(charClass);          // Correct way to return an error on a pointer
	}
	
	printk(KERN_INFO "Simple Driver: device class registered correctly\n");

	// Register the device driver
	charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(charDevice)){               // Clean up if there is an error
		class_destroy(charClass);           // Repeated code but the alternative is goto statements
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Simple Driver: failed to create the device\n");
		return PTR_ERR(charDevice);
	}
	
	printk(KERN_INFO "Simple Driver: device class created correctly\n"); // Made it! device was initialized
		
	return 0;
}


/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit simple_exit(void){
	device_destroy(charClass, MKDEV(majorNumber, 0));     // remove the device
	class_unregister(charClass);                          // unregister the device class
	class_destroy(charClass);                             // remove the device class
	unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
	printk(KERN_INFO "Simple Driver: goodbye from the LKM!\n");
}


/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep){
	numberOpens++;
	printk(KERN_INFO "Simple Driver: device has been opened %d time(s)\n", numberOpens);
	return 0;
}


/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
	int error_count = 0;
   
	// copy_to_user has the format ( * to, *from, size) and returns 0 on success
	error_count = copy_to_user(buffer, message, size_of_message);

	if (error_count==0){            // if true then have success
		printk(KERN_INFO "Simple Driver: sent %d characters to the user\n", size_of_message);
		return (size_of_message=0);  // clear the position to the start and return 0
	}
	else {
		printk(KERN_INFO "Simple Driver: failed to send %d characters to the user\n", error_count);
		return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
	}
}


/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
 static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
    char input[256];
    uint32_t key[4], data_size;
    char command[4];
    int i, ret;

    if (len >= sizeof(input)) return -EINVAL;
    if (copy_from_user(input, buffer, len)) return -EFAULT;
    input[len] = '\0';

    // Parse comando, chave e tamanho
    ret = sscanf(input, "%3s %x %x %x %x %u", command, &key[0], &key[1], &key[2], &key[3], &data_size);
    if (ret != 6) {
        printk(KERN_INFO "XTEA Driver: Invalid input format\n");
        return -EINVAL;
    }

    // Pular os tokens iniciais até encontrar os dados
    char *data_ptr = input;
    int token_count = 0;
    while (token_count < 6 && *data_ptr) {
        if (*data_ptr == ' ') token_count++;
        data_ptr++;
    }

    if (!*data_ptr) {
        printk(KERN_INFO "XTEA Driver: Missing data payload\n");
        return -EINVAL;
    }

    // Verificação do tamanho do dado
    if (data_size % 8 != 0) {
        printk(KERN_INFO "XTEA Driver: data_size must be multiple of 8 bytes\n");
        return -EINVAL;
    }

    size_t num_blocks = data_size / 8;
    uint32_t blocks[num_blocks][2]; // cada bloco tem 2 x 32 bits = 64 bits = 8 bytes

    // Converter string hex para blocos binários
    for (i = 0; i < num_blocks; i++) {
        unsigned int high, low;
        if (sscanf(data_ptr, "%8x%8x", &high, &low) != 2) {
            printk(KERN_INFO "XTEA Driver: Failed to parse hex data at block %d\n", i);
            return -EINVAL;
        }
        blocks[i][0] = high;
        blocks[i][1] = low;
        data_ptr += 16; // avançar 16 caracteres hexadecimais (8+8)
    }

    // Criptografar ou descriptografar
    if (strcmp(command, "enc") == 0) {
        for (i = 0; i < num_blocks; i++) {
            encipher(num_rounds, blocks[i], key);
        }
    } else if (strcmp(command, "dec") == 0) {
        for (i = 0; i < num_blocks; i++) {
            decipher(num_rounds, blocks[i], key);
        }
    } else {
        printk(KERN_WARNING "XTEA Driver: Unknown command '%s'\n", command);
        return -EINVAL;
    }

    // Gerar string de saída (em hexadecimal)
    char *out_ptr = message;
    for (i = 0; i < num_blocks; i++) {
        out_ptr += sprintf(out_ptr, "%08x%08x", blocks[i][0], blocks[i][1]);
    }

    size_of_message = strlen(message);
    printk(KERN_INFO "XTEA Driver: Operation '%s' completed. Output: %s\n", command, message);

    return len;
}


/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "Simple Driver: device successfully closed\n");
	return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(simple_init);
module_exit(simple_exit);
