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

#define MAX_DATA_LEN 1024
#define XTEA_NUM_ROUNDS 32

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
static size_t message_len = 0;      // Armazena o comprimento dos dados em 'message'

static char* key0 = "0";
static char* key1 = "0";
static char* key2 = "0";
static char* key3 = "0";

module_param(key0, charp, 0);
module_param(key1, charp, 0);
module_param(key2, charp, 0);
module_param(key3, charp, 0);

MODULE_PARM_DESC(key0, "Key part 0 in hex (e.g., f0e1d2c3)");

static uint32_t global_key[4];

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

static void xtea_encrypt(uint32_t num_rounds, uint32_t v[2], const uint32_t key[4]) {
	uint32_t i, v0 = v[0], v1 = v[1], sum = 0, delta = 0x9E3779B9;
	for (i = 0; i < num_rounds; i++) {
		v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
		sum += delta;
		v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
	}
	v[0] = v0;
	v[1] = v1;
}

static void xtea_decrypt(uint32_t num_rounds, uint32_t v[2], const uint32_t key[4]) {
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

static void xtea_encrypt_buffer(uint8_t *data, size_t msg_len, const uint32_t key[4]) {
    size_t i;
	for (i = 0; i < msg_len; i += 8) {
        uint32_t v[2];
        memcpy(&v[0], data + i, 4);
        memcpy(&v[1], data + i + 4, 4);
        xtea_encrypt(XTEA_NUM_ROUNDS, v, key);
        memcpy(data + i, &v[0], 4);
        memcpy(data + i + 4, &v[1], 4);
    }
}

static void xtea_decrypt_buffer(uint8_t *data, size_t msg_len, const uint32_t key[4]) {
    size_t i;
	for (i = 0; i < msg_len; i += 8) {
        uint32_t v[2];
        memcpy(&v[0], data + i, 4);
        memcpy(&v[1], data + i + 4, 4);
        xtea_decrypt(XTEA_NUM_ROUNDS, v, key);
        memcpy(data + i, &v[0], 4);
        memcpy(data + i + 4, &v[1], 4);
    }
}

static int __init simple_init(void){
    printk(KERN_INFO "XTEA Driver: Initializing the module with user key\n");

    kstrtouint(key0, 16, &global_key[0]);
    kstrtouint(key1, 16, &global_key[1]);
    kstrtouint(key2, 16, &global_key[2]);
    kstrtouint(key3, 16, &global_key[3]);

    printk(KERN_INFO "XTEA Driver: Key set to: %08x %08x %08x %08x\n",
           global_key[0], global_key[1], global_key[2], global_key[3]);

    // Registro do driver como antes
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0){
        printk(KERN_ALERT "XTEA Driver: failed to register a major number\n");
        return majorNumber;
    }

    charClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(charClass)){
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "XTEA Driver: failed to register device class\n");
        return PTR_ERR(charClass);
    }

    charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(charDevice)){
        class_destroy(charClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "XTEA Driver: failed to create the device\n");
        return PTR_ERR(charDevice);
    }

    printk(KERN_INFO "XTEA Driver: Module loaded successfully\n");
    return 0;
}

static void __exit simple_exit(void){
	device_destroy(charClass, MKDEV(majorNumber, 0));     // remove the device
	class_unregister(charClass);                          // unregister the device class
	class_destroy(charClass);                             // remove the device class
	unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
	printk(KERN_INFO "XTEA Driver: goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
	numberOpens++;
	printk(KERN_INFO "XTEA Driver: device has been opened %d time(s)\n", numberOpens);
	return 0;
}

ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    size_t to_copy;

    // Se o offset for maior ou igual ao tamanho da mensagem, não há mais dados a ler
    if (*offset >= message_len)
        return 0;

    // Calcular quantos dados podem ser copiados
    to_copy = min(len, message_len - *offset);

    // Copiar os dados para o espaço do usuário
    if (copy_to_user(buffer, message + *offset, to_copy) != 0)
        return -EFAULT;

    // Atualizar o offset para o próximo ponto de leitura
    *offset += to_copy;

    printk(KERN_INFO "XTEA Driver: sent %zu characters to the user\n", to_copy);
    return to_copy;
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    char buf[256];
    char *input, *token;
    char mode_str[4];
    char *hex_str;
    unsigned int msg_len;
    int i;
	uint8_t data[MAX_DATA_LEN];

    if (len >= sizeof(buf))
        return -EINVAL;

    if (copy_from_user(buf, buffer, len))
        return -EFAULT;

    buf[len] = '\0'; // Garante terminação nula
    input = buf;

    // Extrair modo (enc/dec)
    token = strsep(&input, " ");
    if (!token)
        return -EINVAL;

    strncpy(mode_str, token, sizeof(mode_str) - 1);
    mode_str[sizeof(mode_str) - 1] = '\0';

    // Extrair tamanho
    token = strsep(&input, " ");
    if (!token || kstrtouint(token, 10, &msg_len) != 0 || msg_len > MAX_DATA_LEN)
        return -EINVAL;

    // Extrair string com dados hexadecimais (resto da linha)
    token = strsep(&input, "\n");
    if (!token)
        return -EINVAL;

    hex_str = token;

    printk(KERN_INFO "XTEA Driver: Received mode=%s, len=%u, data=%s\n", mode_str, msg_len, hex_str);

    if (strlen(hex_str) < msg_len * 2) {
        printk(KERN_INFO "XTEA Driver: hex string too short\n");
        return -EINVAL;
    }

    // Converter a string hex em dados binários
    for (i = 0; i < msg_len; i++) {
        char byte_str[3] = {0};
        byte_str[0] = hex_str[i * 2];
        byte_str[1] = hex_str[i * 2 + 1];

        if (kstrtou8(byte_str, 16, &data[i]) != 0) {
            printk(KERN_INFO "XTEA Driver: failed to convert byte %d\n", i);
            return -EINVAL;
        }
    }

    // Aqui você pode chamar sua função de criptografia/descriptografia
    if (msg_len % 8 != 0) {
		printk(KERN_INFO "XTEA Driver: data length must be multiple of 8 bytes\n");
		return -EINVAL;
	}
	
	if (strcmp(mode_str, "enc") == 0) {
		xtea_encrypt_buffer(data, msg_len, global_key);
	} else if (strcmp(mode_str, "dec") == 0) {
		xtea_decrypt_buffer(data, msg_len, global_key);
	}

    printk(KERN_INFO "XTEA Driver: operation %s completed successfully\n", mode_str);
    memcpy(message, data, msg_len);
	message_len = msg_len;
	return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "XTEA Driver: device successfully closed\n");
	return 0;
}

module_init(simple_init);
module_exit(simple_exit);
