#define DMALLOC_DISABLE 1
#include "dmalloc.hh"
#include <cassert>
#include <cstring>

// c++ libraries necessary for function implementations only
#include <map> // hash map
#include <algorithm> // sorting
#include <string> // maps keyed on std::strings

#include <vector>

struct dmalloc_stats stats_holder;

// hash map (filename,linenumber) ==> allocation_call
// this is a hash map to associate a filename and line number pair with an allocation tracker 
// struct. This allows us to keep data on what specific dmalloc invocations have done what. How 
// many bytes they've allocated, how many allocations they have active etc. 
static std::map<std::pair<std::string,long>, allocation_tracker> at_table;

 void add_allocation_instance(allocation_tracker& at, size_t size, void* pointer) {
    at.total_allocations++;
    at.total_bytes_allocated += size; 
    at.active_allocations.push_back(pointer);
}

void remove_allocation_instance(allocation_tracker& at, void* pointer) {
    for (auto i = at.active_allocations.begin(); i != at.active_allocations.end(); ++i) {
        if (*i == pointer) {
            at.total_allocations--;
            at.active_allocations.erase(i);
            break;
        }
    }
}

/**
 * dmalloc(sz,file,line)
 *      malloc() wrapper. Dynamically allocate the requested amount `sz` of memory and 
 *      return a pointer to it 
 * 
 * @arg size_t sz : the amount of memory requested 
 * @arg const char *file : a string containing the filename from which dmalloc was called 
 * @arg long line : the line number from which dmalloc was called 
 * 
 * @return a pointer to the heap where the memory was reserved
 */
void* dmalloc(size_t sz, const char* file, long line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    // Your code here.

    //if the size plus the metadata is somehow less than the size 
    //then it overflowed
    if (sz + ((3 * sizeof(size_t)) + (24 * sizeof(char)) + sizeof(long)) < sz) {
        stats_holder.nfail = stats_holder.nfail + 1;
        stats_holder.fail_size = stats_holder.fail_size + sz;
        return nullptr;
    }

    void* ptr = base_malloc(sz + ((3 * sizeof(size_t)) + (24 * sizeof(char)) + sizeof(long))); 

    if (ptr == nullptr) {
        //it failed
        stats_holder.nfail = stats_holder.nfail + 1;
        stats_holder.fail_size = stats_holder.fail_size + sz;
    } else {
        //it was successful
        stats_holder.nactive = stats_holder.nactive + 1;
        stats_holder.active_size = stats_holder.active_size + sz;
        stats_holder.ntotal = stats_holder.ntotal + 1;
        stats_holder.total_size = stats_holder.total_size + sz;

        //add metadata of the size
        *((size_t *) ptr) = sz;

        //increment pointer to get to the next part of the metadata
        ptr = (char *) ptr + sizeof(size_t);

        //add metadata of if it is free or not
        //make it 1 because it is not free
        *((size_t *) ptr) = 1;

        //increment pointer to get to its payload
        ptr = (char *) ptr + sizeof(size_t);

        //if heap min is uninitialized or the address is smaller
        if (stats_holder.heap_min == 0 || ((uintptr_t) ptr) < stats_holder.heap_min) {
            stats_holder.heap_min = (uintptr_t) ptr;
        }
        if (((uintptr_t) ((char *) ptr + sz)) > stats_holder.heap_max) {
            stats_holder.heap_max = ((uintptr_t) ((char *) ptr + sz));
        }

        //num to check for out of bounds detection
        size_t *secret_num = (size_t *) ((char *) ptr + sz);
        *secret_num = 5;

        //increment to get to new metadata (file name)
        char* new_ptr = (char *) ((char*) secret_num + sizeof(size_t))

        strcpy(new_ptr, file)

        //increment by 24 bytes to get past the file name to the line number metadata
        long* line_num = (long *) ((char *) new_ptr + 24)

        *line_num = line
    }

    //update the malloc call's tracker or make a tracker if one doesn't exist
    td::pair<std::string, long> key = std::make_pair(file, line);
    if (at_table.find(key) == at_table.end()) {
        // not found make a new allocation tracker and add to hashmap 
        allocation_tracker at;

        strcpy(at.calling_file, file);
        at.calling_line = line; 
        at.total_allocations = 0; 
        at.total_bytes_allocated = 0;
        //enter it into the hashmap
        at_table[key] = at;  
    }

    add_allocation_instance(at_table[key], sz, ptr);

    return ptr;
}

/**
 * dfree(ptr, file, line)
 *      free() wrapper. Release the block of heap memory pointed to by `ptr`. This should 
 *      be a pointer that was previously allocated on the heap. If `ptr` is a nullptr do nothing. 
 * 
 * @arg void *ptr : a pointer to the heap 
 * @arg const char *file : a string containing the filename from which dfree was called 
 * @arg long line : the line number from which dfree was called 
 */
