/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SLAB_DEF_H
#define _LINUX_SLAB_DEF_H

#include <linux/numa.h>
#include <linux/reciprocal_div.h>
#include <linux/mm_types.h>

/*
 * Definitions unique to the original Linux SLAB allocator.
 */

struct kmem_cache {
	struct array_cache __percpu *cpu_cache; // 对象缓冲池

	/* 1) Cache tunables. Protected by slab_mutex */
	unsigned int batchcount; // batchcount++ when: empty(cpu_cache), 并从 shared_pool 中获取对象
	unsigned int limit; // local pool 中空闲对象的数目大于 limit 时, 会主动释放 batchcount 个对象, 便于内核回收和释放 slab
	unsigned int shared; // 用于多核系统

	unsigned int size; // 对象的长度, 这个长度要加上 align 对齐字节
	struct reciprocal_value reciprocal_buffer_size;
	/* 2) touched by every alloc & free from the backend */

	slab_flags_t flags; /* constant flags. 对象的分配掩码 */
	unsigned int num; /* # of objs per slab. 一个 slab 中最多可以有多少个对象 */

	/* 3) cache_grow/shrink */
	/* order of pgs per slab (2^n) */
	unsigned int gfporder; // (global free page order) 一个 slab 中占用 2^gfporder 个页

	/* force GFP flags, e.g. GFP_DMA */
	gfp_t allocflags;

	size_t colour; /* cache colouring range. 一个 slab 中可以有多少个不同的缓存行. 缓存着色 */
	unsigned int colour_off; /* colour offset. 着色区的长度, 和 L1 cacheline 大小相同 */
	struct kmem_cache *freelist_cache; // 每个对象要占用 1Byte 来存放 freelist
	unsigned int freelist_size;

	/* constructor func */
	void (*ctor)(void *obj);

	/* 4) cache creation/removal */
	const char *name;
	struct list_head list;
	int refcount;
	int object_size;
	int align;

/* 5) statistics */
#ifdef CONFIG_DEBUG_SLAB
	unsigned long num_active;
	unsigned long num_allocations;
	unsigned long high_mark;
	unsigned long grown;
	unsigned long reaped;
	unsigned long errors;
	unsigned long max_freeable;
	unsigned long node_allocs;
	unsigned long node_frees;
	unsigned long node_overflow;
	atomic_t allochit;
	atomic_t allocmiss;
	atomic_t freehit;
	atomic_t freemiss;

	/*
	 * If debugging is enabled, then the allocator can add additional
	 * fields and/or padding to every object. 'size' contains the total
	 * object size including these internal fields, while 'obj_offset'
	 * and 'object_size' contain the offset to the user object and its
	 * size.
	 */
	int obj_offset;
#endif /* CONFIG_DEBUG_SLAB */

#ifdef CONFIG_MEMCG
	struct memcg_cache_params memcg_params;
#endif
#ifdef CONFIG_KASAN
	struct kasan_cache kasan_info;
#endif

#ifdef CONFIG_SLAB_FREELIST_RANDOM
	unsigned int *random_seq;
#endif

	unsigned int useroffset; /* Usercopy region offset */
	unsigned int usersize; /* Usercopy region size */

	struct kmem_cache_node *node[MAX_NUMNODES]; // each numa node has a kmem_cache_node
};

static inline void *nearest_obj(struct kmem_cache *cache, struct page *page, void *x)
{
	void *object = x - (x - page->s_mem) % cache->size;
	void *last_object = page->s_mem + (cache->num - 1) * cache->size;

	if (unlikely(object > last_object))
		return last_object;
	else
		return object;
}

/*
 * We want to avoid an expensive divide : (offset / cache->size)
 *   Using the fact that size is a constant for a particular cache,
 *   we can replace (offset / cache->size) by
 *   reciprocal_divide(offset, cache->reciprocal_buffer_size)
 */
static inline unsigned int obj_to_index(const struct kmem_cache *cache, const struct page *page, void *obj)
{
	u32 offset = (obj - page->s_mem);
	return reciprocal_divide(offset, cache->reciprocal_buffer_size);
}

#endif /* _LINUX_SLAB_DEF_H */
