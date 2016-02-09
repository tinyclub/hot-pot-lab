#! /bin/bash

mkdir ./livecd
cd ./livecd
rm * -rf
mkdir boot/
cp ../others/bzImage.linux-2.6.33 boot/linux-2.6.33
cp ../arch/i386/boot/bzImage boot/hot-pot
cp -a /usr/lib/grub/ ./
mv ./grub ./grub2
grub-mkimage -d /usr/lib/grub/i386-pc/ -p /grub2 -o core.img -O i386-pc biosdisk iso9660 search configfile part_msdos fat ntfs ext2
cat /usr/lib/grub/i386-pc/cdboot.img core.img > g2ldr
mkisofs -R -J -v -l -no-emul-boot -boot-info-table -boot-load-size 4 -b g2ldr -o ../hot-pot.iso ./
#cp ../grub2cd.iso /home/xiebaoyou/share/
#将hot-pot.iso作为虚拟机启动盘，然后在grub提示符下运行linux /boot/linux-2.6.33;boot启动linux2.6.33
#运行linux /boot/hot-pot;boot启动hot-pot
