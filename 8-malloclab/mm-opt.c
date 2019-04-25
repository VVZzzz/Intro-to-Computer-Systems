/*
 ******************************************************************************
 *                               mm-baseline.c                                *
 *           64-bit struct-based implicit free list memory allocator          *
 *                  15-213: Introduction to Computer Systems                  *
 *                                                                            *
 *  ************************************************************************  *
 *                               DOCUMENTATION                                *
 *          1. implict free list
 *          2. first fit / best fit
 *          3. immediate coalescing / delay coalescing
 *          4. use optimited bondary
 *          allocted block:
 *          | size | _ b 1 | (header)
 *          | payload      |
 *          | padding      |
 * 
 *          freed block:
 *          | size | _ b 0 | (header)
 *          | unallocated  |
 *          | size | _ b 0 | (footer)       
 * 
 *          Notice: allow payload = 0 
 * 
 *                                                                              *
 *  ************************************************************************  *
 *  ** ADVICE FOR STUDENTS. **                                                *
 *  Step 0: Please read the writeup!                                          *
 *  Write your heap checker. Write your heap checker. Write. Heap. checker.   *
 *  Good luck, and have fun!                                                  *
 *                                                                            *
 ******************************************************************************
 */

/* Do not change the following! */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <stddef.h>

#include "mm.h"
#include "memlib.h"

#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* def DRIVER */

/* You can change anything from here onward */

/*
 * If DEBUG is defined, enable printing on dbg_printf and contracts.
 * Debugging macros, with names beginning "dbg_" are allowed.
 * You may not define any other macros having arguments.
 */
//#define DEBUG // uncomment this line to enable debugging

