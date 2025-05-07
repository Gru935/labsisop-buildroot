#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LENGTH 256

int main(){
	int ret, fd;
	char receive[BUFFER_LENGTH];
	char stringToSend[BUFFER_LENGTH];
	
	printf("Starting XTEA device test...\n");

	fd = open("/dev/xtea_driver", O_RDWR); // atualize o nome do dispositivo
	if (fd < 0){
		perror("Failed to open the device...");
		return errno;
	}

	// Exemplo de entrada: enc 00112233 44556677 8899aabb ccddeeff 8 0001020304050607
	//                     ^^^ comando,  ^ chave 128 bits ^ , tamanho,  ^ dado 8 bytes
	printf("Enter command to send (format: enc <k0> <k1> <k2> <k3> <size> <hexdata>):\n");
	scanf(" %[^\n]%*c", stringToSend);

	printf("Sending to driver: [%s]\n", stringToSend);
	ret = write(fd, stringToSend, strlen(stringToSend));
	if (ret < 0){
		perror("Failed to write to the device.");
		return errno;
	}

	printf("Reading back encrypted data...\n");
	ret = read(fd, receive, BUFFER_LENGTH);
	if (ret < 0){
		perror("Failed to read from the device.");
		return errno;
	}

	printf("Encrypted output: [%s]\n", receive);
	printf("End of program.\n");
	return 0;
}
