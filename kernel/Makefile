CFLAGS_app.o := -Iinclude/kapi/phoenix
obj-y	= cpu.o printk.o sched.o idle.o semaphore.o task.o timer.o irq.o \
		  wait.o signal.o syscall.o

obj-$(CONFIG_KALLSYMS)	+= kallsyms.o

obj-y	+= sh_kapi/
