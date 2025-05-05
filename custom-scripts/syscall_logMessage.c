#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <errno.h>

#define SYS_logMessage 387  // Substitua pelo número correto da sua syscall

int main() {
    const char *mensagem = "Olá, kernel! Esta é uma mensagem de teste.";

    long ret = syscall(SYS_logMessage, mensagem);

    if (ret < 0) {
        perror("Erro ao chamar syscall logMessage");
        return 1;
    }

    printf("Mensagem enviada com sucesso!\n");
    return 0;
}
