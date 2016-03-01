#define DEFAULT_SEEKS 2
#define SHRINK_BATCH 128

typedef int (*shrinker_t)(int nr, unsigned int gfp_mask);

struct shrinker {
	shrinker_t		shrinker;
	struct list_head	list;
	int			seeks;	/* seeks to recreate an obj */
	long			nr;	/* objs pending delete */
	atomic_t	ref_count;
};
extern struct shrinker *set_shrinker(int seeks, shrinker_t theshrinker);
extern void remove_shrinker(struct shrinker *shrinker);
extern int shrink_slab(int objs, unsigned int gfp_mask);