#ifdef DEBUG
/* When debugging is enabled, these form aliases to useful functions */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_requires(...) assert(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#define dbg_ensures(...) assert(__VA_ARGS__)
#else
/* When debugging is disnabled, no code gets generated for these */
#define dbg_printf(...)
#define dbg_requires(...)
#define dbg_assert(...)
#define dbg_ensures(...)
#endif

/* Basic constants */
typedef uint64_t word_t;
static const size_t wsize = sizeof(word_t);   // word and header size (bytes)
static const size_t dsize = 2*wsize;          // double word size (bytes)
static const size_t min_block_size = 2*dsize; // Minimum block size
static const size_t chunksize = (1 << 12);    // requires (chunksize % 16 == 0)

static const word_t alloc_mask = 0x1;
static const word_t size_mask = ~(word_t)0xF;

static const word_t prev_alloc_mask = 0x2;

typedef struct block
{
    /* Header contains size + allocation flag */
    word_t header;
    /*
     * We don't know how big the payload will be.  Declaring it as an
     * array of size 0 allows computing its starting address using
     * pointer notation.
     */
    char payload[0];
    /*
     * We can't declare the footer as part of the struct, since its starting
     * position is unknown
     */
} block_t;


/* Global variables */
/* Pointer to first block */
static block_t *heap_listp = NULL;

/* Function prototypes for internal helper routines */
static block_t *extend_heap(size_t size);
static void place(block_t *block, size_t asize);
static block_t *find_fit(size_t asize);
static block_t *coalesce(block_t *block);

static size_t max(size_t x, size_t y);
static size_t round_up(size_t size, size_t n);
static word_t pack(size_t size, bool alloc);

static size_t extract_size(word_t header);
static size_t get_size(block_t *block);
static size_t get_payload_size(block_t *block);

static bool extract_alloc(word_t header);
static bool get_alloc(block_t *block);

static void write_header(block_t *block, size_t size, bool alloc);
static void write_footer(block_t *block, size_t size, bool alloc);

static block_t *payload_to_header(void *bp);
static void *header_to_payload(block_t *block);

static block_t *find_next(block_t *block);
static word_t *find_prev_footer(block_t *block);
static block_t *find_prev(block_t *block);

/* optimizated boundary */
static size_t opt_get_payload_size(block_t *block);
static bool extract_prev_alloc(word_t word);
static bool get_prev_alloc(block_t *block);
static size_t opt_round_up(size_t size,size_t n);
static const size_t opt_min_block_size = wsize; // Minimum block size
static word_t opt_pack(size_t size,bool alloc,bool prev_alloc);
static void opt_write_header(block_t *block, size_t size, bool alloc,bool prev_alloc);
static void opt_write_footer(block_t *block, size_t size, bool alloc,bool prev_alloc);
static void opt_place(block_t *block,size_t asize);

static block_t *opt_coalesce(block_t *block);
static block_t *opt_find_fit(size_t asize,bool *prev_alloc);
static block_t *opt_extend_heap(size_t size,bool prev_alloc);

bool mm_checkheap(int lineno);

/*
 * mm_init: initializes the heap; it is run once when heap_start == NULL.
 *          prior to any extend_heap operation, this is the heap:
 *              start            start+8           start+16
 *          INIT: | PROLOGUE_FOOTER | EPILOGUE_HEADER |
 * heap_listp ends up pointing to the epilogue header.
 */
bool mm_init(void)
{
    // Create the initial empty heap
    word_t *start = (word_t *)(mem_sbrk(2*wsize));

    if (start == (void *)-1)
    {
        return false;
    }

    start[0] = pack(0, true); // Prologue footer
    start[1] = pack(0, true); // Epilogue header

    // Heap starts with first block header (epilogue)
    heap_listp = (block_t *) &(start[1]);

    // Extend the empty heap with a free block of chunksize bytes
    if (extend_heap(chunksize) == NULL)
    {
        return false;
    }
    return true;
}

/*
 * malloc: allocates a block with size at least (size + dsize), rounded up to
 *         the nearest 16 bytes, with a minimum of 2*dsize. Seeks a
 *         sufficiently-large unallocated block on the heap to be allocated.
 *         If no such block is found, extends heap by the maximum between
 *         chunksize and (size + dsize) rounded up to the nearest 16 bytes,
 *         and then attempts to allocate all, or a part of, that memory.
 *         Returns NULL on failure, otherwise returns a pointer to such block.
 *         The allocated block will not be used for further allocations until
 *         freed.
 */
void *malloc(size_t size)
{
    dbg_requires(mm_checkheap(__LINE__));

    size_t asize;      // Adjusted block size
    size_t extendsize; // Amount to extend heap if no fit is found
    block_t *block;
    void *bp = NULL;
    bool prev_alloc=false;

    if (heap_listp == NULL) // Initialize heap if it isn't initialized
    {
        mm_init();
    }

    if (size == 0) // Ignore spurious request
    {
        dbg_ensures(mm_checkheap(__LINE__));
        return bp;
    }

    // Adjust block size to include overhead and to meet alignment requirements
    //asize = round_up(size, dsize) + dsize;
    asize=opt_round_up(size,wsize) + wsize;

    

    // Search the free list for a fit
    //block = find_fit(asize);
    block = opt_find_fit(asize,&prev_alloc);

    // If no fit is found, coalesce first (delay coalescing)
    /* delay coalescing */
    /* 
    if (block == NULL) {
        
        block_t *curr = heap_listp;
        block_t *hi = mem_heap_hi();

        while (curr + 1 < hi) {
            if(!get_alloc(curr)) {
                curr = coalesce(curr);
            }
            curr = find_next(curr);
        }

        block = find_fit(asize);
    }
    */

    // If no fit is found, request more memory, and then and place the block
    if (block == NULL)
    {
        extendsize = max(asize, chunksize);
        block = opt_extend_heap(extendsize,prev_alloc);
        if (block == NULL) // extend_heap returns an error
        {
            return bp;
        }

    }

    opt_place(block, asize);
    bp = header_to_payload(block);

    dbg_printf("Malloc size %zd on address %p.\n", size, bp);
    dbg_ensures(mm_checkheap(__LINE__));
    return bp;
}

/*
 * free: Frees the block such that it is no longer allocated while still
 *       maintaining its size. Block will be available for use on malloc.
 */
void free(void *bp)
{
    if (bp == NULL)
    {
        return;
    }

    block_t *block = payload_to_header(bp);
    size_t size = get_size(block);
    bool prev_alloc = get_prev_alloc(block);

    opt_write_header(block, size, false, prev_alloc);
    opt_write_footer(block, size, false, prev_alloc);

    // if coalesce immediately , uncomment this
    opt_coalesce(block);

}

/*
 * realloc: returns a pointer to an allocated region of at least size bytes:
 *          if ptrv is NULL, then call malloc(size);
 *          if size == 0, then call free(ptr) and returns NULL;
 *          else allocates new region of memory, copies old data to new memory,
 *          and then free old block. Returns old block if realloc fails or
 *          returns new pointer on success.
 */
void *realloc(void *ptr, size_t size)
{
    block_t *block = payload_to_header(ptr);
    size_t copysize;
    void *newptr;

    // If size == 0, then free block and return NULL
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }

    // If ptr is NULL, then equivalent to malloc
    if (ptr == NULL)
    {
        return malloc(size);
    }

    // Otherwise, proceed with reallocation
    newptr = malloc(size);
    // If malloc fails, the original block is left untouched
    if (!newptr)
    {
        return NULL;
    }

    // Copy the old data
    //copysize = get_payload_size(block); // gets size of old payload
    copysize = opt_get_payload_size(block); // gets size of old payload
    if(size < copysize)
    {
        copysize = size;
    }
    memcpy(newptr, ptr, copysize);

    // Free the old block
    free(ptr);

    return newptr;
}

