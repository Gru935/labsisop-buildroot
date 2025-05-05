#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#define MAX_LOG_MSG_LEN 256  // tamanho máximo da mensagem

asmlinkage long sys_logMessage(const char __user *user_msg) {
    char kbuf[MAX_LOG_MSG_LEN];
    long copied;

    // Copia string do espaço do usuário para o kernel
    copied = strncpy_from_user(kbuf, user_msg, MAX_LOG_MSG_LEN);

    if (copied < 0) {
        return -EFAULT;  // erro ao copiar
    }

    // Garante terminação nula
    kbuf[MAX_LOG_MSG_LEN - 1] = '\0';

    // Imprime no log do kernel
    printk(KERN_INFO "[sys_logMessage] Mensagem do usuário: %s\n", kbuf);

    return 0;
}
