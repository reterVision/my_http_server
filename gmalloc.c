#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>

#include "gmalloc.h"

void* gmalloc(size_t size)
{
  void *ptr;

  if(size == 0) {
    printf("gmalloc: zero size\n");
    exit(-1);
  }
  ptr = malloc(size); 
  if(ptr == NULL) {
    printf("gmalloc: out of memory (allocating %lu bytes)\n", (u_long) size);
    exit(-1);
  }
  return ptr;
}

void* gcalloc(size_t nmemb, size_t size)
{
  void* *ptr;

  if(nmemb == 0 || size == 0) {
    printf("gcalloc: zero size\n");
    exit(-1);
  }
  /*
   * TODO: check the maximum allowed memory block size.
   */

  ptr = calloc(nmemb, size);
  if(ptr == NULL) {
    printf("gcalloc: out of memory (allocating %lu bytes)\n",
           (u_long)nmemb*size);
    exit(-1);
  }

  return ptr;
}

void *grealloc(void *ptr, size_t size)
{
  void *new_ptr;

  if(size == 0) {
    printf("grealloc: zero size\n");
    exit(-1);
  }

  if(ptr == NULL) {
    new_ptr = malloc(size);
  } else {
    new_ptr = realloc(ptr, size);
  }
  if(new_ptr == NULL) {
    printf("grealloc: out of memory (new size %lu bytes)\n", (u_long)size);
    exit(-1);
  }

  return new_ptr;
}
