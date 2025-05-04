#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

asmlinkage long sys_listSleepingProcesses(char __user *buf, size_t size) {
    struct task_struct *task;
    char kbuf[1024] = {0};
    int len = 0;

    for_each_process(task) {
        if (task->state == TASK_INTERRUPTIBLE || task->state == TASK_UNINTERRUPTIBLE) {
            len += snprintf(kbuf + len, sizeof(kbuf) - len,
                            "PID: %d | Name: %s | State: %ld\n",
                            task_pid_nr(task), task->comm, task->state);

            if (len >= sizeof(kbuf)) {
                break; // evitar overflow
            }
        }
    }

    if (len > size) {
        return -1; // buffer do usuário pequeno demais
    }

    if (copy_to_user(buf, kbuf, len)) {
        return -EFAULT;
    }

    return len;
}
