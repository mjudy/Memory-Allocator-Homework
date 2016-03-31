#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <string.h>

void my_malloc_stats(void);
void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_realloc(void *ptr, size_t size);

/**
  * Unit test of your memory allocator implementation. This will
  * allocate and free memory regions.
  */
extern void hw4_test(void);

char frames[16][32];
char reservedFrames[16];
size_t reservedSizes[16];

int main (int argc, char *argv[]) {
  int i, j;
  for (i = 0; i< 16; i++) {
      reservedFrames[i] = 'f';
      reservedSizes[i] = 0;
    for (j = 0; j < 32; j++) {
      frames[i][j] = 0;
    }//end columns loop
  }//end rows loop

  hw4_test();
}//end main()

/**
  * Display information about memory allocations to standard output.
  *
  * Display to standard output the following: 
  * - Memory contents, one frame per line, 16 lines total.
  *   Display the actual bytes stored in memory. If the byte
  *   is unprintable (ASCII value less than 32 or greater 
  *   than 126), then display a dot instead.
  * - Current memory allocations, one line of 32 characters,
  *   where each character corresponds to a frame. Indicate 
  *   reserved frames with R, free memory with f.
  */
void my_malloc_stats(void) {
  int i, j;
  printf("Memory contents:\n");
  for (i = 0; i < 16; i++) {
    for (j = 0; j < 32; j++) {
      if (frames[i][j] <= 32 || frames[i][j] > 126) {
        printf(".");
      } else {
        printf("%c", frames[i][j]);
      }//end if-else
    }//end columns loop
    printf("\n");
  }//end rows loop

  printf("Memory allocations:\n");
  for (i = 0; i < 16; i++) {
    printf("%c", reservedFrames[i]);
  }//end for
  printf("\n");
}//end my_malloc_stats()

/** 
  * Allocate and return a contiguous memory block that is within the
  * memory region.
  *
  * The size of the returned block will be at least @a size bytes,
  * rounded up to the next 32 byte incrememnt.
  *
  * @param size Number of bytes to allocate. if @c 0, your code may do
  * whatever it wants; my_malloc() of @c 0 is "implementation defined",
  * meaning it is up to you if you want to return @c NULL, segfault,
  * whatever.
  *
  * @return Pointer toallocated memory, or @c NULL if no space could
  * be found. If out of memory, set errno to @c ENOMEM.
  */
void *my_malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }//end if

  int numFrames, i, j, freeFrames;
  if ((size%32) != 0) {
    numFrames = (size/32) + 1;
  } else if ((size/32) == 1) {
    numFrames = 1;
  } else {
    numFrames = (size/32);
  }//end if else
  for (i = 0; i < 16; i++) {
    freeFrames = 0;
    for (j = 0; j < numFrames; j++) {
      if ((i+j) >= 16) {
        errno = ENOMEM;
        return NULL;
      }//end if
      if (reservedFrames[i+j] == 'f') {
        freeFrames++;
        if (freeFrames == numFrames) {
          goto alloc;
        }//end if
      }//end if
    }//end for
  }//end for

  alloc:
    for (j = 0; j < numFrames; j++) {
      reservedFrames[i+j] = 'R';
    }//end for
      reservedSizes[i] = numFrames;
    return &frames[i];
}//end my_malloc()

/**
  * Deallocate a memory region that was returned by my_malloc() or
  * my_realloc().
  *
  * If @a ptr is not a pointer returned by my_malloc() or my_realloc(),
  * then send a SIGSEGV signal to the calling process. Likewise,
  * calling my_free() on a previously freed region results in a 
  * SIGSEGV.
  *
  * @param ptr Pointer to memory region to free. If @c NULL, do nothing.
  */
void my_free(void *ptr) {
  size_t memBase = (size_t)&frames;
  size_t memTop = (size_t)&frames[16][32];
  if ((size_t)ptr < memBase || (size_t)ptr > memTop) {
    //send SIGSEGV
    kill(getpid(), SIGSEGV);
  }//end if
  if (ptr == NULL) {
    //do nothing
    return;
  }//end if

  size_t ptrAddress = (size_t)ptr;
  ptrdiff_t frameIndex = ((ptrAddress - memBase)/32);
  ptrdiff_t frameAlign = ((ptrAddress - memBase)%32);
  //Segfault on un-aligned free
  if (frameAlign != 0) {
    //send SIGSEGV
    kill(getpid(), SIGSEGV);
  }//end if

  //Segfault on double-free
  if (reservedFrames[frameIndex] == 'f') {
    //send SIGSEGV
    kill(getpid(), SIGSEGV);
  }//end if

  size_t numFrames = reservedSizes[frameIndex];
  int i;
  for (i = frameIndex; i < (numFrames + frameIndex); i++) {
    reservedFrames[i] = 'f';
  }//end for
  reservedSizes[frameIndex] = 0;
}//end my_free()