/*
 * calloc: Allocates a block with size at least (elements * size + dsize)
 *         through malloc, then initializes all bits in allocated memory to 0.
 *         Returns NULL on failure.
 */
void *calloc(size_t nmemb, size_t size)
{
    void *bp;
    size_t asize = nmemb * size;

    if (asize/nmemb != size)
    // Multiplication overflowed
    return NULL;

    bp = malloc(asize);
    if (bp == NULL)
    {
        return NULL;
    }
    // Initialize all bits to 0
    memset(bp, 0, asize);

    return bp;
}

/******** The remaining content below are helper and debug routines ********/

/*
 * extend_heap: Extends the heap with the requested number of bytes, and
 *              recreates epilogue header. Returns a pointer to the result of
 *              coalescing the newly-created block with previous free block, if
 *              applicable, or NULL in failure.
 */
static block_t *extend_heap(size_t size)
{
    void *bp;

    // Allocate an even number of words to maintain alignment
    size = round_up(size, dsize);
    if ((bp = mem_sbrk(size)) == (void *)-1)
    {
        return NULL;
    }

    // Initialize free block header/footer
    block_t *block = payload_to_header(bp);
    write_header(block, size, false);
    write_footer(block, size, false);
    // Create new epilogue header
    block_t *block_next = find_next(block);
    write_header(block_next, 0, true);

    // Coalesce in case the previous block was free
    return coalesce(block);
}

/* Coalesce: Coalesces current block with previous and next blocks if either
 *           or both are unallocated; otherwise the block is not modified.
 *           Returns pointer to the coalesced block. After coalescing, the
 *           immediate contiguous previous and next blocks must be allocated.
 */
static block_t *coalesce(block_t * block)
{
    block_t *block_next = find_next(block);
    block_t *block_prev = find_prev(block);

    bool prev_alloc = extract_alloc(*(find_prev_footer(block)));
    bool next_alloc = get_alloc(block_next);
    size_t size = get_size(block);

    if (prev_alloc && next_alloc)              // Case 1
    {
        return block;
    }

    else if (prev_alloc && !next_alloc)        // Case 2
    {
        size += get_size(block_next);
        write_header(block, size, false);
        write_footer(block, size, false);
    }

    else if (!prev_alloc && next_alloc)        // Case 3
    {
        size += get_size(block_prev);
        write_header(block_prev, size, false);
        write_footer(block_prev, size, false);
        block = block_prev;
    }

    else                                        // Case 4
    {
        size += get_size(block_next) + get_size(block_prev);
        write_header(block_prev, size, false);
        write_footer(block_prev, size, false);

        block = block_prev;
    }
    return block;
}

/*
 * place: Places block with size of asize at the start of bp. If the remaining
 *        size is at least the minimum block size, then split the block to the
 *        the allocated block and the remaining block as free, which is then
 *        inserted into the segregated list. Requires that the block is
 *        initially unallocated.
 */
