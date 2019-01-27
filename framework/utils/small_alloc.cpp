#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cache_manager.h"
#include "small_alloc.h"

#define SMALL_ELEM_NUM 10 * 1024
#define SMALL_ELEM_SIZE 128



namespace bb {
	static struct cache_allocer* small_allocer = NULL;

	void* small_alloc(int size) {
		if (small_allocer == NULL) {
			small_allocer = cache_manager::create(SMALL_ELEM_NUM, SMALL_ELEM_SIZE);
		}

		return cache_manager::cache_alloc(small_allocer, size);
	}

	void small_free(void* mem) {
		if (small_allocer == NULL) {
			small_allocer = cache_manager::create(SMALL_ELEM_NUM, SMALL_ELEM_SIZE);
		}

		cache_manager::cache_free(small_allocer, mem);
	}

}