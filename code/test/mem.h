#ifndef MEM_H
#define MEM_H

#ifndef NULL
#define NULL 0
#endif

typedef struct mem_block_s {
    unsigned int size;
    struct mem_block_s *next;
}mem_block_t;

typedef struct header_s 
{
    mem_block_t *mem_first_free;
    mem_block_t *mem_first_used;
}header_t;

typedef mem_block_t *mem_fit_function_t(mem_block_t *first_free_block,
                                       unsigned int wanted_size);

void* mem_init(unsigned int size);
void* mem_alloc(unsigned int size);
void  mem_free(void* p);
void mem_set_fit_handler(mem_fit_function_t *);
mem_block_t *mem_first_fit(mem_block_t *first_free_block, unsigned int wanted_size);
mem_block_t *mem_best_fit(mem_block_t *first_free_block, unsigned int wanted_size);
mem_block_t *mem_worst_fit(mem_block_t *first_free_block, unsigned int wanted_size);
void mem_show(void (*print)(void *, unsigned int, int free));

#endif
