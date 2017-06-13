#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>

//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)

//=================================================================================//
//============================ REQUIRED FUNCTION ==================================//
//=================================================================================//
#define MAX_HEAP_PAGES  (KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE
struct Chunk
{
	uint32 start_address;
	uint32 pages;
	int next;
	int prev;
	uint8 allocated;
};

struct Chunk memory_chunks[MAX_HEAP_PAGES];

int free_index_one = -1,free_index_two = -1;
int chunks= 1;
int next_fit_index = 0;
int first_free = 1;
int looped_back = 0;
int first_call = 1;
int cont_index = 0;

int ContVsFirstFitvsNextFit()
{
	if(isKHeapPlacementStrategyCONTALLOC())
		return cont_index;
	if(isKHeapPlacementStrategyNEXTFIT())
		return next_fit_index;
	return 0;
}
int select_index()
{
	int next_index;
	if(free_index_one != -1){
		next_index = free_index_one; free_index_one = -1;
	}
	else if(free_index_two != -1){
		next_index = free_index_two; free_index_two = -1;
	}
	else{
		next_index = chunks; chunks++;
	}
	return next_index;
}
void* kmalloc(unsigned int size)
{
	if(first_call)
	{
		memory_chunks[0].start_address = KERNEL_HEAP_START;
		memory_chunks[0].pages = MAX_HEAP_PAGES;
		memory_chunks[0].next = -1;
		memory_chunks[0].prev = -1;
		memory_chunks[0].allocated = 0;
		first_call = 0;
	}
	//TODO: [PROJECT 2017 - [1] Kernel Heap] kmalloc()
	//TODO: [PROJECT 2017 - BONUS1] Implement a Kernel allocation strategy
	// Instead of the continuous allocation/deallocation, implement both
	// FIRST FIT and NEXT FIT strategies
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy

	uint32 pages_to_allocate = ROUNDUP(size,PAGE_SIZE) / PAGE_SIZE;

	//choose the index based on the allocation strategy (first fit or next fit)
	int index = ContVsFirstFitvsNextFit();
	while(1)
	{
		if(!memory_chunks[index].allocated && memory_chunks[index].pages >= pages_to_allocate)
			break;
		index = memory_chunks[index].next;
		if(index == -1)
		{
			if(isKHeapPlacementStrategyNEXTFIT() && !looped_back)
			{
				//if reached end of allocated chunks and no space found then loop again from 0
				looped_back=1;
				index = 0;
			}
			else
				return NULL;
		}
	}
	if(memory_chunks[index].pages != pages_to_allocate)
	{
		int next_index = select_index();
		memory_chunks[next_index].next = memory_chunks[index].next;
		if(memory_chunks[next_index].next != -1)
			memory_chunks[memory_chunks[next_index].next].prev = next_index;

		memory_chunks[index].next = next_index;
		memory_chunks[next_index].prev = index;

		memory_chunks[next_index].allocated = 0;
		memory_chunks[next_index].pages = memory_chunks[index].pages - pages_to_allocate;
		memory_chunks[next_index].start_address = memory_chunks[index].start_address + (pages_to_allocate*PAGE_SIZE);

		memory_chunks[index].pages = pages_to_allocate;
	}
	memory_chunks[index].allocated = 1;
	uint32 VA = memory_chunks[index].start_address;
	for(int i=0;i<pages_to_allocate;i++)
	{
		struct Frame_Info * info;
		int ret = allocate_frame(&info);
		if(ret!= E_NO_MEM)
		{
			map_frame(ptr_page_directory,info,(void*)VA,PERM_WRITEABLE | PERM_AVAILABLE);
			info->va = VA;
		}
		VA+=PAGE_SIZE;
	}
	next_fit_index = index;
	looped_back = 0;
	cont_index = index;
	return (void*) memory_chunks[index].start_address;
}


