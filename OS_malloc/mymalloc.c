#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

int has_initialized = 0;

// our memory area we can allocate from, here 64 kB
#define MEM_SIZE (64 * 1024)
uint8_t heap[MEM_SIZE];

// start and end of our own heap memory area
void *managed_memory_start;

// this block is stored at the start of each free and used block
struct mem_control_block
{
    int size;
    struct mem_control_block *next;
};

// pointer to start of our free list
struct mem_control_block *free_list_start;


void mymalloc_init()
{

    // our memory starts at the start of the heap array
    managed_memory_start = &heap;

    // allocate and initialize our memory control block
    // for the first (and at the moment only) free block
    struct mem_control_block *m = (struct mem_control_block *)managed_memory_start;
    // SKELETON CHANGE: per Michael's recommendation, use size of whole block.
    // SKELETON CHANGE: Also, measure size in units instead of bytes. 1 unit = sizeof(struct mem_control_block)
    m->size = MEM_SIZE / sizeof(struct mem_control_block);

    // SKELETON CHANGE: per Michael's recommendation, use a circular linked list.
    m->next = m;

    // initialize the start of the free list
    free_list_start = m;

    // We're initialized and ready to go
    has_initialized = 1;
}


void print_free_list()
{
    if (has_initialized == 0)
    {
        mymalloc_init();
    }
    if (free_list_start == (void *)0)
    {
        printf("\n\n------------------------------------------------------------------\n");
        printf("Printing free_list\n");
        printf("There are no free blocks!\n");
        printf("------------------------------------------------------------------\n\n");
        return;
    }
    struct mem_control_block *prev_pointer = free_list_start;
    printf("\n\n------------------------------------------------------------------\n");
    printf("Printing free_list\n");
    printf("managed_memory_start: %p\n", managed_memory_start);
    printf("------------------------------------------------------------------\n");
    for (struct mem_control_block *pointer = prev_pointer->next; /* */; prev_pointer = pointer, pointer = pointer->next)
    {
        if (pointer == (void *)0)
        {
            printf("NULL-pointer in print_free_list. Should not be here. Exiting print-function.\n");
            break;
        }
        printf("Start address: %p\n", pointer);
        printf("Total size(size) in units: %d\n", pointer->size);
        printf("Next free block: %p\n", pointer->next);
        printf("------------------------------------------------------------------\n");
        if (pointer == free_list_start)
        {
            break;
        }
    }
    printf("\n\n");
}


void *mymalloc(long numbytes)
{
    if (has_initialized == 0)
    {
        mymalloc_init();
    }
    if (free_list_start == (void *)0)
    {
        // There are no free blocks left in the heap.
        printf("Found no free blocks large enough\n");
        return (void *)0;
    }

    struct mem_control_block *prev_pointer = free_list_start;
    // minus 1 in the dividend takes care of the a%b = 0 case, since 1 is added after
    // nunits is number of units needed to contain the whole block to be allocated, including its control block.
    int nunits = (numbytes + sizeof(struct mem_control_block) - 1) /
                     sizeof(struct mem_control_block) +
                 1;

    for (struct mem_control_block *pointer = prev_pointer->next; /* */; prev_pointer = pointer, pointer = pointer->next)
    {
        int free_units = (pointer->size - 1) / sizeof(struct mem_control_block) + 1;
        if (pointer->size >= nunits)
        {
            if (pointer->size == nunits)
            {
                // The whole free block will be utilized to allocate the requested space.
                // The size remains the same, as the whole free block is just switched out with a block with content.
                if (pointer->next == pointer)
                {
                    // If this is the only free block, there will be no free blocks after this operation.
                    free_list_start = (void *)0;
                    return pointer + 1;
                }
                else
                {
                    prev_pointer->next = pointer->next;
                }
            }
            else
            {
                pointer->size -= nunits;
                pointer += pointer->size;
                pointer->size += nunits;
            }
            // Optimization. More likely that the next mymalloc call will find space in free blocks that haven't been checked lately
            free_list_start = prev_pointer;
            // +1 to point to the actual allocatable space, not its control block.
            return (void *)(pointer + 1);
        }
        if (pointer == free_list_start)
        {
            printf("Found no free blocks large enough\n");
            return (void *)0;
        }
    }
}


