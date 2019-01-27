#ifndef _CacheManager_H_
#define _CacheManager_H_

#ifdef _cplusplus
extern "C" {
#endif

	namespace bb {
		struct node
		{
			struct node* next;
		};

		struct cache_allocer
		{
			unsigned char* cache_men;
			int capactiy;
			struct node* first_list;
			int elem_size;
		};

		class cache_manager {
		public:
			static struct cache_allocer* create(int capacity, int elem_size);

			static void destroy_cache_allocer(struct cache_allocer* allocer);

			static void* cache_alloc(struct cache_allocer* allocer, int elem_size);
			static void cache_free(struct cache_allocer* allocer, void* men);
		};
		
	}



#ifdef _cplusplus
}
#endif

#endif