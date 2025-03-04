#! /usr/bin/sh

make
mkdir /lib/modules/$(uname -r)/kernel/drivers/usb/bloenk
cp bloenk.ko /lib/modules/$(uname -r)/kernel/drivers/usb/bloenk/bloenk.ko
depmod -a
cp conf/udev/99-bloenk.rules /etc/udev/rules.d/
udevadm control --reload-rules
udevadm trigger