void myfree(void *firstbyte)
{
    if (has_initialized == 0)
    {
        mymalloc_init();
    }
    struct mem_control_block *bp = (struct mem_control_block *)firstbyte - 1;

    if (free_list_start == (void *)0)
    {
        // There are no current free blocks, meaning that when we free this one, it will be the only one.
        free_list_start = bp;
        free_list_start->next = free_list_start;
        return;
    }
    else if (free_list_start->next == free_list_start)
    {
        // Only one block in the free_list

        if (bp < free_list_start)
        {
            // Block to be freed is left of the one free block
            if (bp + bp->size == free_list_start)
            {
                // Block to be freed is right next to the free block
                // Is this correct? I suspect that free_list_start = bp also takes with it bp->size
                bp->size += free_list_start->size;
                bp->next = bp;
                free_list_start = bp;
            }
            else
            {
                // There are occupied blocks between the block to be freed and the free block
                bp->next = free_list_start;
                free_list_start->next = bp;
            }
        }
        else if (bp == free_list_start || bp < free_list_start + free_list_start->size )
        {
            // bp is already considered a free block, or a part of the free block we have. 
            // This happens when you call myfree() on a non-allocated block, or multiple times on the same pointer.
            printf("This block is already free.\n");
            return;
        }
        else
        {
            // Block to be freed is right of the one free block
            if (free_list_start + free_list_start->size == bp)
            {
                // Block to be freed is right next to the free block
                free_list_start->size += bp->size;
            }
            else
            {
                // There are occupied blocks between the block to be freed and the free block
                bp->next = free_list_start;
                free_list_start->next = bp;
            }
        }
        return;
    }

    struct mem_control_block *prev_pointer = free_list_start;
    for (struct mem_control_block *pointer = prev_pointer->next; /* */; prev_pointer = pointer, pointer = pointer->next)
    {
        // Running through the linked list of free blocks, looking between every pair of free blocks next to each other.
        if (pointer == bp || (pointer < bp && bp < pointer + pointer->size))
        {
            // bp is already considered a free block, or a part of the free block we're looking at (pointer)
            printf("This block is already free.\n");
            return;
        }
        if (prev_pointer < bp && bp < pointer)
        {
            // Block to be freed is between the free two free blocks we have pointers for
            if (bp + bp->size == pointer)
            {
                // Block to be freed is right next to right free block
                bp->size = bp->size + pointer->size;
                bp->next = pointer->next;
                prev_pointer->next = bp;
            }
            else
            {
                // There are occupied block(s) between block to be freed and right free block
                bp->next = pointer;
            }
            if (prev_pointer + prev_pointer->size == bp)
            {
                // Block to be freed is right next to left free block
                if (prev_pointer->next == bp)
                {
                    // Was also caught by previous if clause
                    prev_pointer->next = bp->next;
                }
                prev_pointer->size = prev_pointer->size + bp->size;
            }
            else
            {
                // There are occupied block(s) between block to be freed and left free block
                if (prev_pointer->next == bp)
                {
                    // Was also caught by previous if clause
                    prev_pointer->next = bp->next;
                }
                prev_pointer->next = bp;
            }
            return;
        }
        else if (pointer < prev_pointer)
        {
            // the pointers are at the 'last' and 'first' free blocks, from an address perspective.
            if (prev_pointer < bp)
            {
                // Block to be freed is right of the 'last' free block, essentially being between the two free blocks in a linked list sense
                if (prev_pointer + prev_pointer->size == bp)
                {
                    // Block to be freed is right next to 'last' block
                    prev_pointer->size = prev_pointer->size + bp->size;
                }
                else
                {
                    // There are occupied block(s) between the block to be freed and the 'last' block
                    bp->next = pointer;
                    prev_pointer->next = bp;
                }
            }
            else if (bp < pointer)
            {
                // Block to be freed is left of the 'first' free block, essentially being between the two free blocks in a linked list sense
                prev_pointer->next = bp;
                if (bp + bp->size == pointer)
                {
                    // Block to be freed is right next to 'first' block
                    bp->size = bp->size + pointer->size;
                    bp->next = pointer->next;
                }
                else
                {
                    // There are occupied blocks between the block to be freed and the 'first' block'
                    bp->next = pointer;
                }
            }
            return;
        }
        if (pointer == free_list_start)
        {
            printf("Did not find a way to free memory\n");
            return;
        }
    }
}

int unit_size = sizeof(struct mem_control_block);


int get_free_list_size()
{
    if (has_initialized == 0)
    {
        mymalloc_init();
    }
    struct mem_control_block *prev_pointer = free_list_start;
    int counter = 0;
    for (struct mem_control_block *pointer = prev_pointer->next; /* */ ; pointer = pointer->next, prev_pointer = pointer)
    {
        counter += 1;
        if (pointer == free_list_start)
        {
            return counter;
        }
    }
}


