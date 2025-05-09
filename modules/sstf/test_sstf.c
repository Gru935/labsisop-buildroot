#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>

#define SECTOR_SIZE 512
#define NUM_PROCESSES 1000
#define DISK "/dev/sdb"

int main() {
    off_t setores[NUM_PROCESSES];
    srand(time(NULL));

    // Gera os setores aleatórios e imprime na ordem gerada
    for (int i = 0; i < NUM_PROCESSES; i++) {
        setores[i] = rand() % 2000000;
        // printf("%d\n", (int)setores[i]);
    }

    // Cria os processos e faz as leituras
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            int fd = open(DISK, O_RDONLY);
            if (fd < 0) {
                perror("Erro ao abrir disco");
                exit(1);
            }

            off_t offset = setores[i] * SECTOR_SIZE;
            usleep(5000);  // Dá tempo para gerar competição entre requisições
            lseek(fd, offset, SEEK_SET);

            char buffer[SECTOR_SIZE];
            read(fd, buffer, SECTOR_SIZE);

            close(fd);
            exit(0);
        }
    }

    // Espera os filhos terminarem
    for (int i = 0; i < NUM_PROCESSES; i++) {
        wait(NULL);
    }

    return 0;
}
