#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include<limits.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)     ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                            */
   char   padding[3];    /* Padding: IENTRTMzMjAgU3ByaW5nIDIwMjM            */
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */
struct _block *last_alloc;
/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;
   struct _block *best_fit = NULL;
   struct _block *worst_fit = NULL;
   size_t min_size = SIZE_MAX;
   size_t max_size = INT_MIN; 
#if defined FIT && FIT == 0
   /* First fit */
   //
   // While we haven't run off the end of the linked list and
   // while the current node we point to isn't free or isn't big enough
   // then continue to iterate over the list.  This loop ends either
   // with curr pointing to NULL, meaning we've run to the end of the list
   // without finding a node or it ends pointing to a free node that has enough
   // space for the request.
   // 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif


#if defined BEST && BEST == 0
   while(curr)
   {
      if(curr->free && curr->size >= size)
      {
         if(curr->size == size)
         {
            return curr;
         }

         else if(curr->size < min_size)
         {
            best_fit = curr;
            min_size = curr->size;
         }
      }

      *last = curr;
      curr = curr->next;
   }

   return best_fit;
#endif


#if defined WORST && WORST == 0
 while(curr)
   {
      if(curr->free && curr->size > size && (!worst_fit || curr->size > worst_fit->size))
      {
         
         worst_fit = curr;
         *last = curr;
      }

      *last = curr;
      curr = curr->next;
   }
   
   return worst_fit;
#endif


#if defined NEXT && NEXT == 0
   // if (last_alloc==NULL)
   //    curr = heapList; //if last_alloc->next == NULL
   // /*else if (last_alloc->next == NULL)
   //    curr = heapList;*/
   // else
   //    curr = last_alloc->next;
   
   // while (curr && !(curr->free && curr->size >= size) && curr!=last_alloc)
   // {      
   //    *last = curr;
   //    curr  = curr->next; 
   // }

   // if (curr == NULL)
   // {
   //    curr = heapList;

   //    while (curr && curr!= last_alloc && !(curr->free && curr->size >= size))
   //    {
   //       *last = curr;
   //       curr = curr->next;
   //    }
   // }
   if(last_alloc != NULL)
   {
      curr = last_alloc;
   }

   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr = curr->next;
   }

   if(curr == NULL)
   {
      while (curr != last_alloc && !(curr->free && curr->size >= size))//loops until starting point (last allocation)
      {
         *last = curr;
          curr = curr->next;
      }

   }

   if(curr == last_alloc)
   {
      curr = NULL;
   }

#endif
   last_alloc = curr;
   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }
   num_grows++;
   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to previous _block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update _block metadata:
      Set the size of the new block and initialize the new block to "free".
      Set its next pointer to NULL since it's now the tail of the linked list.
   */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   max_heap+= size;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{
   num_mallocs++;
   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block.  If a free block isn't found then we need to grow our heap. */

   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);
   struct _block* last1 = NULL;
   
   if(next)
   {
      if(next->size >= size + sizeof(struct _block) + 4)
      {
         struct _block* new_block = (struct _block*)((char*)next + size + sizeof(struct _block));
         new_block->size = next->size - size - sizeof(struct _block);
         new_block->next = next->next;
         new_block->free = true;
         next->size = size;
         next->next = new_block;
         next->free = false;
         num_splits++;
      }
   }
   
   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
   next->free = false;

   num_mallocs++;
   num_requested +=size;
   num_reuses++;
   /* Return data address associated with _block to the user */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   num_frees++;
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   struct _block *curr1 = heapList;
   struct _block *prev = NULL;
   
   while(curr1 !=NULL && curr1->next !=NULL)
   {
      if(curr1->free && curr1->next->free)
      {
         curr1->size += curr1->next->size + sizeof(struct _block);
         curr1->next = curr1->next->next;
         num_coalesces++;
      }

      else
      {
         prev = curr1;
         curr1 = curr1->next; 
      }
   }
}

void *calloc( size_t nmemb, size_t size )
{
   void *ptr = malloc(nmemb*size);
   if(ptr)
   {
      memset(ptr,0,nmemb*size);
   }
    return ptr;
   
}

void *realloc( void *ptr, size_t size )
{
   if(size==0)
   {
      free(ptr);
      return NULL;
   }
   if(ptr==NULL)
   {
      return malloc(size);
   }
   
   void *ptr1 = malloc(size);
   if(ptr1==NULL)
   {
      return NULL;
   }

   size_t old_size = BLOCK_HEADER(ptr)->size-sizeof(struct _block);
   memcpy(ptr1,ptr,old_size < size? old_size:size);
   free(ptr);

   return ptr1;
}



/* vim: IENTRTMzMjAgU3ByaW5nIDIwMjM= -----------------------------------------*/
/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
