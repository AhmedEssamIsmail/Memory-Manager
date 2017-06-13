
#include <inc/lib.h>

// malloc()
//	This function use FIRST FIT strategy to allocate space in heap
//  with the given size and return void pointer to the start of the allocated space

//	To do this, we need to switch to the kernel, allocate the required space
//	in Page File then switch back to the user again.
//
//	We can use sys_allocateMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls allocateMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the allocateMem function is empty, make sure to implement it.


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
#define UHEAP_PAGES  (USER_HEAP_MAX - USER_HEAP_START) / PAGE_SIZE
struct UserChunk
{
	uint32 start_address;
	uint32 pages;
	int next;
	int prev;
	uint8 allocated;
	int shared_object_id;
};

struct UserChunk user_memory_chunks[UHEAP_PAGES];// = {{USER_HEAP_START,UHEAP_PAGES,-1,-1,0}};
int uchunks = 1;
int Ufree_index_one = -1,Ufree_index_two = -1;
int first_call = 1;

int search_uheap(uint32 size)
{
	if(first_call)
	{
		user_memory_chunks[0].allocated = 0;
		user_memory_chunks[0].next = -1;
		user_memory_chunks[0].prev = -1;
		user_memory_chunks[0].start_address = USER_HEAP_START;
		user_memory_chunks[0].pages = UHEAP_PAGES;
		user_memory_chunks[0].shared_object_id = -1;
		first_call = 0;
	}
	size = ROUNDUP(size,PAGE_SIZE);
	uint32 pages_to_allocate = size / PAGE_SIZE;

	int index = 0;
	while(1)
	{
		if(!user_memory_chunks[index].allocated && user_memory_chunks[index].pages >= pages_to_allocate)
			break;
		index = user_memory_chunks[index].next;
		if(index == -1)
			return -1;
	}
	if(user_memory_chunks[index].pages != pages_to_allocate)
	{
		int next_index;
		if(Ufree_index_one != -1){
			next_index = Ufree_index_one; Ufree_index_one = -1;
		}
		else if(Ufree_index_two != -1){
			next_index = Ufree_index_two; Ufree_index_two = -1;
		}
		else{
			next_index = uchunks; uchunks++;
		}
		user_memory_chunks[next_index].next = user_memory_chunks[index].next;
		if(user_memory_chunks[next_index].next != -1)
			user_memory_chunks[user_memory_chunks[next_index].next].prev = next_index;

		user_memory_chunks[index].next = next_index;
		user_memory_chunks[next_index].prev = index;

		user_memory_chunks[next_index].allocated = 0;
		user_memory_chunks[next_index].pages = user_memory_chunks[index].pages - pages_to_allocate;
		user_memory_chunks[next_index].start_address = user_memory_chunks[index].start_address + (pages_to_allocate*PAGE_SIZE);

		user_memory_chunks[index].pages = pages_to_allocate;
	}
	user_memory_chunks[index].allocated = 1;
	return index;

}

uint32 free_uheap(uint32 virtual_address)
{
	int index = 0;
	while(index != -1)
	{
		if(user_memory_chunks[index].start_address == virtual_address)
			break;
		index = user_memory_chunks[index].next;
		if(index == -1)
			return 0;
	}
	user_memory_chunks[index].allocated = 0;
	user_memory_chunks[index].shared_object_id = -1;
	uint32 pages = user_memory_chunks[index].pages;
	uint32 size = pages * PAGE_SIZE;

	int next = user_memory_chunks[index].next;
	int prev = user_memory_chunks[index].prev;
	if(next != -1 && !user_memory_chunks[next].allocated)
	{
		user_memory_chunks[index].pages += user_memory_chunks[next].pages;
		user_memory_chunks[next].prev = index;
		user_memory_chunks[index].next = user_memory_chunks[next].next;
		Ufree_index_one = next;
	}
	next = user_memory_chunks[index].next;
	prev = user_memory_chunks[index].prev;

	if(prev != -1 && !user_memory_chunks[prev].allocated)
	{
		user_memory_chunks[prev].pages += user_memory_chunks[index].pages;
		user_memory_chunks[next].prev = prev;
		user_memory_chunks[prev].next = next;
		Ufree_index_two = index;
	}
	return size;
}

