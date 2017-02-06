# HOT POT Operating System

Hot Pot OS is an operating system developed by some Chinese hackers, the core developer is Xie Baoyou. it is developed for OS study.

## Quick Start

## Clone the code

    $ git clone https://github.com/tinyclub/hot-pot-lab.git

## Build the environment

Build the docker env (image):

    $ cd hot-pot/lab
    $ sudo ./lab-env

Start the docker env (VM), only for the first time:

    $ ./lab-build
    root@687031bd8f37:/hot-pot-lab# exit

After exit, start it again simply via (for the container id is saved in lab-container):

    $ ./lab-start
    root@687031bd8f37:/hot-pot-lab#

**Note**: The lab-env is only for Ubuntu >= 14.04, for the other distributions, please install [docker-engine](https://docs.docker.com/engine/installation/linux/) at first, then run `docker build -t tinylab/hot-pot-lab .` to build the image.

## Configure/Compile/Boot

    root@687031bd8f37:/hot-pot-lab# ./arm.config
    root@687031bd8f37:/hot-pot-lab# ./arm.compile
    root@687031bd8f37:/hot-pot-lab# ./arm.run
    Texas Instruments X-Loader 1.5.1 (Jul 26 2011 - 00:39:12)
    Beagle xM
    Reading boot sector
    Loading u-boot.bin from mmc


    U-Boot 2011.06 (Aug 19 2011 - 17:43:34)

    OMAP36XX/37XX-GP ES2.0, CPU-OPP2, L3-165MHz, Max CPU Clock 1 Ghz
    OMAP3 Beagle board + LPDDR/NAND
    I2C:   ready
    DRAM:  512 MiB
    NAND:  256 MiB
    MMC:   OMAP SD/MMC: 0
    *** Warning - bad CRC, using default environment

    ERROR : Unsupport USB mode
    Check that mini-B USB cable is attached to the device
    In:    serial
    Out:   serial
    Err:   serial
    Beagle xM Rev A
    No EEPROM on expansion board
    Die ID #51454d5551454d555400000051454d55
    Hit any key to stop autoboot:  0 
    SD/MMC found on device 0
    reading boot.scr

    508 bytes read
    Running bootscript from mmc ...
    ## Executing script at 82000000
    reading uImage

    207208 bytes read
    reading uInitrd

    1856548 bytes read
    reading board.dtb

    316 bytes read
    ## Booting kernel from Legacy Image at 80000000 ...
       Image Name:   Linux-2.6.36.1xby.0217001-g4f759
       Image Type:   ARM Linux Kernel Image (uncompressed)
       Data Size:    207144 Bytes = 202.3 KiB
       Load Address: 80008000
       Entry Point:  80008000
       Verifying Checksum ... OK
    ## Loading init Ramdisk from Legacy Image at 81600000 ...
       Image Name:   initramfs
       Image Type:   ARM Linux RAMDisk Image (uncompressed)
       Data Size:    1856484 Bytes = 1.8 MiB
       Load Address: 00000000
       Entry Point:  00000000
       Verifying Checksum ... OK
    ## Flattened Device Tree blob at 815f0000
       Booting using the fdt blob at 0x815f0000
       Loading Kernel Image ... OK
    OK
       Using Device Tree in place at 815f0000, end 815f313b

    Starting kernel ...

    Uncompressing Linux... done, booting the kernel.
    Serial driver version 4.11 with no serial options enabled
    tty00 at 0xf9e09000 (irq = 4) is a 8250
    omap_serial_init
    RAMDISK: 4194304 bytes, starting at 0xc0902820

    VFS: Insert ramdisk floppy and press ENTER
    VFS: Mounted root (msdos filesystem) readonly.
                                    
        ##############################################
        #                                            #
        #  *   *   ***   *****  ****    ***   *****  #
        #  *   *  *   *    *    *   *  *   *    *    #
        #  *   *  *   *    *    *   *  *   *    *    #
        #  *****  *   *    *    ****   *   *    *    #
        #  *   *  *   *    *    *      *   *    *    #
        #  *   *  *   *    *    *      *   *    *    #
        #  *   *   ***     *    *       ***     *    #
        #                                            #
        ##############################################
                                  
    [dim-sum@hot pot]#