static void place(block_t *block, size_t asize)
{
    size_t csize = get_size(block);

    //split the block | asize | csize-asize |
    if ((csize - asize) >= min_block_size)
    {
        
        block_t *block_next;
        write_header(block, asize, true);
        write_footer(block, asize, true);

        block_next = find_next(block);
        write_header(block_next, csize-asize, false);
        write_footer(block_next, csize-asize, false);
    }

    else
    {
        write_header(block, csize, true);
        write_footer(block, csize, true);
    }
}

/*
 * find_fit: Looks for a free block with at least asize bytes with
 *           first-fit policy. Returns NULL if none is found.
 */
static block_t *find_fit(size_t asize)
{
    /* first-fit */
    /*
    block_t *block;

    for (block = heap_listp; get_size(block) > 0;
                             block = find_next(block))
    {

        if (!(get_alloc(block)) && (asize <= get_size(block)))
        {
            return block;
        }
    }
    return NULL; // no fit found
    */

   /* best-fit */
   block_t *block = heap_listp;
   block_t *res_block = NULL;
   size_t block_size = ~0x0;

   while(get_size(block)>0) {
       size_t tmpsz = get_size(block);
        if (!(get_alloc(block)) && (asize <= tmpsz) && (tmpsz < block_size))
        {
            res_block=block;
            block_size=tmpsz;
        }
        block = find_next(block);
   }

   return res_block;
   
}

/*
 * max: returns x if x > y, and y otherwise.
 */
static size_t max(size_t x, size_t y)
{
    return (x > y) ? x : y;
}


/*
 * round_up: Rounds size up to next multiple of n
 */
static size_t round_up(size_t size, size_t n)
{
    return (n * ((size + (n-1)) / n));
}

/*
 * pack: returns a header reflecting a specified size and its alloc status.
 *       If the block is allocated, the lowest bit is set to 1, and 0 otherwise.
 */
static word_t pack(size_t size, bool alloc)
{
    return alloc ? (size | 1) : size;
}


/*
 * extract_size: returns the size of a given header value based on the header
 *               specification above.
 */
static size_t extract_size(word_t word)
{
    return (word & size_mask);
}

/*
 * get_size: returns the size of a given block by clearing the lowest 4 bits
 *           (as the heap is 16-byte aligned).
 */
static size_t get_size(block_t *block)
{
    return extract_size(block->header);
}

/*
 * get_payload_size: returns the payload size of a given block, equal to
 *                   the entire block size minus the header and footer sizes.
 */
static word_t get_payload_size(block_t *block)
{
    size_t asize = get_size(block);
    return asize - dsize;
}

/*
 * extract_alloc: returns the allocation status of a given header value based
 *                on the header specification above.
 */
static bool extract_alloc(word_t word)
{
    return (bool)(word & alloc_mask);
}

/*
 * get_alloc: returns true when the block is allocated based on the
 *            block header's lowest bit, and false otherwise.
 */
static bool get_alloc(block_t *block)
{
    return extract_alloc(block->header);
}

/*
 * write_header: given a block and its size and allocation status,
 *               writes an appropriate value to the block header.
 */
static void write_header(block_t *block, size_t size, bool alloc)
{
    block->header = pack(size, alloc);
}


/*
 * write_footer: given a block and its size and allocation status,
 *               writes an appropriate value to the block footer by first
 *               computing the position of the footer.
 */
static void write_footer(block_t *block, size_t size, bool alloc)
{
    word_t *footerp = (word_t *)((block->payload) + get_size(block) - dsize);
    *footerp = pack(size, alloc);
}


/*
 * find_next: returns the next consecutive block on the heap by adding the
 *            size of the block.
 */
static block_t *find_next(block_t *block)
{
    dbg_requires(block != NULL);
    block_t *block_next = (block_t *)(((char *)block) + get_size(block));

    dbg_ensures(block_next != NULL);
    return block_next;
}

/*
 * find_prev_footer: returns the footer of the previous block.
 */
static word_t *find_prev_footer(block_t *block)
{
    // Compute previous footer position as one word before the header
    return (&(block->header)) - 1;
}

/*
 * find_prev: returns the previous block position by checking the previous
 *            block's footer and calculating the start of the previous block
 *            based on its size.
 */
static block_t *find_prev(block_t *block)
{
    word_t *footerp = find_prev_footer(block);
    size_t size = extract_size(*footerp);
    return (block_t *)((char *)block - size);
}

