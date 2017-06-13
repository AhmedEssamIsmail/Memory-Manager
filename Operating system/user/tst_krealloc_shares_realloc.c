// Test the SPECIAL CASES during the creation of shared variables (create_shared_memory)
#include <inc/lib.h>
#define ARRAY_OF_TEN_INTS (10 * sizeof(int))
void _main(void) {
	//Initial test to ensure it works on "PLACEMENT" not "REPLACEMENT"
	{
		uint8 fullWS = 1;
		for (int i = 0; i < myEnv->page_WS_max_size; ++i) {
			if (myEnv->__uptr_pws[i].empty) {
				fullWS = 0;
				break;
			}
		}
		if (fullWS)
			panic("Please increase the WS size");
	}
	cprintf("************************************************\n");
	cprintf(
			"MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("************************************************\n\n\n");

	uint32 *x, *y, *z, *outOfBound;
	cprintf("STEP A: creating shared arrays of 10 items...\n\n");
	{
		int freeFrames = sys_calculate_free_frames();
		x = smalloc("x", ARRAY_OF_TEN_INTS, 1);
		if (x != (uint32*) USER_HEAP_START)
			panic(
					"Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");
		if ((freeFrames - sys_calculate_free_frames()) != 1 + 1 + 2)
			panic(
					"Wrong allocation: make sure that you allocate the required space in the user environment and add its frames to frames_storage. alocc frames = %d\n",
					(freeFrames - sys_calculate_free_frames()));
		freeFrames = sys_calculate_free_frames();
		y = smalloc("y", ARRAY_OF_TEN_INTS, 1);
		if (y != (uint32*) (USER_HEAP_START + 1 * PAGE_SIZE))
			panic(
					"Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address.ret va = %x\n",
					y);
		if ((freeFrames - sys_calculate_free_frames()) != 1 + 0 + 2)
			panic(
					"Wrong allocation: make sure that you allocate the required space in the user environment and add its frames to frames_storage");

		freeFrames = sys_calculate_free_frames();
		z = smalloc("z", ARRAY_OF_TEN_INTS, 1);
		if (z != (uint32*) (USER_HEAP_START + 2 * PAGE_SIZE))
			panic(
					"Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");
		if ((freeFrames - sys_calculate_free_frames()) != 1 + 0 + 2)
			panic(
					"Wrong allocation: make sure that you allocate the required space in the user environment and add its frames to frames_storage");

	}

	cprintf("STEP B: Initializing shared arrays' elements... \n\n");
	{
		//Initializing shared array x
		for (int i = 0; i < ARRAY_OF_TEN_INTS / sizeof(int); i++) {
			x[i] = 1;
		}

		//Initializing shared array z
		for (int i = 0; i < ARRAY_OF_TEN_INTS / sizeof(int); i++) {
			y[i] = 2;
		}

		//Initializing shared array z
		for (int i = 0; i < ARRAY_OF_TEN_INTS / sizeof(int); i++) {
			z[i] = 3;
		}
	}

	cprintf(
			"STEP C: checking the creation of a number of shared objects that exceeds the MAX ALLOWED NUMBER of OBJECTS... \n\n");
	{
		uint32 maxShares = sys_getMaxShares();
		int i;
		for (i = 0; i < maxShares - 3; i++) {
			char shareName[10];
			ltostr(i, shareName);
			outOfBound = smalloc(shareName, 1, 1);
			//cprintf("smalloc finished\n");
			if (outOfBound == NULL)
				panic("WRONG... supposed no problem in creation here!!");
		}
		//cprintf("loop finished\n");
		outOfBound = smalloc("outOfBounds", 1, 1);
		//cprintf("out of bounds created\n");
		if (outOfBound == NULL)
			panic(
					"Creation of a new shared variable failed after consuming all available shared variables");
	}

	cprintf(
			"STEP D: Testing memory access after re-allocating shares array...\n\n");
	{
		int32 sum = 0;
		//Check sum of shared array x
		for (int i = 0; i < ARRAY_OF_TEN_INTS / sizeof(int); i++) {
			sum += x[i];
		}

		if (sum != 1 * ARRAY_OF_TEN_INTS / sizeof(int))
			panic("Invalid memory values after re-allocating shares array");

		sum = 0;
		//Check sum of shared array y
		for (int i = 0; i < ARRAY_OF_TEN_INTS / sizeof(int); i++) {
			sum += y[i];
		}

		if (sum != 2 * ARRAY_OF_TEN_INTS / sizeof(int))
			panic("Invalid memory values after re-allocating shares array");

		sum = 0;
		//Check sum of shared array z
		for (int i = 0; i < ARRAY_OF_TEN_INTS / sizeof(int); i++) {
			sum += z[i];
		}

		if (sum != 3 * ARRAY_OF_TEN_INTS / sizeof(int))
			panic("Invalid memory values after re-allocating shares array");
	}
	cprintf(
			"Congratulations!! Test of krealloc of shares array completed successfully!!\n\n\n");

	return;
}