void* malloc(uint32 size)
{
	//TODO: [PROJECT 2017 - [5] User Heap] malloc() [User Side]
	// Write your code here, remove the panic and write your code
	//panic("malloc() is not implemented yet...!!");
	// Steps:
	//	1) Implement FIRST FIT strategy to search the heap for suitable space
	//		to the required allocation size (space should be on 4 KB BOUNDARY)
	//	2) if no suitable space found, return NULL
	//	 Else,
	//	3) Call sys_allocateMem to invoke the Kernel for allocation
	// 	4) Return pointer containing the virtual address of allocated space,

	//This function should find the space of the required range
	// ******** ON 4KB BOUNDARY ******************* //
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() to check the current strategy
	//change this "return" according to your answer
	size = ROUNDUP(size,PAGE_SIZE);
	int index = search_uheap(size);
	if(index == -1)
		return NULL;

	user_memory_chunks[index].shared_object_id = -1;
	sys_allocateMem(user_memory_chunks[index].start_address,size);
	return (void*) user_memory_chunks[index].start_address;
}

void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//TODO: [PROJECT 2017 - [6] Shared Variables: Creation] smalloc() [User Side]
	// Write your code here, remove the panic and write your code
	//panic("smalloc() is not implemented yet...!!");

	// Steps:
	//	1) Implement FIRST FIT strategy to search the heap for suitable space
	//		to the required allocation size (space should be on 4 KB BOUNDARY)
	//	2) if no suitable space found, return NULL
	//	 Else,
	//	3) Call sys_createSharedObject(...) to invoke the Kernel for allocation of shared variable
	//		sys_createSharedObject(): if succeed, it returns the ID of the created variable. Else, it returns -ve
	//	4) If the Kernel successfully creates the shared variable, return its virtual address
	//	   Else, return NULL

	//This function should find the space of the required range
	// ******** ON 4KB BOUNDARY ******************* //

	//Use sys_isUHeapPlacementStrategyFIRSTFIT() to check the current strategy

	//change this "return" according to your answer
	int index = search_uheap(size);
	if(index == -1)
		return NULL;

	uint32 virtual_address = user_memory_chunks[index].start_address;

	int shared_var_id = sys_createSharedObject(sharedVarName,size,isWritable,(void*)virtual_address);
	if(shared_var_id < 0)
	{
		free_uheap(virtual_address);
		return NULL;
	}
	user_memory_chunks[index].shared_object_id = shared_var_id;
	return (void*)virtual_address;
}

void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//TODO: [PROJECT 2017 - [6] Shared Variables: Get] sget() [User Side]
	// Write your code here, remove the panic and write your code
	//panic("sget() is not implemented yet...!!");

	// Steps:
	//	1) Get the size of the shared variable (use sys_getSizeOfSharedObject())
	//	2) If not exists, return NULL
	//	3) Implement FIRST FIT strategy to search the heap for suitable space
	//		to share the variable (should be on 4 KB BOUNDARY)
	//	4) if no suitable space found, return NULL
	//	 Else,
	//	5) Call sys_getSharedObject(...) to invoke the Kernel for sharing this variable
	//		sys_getSharedObject(): if succeed, it returns the ID of the shared variable. Else, it returns -ve
	//	6) If the Kernel successfully share the variable, return its virtual address
	//	   Else, return NULL
	uint32 size = sys_getSizeOfSharedObject(ownerEnvID,sharedVarName);
	if(size == E_SHARED_MEM_NOT_EXISTS)
		return NULL;

	int index = search_uheap(size);
	if(index == -1)
		return NULL;

	int ret = sys_getSharedObject(ownerEnvID,sharedVarName,(void*)user_memory_chunks[index].start_address);
	if(ret < 0)
	{
		free_uheap(user_memory_chunks[index].start_address);
		return NULL;
	}
	user_memory_chunks[index].shared_object_id = ret;
	return (void*)user_memory_chunks[index].start_address;
}