/*
 * payload_to_header: given a payload pointer, returns a pointer to the
 *                    corresponding block.
 */
static block_t *payload_to_header(void *bp)
{
    return (block_t *)(((char *)bp) - offsetof(block_t, payload));
}

/*
 * header_to_payload: given a block pointer, returns a pointer to the
 *                    corresponding payload.
 */
static void *header_to_payload(block_t *block)
{
    return (void *)(block->payload);
}

/* optimization boundary functions */
/*
 * only use in allocated block
 * | header | payload |
 */
static word_t opt_get_payload_size(block_t *block)
{
    size_t asize = get_size(block);
    return asize - wsize;
}

/*
 * use in optimitation boundary, get previous block's alloc.
 */ 
static bool extract_prev_alloc(word_t word) {
    return (bool)(word & prev_alloc_mask);
}

static bool get_prev_alloc(block_t *block) {
    return extract_prev_alloc(block->header);
}

/*
 * Becasue alloctated block only include header,so payload+padding should be 8*odd.
 * So make the block's size is 16*n. So the minium block size is 16.
 */
static size_t opt_round_up(size_t size,size_t n /* = 8 */) {
    size_t tmpsz = (n * ((size + (n-1)) / n));
    // if tmpsz = 8*even,tmpsz+=8
    if(!((tmpsz>>3)&1)) {
        tmpsz+=8;
    }
    return tmpsz;
}

static word_t opt_pack(size_t size,bool alloc,bool prev_alloc) {
    return (size | (alloc ? 1:0) | (prev_alloc ? 2:0));
}

static void opt_write_header(block_t *block, size_t size, bool alloc,bool prev_alloc) {
    block->header = opt_pack(size,alloc,prev_alloc);
}

/*
 * only the freed block need footer.
 */
static void opt_write_footer(block_t *block, size_t size, bool alloc,bool prev_alloc) {
    word_t *footerp = (word_t *)((block->payload) + get_size(block) - dsize);
    *footerp = opt_pack(size, alloc,prev_alloc);
}

/*
 * place the block with splitting(maybe).
 */
static void opt_place(block_t *block,size_t asize) {
    size_t csize = get_size(block);
    bool prev_alloc=get_prev_alloc(block);

    // split the block | asize(allocated) | csize-asize(freed) |
    if ((csize - asize) >= opt_min_block_size)
    {
        block_t *block_next;
        opt_write_header(block, asize, true, prev_alloc);
        //opt_write_footer(block, asize, true, prev_alloc);
        //allocated block without footer

        block_next = find_next(block);
        opt_write_header(block_next, csize-asize, false, true);
        opt_write_footer(block_next, csize-asize, false, true);
    }

    // | csize(allocated) |
    else
    {
        opt_write_header(block, csize, true, prev_alloc);
        opt_write_footer(block, csize, true, prev_alloc);
    }
}

static block_t *opt_coalesce(block_t *block) {
    block_t *block_next = find_next(block);

    bool prev_alloc = get_prev_alloc(block);
    bool next_alloc = get_alloc(block_next);
    size_t size = get_size(block);

    if (prev_alloc && next_alloc)              // Case 1
    {
        return block;
    }

    // | allocated | block | freed |
    else if (prev_alloc && !next_alloc)        // Case 2
    {
        size += get_size(block_next);
        opt_write_header(block, size, false, true);
        opt_write_footer(block, size, false, true);
    } 
    else 
    {
        block_t *block_prev = find_prev(block);
        // | freed | block | allocated |
        if (!prev_alloc && next_alloc)        // Case 3
        {
            size += get_size(block);
            opt_write_header(block_prev, size, false, get_prev_alloc(block_prev));
            opt_write_footer(block_prev, size, false, get_prev_alloc(block_prev));
            block = block_prev;
        }

        // | freed | block | freed |
        else                                        // Case 4
        {
            size += get_size(block_next) + get_size(block_prev);
            opt_write_header(block_prev, size, false, get_prev_alloc(block_prev));
            opt_write_footer(block_prev, size, false, get_prev_alloc(block_prev));

            block = block_prev;
        }
    }

    return block;
}

