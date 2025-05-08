#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>

#define NUM_PROCESSES 50
#define SECTOR_SIZE 512
#define DISK "/dev/sdb" // Altere para o dispositivo usado

int main() {
    srand(time(NULL));

    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();

        if (pid == 0) { // processo filho
            int fd = open(DISK, O_RDONLY);
            if (fd < 0) {
                perror("Erro ao abrir disco");
                exit(1);
            }

            off_t offset = (rand() % 2000000) * 512;  // exemplo de range de setores
            lseek(fd, offset, SEEK_SET);

            char buffer[SECTOR_SIZE];
            read(fd, buffer, SECTOR_SIZE);
            close(fd);
            exit(0);
        }
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        wait(NULL); // espera todos os filhos terminarem
    }

    return 0;
}
