# Installing the kernel module:
1. make
2. mkdir /lib/modules/$(uname -r)/kernel/drivers/usb/bloenk
2. cp bloenk.ko /lib/modules/$(uname -r)/kernel/drivers/usb/bloenk/bloenk.ko
3. depmod -a
4. cp conf/udev/99-bloenk.rules /etc/udev/rules.d/

Or simply run install.sh
