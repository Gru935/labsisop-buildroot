BUILDROOT_DIR := ..
COMPILER := $(BUILDROOT_DIR)/output/host/bin/i686-buildroot-linux-gnu-gcc

all:
	$(COMPILER) -o syscall_listSleepingProcesses syscall_listSleepingProcesses.c
	$(COMPILER) -o syscall_logMessage syscall_logMessage.c
	cp syscall_listSleepingProcesses $(BUILDROOT_DIR)/output/target/bin
	cp syscall_logMessage $(BUILDROOT_DIR)/output/target/bin

clean:
	rm -f syscall_listSleepingProcesses
	rm -f syscall_logMessage