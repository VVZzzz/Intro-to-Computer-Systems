#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

/* mutex: only one thread once time can exchange cache info. 
 * w: only one writer.
 * u: when walk the cache, need put the LRU node to the head.
 *    And only one thread once time.
 */

typedef struct cnode {
    char *key;
    char *value;
    struct cnode *prev;
    struct cnode *next;
} cnode_t;

typedef struct cache {
    cnode_t *head;
    cnode_t *tail;
    size_t size;
} cache_t;


void cache_init();
void cache_place(char *key,char *value);
int  cache_get(char *key,char *value);
void cache_destroy();

#endif