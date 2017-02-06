# Hello hot-pot

Hot Pot OS is on operating system developed by some Chinese hackers, the core developer is Xie Baoyou. it is developed for OS study.

## Quick Start

## Prepare the env 

* Local Compiler

      $ sudo apt-get install git vim gcc g++

* Cross Compiler

      $ sudo apt-get install gcc-arm-linux-gnueabihf

* Qemu with beagle support

      $ sudo apt-get install libglib2.0-dev zlib1g-dev libpixman-1-dev libfdt-dev libtool
      $ git clone https://github.com/choonho/qemu-beagle.git
      $ cd qemu-beagle/
      $ mkdir build/
      $ ../configure --target-list=arm-softmmu --disable-strip --enable-debug
      $ make -j8
      $ sudo make install

**NOTE**: The official beagle board support is git://git.linaro.org/qemu/qemu-linaro.git, the qemu-beagle is a clone.

## Clone the code

    $ git clone https://code.csdn.net/xiebaoyou/hot-pot.git

## Configure

    $ cd hot-pot
    $ ./arm.config

## Compile

    $ ./arm.compile

## Run on Qemu

It mount the beagle.img, put the kernel uImage in and boot the image on Qemu.

    $ arm.run
