#include <inc/stdio.h>
#include <kern/priority_manager.h>
#include <inc/assert.h>
#include <kern/helpers.h>
#include <kern/memory_manager.h>
#include <kern/kheap.h>
#include <kern/file_manager.h>
struct Priority
{
	uint32 env_id;
	uint8 cur_priority;
};

struct Priority priorities[1285];
int no_of_envs = 0;
void set_program_priority(struct Env* env, int priority)
{
	//panic("Program Priority function is not implemented yet\n");
	for(int i=0;i<no_of_envs;i++)
		if(priorities[i].env_id == env->env_id && priorities[i].cur_priority == priority)
			return;
	int max_ws_size = env->page_WS_max_size;
	int cur_ws_size = env_page_ws_get_size(env);
	if(priority == 1)
	{
		struct WorkingSetElement * new_ws = kmalloc((max_ws_size/2)*sizeof(struct WorkingSetElement));
		uint32 index = cur_ws_size/2;
		for(int i=0;i<cur_ws_size/2;i++,index++)
		{
			unmap_frame(env->env_page_directory,(void*)env_page_ws_get_virtual_address(env,i));
			new_ws[i].virtual_address = env->ptr_pageWorkingSet[index].virtual_address;
			new_ws[i].time_stamp = env->ptr_pageWorkingSet[index].time_stamp;
			new_ws[i].empty = env->ptr_pageWorkingSet[index].empty;
		}
		kfree((void*)env->ptr_pageWorkingSet);
		env->ptr_pageWorkingSet = new_ws;
		env->page_WS_max_size = max_ws_size/2;

		priorities[no_of_envs].cur_priority = priority;
		priorities[no_of_envs].env_id = env->env_id;
		no_of_envs++;
	}
	if(priority == 2 && max_ws_size/2 == cur_ws_size)
	{
		struct WorkingSetElement * new_ws = kmalloc((max_ws_size/2)*sizeof(struct WorkingSetElement));
		uint32 index = cur_ws_size/2;
		for(int i=0;i<cur_ws_size/2;i++,index++)
		{
			unmap_frame(env->env_page_directory,(void*)env_page_ws_get_virtual_address(env,i));
			new_ws[i].virtual_address = env->ptr_pageWorkingSet[index].virtual_address;
			new_ws[i].time_stamp = env->ptr_pageWorkingSet[index].time_stamp;
			new_ws[i].empty = env->ptr_pageWorkingSet[index].empty;
		}
		kfree((void*)env->ptr_pageWorkingSet);
		env->ptr_pageWorkingSet = new_ws;
		env->page_WS_max_size = max_ws_size/2;

		priorities[no_of_envs].cur_priority = priority;
		priorities[no_of_envs].env_id = env->env_id;
		no_of_envs++;
	}
	if(priority == 4 && max_ws_size == cur_ws_size)
	{
		for(int i=0;i<no_of_envs;i++)
			if(priorities[i].env_id == env->env_id && priorities[i].cur_priority == priority)
				return;

		struct WorkingSetElement * new_ws = kmalloc((max_ws_size*2)*sizeof(struct WorkingSetElement));
		for(int i=0;i<max_ws_size;i++)
		{
			new_ws[i].virtual_address = env->ptr_pageWorkingSet[i].virtual_address;
			new_ws[i].time_stamp = env->ptr_pageWorkingSet[i].time_stamp;
			new_ws[i].empty = env->ptr_pageWorkingSet[i].empty;
		}
		kfree((void*)env->ptr_pageWorkingSet);
		env->ptr_pageWorkingSet = new_ws;
		env->page_WS_max_size = max_ws_size*2;

		priorities[no_of_envs].cur_priority = priority;
		priorities[no_of_envs].env_id = env->env_id;
		no_of_envs++;
	}
	if(priority == 5)
	{
		struct WorkingSetElement * new_ws = kmalloc((max_ws_size*2)*sizeof(struct WorkingSetElement));
		for(int i=0;i<max_ws_size;i++)
		{
			new_ws[i].virtual_address = env->ptr_pageWorkingSet[i].virtual_address;
			new_ws[i].time_stamp = env->ptr_pageWorkingSet[i].time_stamp;
			new_ws[i].empty = env->ptr_pageWorkingSet[i].empty;
		}
		kfree((void*)env->ptr_pageWorkingSet);
		env->ptr_pageWorkingSet = new_ws;
		env->page_WS_max_size = max_ws_size*2;

		priorities[no_of_envs].cur_priority = priority;
		priorities[no_of_envs].env_id = env->env_id;
		no_of_envs++;
	}
}
