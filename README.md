# Implementing system calls

## Environment: Ubuntu 22.05

## Basic setup
```
sudo apt update && sudo apt upgrade
sudo apt install build-essential libncurses-dev libssl-dev libelf-dev bison flex
sudo apt autoremove
wget -P ~/ https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.9.1.tar.gz
tar -xvf ~/linux-6.9.1.tar.gz -C ~/
```

## Syscall support
Copy ```/kernel``` contents to ```/linux-6.9.1``` direclty.

## Build kernel
```
make menuconfig
```

Set ```CONFIG_SYSTEM_TRUSTED_KEYS=""``` and ```CONFIG_SYSTEM_REVOCATION_KEYS=""``` in ```.config``` generated. If there is an error in build.

```
./buildKernel.sh
reboot
```

## Run test applications
```
./linux-6.9.1/linux_syscalls/test/build.sh
```
```sender``` and ```receiver``` applications can be executed on different consoles. ```sender``` can also take string to be sent as commandline argument.
Other applications are also available to test system calls separately.

## System calls implementation
```messagequeue.c``` contains system calls implementation.

