#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cache_manager.h"

namespace bb {
	
	struct cache_allocer* cache_manager::create(int capacity, int elem_size) {
		struct cache_allocer* allocer = (struct cache_allocer*)malloc(sizeof(struct cache_allocer));
		memset(allocer, 0, sizeof(struct cache_allocer));

		elem_size = (elem_size < sizeof(struct node) ? sizeof(struct node) : elem_size);
		allocer->capactiy = capacity;
		allocer->elem_size = elem_size;
		allocer->cache_men = (unsigned char*)malloc(capacity * elem_size);
		memset(allocer->cache_men, 0, capacity * elem_size);

		allocer->first_list = nullptr;

		for (int i = 0; i < capacity; i++) {
			struct node* walk = (struct node*)(allocer->cache_men + i * elem_size);
			walk->next = allocer->first_list;
			allocer->first_list = walk;
		}
		return allocer;
	}

	void  cache_manager::destroy_cache_allocer(struct cache_allocer* allocer) {
		if (allocer->cache_men != nullptr) {
			free(allocer->cache_men);
		}
		free(allocer);
	}

	void*  cache_manager::cache_alloc(struct cache_allocer* allocer, int elem_size) {
		if (allocer->elem_size < elem_size) {
			return malloc(elem_size);
		}

		if (allocer->first_list != nullptr) {
			void* now = allocer->first_list;
			allocer->first_list = allocer->first_list->next;
			return now;
		}

		return malloc(elem_size);
	}

	void  cache_manager::cache_free(struct cache_allocer* allocer, void* men) {
		if (men >= allocer->cache_men && men < allocer->cache_men + allocer->capactiy * allocer->elem_size) {
			struct node* node = (struct node*)men;
			node->next = allocer->first_list;
			allocer->first_list = node;
			return;
		}

		free(men);
	}
}