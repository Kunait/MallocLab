/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * ******************************
 * |Size/Flag|     Payload      |   Allocated block
 * ******************************
 * 
 * 
 * **************************************************************************
 * |Size/Flag| Next free | Previous free|    Empty memory        | Size/Flag|  Free Block
 * **************************************************************************
 * 
 * Size/Set - 4 Bytes
 * Next free block - 4 Bytes
 * Previous free block - 4 Bytes
 * 
 * 
 * In an allocated block the size of the block is stored (and a flag to check if allocated)
 * 
 * In a free block the size is stored as header and footer, furthermore the next and previous free blocks are
 * stored after the header.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Gripper",
    /* First member's full name */
    "Cüneyd Tas",
    /* First member's email address */
    "cueneyt.tas@stud.uni-due.de",
    /* Second member's full name (leave blank if none) */
    "Dennis Dirk Becker",
    /* Second member's email address (leave blank if none) */
    "dennis.becker.92@stud.uni-due.de"
};

#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

//size_t aligned
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//Size of header
#define SIZE_HEADER sizeof(size_t)

//Our defined number of segregated lists
#define LIST 32

//With this mask we can retrieve the size of the block without the allocated flag
#define SMASK ~0x7

//Minimum size which can be used for allocation
#define MBUFFER 1 << 12

//This macro helps us with the block alignment, if the size is smaller than our alignment (8), then the block automatically gets the size 16
//If the size is larger than our alignment, then size_header and alignment is added, 1 is subtracted and our SMASK is applied
//To get the size without the allocated flag
#define BLOCK_ALIGNMENT(size) (((size) < ALIGNMENT) ? (ALIGNMENT << 1) : (((size) + SIZE_HEADER + ALIGNMENT - 1) & SMASK))

//This macro just jumps to the next header
#define NEXT_HEADER(p) ((p) + (SIZE_HEADER << 1))


//These two macros return the value/pointer of next free block
#define NEXT_FREE_BLOCK_VALUE(p) (*(size_t*)((p) + SIZE_HEADER))

#define NEXT_FREE_BLOCK_POINTER(p) ((p) + NEXT_FREE_BLOCK_VALUE(p))

//This macro returns size of block by applying the SMASK
#define SIZE_BLOCK(p) (*(size_t*)(p) & SMASK)

#define ADDR_PAYLOAD(p) ((p)+SIZE_HEADER)

//Macro to define when list ends -> if next free block value = 0
#define LIST_END(p) ((NEXT_FREE_BLOCK_VALUE(p))==0)

//Macro sets header and footer of free block
#define FREE_HF(p, size) (*(size_t*)(p) = (size)); (*(size_t*)((p)+(size)-SIZE_HEADER)=(size))

//Macro calculated next block in heap
#define NEXT_BLOCK(p) ((p)+(*(size_t*)(p) & SMASK))

//Macro calculates block address from payload address
#define ADDR_HEADER(p) ((char*)(p) - SIZE_HEADER)

//Macro checks if address is within heap with mem_heap_hi
#define WITHIN_HEAP(p) ((p) <= (char*)mem_heap_hi())

//FMASK is used to retrieve allocated flag
#define FMASK 0x7

//Check if allocated
#define SET(p) ((*(size_t*)(p)&FMASK)==0x1)

#define PREV_FREE_BLOCK_VALUE(p) (*(size_t*)((p)+(SIZE_HEADER<<1)))

//Macro gets pointer to previous free block by adding the value of the previous free block to p
#define PREV_FREE_BLOCK_POINTER(p) ((p) + PREV_FREE_BLOCK_VALUE(p))

//Macro sets next free block
#define SET_NEXT_BLOCK(p, np) (NEXT_FREE_BLOCK_VALUE(p) = (size_t)((np)-(p)))

//Macro sets previous free block
#define SET_PREV_BLOCK(p, pp) (PREV_FREE_BLOCK_VALUE(p) = (size_t)((pp)-(p)))

//Macro sets header and sets allocated flag
#define SET_HEADER(p, size) (*(size_t*)(p) = (size) | 0x1)

int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
char *get_free_list(size_t size);
void remove_list(char *p);
void split(char *p, size_t size);
char *get_prev_block(char *p);
void insert(char *p);
int mm_check(void);

char *start;
char *first;

/*
 * mm_init - initialize the malloc package.
 *
 * In this function the lists are defined by creating memory at the beginning of our heap to store all headers.
 * Then a free block is created by allocating and freeing a block.
 */
