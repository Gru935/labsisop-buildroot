#!/bin/sh

set -u
set -e

# Add a console on tty1
if [ -e ${TARGET_DIR}/etc/inittab ]; then
    grep -qE '^tty1::' ${TARGET_DIR}/etc/inittab || \
	sed -i '/GENERIC_SERIAL/a\
tty1::respawn:/sbin/getty -L  tty1 0 vt100 # QEMU graphical window' ${TARGET_DIR}/etc/inittab
fi

#Compile the syscall_test.c
BUILDROOT_DIR=$BASE_DIR/..
COMPILER=$BUILDROOT_DIR/output/host/bin/i686-buildroot-linux-gnu-gcc
$COMPILER -o $BUILDROOT_DIR/custom-scripts/syscall_test.c $BUILDROOT_DIR/output/target/usr/bin/syscall_test
$COMPILER -o $BUILDROOT_DIR/custom-scripts/syscall_listSleepingProcesses.c $BUILDROOT_DIR/output/target/usr/bin/syscall_listSleepingProcesses