/**
  * Change the size of the memory block pointed to by @a ptr.
  * 
  * - If @a ptr is @c NULL, then treat this as if a call to
  * my_malloc() for the requested size.
  * - Else if @a size is @c -, then treat this as if a call to
  * my_free().
  * - Else if @a ptr is not a pointer returned by my_malloc() or
  * my_realloc, then send a SIGSEGV signal to the calling process.
  * 
  * Otherwise reallocate @a ptr as follows:
  *
  * - If @a size is smaller than the previously allocated size, then
  * reduce the size of the memory block. Mark the Excess memory as
  * available. Memory sizes are rounded up to the next 32-byte
  * increment.
  * - If @a size is the same size as the previously allocated size,
  * then do nothing.
  * - If @a size is greater than the previously allocated size, then
  * allocate a new contiguous block of at least @a size bytes,
  * rounded up to the next 32-byte increment. Copy the contents from
  * the old to the new block, then free the old block.
  *
  * @param ptr Pointer to memory region to reallocate
  * @param size Number of bytes to reallocate
  *
  * @return If allocating a new memory block or if resizing a block,
  * then pointer to allocated memory; @a ptr will become invalid. If
  * freeing a memory region or if allocation fails, return @c NULL. If
  * out of memory, set errno to @c ENOMEM.
  */
void *my_realloc(void *ptr, size_t size) {
  size_t memBase = (size_t)&frames;
  size_t memTop = (size_t)&frames[16][32];
  if (ptr == NULL) {
    return my_malloc(size);
  } else if (size == 0) {
    my_free(ptr);
    return NULL;
  } else if ((size_t)ptr < memBase || (size_t)ptr > memTop) {
    //send SIGSEGV
    kill(getpid(), SIGSEGV);
  }//end if else

  //Calculate array index for ptr
  size_t ptrAddress = (size_t)ptr;
  ptrdiff_t frameIndex = ((ptrAddress - memBase)/32);
  ptrdiff_t frameAlign = ((ptrAddress - memBase)%32);

  //Segfault on un-aligned realloc
  if (frameAlign != 0) {
    //send SIGSEGV
    kill(getpid(), SIGSEGV);
  }//end if

  size_t oldSize = reservedSizes[frameIndex];
  size_t newSize;
  if ((size%32) != 0) {
    newSize = (size/32) + 1;
  } else if ((size/32) == 1) {
    newSize = 1;
  } else {
    newSize = (size/32);
  }//end if else

  int i, j, freeFrames;
  if (oldSize == newSize) {
    return ptr;
  } else if (oldSize > newSize) {
    for (i = frameIndex + newSize; i < (frameIndex + oldSize); i++) {
      reservedFrames[i] = 'f';
    }//end for
    reservedSizes[frameIndex] = newSize;
    return ptr;
  } else if (oldSize < newSize) {
    for (i = 0; i < 16; i++) {
      freeFrames = 0;
      for (j = 0; j < newSize; j++) {
        if ((i+j) >= 16) {
          errno = ENOMEM;
          return NULL;
        }//end if
        if (reservedFrames[i+j] == 'f') {
          freeFrames++;
          if (freeFrames == newSize) {
            goto realloc;
          }//end if
        }//end if
      }//end for
    }//end for    
    realloc:
      for (j = 0; j < newSize; j++) {
        reservedFrames[i+j] = 'R';
      }//end for
      reservedSizes[i] = newSize;
      
      //save ptr to return later
      ptr = &frames[i];
      int oldIndex = (int)frameIndex;
      for (; frameIndex < (oldIndex + oldSize); frameIndex++) {
        for (j = 0; j < 32; j++) {
          frames[i][j] = frames[frameIndex][j];
        }//end for
        i++;
      }//end for

      //free old memory
      for (i = oldIndex; i < (oldIndex + oldSize); i++) {
        reservedFrames[i] = 'f';
      }//end for
      reservedSizes[oldIndex] = 0;

      //return ptr
      return ptr;
  }//end if else
  return NULL;
}//end my_realloc()
