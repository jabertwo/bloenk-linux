Bloenk Kernel Module
========
This is a kernel module for https://github.com/johannes85/bloenk-hardware, which exposes LED control to the `/sys/class/led`
interface.

# Build the kernel module
Simply run `make` in the root of the repository. You will need to have the linux kernel headers installed.

Depending on your distribution you can install them by running:
- **Debian/Ubuntu**  
  `# apt-get install linux-headers-$(uname -r)`
- **Archlinux**  
  `# pacman -Sy linux-headers`
- **Fedora**  
  `# dnf install kernel-headers kernel-devel`

# Installing the kernel module:
- In the generic case
  - `mkdir /lib/modules/$(uname -r)/kernel/drivers/usb/bloenk`
  - `cp bloenk.ko /lib/modules/$(uname -r)/kernel/drivers/usb/bloenk/bloenk.ko`
- or if your distribution has a specific place for out-of-tree modules
  - **Archlinux**:  
    `cp bloenk.ko /lib/modules/$(uname -r)/extramodules/bloenk.ko`
  - **Fedora**:  
    `cp bloenk.ko /lib/modules/$(uname -r)/extra/bloenk.ko`
- `depmod -a`
- `cp conf/udev/99-bloenk.rules /etc/udev/rules.d/`

Or simply run `install.sh` which will install it the generic way.
