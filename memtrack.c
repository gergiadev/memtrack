#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

typedef struct { 
    size_t size; 
} BlockHeader;

#define HEADER_SIZE (sizeof(BlockHeader))

typedef struct {
    size_t total_allocated, total_freed;
    size_t current_usage, peak_usage;
    size_t alloc_count, free_count;
} MemStats;

void *mem_alloc(size_t size);
void *mem_calloc(size_t nmemb, size_t size);
void mem_free(void *ptr);
char *mem_strdup(const char *s);
void memStats_print(void);

#ifdef MEMTRACK
#  define malloc(s) mem_alloc(s)
#  define calloc(n,s) mem_calloc(n,s)
#  define free(p) mem_free(p)
#  define strdup(s) mem_strdup(s)
#endif

MemStats memStats = {0};

void *sys_alloc(size_t size) {
    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE,  MAP_PRIVATE | MAP_ANONYMOUS,  -1, 0);
    if(p == MAP_FAILED) {
        return NULL;
    } else {
        return p;
    }
}

void sys_free(void *ptr, size_t size) {
    munmap(ptr, size);
}
void *mem_alloc(size_t size) {
    size_t total = HEADER_SIZE + size;
    void *raw;

    if (size == 0) {
        return NULL;
    }

    raw = sys_alloc(total);

    if (!raw) {
        return NULL;
    }

    BlockHeader *hdr = raw;
    hdr->size = total;
    memStats.total_allocated += size;
    memStats.current_usage   += size;
    memStats.alloc_count++;

    if (memStats.current_usage > memStats.peak_usage) {
        memStats.peak_usage = memStats.current_usage;
    }
    return (char *)raw + HEADER_SIZE;
}

void mem_free(void *ptr) {
    if (!ptr) {
        return;
    }
    BlockHeader *hdr = (BlockHeader *)((char *)ptr - HEADER_SIZE);
    memStats.total_freed += hdr->size;
    memStats.current_usage -= hdr->size;
    memStats.free_count++;

    sys_free(hdr, hdr->size);
}

void *mem_calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *p = mem_alloc(total);
    if (p) {
        memset(p, 0, total);
    }
    return p;
}

char *mem_strdup(const char *s) {
    if (!s) {
        return NULL;
    }
    size_t len = strlen(s) + 1;
    char *p = mem_alloc(len);
    if (p) {
        memcpy(p, s, len);
    }
    return p;
}

void memStats_print(void) {
    printf("\nDEBUG - Memory Usage\n");
    printf("\tallocations : %zu\n",   memStats.alloc_count);
    printf("\tfrees       : %zu\n",   memStats.free_count);
    printf("\tallocated   : %zu B\n", memStats.total_allocated);
    printf("\tfreed       : %zu B\n", memStats.total_freed);
    printf("\tcurrent     : %zu B\n", memStats.current_usage);
    printf("\tpeak        : %zu B\n", memStats.peak_usage);
    if (memStats.current_usage > 0) {
        printf("  *** LEAK: %zu B non liberati ***\n", memStats.current_usage);
    }
}

// Example usage
/*
int main(void) {
    atexit(memStats_print);

    char *a = malloc(64); 
    strcpy(a, "hello");
    free(a); 

    char *b = malloc(32); 
    strcpy(b, "intentional leak");
    (void)b;

    char *c = strdup("testtest"); 
    free(c); 

    int *arr = calloc(8, sizeof(int));
    for (int i = 0; i < 8; i++) {
        arr[i] = i * 2;
    }

    free(arr);

    char *d = malloc(1024*1024);  

    return 0;
}
*/