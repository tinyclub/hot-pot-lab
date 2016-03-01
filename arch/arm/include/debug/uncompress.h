#ifdef CONFIG_DEBUG_UNCOMPRESS
extern void putc(int c);
extern void arch_decomp_setup(void);
#else
static inline void putc(int c) {}
static inline void arch_decomp_setup(void) {}
#endif
static inline void flush(void) {}
//static inline void arch_decomp_setup(void) {}

