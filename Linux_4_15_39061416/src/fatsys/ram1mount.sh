sudo dd if=/dev/zero of=/dev/ram1 bs=1M count=1
sudo mkfs.vfat /dev/ram1