void test_mymalloc_1k()
{
    printf("This test tests allocating a some bytes.\n");
    void *a = mymalloc(1024);
    bool success = (a != (void *)0 && free_list_start->size == (63 * 1024 - unit_size) / unit_size);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_mymalloc_max_available_space()
{
    printf("This test tests if allocating the max amount of bytes (64*1024 - block size) behaves correctly.\n");

    void *a = mymalloc(64 * 1024 - unit_size);
    bool success = (a != (void *)0 && free_list_start == (void *)0);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_not_enough_space()
{
    printf("This test tests if trying to allocate more bytes than what's available behaves correctly.\n");
    void *a = mymalloc(65 * 1024);
    bool success = (a == (void *)0);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_mymalloc_0_bytes()
{
    // Exactly how malloc should work here we don't know, but ours allocates a block to it.
    printf("This test tests if trying to allocate more bytes than what's available behaves correctly.\n");
    void *a = mymalloc(0);
    bool success = (a != (void *)0 && free_list_start->size == (64 * 1024 - unit_size) / unit_size);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_mymalloc_multiple_times()
{
    printf("This test tests if mymalloc behaves correctly towards free_list when allocating multiple times.\n");
    int bytes = 48;
    void *b = mymalloc(bytes);
    void *c = mymalloc(bytes * 2);
    int num_free_blocks = get_free_list_size();
    bool success = (num_free_blocks == 1 && free_list_start->size + ((struct mem_control_block *)b - 1)->size + ((struct mem_control_block *)c - 1)->size == 64 * 1024 / unit_size);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_mymalloc_and_myfree()
{
    printf("This test tests if mymalloc and myfree in unison.\n");
    void *a = mymalloc(4567);
    bool success = (free_list_start->size != 64 * 1024);
    myfree(a);
    int num_free_blocks = get_free_list_size();
    success = (success && free_list_start->size == 64 * 1024 / unit_size && num_free_blocks == 1);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_mymalloc_and_myfree_multiple_times()
{
    printf("This test tests if mymalloc and myfree behaves correctly with some calls.\n");
    void *a = mymalloc(33);
    void *b = mymalloc(40);
    void *c = mymalloc(1500);
    myfree(a);
    bool success = (get_free_list_size() == 2);
    void *d = mymalloc(33);
    success = (success && get_free_list_size() == 1);
    void *e = mymalloc(40);
    myfree(c);
    void *f = mymalloc(1000);
    success = (success && get_free_list_size() == 2);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_myfree_block_next_to_occupied_right()
{
    printf("This test tests if freeing a block directly next to an occupied block right of it is kept as a separate free block.\n");
    void *a = mymalloc(200);
    void *b = mymalloc(200);
    void *c = mymalloc(200);
    myfree(a);
    bool success = (get_free_list_size() == 2);
    myfree(c);
    success = (success && get_free_list_size() == 2);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_myfree_block_next_to_occupied_left()
{
    printf("This test tests if freeing a block directly next to an occupied block left of it is kept as a separate free block.\n");
    void *a = mymalloc(200);
    void *b = mymalloc(200);
    void *c = mymalloc(200);
    void *d = mymalloc(200);
    myfree(c);
    bool success = (get_free_list_size() == 2);
    myfree(b);
    success = (success && get_free_list_size() == 2);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_myfree_block_next_to_free_right()
{
    printf("This test tests if freeing a block directly next to a free block right of it fuses the blocks together.\n");
    void *a = mymalloc(200);
    void *b = mymalloc(200);
    void *c = mymalloc(200);
    myfree(a);
    bool success = (get_free_list_size() == 2);
    myfree(b);
    success = (success && get_free_list_size() == 2);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_myfree_block_next_to_free_left()
{
    printf("This test tests if freeing a block directly next to a free block left of it fuses the blocks together.\n");
    void *a = mymalloc(200);
    void *b = mymalloc(200);
    void *c = mymalloc(200);
    myfree(b);
    bool success = (get_free_list_size() == 2);
    myfree(a);
    success = (success && get_free_list_size() == 2);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_myfree_block_between_free_blocks()
{
    printf("This test tests if freeing a block directly between two free blocks fuses them all to one free block.\n");
    void *a = mymalloc(200);
    void *b = mymalloc(200);
    myfree(a);
    bool success = (get_free_list_size() == 2);
    myfree(b);
    success = (success && get_free_list_size() == 1);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_free_block_left_of_free_list()
{
    printf("This test tests that free blocks are linked together when the block being freed is to the left of the free list. Since blocks are allocated from right to left we have to allocate all the memory and free the first allocated block. Now the last occupied block will be to the left of the free list, and we can release it. \n");
    void *a = mymalloc(62 * 1024 - unit_size);
    void *b = mymalloc(1 * 1024 - unit_size);
    void *c = mymalloc(1 * 1024 - unit_size);
    myfree(a);
    bool success = (get_free_list_size() == 1);
    myfree(b);
    success = (success && get_free_list_size() == 1);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_free_block_right_of_free_list()
{
    printf("This test tests that free blocks are linked together when the block being freed is to the right of the free list. \n");
    void *a = mymalloc(1 * 1024 - unit_size);
    void *b = mymalloc(63 * 1024 - unit_size);
    myfree(b);
    bool success = (get_free_list_size() == 1);
    myfree(a);
    success = (success && get_free_list_size() == 1);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_free_block_left_of_free_list_occupied_between()
{
    printf("This test tests that myfree works correctly with the block to be freed left of the free list with occupied blocks betwen.\n");
    void *a = mymalloc(1 * 1024 - unit_size);
    void *b = mymalloc(1 * 1024 - unit_size);
    void *c = mymalloc(1 * 1024 - unit_size);
    void *d = mymalloc(61 * 1024 - unit_size);
    myfree(a);
    bool success = (get_free_list_size() == 1);
    myfree(c);
    success = (success && get_free_list_size() == 2);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_free_block_right_of_free_list_occupied_between()
{
    printf("This test tests that myfree works correctly with the block to be freed right of the free list with occupied blocks betwen.\n");
    void *a = mymalloc(1 * 1024 - unit_size);
    void *b = mymalloc(1 * 1024 - unit_size);
    void *c = mymalloc(1 * 1024 - unit_size);
    void *d = mymalloc(61 * 1024 - unit_size);
    myfree(c);
    bool success = (get_free_list_size() == 1);
    myfree(a);
    success = (success && get_free_list_size() == 2);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_free_already_free_block_one_free_block()
{
    printf("This test tests if calling myfree on an unallocated block breaks the code.\n");
    void *a = mymalloc(200);
    myfree(a);
    myfree(a);
    bool success = (get_free_list_size() == 1);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


void test_free_already_free_block_multiple_free_blocks()
{
    printf("This test tests if calling myfree on an unallocated block breaks the code. This tests with multiple blocks in the free list.\n");
    void *a = mymalloc(200);
    void *b = mymalloc(200);
    void *c = mymalloc(200);
    void *d = mymalloc(200);
    void *e = mymalloc(200);
    bool success = (get_free_list_size() == 1);
    myfree(a);
    myfree(c);
    myfree(d);
    success = (success && get_free_list_size() == 3);
    myfree(a);
    myfree(c);
    myfree(d);
    success = (success && get_free_list_size() == 3);
    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


//Create allocation with only small gaps left, then try to allocate a black that doesn't fit
void test_allocation_only_small_gaps_left()
{
    printf("This test tests that allocating less than the remaining free space, but more than each free block, won't work.");
    void *a = mymalloc(15 * 1024 - unit_size);
    void *b = mymalloc(1 * 1024 - unit_size);
    void *c = mymalloc(15 * 1024 - unit_size);
    void *d = mymalloc(1 * 1024 - unit_size);
    void *e = mymalloc(15 * 1024 - unit_size);
    void *f = mymalloc(1 * 1024 - unit_size);
    void *g = mymalloc(15 * 1024 - unit_size);

    myfree(b);
    myfree(d);
    myfree(f);
    //Only blocks with 1024 bytes left, 2*1024 should be too much and the program should print an Error.
    void *i = mymalloc(2 * 1024 - unit_size);

    bool success = (get_free_list_size() == 4);

    if (success)
    {
        printf("Test succesful\n");
    }
    else
    {
        printf("Test failure\n");
    }
}


int main(int argc, char **argv)
{
    mymalloc_init();
    // print_free_list();

    // test_mymalloc_1k();
    // test_mymalloc_max_available_space();
    // test_not_enough_space();
    // test_mymalloc_0_bytes();
    // test_mymalloc_multiple_times();
    // test_mymalloc_and_myfree();
    // test_mymalloc_and_myfree_multiple_times();
    // test_myfree_block_next_to_occupied_right();
    // test_myfree_block_next_to_occupied_left();
    // test_myfree_block_next_to_free_right();
    // test_myfree_block_next_to_free_left();
    // test_myfree_block_between_free_blocks();
    // test_free_block_left_of_free_list();
    // test_free_block_right_of_free_list();
    // test_free_block_left_of_free_list_occupied_between();
    // test_free_block_right_of_free_list_occupied_between();
    // test_free_already_free_block_one_free_block();
    // test_free_already_free_block_multiple_free_blocks();
    // test_allocation_only_small_gaps_left();

    return 0;
}
