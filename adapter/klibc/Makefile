subdir-ccflags-y := -I$(srctree)/include/uapi -I$(srctree)/arch/arm/include/uapi -I$(srctree)/arch/arm/include/generated -I$(srctree)/arch/arm/include/generated/uapi -Iinclude/kapi/klibc -Iinclude/kapi/klibc/arch/arm -Iinclude/kapi
ifeq ($(CONFIG_64BIT),y)
subdir-ccflags-y += -I$(srctree)/include/kapi/klibc/bits64
else
subdir-ccflags-y += -I$(srctree)/include/kapi/klibc/bits32
endif

subdir-ccflags-y += -D__KLIBC__ -D_BITSIZE=32 -DUSE_GLIBC_STDIO -D_KLIBC_USE_RT_SIG

obj-y := dummy.o  globals.o\
	  vsscanf.o sscanf.o ctypes.o \
	  strntoumax.o strntoimax.o \
	  atoi.o atol.o atoll.o \
	  strtol.o strtoll.o strtoul.o strtoull.o \
	  strtoimax.o strtoumax.o \
	  memcpy.o memcmp.o memset.o memccpy.o memmem.o memswap.o \
	  memmove.o memchr.o memrchr.o bzero.o \
	  strcasecmp.o strncasecmp.o strndup.o strerror.o  \
	  strcat.o strchr.o strcmp.o strcpy.o strdup.o strlen.o strnlen.o \
	  strncat.o strlcpy.o strlcat.o \
	  strstr.o strncmp.o strncpy.o strrchr.o \
	  strxspn.o strspn.o strcspn.o strpbrk.o strsep.o strtok.o \
	  strtok_r.o \
	  fnmatch.o sleep.o \
	  inet/inet_ntoa.o inet/inet_aton.o inet/inet_addr.o \
	  ctype/isalnum.o ctype/isalpha.o ctype/isascii.o \
	  ctype/isblank.o ctype/iscntrl.o ctype/isdigit.o \
	  ctype/isgraph.o ctype/islower.o ctype/isprint.o \
	  ctype/ispunct.o ctype/isspace.o ctype/isupper.o \
	  ctype/isxdigit.o ctype/tolower.o ctype/toupper.o \
	  fputc.o stdio/fwrite.o stdio/fflush.o \
	  stdio/fgetc.o stdio/fclose.o stdio/fopen.o stdio/fread.o \
	  stdio/fseek.o stdio/fdopen.o lseek.o zalloc.o isatty.o \
	  asprintf.o fprintf.o vprintf.o vfprintf.o perror.o exit.o \
	  fputs.o bsearch.o getopt.o putchar.o puts.o \
	  readdir.o open.o fwrite2.o getcwd.o creat.o printf.o realloc.o	\
	  sigsuspend.o sigaction.o sigpending.o sigqueue.o sigwait.o sigtimedwait.o \
	  sigwaitinfo.o sighold.o sigignore.o sigpause.o sigrelse.o __signal.o sysv_signal.o raise.o sigprocmask.o 
obj-y += cmd/
#userdb/getgrgid.o userdb/getgrnam.o userdb/getpwnam.o \
#userdb/getpwuid.o userdb/root_group.o userdb/root_user.o \
#sigsuspend.o getopt_long.o lrand48.o mrand48.o jrand48.o nrand48.o srand48.o sbrk.o \
#globals.o exit.o atexit.o onexit.o \
#	  execl.o execle.o execv.o execvpe.o execvp.o execlp.o execlpe.o \
#	  fork.o vfork.o wait.o wait3.o waitpid.o system.o \
#	  setpgrp.o getpgrp.o daemon.o \
#	  printf.o vprintf.o fprintf.o vfprintf.o perror.o \
#	  statfs.o fstatfs.o umount.o \
#	  creat.o open.o openat.o open_cloexec.o \
#	  fread2.o fwrite2.o fgets.o fputc.o fputs.o puts.o putchar.o \
#	  sleep.o usleep.o strtotimespec.o strtotimeval.o \
#	  raise.o abort.o assert.o alarm.o pause.o \
#	  __signal.o sysv_signal.o bsd_signal.o siglist.o sigabbrev.o \
#	  siglongjmp.o \
#	  sigaction.o sigpending.o sigprocmask.o \
#	  pselect.o ppoll.o \
#	  brk.o malloc.o realloc.o zalloc.o calloc.o \
#	  mmap.o shm_open.o shm_unlink.o \
#	  vsnprintf.o snprintf.o vsprintf.o sprintf.o \
#	  asprintf.o vasprintf.o
#	  gethostname.o getdomainname.o getcwd.o \
#	  seteuid.o setegid.o \
#	  getenv.o setenv.o putenv.o __put_env.o unsetenv.o \
#	  clearenv.o nullenv.o \
#	  getopt.o readdir.o scandir.o alphasort.o remove.o \
#	  syslog.o closelog.o pty.o getpt.o posix_openpt.o isatty.o reboot.o \
#	  time.o utime.o lseek.o nice.o getpriority.o \
#	  qsort.o bsearch.o \
#	  seed48.o \
#	  send.o recv.o \
#	  setmntent.o endmntent.o getmntent.o \
#	  stdio/fclose.o stdio/fopen.o stdio/fdopen.o \
#	  stdio/fread.o stdio/fwrite.o stdio/fflush.o \
#	  stdio/ungetc.o stdio/fgetc.o \
#	  stdio/fseek.o stdio/ftell.o stdio/rewind.o \
#	  stdio/fileno.o stdio/feof.o stdio/ferror.o
#	  strndup.o strsignal.o inet/inet_ntop.o inet/inet_pton.o 
#	  inet/bindresvport.o \
	  
klib-$(CONFIG_KLIBC_ERRLIST) += errlist.o
ifeq ($(CONFIG_KLIBC_ERRLIST),y)
CCFLAGS_strerror.o += -DWITH_ERRLIST
endif

#obj-y += arch/$(ARCH)/ 