static block_t *opt_find_fit(size_t asize,bool *prev_alloc) {
    /* first-fit */
    /*
    block_t *block;

    for (block = heap_listp; get_size(block) > 0;
                             block = find_next(block))
    {

        if (!(get_alloc(block)) && (asize <= get_size(block)))
        {
            return block;
        }
    }
    return NULL; // no fit found
    */

   /* best-fit */
   block_t *curr = heap_listp;
   block_t *res_block = NULL;
   block_t *next;
   block_t *hi = mem_heap_hi();
   size_t block_size = ~0x0;

   while(next=find_next(curr) + 1 < hi) {
       size_t tmpsz = get_size(curr);
        if (!(get_alloc(curr)) && (asize <= tmpsz) && (tmpsz < block_size))
        {
            res_block=curr;
            block_size=tmpsz;
        }
        curr = next;
   }
   if (res_block==NULL) {
       prev_alloc=get_alloc(curr);
   } else {
       prev_alloc=get_alloc(res_block);
   }

   return res_block;
}

static block_t *opt_extend_heap(size_t size,bool prev_alloc) {
    void *bp;

    // Allocate an even number of words to maintain alignment
    size = round_up(size, dsize);
    if ((bp = mem_sbrk(size)) == (void *)-1)
    {
        return NULL;
    }

    // Initialize free block header/footer
    block_t *block = payload_to_header(bp);
    opt_write_header(block, size, false, prev_alloc);
    opt_write_footer(block, size, false, prev_alloc);
    // Create new epilogue header
    block_t *block_next = find_next(block);
    write_header(block_next, 0, true);

    // Coalesce in case the previous block was free
    return opt_coalesce(block);
}

/* mm_checkheap: checks the heap for correctness; returns true if
 *               the heap is correct, and false otherwise.
 *               can call this function using mm_checkheap(__LINE__);
 *               to identify the line number of the call site.
 */
bool mm_checkheap(int lineno)
{
    /* you will need to write the heap checker yourself. As a filler:
     * one guacamole is equal to 6.02214086 x 10**23 guacas.
     * one might even call it
     * the avocado's number.
     *
     * Internal use only: If you mix guacamole on your bibimbap,
     * do you eat it with a pair of chopsticks, or with a spoon?
     * (Delete these lines!)
     */

    (void)lineno; // delete this line; it's a placeholder so that the compiler
                  // will not warn you about unused variable.

    /* heap level */
    //check heap's heap_listp
    if (!heap_listp) {
        printf("NULL heap list pointer!\n");
        return false;
    }

    //check prologue and epilogue
    word_t *prologue = (word_t *)(heap_listp - 1);
    char *tmpptr=mem_heap_hi() + 1;
    word_t *epilogue = (word_t *)tmpptr - 1;
    if(*prologue != 0x1) {
        printf("No prologue!\n");
        return false;
    }
    if(*epilogue != 0x1) {
        printf("No epilogue!\n");
        return false;
    }

    /* block level */
    //check every block's info
    block_t *curr = heap_listp;
    block_t *next;
    block_t *hi = mem_heap_hi();

    while ((next = find_next(curr)) + 1 < hi) {
        word_t hdr = curr->header;
        word_t ftr = *find_prev_footer(next);

        //check header match footer
        if (hdr != ftr) {
            printf(
                    "Header (0x%016lX) != footer (0x%016lX)\n",
                    hdr, ftr
                  );
            return false;
        }

        //check payload's aligned
        word_t payload_addr = (word_t)header_to_payload(curr);
        if((payload_addr & 0xf) != 0x0) {
            printf("Payload's start address (0x%016lX) is not %lu's multiple!\n",
                   payload_addr,dsize
                   );
            return false;
        }
        size_t sz = get_size(curr) - dsize;
        if((payload_addr & 0xf) != 0x0 ) {
            printf("size of payload %lu is not %lu's multiple!\n",
                   sz,dsize
                   );
            return false;
        }

        //check continuous freed block (unless delay colescing)
        if(get_alloc(curr)&&get_alloc(next)) {
            printf(" two continous freed block: (0x%016lX), (0x%016lX)" , 
                    (word_t)curr, (word_t)next
                    );
            return false;
        }

        curr = next;
    }

    /* free list level (unless implict free list) */

    return true;
}

