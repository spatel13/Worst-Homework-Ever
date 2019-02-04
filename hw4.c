/**
 * @Author Sahil Patel (spatel32@umbc.edu)
 * This file contains a program that gets two arguements from the command line
 * and parses the information to create optimal trick-or-treating routes
 *
 * manpages
 * Lectures
 * http://www.asciitable.com/
 * https://stackoverflow.com/questions/1472581/printing-chars-and-their-ascii-code-in-c
 * https://www.cs.umd.edu/class/sum2003/cmsc311/Notes/BitOp/pointer.html
 * http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Pointers.html
 * 
 */

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int NUM_OF_PAGES = 12;
float PAGE_SIZE = 32;
char MEMORY[384];
int PAGES[12];
int ALLOCATIONS = 0;
pthread_mutex_t lock;

/**
 * Unit test of your memory allocator implementation. This will
 * allocate and free memory regions.
 */
extern void hw4_test(void);

int my_ceil(float raw_num)
{
    int num = 0;
    if (raw_num > (int) raw_num)
    {
	num = (int) raw_num + 1;
    }
    else
    {
	num = (int) raw_num;
    }
    return num;
}

void initialize_memory()
{
    pthread_mutex_init(&lock, NULL);
    for(int i = 0; i < (NUM_OF_PAGES * PAGE_SIZE); i++)
    {
	MEMORY[i] = 0;
    }

    for(int i = 0; i < NUM_OF_PAGES; i++)
    {
	PAGES[i] = 0;
    }
}

/**
 * Write to standard output information about the current memory
 * allocations.
 *
 * Display to standard output the following:
 * - Memory contents, one frame per line, 12 lines total. Display the
 *   actual bytes stored in memory. If the byte is unprintable (ASCII
 *   value less than 32 or greater than 126), then display a dot
 *   instead.
 * - Current memory allocation table, one line of 12 columns. For each
 *   column, display a 'f' if the frame is free, 'R' if reserved.
 */
void my_malloc_stats(void)
{
    pthread_mutex_lock(&lock);
    printf("Memory contents: \n");
    for(int i = 0; i < (NUM_OF_PAGES * PAGE_SIZE); i += 32)
    {
	for(int j = i; j < (i + PAGE_SIZE); j++)
	{
	    if(MEMORY[j] >= 32 && MEMORY[j] <= 126)
	    {
		printf("%c", MEMORY[j]);
	    }
	    else
	    {
		printf(".");
	    }
	}
	printf("\n");
    }
    printf("\n");
    
    printf("Memory allocations: \n");
    for(int i = 0; i < NUM_OF_PAGES; i++)
    {
	if(PAGES[i] == 0)
	{
	    printf("f");
	}
	else
	{
	    printf("R");
	}
    }
    printf("\n");
    pthread_mutex_unlock(&lock);
}

/**
 * Allocate and return a contiguous memory block that is within the
 * memory region. The allocated memory remains uninitialized.
 *
 * Search through all of available for the largest free memory region,
 * then return a pointer to the beginning of the region. If multiple
 * regions are the largest, return a pointer to the region closest to
 * address zero.
 *
 * The size of the returned block will be at least @a size bytes,
 * rounded up to the next 32-byte increment.
 *
 * @param size Number of bytes to allocate. If @c 0, your code may do
 * whatever it wants; my_malloc() of @c 0 is "implementation defined",
 * meaning it is up to you if you want to return @c NULL, segfault,
 * whatever.
 *
 * @return Pointer to allocated memory, or @c NULL if no space could
 * be found. If out of memory, set errno to @c ENOMEM.
 */
void* pseudo_malloc(size_t size)
{
    if(size == 0)
    {
	pthread_mutex_unlock(&lock);
	return NULL;
    }
    
    // Get a rough idea of how many pages I will need
    float pages_needed_raw = size / PAGE_SIZE;
    // Round raw number above to a square number
    int pages_needed = my_ceil(pages_needed_raw);
    
    // Find first set of open pages
    int starting_page = -1;
    int spaceSize = 0;
    int idx;
    for(idx = 0; idx < NUM_OF_PAGES; idx++)
    {
	if(PAGES[idx] == 0)
	{
	    spaceSize++;
	}
	else
	{
	    if(spaceSize >= pages_needed)
	    {
		starting_page = idx - spaceSize;
	    }
	    spaceSize = 0;
	}
    }

    if(spaceSize >= pages_needed)
    {
	starting_page = idx - spaceSize;
    }
    
    if( starting_page == -1 || pages_needed > NUM_OF_PAGES)
    {
	errno = ENOMEM;
	pthread_mutex_unlock(&lock);
	return NULL;
    }
    
    ALLOCATIONS += 1;
    for(int i = starting_page; i < (starting_page + pages_needed); i++)
    {
	PAGES[i] = ALLOCATIONS;
    }
    
    // Starting position in memory array
    int memory_position = starting_page * PAGE_SIZE;

    return MEMORY + memory_position;
}

void *my_malloc(size_t size)
{
    pthread_mutex_lock(&lock);
    void* ptr = pseudo_malloc(size);
    pthread_mutex_unlock(&lock);
    return ptr;
}

/**
 * Deallocate a memory region that was returned by my_malloc() or
 * my_realloc().
 *
 * If @a ptr is not a pointer returned by my_malloc() or my_realloc(),
 * then raise a SIGSEGV signal to the calling thread. Likewise,
 * calling my_free() on a previously freed region results in a
 * SIGSEGV.
 *
 * @param ptr Pointer to memory region to free. If @c NULL, do
 * nothing.
 */

