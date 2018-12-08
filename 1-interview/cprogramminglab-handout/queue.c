/*
 * Code for basic C skills diagnostic.
 * Developed for courses 15-213/18-213/15-513 by R. E. Bryant, 2017
 */

/*
 * This program implements a queue supporting both FIFO and LIFO
 * operations.
 *
 * It uses a singly-linked list to represent the set of queue elements
 */

#include <stdio.h>
#include <stdlib.h>

#include "harness.h"
#include "queue.h"

/*
  Create empty queue.
  Return NULL if could not allocate space.
*/
queue_t *q_new() {
  queue_t *q = malloc(sizeof(queue_t));
  /* What if malloc returned NULL? */
  if (q == NULL) return NULL;
  q->head = NULL;
  q->tail = NULL;
  q->size=0;
  return q;
}

/* Free all storage used by queue */
void q_free(queue_t *q) {
  /* How about freeing the list elements? */
  /* Free queue structure */
    if(q==NULL) return;
    while(q->head!=NULL){
        list_ele_t *temp=q->head;
        q->head=q->head->next;
        free(temp);
    }
  free(q);
}

/*
  Attempt to insert element at head of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
 */
bool q_insert_head(queue_t *q, int v) {
  list_ele_t *newh;
  /* What should you do if the q is NULL? */
  if (q == NULL) return false;
  newh = malloc(sizeof(list_ele_t));
  /* What if malloc returned NULL? */
  if (newh == NULL) {
    printf("can't allocate space!\n");
    return false;
  }
  newh->value = v;
  newh->next = q->head;
  if (q->head == NULL) q->tail = newh;
  q->head = newh;
  q->size++;
  return true;
}

/*
  Attempt to insert element at tail of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
 */
bool q_insert_tail(queue_t *q, int v) {
  /* You need to write the complete code for this function */
  /* Remember: It should operate in O(1) time */
  list_ele_t *newh;
  if (q == NULL) return false;
  newh = malloc(sizeof(list_ele_t));
  if (newh == NULL) {
    printf("can't allocate space!\n");
    return false;
  }
  newh->value = v;
  newh->next = NULL;
  if (q->head == NULL) q->head = newh;
  if (q->tail != NULL) q->tail->next = newh;
  q->tail = newh;
  q->size++;
  return true;
}

/*
  Attempt to remove element from head of queue.
  Return true if successful.
  Return false if queue is NULL or empty.
  If vp non-NULL and element removed, store removed value at *vp.
  Any unused storage should be freed
*/
bool q_remove_head(queue_t *q, int *vp) {
  if (q == NULL || q->size == 0 || vp == NULL) return false;
  /* You need to fix up this code. */
  if (q->head != NULL) {
    list_ele_t *p = q->head;
    q->head = p->next;
    *vp=p->value;
    free(p);
    q->size--;
  }
  q->tail = NULL;
  return true;
}

/*
  Return number of elements in queue.
  Return 0 if q is NULL or empty
 */
int q_size(queue_t *q) {
  /* You need to write the code for this function */
  /* Remember: It should operate in O(1) time */
  if (q == NULL) return 0;
  return q->size;
}

/*
  Reverse elements in queue.

  Your implementation must not allocate or free any elements (e.g., by
  calling q_insert_head or q_remove_head).  Instead, it should modify
  the pointers in the existing data structure.
 */
void q_reverse(queue_t *q) {
  /* You need to write the code for this function */
  if (q==NULL || q->size == 0 || q->size == 1) return;
  list_ele_t *p = q->head->next;
  q->head->next = NULL;
  q->tail = q->head;
  while (p != NULL) {
    list_ele_t *temp = p->next;
    p->next = q->head;
    q->head = p;
    p = temp;
  }
}
