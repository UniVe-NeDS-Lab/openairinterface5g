/*
  queue_t is a basic thread-safe non-blocking fixed-size queue.

  put_queue returns false if the queue is full.
  get_queue returns NULL if the queue is empty.

  Example usage:

    // Initialization:
    queue_t fooq;
    init_queue(&fooq);

    // Producer:
    foo_t *item = new_foo();
    if (!put_queue(&fooq, item))
      delete_foo(item);

    // Consumer:
    foo_t *item = get_queue(&fooq);
    if (item)
    {
      do_something_with_foo(item);
      delete_foo(item);
    }
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#define MAX_QUEUE_SIZE 512

typedef struct queue_t {
  void *items[MAX_QUEUE_SIZE];
  size_t read_index, write_index;
  size_t num_items;
  pthread_mutex_t mutex;
} queue_t;


void init_queue(queue_t *q);
bool put_queue(queue_t *q, void *item);
void *get_queue(queue_t *q);
void *unqueue(queue_t *q);
