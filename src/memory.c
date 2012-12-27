#include "acdc.h"
#include "memory.h"

void *allocate(size_t size, MContext *mc) {
	void *ptr;
	//TODO: log allocator activity
	
	if (size < sizeof(Object)) {
		printf("Error: min object size is %d. Requested: %d\n",
				sizeof(Object),
				size);
		exit(1);
	}

	ptr = malloc(size);
	
	//set header information
	Object *o = (Object*)ptr;
	o->rctm = 0;
	o->size = size;

	//update mutator stats
	mc->stat->bytes_allocated += size;
	mc->stat->objects_allocated++;

	return ptr;
}

void deallocate(void *ptr, MContext *mc) {
	Object *o = (Object*)ptr;

	//update mutator stats
	mc->stat->bytes_deallocated += o->size;
	mc->stat->objects_deallocated++;

	free(ptr);
}


void access_object(Object *o) {
	int i;
	//payload size can be calculated from object and header size
	size_t pl_sz = o->size - sizeof(Object) + sizeof(char*);
	
	for (i = 1; i < pl_sz; ++i) {
		o->payload[i] = (o->payload[i-1]) + 1;
	}
}


unsigned int get_sizeclass(size_t size) {
	//TODO: implement
	return 0;
}