void dfree(void* ptr, const char* file, long line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    //if there is nothing to free
    if (ptr == nullptr) {
        return;
    }

    // if (ptr < ((void *) 2 * sizeof(size_t))) {
    //     fprintf(stderr, "MEMORY BUG: invalid free of pointer %p, not in heap\n", ptr);
    // }

    if ((uintptr_t) ptr < stats_holder.heap_min || (uintptr_t) ptr > stats_holder.heap_max) {
        fprintf(stderr, "MEMORY BUG: invalid free of pointer %p, not in heap\n", ptr);
        abort();
    }
    
    //decrement the pointer to get to whether it's free
    char * metadata_ptr = (char *) ptr - sizeof(size_t);
    size_t* free_bool_ptr = ((size_t *) metadata_ptr);

    //if the block hasn't been freed (it's allocated)
    if (*free_bool_ptr == 1) {
        //mark it as free now
        *free_bool_ptr = 2;
    } else if (*free_bool_ptr == 2){
        //error because we are trying to free a free block again
        fprintf(stderr, "MEMORY BUG: invalid free of pointer %p, double free\n", ptr);
        abort();
    } else {
        //error because we are trying to free something that has not been allocated
        fprintf(stderr, "MEMORY BUG: invalid free of pointer %p, not allocated\n", ptr);
        abort();
    }

    //decrement to get to the size metadata
    metadata_ptr = (char *) metadata_ptr - sizeof(size_t);
    size_t sz = *((size_t *) metadata_ptr);

    size_t* secret_ptr = (size_t *) ((char *) ptr + sz);
    if (*secret_ptr != 5) {
        fprintf(stderr, "MEMORY BUG: detected wild write during free of pointer %p\n", ptr);
        return;
    }

    stats_holder.active_size = stats_holder.active_size - sz;
    stats_holder.nactive = stats_holder.nactive - 1;


    //increment to get to new metadata (file name)
    char* new_ptr = (void *) ((char*) secret_num + sizeof(size_t))

    //increment by 24 bytes to get past the file name to the line number metadata
    long* line_num = (long *) ((char *) new_ptr + 24)

     // look for allocation tracker to update
    std::pair<std::string, long> key = std::make_pair(new_ptr, *line_num);
    if (at_table.find(key) == at_table.end()) {
        // the allocation tracker must exist at this point because this was a valid dfree() call
        fprintf(stderr, "CRITICAL ERROR: lost allocation tracker for %s:%ld \n",new_ptr, *line_num);
        exit(0);
    }

    remove_allocation_instance(at_table[key], ptr);
    

    base_free(metadata_ptr);
}

/**
 * dcalloc(nmemb, sz, file, line)
 *      calloc() wrapper. Dynamically allocate enough memory to store an array of `nmemb` 
 *      number of elements with wach element being `sz` bytes. The memory should be initialized 
 *      to zero  
 * 
 * @arg size_t nmemb : the number of items that space is requested for
 * @arg size_t sz : the size in bytes of the items that space is requested for
 * @arg const char *file : a string containing the filename from which dcalloc was called 
 * @arg long line : the line number from which dcalloc was called 
 * 
 * @return a pointer to the heap where the memory was reserved
 */
void* dcalloc(size_t nmemb, size_t sz, const char* file, long line) {
    // Your code here (to fix test014).
    //if there is overflow so that the multiplication is not correct
    if (((nmemb * sz)/sz) != nmemb) {
        stats_holder.nfail = stats_holder.nfail + 1;
        stats_holder.fail_size = stats_holder.fail_size + (nmemb * sz);
        return nullptr;
    }


    void* ptr = dmalloc(nmemb * sz, file, line);
    if (ptr) {
        memset(ptr, 0, nmemb * sz);
    }
    return ptr;
}

/**
 * get_statistics(stats)
 *      fill a dmalloc_stats pointer with the current memory statistics  
 * 
 * @arg dmalloc_stats *stats : a pointer to the the dmalloc_stats struct we want to fill
 */
void get_statistics(dmalloc_stats* stats) {
    // Stub: set all statistics to enormous numbers
    memset(stats, 255, sizeof(dmalloc_stats));
    // Your code here.
    stats->nactive = stats_holder.nactive;
    stats->active_size = stats_holder.active_size;
    stats->ntotal = stats_holder.ntotal;
    stats->total_size = stats_holder.total_size;
    stats->nfail = stats_holder.nfail;
    stats->fail_size = stats_holder.fail_size;
    stats->heap_max = stats_holder.heap_max;
    stats->heap_min = stats_holder.heap_min;
}

/**
 * print_statistics()
 *      print the current memory statistics to stdout       
 */
void print_statistics() {
    dmalloc_stats stats;
    get_statistics(&stats);

    printf("alloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("alloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}

/**  
 * print_leak_report()
 *      Print a report of all currently-active allocated blocks of dynamic
 *      memory.
 */
void print_leak_report() {
    // Your code here.
    // Dont mind the c++ syntax, just a simple iteration through the at_table to find all 
    // the active allocation instances
    for (auto i = at_table.begin(); i != at_table.end(); ++i) {
        std::pair<std::string, long> key = i->first; 
        allocation_tracker at = i->second; 

        for (auto ii = at.active_allocations.begin(); ii != at.active_allocations.end(); ++ii){
            size_t *size_ptr = (size_t *) ((char *) ii - 2*sizeof(size_t));
            fprintf(stdout, "LEAK CHECK: %s:%ld: allocated object %p with size %lu\n",key.first.c_str(),key.second,
                ii, *size_ptr);
        }
    }

}
