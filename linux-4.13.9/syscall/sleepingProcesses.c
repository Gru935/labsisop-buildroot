#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/slab.h>  // necessário para kmalloc e kfree

asmlinkage long sys_listSleepingProcesses(char __user *buf, size_t size) {
    struct task_struct *task;
    char *kbuf;
    int len = 0;

    // validação básica do tamanho
    if (size == 0 || size > 1 << 20) // evita alocações absurdas (>1MB)
        return -EINVAL;

    kbuf = kmalloc(size, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    for_each_process(task) {
        if (task->state == TASK_INTERRUPTIBLE || task->state == TASK_UNINTERRUPTIBLE) {
            int remaining = size - len;

            if (remaining <= 0)
                break;

            int written = snprintf(kbuf + len, remaining,
                                   "PID: %d | Name: %s | State: %ld\n",
                                   task_pid_nr(task), task->comm, task->state);

            if (written >= remaining)
                break;

            len += written;
        }
    }

    if (copy_to_user(buf, kbuf, len)) {
        kfree(kbuf);
        return -EFAULT;
    }

    kfree(kbuf);
    return len;
}
