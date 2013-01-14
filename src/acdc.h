#ifndef ACDC_H
#define ACDC_H

#include <pthread.h>
#include <sys/types.h>
#include <glib.h>

//global acdc options
typedef enum {
  ACDC, //default mode
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
  int min_object_sc; //-s: minimal sizeclass
  int max_object_sc; //-S: max sizeclass
  int list_ratio; //-q:
  int btree_ratio; //-b:
  int false_sharing_ratio; //-f

  //sharing options
  int share_objects; //-O:
  int share_ratio; //-R: share_ratio% of all objects will be shared
  int share_thread_ratio; //-T: share_thread_ratio% of all threads will be involved

  //misc options
  int verbosity; //-v
};

//thread local mutator options
typedef struct mutator_options MOptions;
struct mutator_options {
  int thread_id;
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
};


//memory object abstraction
/*
 * Some Makros to access the reference counter and the thread mask
 * out of the combined rctm field. RCTM creates the combination
 * of a RC and a TM
 */
#define RC(_rctm) (_rctm >> 58)
#define TM(_rctm) ((_rctm << 6) >> 6)
#define RCTM(_rc, _tm) ((_rc << 58) | _tm)


//object header for every allocated object
//the min. size for an object must be sizeof(Object)
//typedef struct mem_object Object;
typedef void Object;
typedef struct shared_mem_object SharedObject;
struct shared_mem_object {
  u_int64_t rctm; //6bit rc, 58 bit thread map
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
} collection_t;

//object pool where threads keep refs to the memory chunks
typedef struct object_collection OCollection;
struct object_collection {
  size_t object_size;
  size_t num_objects;
  unsigned int id; // in case we have more collections of the same size
  collection_t type;

  //sharing and synchronization
  SharedObject shared_object;
  pthread_barrier_t barrier;
  
  //pointer to start of collection
  Object *start;
};

typedef struct collection_pool CollectionPool;
struct collection_pool {
  unsigned int remaining_lifetime;
  GHashTable *collections; //hash table with one OCollection per size and id
};




typedef struct mutator_context MContext;

OCollection *allocate_collection(MContext *mc, collection_t ctype, size_t sz,
		unsigned long nelem);
void deallocate_collection(MContext *mc, OCollection *oc); 
void traverse_collection(MContext *mc, OCollection *oc);
int collection_is_shared(MContext *mc, OCollection *oc);


OCollection *new_collection(MContext *mc, collection_t t, size_t sz, 
                            unsigned long nelem);

void share_collection(OCollection *oc, u_int64_t rctm);

//thread context specific data
struct mutator_context {
  GOptions *gopts; //pointer to global options. same for all threads
  MOptions opt; //thread local options
  MStat *stat; //mutator stats
  CollectionPool *collection_pools; //one pool for each possible lifetime
  unsigned int time;
};


void run_acdc(GOptions *gopts);



Object *allocate(MContext *mc, size_t size);
void deallocate(MContext *mc, Object *o, size_t size);
Object *allocate_aligned(MContext *mc, size_t size, size_t alignment);
void deallocate_aligned(MContext *mc, Object *o, size_t size, size_t alignment);
void access_object(Object *o, size_t size, size_t offset);
unsigned int get_sizeclass(size_t size);

GRand *init_rand(unsigned int seed);
void free_rand(GRand *rand);
void get_random_object_props(MContext *mc, 
		size_t *size, 
		unsigned int *lifetime, 
		unsigned int *num_objects,
    collection_t *type,
    u_int64_t *rctm
    );


#endif
