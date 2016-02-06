#
# Makefile for some libs needed in the kernel.
#


libfdt_files = fdt.o fdt_ro.o fdt_wip.o fdt_rw.o fdt_sw.o fdt_strerror.o \
	       fdt_empty_tree.o
$(foreach file, $(libfdt_files), \
	$(eval CFLAGS_$(file) = -I$(src)/../scripts/dtc/libfdt))
lib-$(CONFIG_LIBFDT) += $(libfdt_files)


#obj-$(CONFIG_FONT_SUPPORT) += fonts/

obj-y   := dummy.o
lib-y   := dummy.o