int mm_init(void){
	
	//size_t is defined as value large enough to hold all heads of lists
	//2 headers, 32 lists -> we store 64 headers, 2 headers per list
    size_t listsSize = (ALIGN(2* SIZE_HEADER * LIST)) - SIZE_HEADER;
    if ((start = (char*)mem_sbrk(listsSize)) == (char*)-1) return -1;
    memset(start, 0, mem_heapsize());

	//first pointer is our first real usable address,as we use the first part for our lists
	first = (char*)mem_heap_hi()+1;
	mm_free(mm_malloc(MBUFFER));
	return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 * 
 * If the size of the block is 0 -> NULL is returned.
 * 
 * If size < 4096 -> round up to next power of 2
 * 
 * Then the block is aligned and all lists are iterated through until a list with a free block (that is large enough) is found. 
 * 
 * If no block is found: New block (min. size 4096) is initialized, and split to fit our size.
 * 
 * The address of payload is returned. 
 * 
 */
void *mm_malloc(size_t size){
    if(size == 0)return NULL;
	//if size < 4096 we round up to nearest divisible by 2 by shifting left until our value is larger than original
	if(size < MBUFFER){
		size_t nearest = 1;
		while(nearest < size) nearest <<= 1;
		size = nearest;
	}
	char *list, *p;
	//we align our size here
	size_t newsize = BLOCK_ALIGNMENT(size);

	//Here we iterate through the free lists
	for(list = get_free_list(newsize); list <= first - ALIGNMENT; list = NEXT_HEADER(list)){
		//When we have chosen a list we take its first element (free block)
		for(p = NEXT_FREE_BLOCK_POINTER(list);; p = NEXT_FREE_BLOCK_POINTER(p)){
			if(SIZE_BLOCK(p) >= newsize){
				//If the selected block is large enough we remove it from the list
				remove_list(p);
				//And split it
				split(p, newsize);
				//then we return the allocated pointer at the payload
				return (void*)ADDR_PAYLOAD(p);
			}else if(LIST_END(p))break; //If the block is not large enough ,we iterate through the others until we find a
			//Block that is large enough or the list ends. If list ends we choose the next bigger list
		}	
	}

	//If we were unable to find a free block large enough:

	//We never want to initialize less than 4096 (MBUFFER) new memory, so we check if newsize is large enough
	size_t bufsize = newsize > MBUFFER ? newsize : MBUFFER;
	if((p = (char*)mem_sbrk(bufsize)) == (char*)-1)return NULL;

	//The newly created free block gets header and footer
	FREE_HF(p, bufsize);
	//and is split
	split(p, newsize);
	return (void*)ADDR_PAYLOAD(p);
}

/*
 * mm_free - 
 * 
 * A given block is freed. If the given pointer is NUll the function just returns, if it is not NULL: previous and next blocks are calculated and size of block is determined,
 * if the previous block is not null: the previous block is removed from its list and the blocks are combined.
 * 
 * If the next block pointer is within our heap and not allocated: it is also removed from its list and added to our free block.
 * 
 * At the end the free block receives a new header and footer, and is inserted into a list.
 */
void mm_free(void *ptr){
	if(ptr == NULL)return;


	char *p = ADDR_HEADER(ptr);
	char *pp = get_prev_block(p);
	char *np = NEXT_BLOCK(p);

	size_t size = SIZE_BLOCK(p);
	if(pp != NULL){
		remove_list(pp);
		size += SIZE_BLOCK(pp);
		p = pp;
	}
	if(WITHIN_HEAP(np)&&!SET(np)){
		remove_list(np);
		size += SIZE_BLOCK(np);
	}
	FREE_HF(p, size);
	insert(p);
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    /*
     * ATTENTION: You do not need to implement realloc for this assignment
     */
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/*
* Here we choose our free list, we start with the first list stored in the beginning of start
* We shift size right 4 bits and store it in order, we then enter a loop which further shifts order
* to the right until order==0 or we have no more lists
* This way we select lists by powers of two, e.g. if our size is 16 we would choose the first list
* as the loop is skipped and we choose the first list at the beginning of start.
* If size = 4096 then order starts at 256->128->64->32->16->8->4->2->1 we would use the 9th list
*
* Pointer to free list is returned
*/
char *get_free_list(size_t size){
	char *list = start;
	size_t order = size >> 4;
	size_t n = 1;
	while(order > 1 && n < LIST){
		order >>= 1;
		++n;
		list = NEXT_HEADER(list);
	}
	return list;
}
/*
* Item is removed from its list. If the item is at end of list: the previous item becomes end of list (next block of pp becomes pp, points to itself)
*
* 
*
*/
void remove_list(char *p){
	char *pp = PREV_FREE_BLOCK_POINTER(p);
	char *np = NEXT_FREE_BLOCK_POINTER(p);
	if(LIST_END(p)) SET_NEXT_BLOCK(pp, pp);
	else{
		SET_NEXT_BLOCK(pp, np);
		SET_PREV_BLOCK(np, pp);
	}
}

/*
* This function splits the given block into two.
* If the needed block is smaller than the block to be splitted:
*
* We set the header of the resulting first block to the size of the block needed,
* then we calculate the pointer to the next pointer which will be created by the split method.	
* This block gets a new header and footer with the relevant size (old block size - size of newly created block)
* At the end the (now smaller) free block is inserted into a list.
*
*
* If the needed block is not smaller: Block header is set to its old size but *allocated*.
*/
void split(char *p, size_t size){
	char *np;
	size_t old = SIZE_BLOCK(p);
	if(size < old - ALIGNMENT){
		SET_HEADER(p, size);
		//np is pointer to next block
		np = NEXT_BLOCK(p);
		//header and footer of resulting block is set
		FREE_HF(np, old - size);
		//free block is inserted to free list
		insert(np);
	}else SET_HEADER(p, old);
}

/*
* This function returns a pointer to the previous block if it is free and exists
* To do that we firstly check if the theoretical header and footer of the previous block have possible values
* If they do we check the relevant list, if the list contains the block then it is a previous free block and exists -> Pointer is returned
* If previous block does not exist, is not large enough, is not aligned or is not free -> NULL is returned
*
* Pointer to previous block is returned
*/
char *get_prev_block(char *p){
	char *list, *t;

	//Footer of previous block
	char *pf = p - SIZE_HEADER;

	//Header of previous block
	char *ph = p - SIZE_BLOCK(pf);

	//If allocated or ph before first possible address or block too small or not aligned or header and footer don't have same size
	if(SET(pf) || ph < first || ph > pf - SIZE_HEADER * 3 ||(size_t)(ph + SIZE_HEADER) % ALIGNMENT != 0 || SIZE_BLOCK(ph) != SIZE_BLOCK(pf) || SET(ph)) return NULL;
	//We get the free list with large enough blocks
	list = get_free_list(SIZE_BLOCK(ph));
	//We iterate through list
	for(t = NEXT_FREE_BLOCK_POINTER(list);; t = NEXT_FREE_BLOCK_POINTER(t)){
		//If the address of the current list item is equal to ph -> previous block was found and can be returned!
		if(ph == t) return ph;
		else if(LIST_END(t)) break;
	}
	//Previous block not found in lists -> not empty or does not exist
	return NULL;
}
/*
* This function inserts a free block into the relevant list (fitting size)
* If the list is empty the inserted block is inserted as the first item. (next of list becomes p, previous of p becomes list)
*
* If the list is not empty:
*
* We iterate through the list, until we find a block that is larger than our block. If a block is found, then our block is inserted before the larger block.
* If no larger block is found, then the block is added to the end of the list.
* 
*/
void insert(char *p){
	char *list = get_free_list(SIZE_BLOCK(p));
	if(!LIST_END(list)){
		while(1){
			if(SIZE_BLOCK(NEXT_FREE_BLOCK_POINTER(list)) >= SIZE_BLOCK(p)){
				SET_NEXT_BLOCK(p, NEXT_FREE_BLOCK_POINTER(list));
				SET_PREV_BLOCK(NEXT_FREE_BLOCK_POINTER(list), p);
				SET_NEXT_BLOCK(list, p);
				SET_PREV_BLOCK(p, list);
				return;
			}else if(LIST_END(NEXT_FREE_BLOCK_POINTER(list))){
				SET_NEXT_BLOCK(NEXT_FREE_BLOCK_POINTER(list), p);
				SET_NEXT_BLOCK(p, p);
				SET_PREV_BLOCK(p, NEXT_FREE_BLOCK_POINTER(list));
				return;
			}
			list = NEXT_FREE_BLOCK_POINTER(list);
		}
	}else{
		SET_NEXT_BLOCK(list, p);
		SET_NEXT_BLOCK(p, p);
		SET_PREV_BLOCK(p, list);
		return;
	}
}
