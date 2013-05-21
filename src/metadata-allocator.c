 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"

static void *metadata_heap_start;
static void *metadata_heap_end;
static void *metadata_heap_bump_pointer;

static void *align_address(void *ptr, size_t alignment) {
	long addr = ((long)ptr)+ (alignment-1);
	addr = addr & ~(alignment-1);
	return (void*)addr;
}

void init_metadata_heap(size_t heapsize) {
	
	metadata_heap_start = sbrk(heapsize * 1024); //parameter is in kB
	if (metadata_heap_start == (void*)-1) {
		printf("unable to allocate metadata heap\n");
		exit(EXIT_FAILURE);
	}
	metadata_heap_end = sbrk(0);
	metadata_heap_bump_pointer = metadata_heap_start;

	//make heap hot
	int i;
	volatile void *ptr;
	for (i = 1; i < heapsize * 1024; i = i + 64) {
		ptr = (void*)((long)metadata_heap_start + i);
		*(int*)ptr = i;
	}
}

static void *get_chunk(size_t size) {
	void *ptr = metadata_heap_bump_pointer;
	metadata_heap_bump_pointer += size;
	if (metadata_heap_bump_pointer >= metadata_heap_end) {
		printf("out of metadata space. Increase -H option\n");
		exit(EXIT_FAILURE);
	}
	return ptr;
}

void *malloc_meta(size_t size) {
	//default alignment: 8 bytes
	return align_address(get_chunk(size + 7), 8);
}

void *calloc_meta(size_t nelem, size_t size) {
	void *ptr = malloc_meta(nelem * size);
	int i;
	for (i = 0; i < nelem*size; ++i) {
		((char*)ptr)[i] = '\0';
	}
	return ptr;
}

void *malloc_meta_aligned(size_t size, size_t alignment) {
	return align_address(get_chunk(size + alignment - 1), alignment);
}

void *calloc_meta_aligned(size_t nelem, size_t size, size_t alignment) {
	void *ptr = malloc_meta_aligned(nelem * size, alignment);
	int i;
	for (i = 0; i < nelem*size; ++i) {
		((char*)ptr)[i] = '\0';
	}
	return ptr;
}
