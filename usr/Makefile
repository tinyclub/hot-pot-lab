LINUXINCLUDE := $(KLIBCINCLUDE)

subdir-ccflags-y := -Iusr/include  -Iinclude/kapi -Iinclude/kapi/klibc -Iinclude/kapi/klibc/arch/arm
subdir-ccflags-y += -I$(srctree)/include/uapi -I$(srctree)/arch/arm/include/uapi -I$(srctree)/arch/arm/include/generated -I$(srctree)/arch/arm/include/generated/uapi 
ifeq ($(CONFIG_64BIT),y)
subdir-ccflags-y += -I$(srctree)/include/kapi/klibc/bits64
else
subdir-ccflags-y += -I$(srctree)/include/kapi/klibc/bits32
endif

subdir-ccflags-y += -D__KLIBC__ -D_BITSIZE=32

obj-y	= dummy.o app.o lwip_file.o
obj-y	+= shell/
