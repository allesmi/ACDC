/*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#ifndef ACDC_H
#define ACDC_H

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include "arch.h"

#define BIT_ZERO 1UL

#define HUGEPAGE_KB 2048

// function pointers to allocate and deallocate dynamic memory.
// ACDC uses these functions. By default, they will point to malloc and free,
// respectively, but one can point them to some custom allocation routines
// if necessary.
void* (*acdc_alloc)(size_t);
void (*acdc_free)(void*);


// Global options from command line
typedef struct global_options GOptions;

// thread local meta data and statistics
typedef struct mutator_context MContext;
// measurement data. part of MContext
typedef struct mutator_stat MStat;

// lifetime-size-class
typedef struct lifetime_size_class LSClass;
// lifetime-class
typedef struct lifetime_class LClass;
// node to organize lifetime-size-classes in a lifetime-class
typedef struct lifetime_size_class_node LSCNode;

// We have different types of memory objects. Based on how they are
// used, they get different types. Usually, an object comes with
// a payload, i.e., the memory right after it is also allocated.
// Object is a regular object.
typedef void Object;
// objects including ownership information
typedef struct shared_mem_object SharedObject; //TODO(martin): refactor away
// Objects that build up list-based lifetime-size-classes
typedef struct mem_object_lnode LObject;
// Objects that build up tree-based lifetime-size-classes
typedef struct mem_object_btnode BTObject;


#define debug(__mc, ...) _debug(__mc, __FILE__, __LINE__, __VA_ARGS__)
void _debug(MContext *mc, char *filename, int linenum, const char *format, ...);

//global acdc options
typedef enum {
  ACDC, //default mode
  FS
} benchmark_mode_t;
struct global_options {
  //benchmark options
  benchmark_mode_t mode; //-m: acdc, false-sharing, ...
  int num_threads;  //-n: number of mutator threads
  unsigned long time_quantum; //-t: allocated bytes until time advances
  int benchmark_duration; //-d: How long acdc will run
  int seed; //-r:
  size_t metadata_heap_sz; //-H: in kB
  
  //options for object creation
  int min_liveness; //-l: must be >= 1 and <= max_lifetime
  int max_liveness; //-L:
  int fixed_number_of_objects; //-F:
  int deallocation_delay; //-D:
  int max_time_gap; //-g: defaults to max_lifetime
  int min_object_sc; //-s: minimal sizeclass
  int max_object_sc; //-S: max sizeclass
  int list_based_ratio; //-q:
  int btree_based_ratio; //derived from -q
  size_t node_buffer_size; //-N: used to recycle nodes for LSClasses
  size_t class_buffer_size; //-C: used to recycle nodes for LSClasses
  
  //options for object access
  int write_iterations; //-i:
  int access_live_objects; //-A
  int write_access_ratio; //-w: 

  //sharing options
  int shared_objects; //-O:
  int shared_objects_ratio; //-R: share_ratio% of all objects will be shared
  int receiving_threads_ratio; //-T: share_thread_ratio% of all threads will be involved

  //misc options
  int verbosity; //-v
  char *allocator_name;
  int do_baseline_rss; //if 1, report acdc's memory baseline instead of RSS from OS
  pid_t pid;
  int use_hugepages;
  int use_compact_allocation; // this used to be OPTIMAL_MODE
};


//mutator measurement data
struct mutator_stat {
  unsigned long long running_time;
  unsigned long long allocation_time;
  unsigned long long deallocation_time;
  unsigned long long access_time;
  unsigned long bytes_allocated;
  unsigned long bytes_deallocated;
  unsigned long objects_allocated;
  unsigned long objects_deallocated;
  unsigned long *lt_histogram;
  unsigned long *sz_histogram;
  long current_rss;
  long resident_set_size_counter;
};

//memory objects
//the min. size for an object must be sizeof(Object)
struct shared_mem_object {
  //reference_map_t reference_map; 
  //a bit at pos i indicates that thread i may access this object
};
struct mem_object_lnode {
  LObject *next;
};
struct mem_object_btnode {
  BTObject *left;
  BTObject *right;
};

//implementation types for lifetime-size-classes
typedef enum {
  LIST,
  BTREE,
  FALSE_SHARING,
  OPTIMAL_LIST, 
  OPTIMAL_BTREE,
  OPTIMAL_FALSE_SHARING
} lifetime_size_class_type;

//set of objects with common size and lifetime
struct lifetime_size_class {
  //mark which threads share this LSClass
  //ReferenceMap reference_map;
  uint64_t reference_counter;
  
  size_t object_size;
  unsigned int lifetime;
  size_t num_objects;
  lifetime_size_class_type type;
  
  //pointer to the start of the objects
  Object *start;
};

//nodes to build lists of lifetime-size-classes
struct lifetime_size_class_node {
  LSCNode *prev;
  LSCNode *next;
  LSClass *ls_class;
};

//list of LSClasses with the same lifetime
struct lifetime_class {
  LSCNode *first;
  LSCNode *last;
};

//thread context specific data
struct mutator_context {
  GOptions *gopts; //pointer to global options. same for all threads
  int thread_id;
  int rand;
  MStat *stat; //mutator stats
  unsigned int time;
  LClass *heap_class; // one LClass for each possible lifetime

  LSCNode *node_buffer_memory;
  int node_buffer_counter;
  LClass node_cache;

  LSClass *class_buffer_memory;
  int class_buffer_counter;
  LClass class_cache;
  int *thread_id_buffer; //to temporarily store a couple of thread id's
};

/*
 * allocates a lifetime-size-class, i.e., a set of objects with
 * the same size and lifetime.
 * type defines the implementation type (e.g. list-based)
 */
LSClass *allocate_LSClass(MContext *mc, lifetime_size_class_type type, size_t sz, 
		unsigned long nelem);
void deallocate_LSClass(MContext *mc, LSClass *oc); 
void traverse_LSClass(MContext *mc, LSClass *oc);


/*
 * lifetime-classes are doubly-linked lists of lifetime-size-classes.
 * more specifically, lists of LSCNodes that represent lifetime-size-classes
 */
void lclass_insert_after(LClass *list, LSCNode *after, LSCNode *c);
void lclass_insert_before(LClass *list, LSCNode *before, LSCNode *c);
void lclass_insert_beginning(LClass *list, LSCNode *c);
void lclass_insert_end(LClass *list, LSCNode *c);
void lclass_remove(LClass *list, LSCNode *c);



/*
 * abstractions of allocator and memory access
 */
Object *allocate(MContext *mc, size_t size);
void deallocate(MContext *mc, Object *o, size_t size);
Object *allocate_aligned(MContext *mc, size_t size, size_t alignment);
void deallocate_aligned(MContext *mc, Object *o, size_t size, size_t alignment);
unsigned int get_sizeclass(size_t size);


void get_random_object_props(MContext *mc, 
		size_t *size, 
		unsigned int *liveness, 
		unsigned int *num_objects,
    lifetime_size_class_type *type);
unsigned int get_sharing_dist(MContext *mc);
void get_random_thread_selection(MContext *mc, int *thread_id_array, int *thread_id_array_sz);

/*
 * benchmark invocation
 */
void run_acdc(GOptions *gopts);

#endif