void pseudo_free(void *ptr)
{
    if( ptr != NULL )
    {
	// point to the beginning of the memory block
	void* memPtr = MEMORY;
	// difference betweem the ptr passed in and memPtr above
	int pageDiff = ptr - memPtr;
	
	int pageNum = pageDiff / PAGE_SIZE;
	
	if(PAGES[pageNum] == 0)
	{
	    pthread_mutex_unlock(&lock);
	    raise(SIGSEGV);
	}
	
	int allocationNum = PAGES[pageNum];
	PAGES[pageNum] = 0;
	for(int i = pageNum; i < NUM_OF_PAGES; i++)
	{
	    if( PAGES[i] == allocationNum )
	    {
		PAGES[i] = 0;
	    }
	}
    }
}

void my_free(void *ptr)
{
    pthread_mutex_lock(&lock);
    pseudo_free(ptr);
    pthread_mutex_unlock(&lock);
}

/**
 * Change the size of the memory block pointed to by @a ptr.
 *
 * - If @a ptr is @c NULL, then treat this as if a call to
 *   my_malloc() for the requested size.
 * - Else if @a size is @c 0, then treat this as if a call to
 *   my_free().
 * - Else if @a ptr is not a pointer returned by my_malloc() or
 *   my_realloc(), then send a SIGSEGV signal to the calling process.
 *
 * Otherwise reallocate @a ptr as follows:
 *
 * - If @a size is smaller than the previously allocated size, then
 *   reduce the size of the memory block. Mark the excess memory as
 *   available. Memory sizes are rounded up to the next 32-byte
 *   increment.
 * - If @a size is the same size as the previously allocated size,
 *   then do nothing.
 * - If @a size is greater than the previously allocated size, then
 *   allocate a new contiguous block of at least @a size bytes,
 *   rounded up to the next 32-byte increment. Copy the contents from
 *   the old to the new block, then free the old block.
 *
 * @param ptr Pointer to memory region to reallocate.
 * @param size Number of bytes to reallocate.
 *
 * @return If allocating a new memory block or if resizing a block,
 * then pointer to allocated memory; @a ptr will become invalid. If
 * freeing a memory region or if allocation fails, return @c NULL. If
 * out of memory, set errno to @c ENOMEM.
 */
void *my_realloc(void *ptr, size_t size)
{
    pthread_mutex_lock(&lock);
    // point to the beginning of the memory block
    void* memPtr = MEMORY;
    // difference betweem the ptr passed in and memPtr above
    int pageDiff = ptr - memPtr;
    
    int pageNum = pageDiff / PAGE_SIZE;
    
    if(ptr == NULL)
    {
	void* newPtr = pseudo_malloc(size);
	pthread_mutex_unlock(&lock);
	return newPtr;
    }
    else if(size == 0)
    {
	pseudo_free(ptr);
    }
    else if(PAGES[pageNum] == 0)
    {
	pthread_mutex_unlock(&lock);
	raise(SIGSEGV);
    }
    
    int memSize = 0;
    int allocationNum = PAGES[pageNum];

    for(int i = pageNum; i < NUM_OF_PAGES; i++)
    {
	if( PAGES[i] == allocationNum )
	{
	    memSize += 32;
	}
    }
    
    if(size > memSize)
    {
	void* newMem = pseudo_malloc(size);
	memcpy(newMem, ptr, memSize);
	pseudo_free(ptr);
	ptr = NULL;
	pthread_mutex_unlock(&lock);
	return newMem;
    }
    else if(size < memSize)
    {
	// Get a rough idea of how many pages I will need
	float pages_needed_raw = size / PAGE_SIZE;
	// Round raw number above to a square number
	int pages_needed = my_ceil(pages_needed_raw);

	int curr_num_of_pages = memSize / PAGE_SIZE;
	int curr_page = pageNum + (curr_num_of_pages - pages_needed) + 1;
	
	for(int i = curr_page; i < curr_num_of_pages; i++)
	{
	    PAGES[i] = 0;
	}
    }
    pthread_mutex_unlock(&lock);
    return ptr;
}

/**
 * Retrieve the size of an allocation block.
 *
 * If @a ptr is a pointer returned by my_malloc() or my_realloc(),
 * then return the size of the allocation block. Because my_malloc()
 * and my_realloc() round up to the next 32-byte increment, the
 * returned value may be larger than the originally requested amount.
 *
 * @return Usable size pointed to by @a ptr, or 0 if @a ptr is not a
 * pointer returned by my_malloc() or my_realloc() (such as @c NULL).
 */
size_t my_malloc_usable_size(void *ptr)
{
    pthread_mutex_lock(&lock);
    if(ptr == NULL)
    {
	pthread_mutex_unlock(&lock);
	return 0;
    }

    // point to the beginning of the memory block
    void* memPtr = MEMORY;
    // difference betweem the ptr passed in and memPtr above
    int pageDiff = ptr - memPtr;
    
    int pageNum = pageDiff / PAGE_SIZE;
    
    if(PAGES[pageNum] != 0)
    {
	int allocationNum = PAGES[pageNum];
	int numOfPages = 0;
	for(int i = pageNum; i < NUM_OF_PAGES; i++)
	{
	    if( PAGES[i] == allocationNum )
	    {
		numOfPages++;
	    }
	}
	
	int memSize = numOfPages * 32;
	pthread_mutex_unlock(&lock);
	return memSize;

    }
    pthread_mutex_unlock(&lock);
    return 0;
}

int main(int argc, char *argv[])
{
    initialize_memory();
    
    hw4_test();

    return 0;
}
