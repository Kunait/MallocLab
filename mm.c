/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
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
    "NES",
    /* First member's full name */
    "Cüneyd Tas",
    /* First member's email address */
    "cueneyt.tas@stud.uni-due.de",
    /* Second member's full name (leave blank if none) */
    "Dennis Becker",
    /* Second member's email address (leave blank if none) */
    "dennis.becker@stud.uni-due.de"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define PRINTABLEBOOL(x) ( x ? "true" : "false")

/*
 * mm_init - initialize the malloc package.
 */

void *heapBeginning;
void *heapEnd;
//REMOVE!
int globalCounter = 0;


int mm_init(void)
{
    
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void malloc_print(void *p, size_t size){

    int newSize = ALIGN(size + (SIZE_T_SIZE * 2));

    printf("Size: %d, Size with header and footer: %d |",size, newSize);

    printf("Pointer to beginning: %p |",p);

    printf("Size in header: %d |", *(size_t *)p);

    void *temp = ((char* ) p + newSize - SIZE_T_SIZE);
    printf("Address of footer: %p |",temp);
    
    printf("Size in footer: %d \n", *(size_t *)temp);

    printf("Allocated Memory with size %d (useable %d) from address %p to address %p \n", newSize, size ,p, (char *)p + newSize);

}

void printCurrentHeap(){

    if(heapBeginning > heapEnd){ 
        printf("HeapBeginning address: %p,Heap is empty! \n",heapBeginning); return;}


    printf("Heap: \n");
    void* currentPointer = heapBeginning;
    void* payloadBeginning;
    size_t sizeHeader;
    size_t payloadSize;
    void* footerBeginning;
    size_t sizeFooter;
    int allocated;
    int correctedSize;
    int counter = 0;
    while(currentPointer<heapEnd){
        
        payloadBeginning = (char*)currentPointer + SIZE_T_SIZE;
        sizeHeader = *(size_t *)currentPointer;
        correctedSize = sizeHeader;
        allocated = sizeHeader & 0x1;
        
        if(allocated) correctedSize--;
        payloadSize = correctedSize - SIZE_T_SIZE*2;
        footerBeginning = (char*)currentPointer + (correctedSize - SIZE_T_SIZE);
        sizeFooter = *(size_t*)footerBeginning;
        allocated = sizeHeader & 0x1;
        
        printf("Block %d: Adress from %p to %p, Allocated: %d, Header Address: %p, Size in Header: %d, Payload Beginning: %p, Payload Size: %d, Footer Beginning: %p, Size in Footer: %d \n",
               counter, currentPointer, (char*) footerBeginning + SIZE_T_SIZE, allocated, (char*) currentPointer, sizeHeader, payloadBeginning, payloadSize, footerBeginning, sizeFooter);

        currentPointer = (char*) currentPointer + correctedSize;
        counter++;


    }

    printf("Reached end of heap! currentPointer: %p, heapEnd: %p\n\n",currentPointer,heapEnd);
}

void *mm_malloc(size_t size)
{
    /*int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }*/

    

    heapBeginning = mem_heap_lo();
    heapEnd = mem_heap_hi();
    globalCounter++;
    printf("%d \n",globalCounter);
    
    //If heapBeginning equals heapEnd then no memory was allocated yet-> we have to get memory with sbrk and put header and footer
    if(globalCounter>4720){
    printf("HeapBeginning = %p \n",heapBeginning);
    printf("HeapEnd = %p \n",heapEnd);
    }
    if(heapBeginning > heapEnd){

        int newSize = ALIGN(size + (SIZE_T_SIZE * 2));

        
        void *p = mem_sbrk(newSize);

        if(p == (void * )-1){
            return NULL;
        }else{
            if(globalCounter>4720){
            printf("SBRK called! heapBeginning > heapEnd \n");
            }
            *(size_t *)p = newSize + 1;
        
            void *temp = ((char* ) p + newSize - SIZE_T_SIZE);
            
            *(size_t *)temp = newSize + 1;
            heapEnd = mem_heap_hi();
            if(globalCounter>4720){
            printf("\n\n");  
            printCurrentHeap();
            }
            return (void *) ((char *)p + SIZE_T_SIZE);

            

        }
        //If heapBeginning does not equal heapEnd then memory was allocated before -> we have to check all blocks if we can find an empty block
    }else{

        void *currentPointer = heapBeginning;
        size_t currentSize;

        while(true){

            //If the pointer reaches a point outside of our current heap we need to allocate new memory!
            if(currentPointer > mem_heap_hi()){
                int newSize = ALIGN(size + (SIZE_T_SIZE * 2));

                void *p = mem_sbrk(newSize);
                if(globalCounter>4720){
                printf("SBRK called! currentPointer > mem_heap_hi()\n");
                }
                if(p == (void * )-1){
                    return NULL;
                }else{
                *(size_t *)p = newSize + 1;
                void *temp = ((char* ) p + newSize - SIZE_T_SIZE);
                *(size_t *)temp = newSize + 1;
                heapEnd = mem_heap_hi();
                if(globalCounter>4720){malloc_print(p, size);
                printf("\n\n");  
                printCurrentHeap();
                }

                
                
                return (void *) ((char *)p + SIZE_T_SIZE);

                }
            }
            
            //Size of current block is determined by setting last bit to 0, as that bit is only relevant to determine if the block is allocated
            currentSize = *(size_t *)currentPointer & ~0x1;
            if(globalCounter>4720){printf("Current size: %x \n", currentSize);}
            //In this variable we store if the last bit is 1 (allocated) or 0 (free);
            char allocated = *(size_t *)currentPointer & 0x1;

            //If the block is allocated we skip it by adding the size to the pointer, which leaves us at the beginning of the next block
            if(allocated == 1){
                if(globalCounter>4720){printf("Block at address %p checked, allocated! \n", currentPointer);}
                currentPointer = (char *)currentPointer + currentSize;
                continue;
            //If the block is not allocated:
            }else{
                //If our block is large enough to store our data we select it -> First Fit!
                
                if(currentSize >= ALIGN(size + SIZE_T_SIZE*2)){
                    if(globalCounter>4720){printf("Unallocated block found at address %p \n",currentPointer);}
                    
                    if(*(size_t *)currentPointer < ALIGN(size + SIZE_T_SIZE*2) + 24){
                    if(globalCounter>4720){printf("Block fits good enough! \n");}
                    *(size_t *) ((char *) currentPointer) =  *(size_t *) ((char *) currentPointer)+ 1;
                    if(globalCounter>4720){printf("Header: %d \n\n",*(size_t *) ((char *) currentPointer - SIZE_T_SIZE));}
                    *(size_t *) ((char *) currentPointer + currentSize - SIZE_T_SIZE) = *(size_t *) ((char *) currentPointer + currentSize - SIZE_T_SIZE) + 1;
                    if(globalCounter>4720){printf("Footer: %d \n\n",*(size_t *) ((char *) currentPointer + currentSize - SIZE_T_SIZE *2));}
                    heapEnd = mem_heap_hi();
                    }else{
                        if(globalCounter>4720){printf("Block needs to be split! \n");}
                        size_t prevSize = *(size_t *)currentPointer;
                        size_t newSize = prevSize - ALIGN(size + SIZE_T_SIZE*2);

                        *(size_t *) ((char *) currentPointer + prevSize - SIZE_T_SIZE) = newSize;
                        *(size_t *) ((char *) currentPointer + ALIGN(size + SIZE_T_SIZE*2)) = newSize;

                        *(size_t *) ((char *) currentPointer) = ALIGN(size + SIZE_T_SIZE*2) + 1;
                        *(size_t *) ((char *) currentPointer + ALIGN(size + SIZE_T_SIZE*2) - SIZE_T_SIZE )= ALIGN(size + SIZE_T_SIZE*2)+1;

                        if(globalCounter>4720){printf("Header: %d \n\n",*(size_t *) ((char *) currentPointer - SIZE_T_SIZE));}
                        if(globalCounter>4720){printf("Footer: %d \n\n",*(size_t *) ((char *) currentPointer + currentSize - SIZE_T_SIZE *2));}
                        heapEnd = mem_heap_hi();
                    }
                    
                                       
                    if(globalCounter>4720){printf("\n\n"); malloc_print(currentPointer, currentSize - 2* SIZE_T_SIZE);printCurrentHeap();printf("Allocated Memory with size %d from address %p to address %p \n", currentSize, currentPointer, (char *)currentPointer + SIZE_T_SIZE*2);}
                    //printCurrentHeap();
                    ////printf("Allocated Memory with size %d from address %p to address %p \n", currentSize, currentPointer, (char *)currentPointer + SIZE_T_SIZE*2);
                    return (char *)currentPointer + SIZE_T_SIZE;
                //If it is not large enough, we again skip this block and look for the next!
                }else{
                     if(globalCounter>4720){printf("Unallocated block found at address %p, too small! \n",currentPointer);}
                    currentPointer = (char *)currentPointer + currentSize;
                    continue;
                }
            }


        }


    }






}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{   
    globalCounter++;
    printf("%d \n",globalCounter);
    //Size is decremented by one to show it is not allocated -> last bit is set to 0
    //ptr points to end of header (beginning of payload) so we have to subtract our size (cast to char pointer as char is one byte large, so we only remove as much as we want)
    size_t size = *(size_t *)((char *) ptr-SIZE_T_SIZE) - 1;
    if(globalCounter>4720){printf("Size of block to free: %d \n",size);}
    //We move one SIZE_T_SIZE back to reach header and set the header to the new size without the last bit 
    *(size_t *)((char *) ptr-SIZE_T_SIZE) = size;

    //Temp pointer is set to footer, ptr points to end of header (beginning of payload), so we add our size and remove SIZE_T_SIZE twice, as one needs to be removed 
    //as we start after header, and one has to be removed to reach beginning of footer
    void *temp = ((char* ) ptr + size - SIZE_T_SIZE * 2); 
    //Footer size is set to size with last bit 0
    *(size_t *)temp = size;
    if(globalCounter>4720){printf("FREE! \n\n");
    printf("Freed memory from address %p to %p |",(char *) ptr-SIZE_T_SIZE, (char *)temp+SIZE_T_SIZE );

   printf("Size in header: %d |", *(size_t *)((char *) ptr-SIZE_T_SIZE));
    printf("Size in footer: %d \n\n", *(size_t *)temp );
    printCurrentHeap();}

    
    coalesce((char *) ptr-SIZE_T_SIZE,temp);
    
    //printCurrentHeap();
}

void coalesce(void *ptr, void* tmp){

    
    //check if block after our freed block is free:
    void* nextBlockHeader = NULL;
    void* nextBlockFooter = NULL;
    void* previousBlockHeader = NULL;
    void* previousBlockFooter = NULL;
    void* currentBlockFooter = NULL;

    bool didSomething = false;
    size_t nextBlockSize;
    size_t previousBlockSize;
    bool prevAllocated = true, nextAllocated = true;

    size_t currentBlockSize = *(size_t *)ptr;
    currentBlockFooter = (char *)tmp - SIZE_T_SIZE;
    if(((char *) tmp + SIZE_T_SIZE)< mem_heap_hi()){
    nextBlockHeader = (char *)ptr + (currentBlockSize & ~0x1);
    nextBlockSize = *(size_t *)nextBlockHeader;
    if((nextBlockSize & 0x1) == 0) nextAllocated = false;
    nextBlockFooter = (char *)nextBlockHeader + ((nextBlockSize & ~0x1) - SIZE_T_SIZE);
    }
    if((ptr > mem_heap_lo())){
        if(globalCounter>4720){printf("ptr > mem_heap_lo(), memheaplo: %p \n", mem_heap_lo());}
    previousBlockFooter = (char *)ptr - SIZE_T_SIZE;
    previousBlockSize = *(size_t *)previousBlockFooter;
    if((previousBlockSize & 0x1) == 0) prevAllocated = false;
    previousBlockHeader = (char *)ptr - (previousBlockSize & ~0x1);
    }
    
    //if nextBlock not allocated:

    if(globalCounter>4720){printf("Current Block Header: %p, Current Block Footer: %p, Next Header: %p, Next Footer: %p, Prev Header: %p, Prev Footer: %p, currentBlockSize: %d, nextSize: %d, prevSize: %d \n\n",
            ptr,currentBlockFooter,nextBlockHeader,nextBlockFooter,previousBlockHeader,previousBlockFooter,currentBlockSize,nextBlockSize,previousBlockSize);}
    
    if( nextBlockHeader != NULL && nextBlockFooter != NULL && !nextAllocated){
        currentBlockSize = currentBlockSize + nextBlockSize;
        *(size_t *)ptr = currentBlockSize;
        *(size_t *)nextBlockFooter = currentBlockSize;

        //Small trick, we set currentBlockFooter to the footer of the nextBlock, because if we coalesced these two blocks the footer of our new block is
        //the footer of nextBlock
        currentBlockFooter = nextBlockFooter;

        if(globalCounter>4720){printf("Coalesced with next block! \n");}
        didSomething = true;
    }

    if(previousBlockHeader != NULL && previousBlockFooter != NULL && !prevAllocated ){
        currentBlockSize = currentBlockSize + previousBlockSize;
       if(globalCounter>4720){printf("previousBlockHeader: %p, nextblockheader: %p \n", previousBlockHeader, nextBlockHeader);}
        *(size_t *)previousBlockHeader = currentBlockSize;
        *(size_t *)currentBlockFooter = currentBlockSize;

        ptr = previousBlockHeader;

        if(globalCounter>4720){printf("Coalesced with previous block! \n");}
        didSomething = true;

        
    }

    if(didSomething){

        
        coalesce(ptr,currentBlockFooter);
        if(globalCounter>4720){printCurrentHeap();}
    } 

    
    //
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
