/*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#ifndef ACDC_H
#define ACDC_H

#include <pthread.h>
#include <sys/types.h>
#include <glib.h>

typedef void OCollection;

//global acdc options
typedef enum {
  ACDC, //default mode
  FS
} benchmark_mode_t;
typedef struct global_options GOptions;
struct global_options {
  //benchmark options
  benchmark_mode_t mode; //-m: acdc, false-sharing, ...
  int num_threads;  //-n: number of mutator threads
  int time_threshold; //-t: allocated bytes until time advances
  int benchmark_duration; //-d: How long acdc will run
  int seed; //-r:
  
  //options for object creation
  int min_lifetime; //-l: must be >= 1 and <= max_lifetime
  int max_lifetime; //-L:
  int deallocation_delay; //-D:
  int max_time_gap; //-g: defaults to max_lifetime
  int min_object_sc; //-s: minimal sizeclass
  int max_object_sc; //-S: max sizeclass
  int list_ratio; //-q:
  int btree_ratio; //-b:
  
  //options for object access
  int write_iterations; //-i:
  int skip_traversal; //-k
  int write_ratio; //-w: 

  //sharing options
  int share_objects; //-O:
  int share_ratio; //-R: share_ratio% of all objects will be shared
  int share_thread_ratio; //-T: share_thread_ratio% of all threads will be involved

  //misc options
  int verbosity; //-v
  pid_t pid;
};

//thread local mutator options
typedef struct mutator_options MOptions;
struct mutator_options {
  //int thread_id;
  GRand *rand; //GLib's Mersenne Twister PRNG
};

//mutator measurement data
typedef struct mutator_stat MStat;
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
  long vm_peak;
  long rss_hwm;
  long current_rss;
  long resident_set_size_counter;
};


//memory object abstraction

//object header for every allocated object
//the min. size for an object must be sizeof(Object)
//typedef struct mem_object Object;
typedef void Object;
typedef struct shared_mem_object SharedObject;
struct shared_mem_object {
  u_int64_t sharing_map; 
  //a bit at pos i indicates that thread i may access this object
};
typedef struct mem_object_lnode LObject;
struct mem_object_lnode {
  //Object o;
  LObject *next;
};
typedef struct mem_object_btnode BTObject;
struct mem_object_btnode {
  //Object o;
  BTObject *left;
  BTObject *right;
};



//Collection stuff
typedef enum {
  LIST,
  BTREE,
  FALSE_SHARING,
  OPTIMAL_LIST, 
  OPTIMAL_BTREE,
  OPTIMAL_FALSE_SHARING
} collection_type;

//set of objects with common size and lifetime
typedef struct lifetime_size_class LSClass;
struct lifetime_size_class {
  LSClass *prev; //to construct a list
  LSClass *next; //to construct a list

  size_t object_size;
  size_t num_objects;
  collection_type type;

  //which threads should share an object sharing
  volatile u_int64_t sharing_map;

  //mark which threads already have this OColelction
  volatile u_int64_t reference_map;
  
  //pointer to the start of the objects
  Object *start;
};

//list of LSClasses with the same lifetime
typedef struct lifetime_class {
  LSClass *first;
  LSClass *last;
} LClass;






typedef struct mutator_context MContext;

LSClass *allocate_LSClass(MContext *mc, collection_type ctype, size_t sz,
		unsigned long nelem, u_int64_t sharing_map);

void deallocate_LSClass(MContext *mc, LSClass *oc); 
void traverse_LSClass(MContext *mc, LSClass *oc);

//int LSClass_is_shared(MContext *mc, OCollection *oc);


LSClass *new_LSClass(MContext *mc, collection_type t, size_t sz, 
                            unsigned long nelem, u_int64_t sharing_map);

//void share_collection(OCollection *oc, u_int64_t sharing_map);

//thread context specific data
struct mutator_context {
  GOptions *gopts; //pointer to global options. same for all threads
  int thread_id;
  MOptions opt; //thread local options TODO: remove
  MStat *stat; //mutator stats
  unsigned int time;
  LClass **expiration_class; // one LClass for each possible lifetime
};


void run_acdc(GOptions *gopts);


Object *allocate(MContext *mc, size_t size);
void deallocate(MContext *mc, Object *o, size_t size);
Object *allocate_aligned(MContext *mc, size_t size, size_t alignment);
void deallocate_aligned(MContext *mc, Object *o, size_t size, size_t alignment);
void write_object(Object *o, size_t size, size_t offset);
unsigned int get_sizeclass(size_t size);

GRand *init_rand(unsigned int seed);
void free_rand(GRand *rand);
void get_random_object_props(MContext *mc, 
		size_t *size, 
		unsigned int *lifetime, 
		unsigned int *num_objects,
    collection_type *type,
    u_int64_t *sharing_map
    );

unsigned int get_random_thread(MContext *mc);


#endif
