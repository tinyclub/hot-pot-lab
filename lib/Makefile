lib-y      += rbtree.o dump_stack.o
libs-y     += dummy.o
obj-y      += string.o ctype.o vsprintf.o find_next_bit.o sort.o
ifneq ($(CONFIG_HAVE_DEC_LOCK),y)
  lib-y += dec_and_lock.o
endif