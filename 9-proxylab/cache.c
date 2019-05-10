#include "cache.h"
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* global var */

sem_t mutex, w, u;
int readcnt;
static cache_t *g_cache;

void cache_init() {
  g_cache = calloc(1, sizeof(cache_t));
  Sem_init(&mutex, 0, 1);
  Sem_init(&w, 0, 1);
  Sem_init(&u, 0, 1);
  readcnt = 0;
}

int cache_get(char *key, char *value) {
  P(&mutex);
  if (++readcnt == 1) P(&w);
  V(&mutex);

  /* critical section */
  int hit;
  cnode_t *elem;
  hit = 0;
  elem = g_cache->head;
  while (elem != NULL) {
    if (!strcmp(elem->key, key)) {
      if (elem != g_cache->head) {
        P(&u);
        /* critical section */
        /* update the node prev next */
        elem->prev->next = elem->next;
        if (elem == g_cache->tail)
          g_cache->tail = elem->prev;
        else
          elem->next->prev = elem->prev;
        /* put the node to the head */
        elem->prev = NULL;
        elem->next = g_cache->head;
        if (g_cache->head != NULL) g_cache->head->prev = elem;
        g_cache->head = elem;
        V(&u);
      }
      strcpy(value, elem->value);
      hit = 1;
      break;
    }
    elem = elem->next;
  }

  P(&mutex);
  if (--readcnt == 0) V(&w);
  V(&mutex);

  return hit;
}

void cache_place(char *key, char *value) {
  P(&w);
  cnode_t *elem;
  size_t size = strlen(key) + strlen(value) + sizeof(elem);
  /* critical section */
  g_cache->size += size;
  while ((g_cache->tail != NULL) && (g_cache->size > MAX_CACHE_SIZE)) {
    /* evict the node beacause there is no enough space. */
    elem = g_cache->tail;
    size = strlen(elem->key) + strlen(elem->value) + sizeof(elem);

    g_cache->size -= size;
    g_cache->tail = g_cache->tail->prev;
    g_cache->tail->next = NULL;
    free(elem->key);
    free(elem->value);
    free(elem);
  }

  /* put the elem node into the list */
  elem = (cnode_t *)malloc(sizeof(cnode_t));
  elem->key = (char *)malloc(strlen(key) + 1);
  elem->value = (char *)malloc(strlen(value) + 1);
  strcpy(elem->key, key);
  strcpy(elem->value, value);

  elem->prev = NULL;
  elem->next = g_cache->head;
  if (g_cache->head == NULL)
    g_cache->tail = elem;
  else
    g_cache->head->prev = elem;
  g_cache->head = elem;

  V(&w);
}

void cache_destroy() {
  cnode_t *elem, *tmp;
  if (g_cache != NULL) {
    elem = g_cache->head;
    while (elem != NULL) {
      tmp = elem->next;
      free(elem->key);
      free(elem->value);
      free(elem);
      elem = tmp;
    }
  }
}