void kfree(void* virtual_address)
{
	//TODO: [PROJECT 2017 - [1] Kernel Heap] kfree()

	//if this is the first time to free memory set the next fit index to 0

	if(first_free)
	{
		first_free = 0;
		next_fit_index = 0;
	}
	int index = 0;
	while(index != -1)
	{
		if(memory_chunks[index].start_address == (uint32) virtual_address)
			break;
		index = memory_chunks[index].next;
		if(index == -1)
			return;
	}
	memory_chunks[index].allocated = 0;
	uint32 pages = memory_chunks[index].pages;

	if(!isKHeapPlacementStrategyCONTALLOC())
	{
		int next = memory_chunks[index].next;
		int prev = memory_chunks[index].prev;
		if(next != -1 && !memory_chunks[next].allocated)
		{
			memory_chunks[index].pages += memory_chunks[next].pages;
			memory_chunks[next].prev = index;
			memory_chunks[index].next = memory_chunks[next].next;
			free_index_one = next;
		}
		next = memory_chunks[index].next;
		prev = memory_chunks[index].prev;

		if(prev != -1 && !memory_chunks[prev].allocated)
		{
			memory_chunks[prev].pages += memory_chunks[index].pages;
			memory_chunks[next].prev = prev;
			memory_chunks[prev].next = next;
			free_index_two = index;
		}
	}
	for(int i=0;i<pages;i++,virtual_address+=PAGE_SIZE)
		unmap_frame(ptr_page_directory,virtual_address);

}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT 2017 - [1] Kernel Heap] kheap_virtual_address()
	return frames_info[physical_address >> 12].va;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT 2017 - [1] Kernel Heap] kheap_physical_address()
	uint32 *ptr = NULL;
	get_page_table(ptr_page_directory,(void*)virtual_address,&ptr);
	if(ptr != NULL)
		return ptr[PTX(virtual_address)] & 0xFFFFF000;

	return 0;
}


//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT 2017 - BONUS2] Kernel Heap Realloc
	// Write your code here, remove the panic and write your code
	if(new_size == 0){
		kfree(virtual_address);
		return NULL;
	}
	if(virtual_address == NULL)
		return kmalloc(new_size);

	int index = 0;
	while(1)
	{
		if(memory_chunks[index].start_address == (uint32) virtual_address)
			break;
		index = memory_chunks[index].next;
		if(index == -1)
			return NULL;
	}
	uint32 new_pages = ROUNDUP(new_size,PAGE_SIZE) / PAGE_SIZE;
	uint32 cur_pages = memory_chunks[index].pages;
	uint32 extra_pages = new_pages - cur_pages;
	int next = memory_chunks[index].next;
	struct Frame_Info * info ;

	if(new_pages <= cur_pages)
		return (void*)memory_chunks[index].start_address;

	if(next != -1 && !memory_chunks[next].allocated
	&& new_pages <= memory_chunks[index].pages + memory_chunks[next].pages)
	{
		memory_chunks[index].pages += extra_pages;
		memory_chunks[next].pages -= extra_pages;
		uint32 tmp_va = memory_chunks[next].start_address;
		for(int i=0;i<extra_pages;i++,tmp_va+=PAGE_SIZE)
		{
			allocate_frame(&info);
			map_frame(ptr_page_directory,info,(void*)tmp_va,PERM_WRITEABLE | PERM_AVAILABLE);
			info->va = (uint32)tmp_va;
		}
		return (void*)memory_chunks[index].start_address;
	}

	void* new_va = kmalloc(new_size);
	if(new_va == NULL)
		return NULL;

	void* cur_va = (void*)memory_chunks[index].start_address;
	void* new_va_tmp = new_va;
	void* cur_va_tmp = cur_va;


	for(int i=0;i<cur_pages;i++, cur_va +=PAGE_SIZE , new_va+=PAGE_SIZE)
	{
		uint32 * ptr;
		info = get_frame_info(ptr_page_directory,cur_va,&ptr);
		map_frame(ptr_page_directory,info,new_va,PERM_WRITEABLE | PERM_AVAILABLE);
		info->va = (uint32)new_va;
	}

	kfree(cur_va_tmp);

	for(int i=0;i<extra_pages;i++,new_va+=PAGE_SIZE)
	{
		allocate_frame(&info);
		map_frame(ptr_page_directory,info,new_va,PERM_WRITEABLE | PERM_AVAILABLE);
		info->va = (uint32)new_va;
	}

	return new_va_tmp;
	//panic("krealloc() is not implemented yet...!!");

}