// free():
//	This function frees the allocation of the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from page file and main memory then switch back to the user again.
//
//	We can use sys_freeMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls freeMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the freeMem function is empty, make sure to implement it.

void free(void* virtual_address)
{
	//TODO: [PROJECT 2017 - [5] User Heap] free() [User Side]
	// Write your code here, remove the panic and write your code
	//panic("free() is not implemented yet...!!");
	//you should get the size of the given allocation using its address
	//you need to call sys_freeMem()
	//refer to the project presentation and documentation for details

	uint32 size = free_uheap((uint32)virtual_address);
	if(size == 0)
		return;

	sys_freeMem((uint32)virtual_address,size);
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=============
// [1] sfree():
//=============
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT 2017 - BONUS4] Free Shared Variable [User Side]
	// Write your code here, remove the panic and write your code
	//panic("sfree() is not implemented yet...!!");

	//	1) you should find the ID of the shared variable at the given address
	//	2) you need to call sys_freeSharedObject()
	int index = 0;
	while(1)
	{
		if(user_memory_chunks[index].start_address == (uint32) virtual_address)
			break;
		index = user_memory_chunks[index].next;
		if(index == -1)
			return;
	}
	if(user_memory_chunks[index].shared_object_id != -1)
		sys_freeSharedObject(user_memory_chunks[index].shared_object_id,virtual_address);
}

//===============
// [2] realloc():
//===============

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_moveMem(uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		which switches to the kernel mode, calls moveMem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		in "memory_manager.c", then switch back to the user mode here
//	the moveMem function is empty, make sure to implement it.

void *realloc(void *virtual_address, uint32 new_size)
{

	//TODO: [PROJECT 2017 - BONUS3] User Heap Realloc [User Side]
	// Write your code here, remove the panic and write your code
	//panic("realloc() is not implemented yet...!!");
	if(new_size == 0){
		free(virtual_address);
		return NULL;
	}
	if(virtual_address == NULL)
		return malloc(new_size);

	int index = 0;
	while(1)
	{
		if(user_memory_chunks[index].start_address == (uint32) virtual_address)
			break;
		index = user_memory_chunks[index].next;
		if(index == -1)
			return NULL;
	}
	uint32 new_pages = ROUNDUP(new_size,PAGE_SIZE) / PAGE_SIZE;
	uint32 cur_pages = user_memory_chunks[index].pages;
	uint32 extra_pages = new_pages - cur_pages;
	int next = user_memory_chunks[index].next;

	if(new_pages <= cur_pages)
		return (void*)user_memory_chunks[index].start_address;

	if(next != -1 && !user_memory_chunks[next].allocated
	&& new_pages <= user_memory_chunks[index].pages + user_memory_chunks[next].pages)
	{
		user_memory_chunks[index].pages += extra_pages;
		user_memory_chunks[next].pages -= extra_pages;
		sys_allocateMem(user_memory_chunks[next].start_address,extra_pages*PAGE_SIZE);
		user_memory_chunks[next].start_address += extra_pages*PAGE_SIZE;
		return (void*)user_memory_chunks[index].start_address;
	}

	void* new_va = malloc(new_size);
	if(new_va == NULL)
		return NULL;

	void* cur_va = (void*)user_memory_chunks[index].start_address;
	sys_moveMem((uint32)cur_va,(uint32)new_va,cur_pages);
	sys_allocateMem((uint32)(new_va+(cur_pages*PAGE_SIZE)),extra_pages*PAGE_SIZE);
	free_uheap((uint32)cur_va);
	return new_va;
}
