#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <errno.h>

#define SYS_listSleepingProcesses 386  // substitua pelo número real da sua syscall, se for diferente

int main() {
    char buffer[2048];
    long ret;

    memset(buffer, 0, sizeof(buffer));

    ret = syscall(SYS_listSleepingProcesses, buffer, sizeof(buffer));
    
    if (ret < 0) {
        perror("syscall listSleepingProcesses");
        return 1;
    }

    printf("Processos em estado de sleep:\n%s\n", buffer);
    return 0;
}
