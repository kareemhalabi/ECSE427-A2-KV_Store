//
// Created by KareemHalabi on 3/10/2017.
//

#ifndef A2_TESTCASE_A2_LIB_H
#define A2_TESTCASE_A2_LIB_H

#endif //A2_TESTCASE_A2_LIB_H

#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <stdlib.h>

#define DEF_KV_STORE(num_pods, pairs_per_pod, key_size, value_size) \
typedef struct {    \
    char key[key_size];       \
    char value[value_size];    \
} kv_pair; \
\
typedef struct {    \
    int write_ptr;  \
    int read_ptr;   \
    int read_cnt;   \
    kv_pair kv_pairs[pairs_per_pod]; \
} pod; \
\
typedef struct {    \
    char *name;     \
    pod pods[num_pods]; \
} kv_store;

DEF_KV_STORE(32,32,32,32); // Define kv_store params here
// it is recommended that you select num_pods and pairs per pod such that
// num_pods * pairs_per_pod >= 2 * max_pairs
// to ensure that all pairs are written to the store

#define SIZE_OF(element, member) \
sizeof(((element *)0)->member)

int kv_store_create(char *name);
int kv_store_write(char *key, char *value);
char *kv_store_read(char *key);
char **kv_store_read_all(char *key);
int kv_delete_db();
int hash_index(char *string, size_t len);

kv_store *kvStore;

/**
 * Simple hash function for char * to an unsigned long
 *
 * @param string
 * @param len Length of string
 * @return Hash value of string
 */
int hash_index(char *string, size_t len){
    unsigned long hashAddress = 5381;
    for (int counter = 0; counter < len; counter++){
        hashAddress = ((hashAddress << 5) + hashAddress) + string[counter];
    }
    int pod_size =  SIZE_OF(kv_store, pods) / sizeof(pod);
    return (int) (hashAddress % pod_size);
}

/**
 * Creates a new key-value store or opens a store with the same name if it exists.
 *
 * @param name
 * @return 0 if creation successful, -1 if error occurred
 */
int kv_store_create(char *name) {

    int erase = 1;

    // test if store already exists
    int fd = shm_open(name, O_CREAT|O_RDWR|O_EXCL, S_IRWXU);

    if(fd == -1) {
        if (errno == EEXIST) { // store does exist
            fd = shm_open(name, O_CREAT | O_RDWR, S_IRWXU);
            erase = 0;
        } else // some other error occured
            return -1;
    }

    ftruncate(fd, sizeof(kv_store)); // resize for kv_store
    kvStore =  mmap(NULL, sizeof(kv_store), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (kvStore == (void *) -1) // check for memory map error
        return -1;

    if(erase) { // clear memory and set name if new store created
        memset(kvStore,0, sizeof(kv_store));
        kvStore->name = name;
    }

    return 0;
}

/**
 * Writes a key value pair to a the store. Cannot write a null key or null value
 *
 * @param key
 * @param value
 * @return 0 if successful, -1 if unsuccessful
 */
int kv_store_write(char *key, char *value) {

    // Null pointer (and null value) check
    if(kvStore == 0 || key == NULL || *key == '\0' || value == NULL || *value == '\0')
        return -1;

    size_t key_size = strlen(key);

    if(key_size > SIZE_OF(kv_pair, key)) // truncate key if necessary
        key_size = SIZE_OF(kv_pair, key);

    size_t value_size = strlen(value); // truncate value if necessary
    if(value_size > SIZE_OF(kv_pair, value))
        value_size = SIZE_OF(kv_pair, value);

    int pod_index = hash_index(key, key_size);

    char * pod_sem_name = calloc(strlen(kvStore->name)*2, sizeof(char));
    sprintf(pod_sem_name, "/%s_pod%d", kvStore->name, pod_index);

    //-------------Begin Critical Section-----------------
    sem_t * pod_sem = sem_open(pod_sem_name, O_CREAT| O_RDWR, S_IRWXU, 1);
    sem_wait(pod_sem);

    pod *p = & kvStore->pods[pod_index];
    kv_pair *pair = & p->kv_pairs[p->write_ptr];

    // Write the key and value to the store
    memcpy(pair->key, key, key_size);
    memcpy(pair->value, value, value_size);

    // Update pointer to location of next write
    size_t num_pairs = SIZE_OF(pod, kv_pairs) / sizeof(kv_pair);
    p->write_ptr = (int) ((p->write_ptr + 1) % num_pairs);

    sem_post(pod_sem);
    //-------------End Critical Section-------------------

    free(pod_sem_name);

    return 0;
}

/**
 * Searches the kv_store for a key-value pair matching key. If found, it returns a copy of the value, else returns NULL.
 * If this function is called with the same key consecutively, it will return the next matching value
 *
 * @param key
 * @return A copy of the value if found, else returns NULL.
 */
char * kv_store_read(char *key) {

    if(key == NULL || *key == '\0')
        return NULL;

    size_t key_size = strlen(key);

    if(key_size > SIZE_OF(kv_pair, key)) // truncate key if necessary
        key_size = SIZE_OF(kv_pair, key);

    int pod_index = hash_index(key, key_size);

    size_t num_pairs = SIZE_OF(pod, kv_pairs) / sizeof(kv_pair);

    char * pod_sem_name = calloc(strlen(kvStore->name)*2, sizeof(char));
    sprintf(pod_sem_name, "/%s_pod%d", kvStore->name, pod_index);

    char * pod_read_cnt_name = calloc(strlen(kvStore->name)*2, sizeof(char));
    sprintf(pod_read_cnt_name, "/%s_pod_rc%d", kvStore->name, pod_index);

    //-------------Begin Critical Section-----------------
    sem_t * pod_sem = sem_open(pod_sem_name, O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_t * pod_rc_sem = sem_open(pod_read_cnt_name, O_CREAT | O_RDWR, S_IRWXU, 1);

    pod *p = & kvStore->pods[pod_index];
    kv_pair *pairs = p->kv_pairs;

    // increment reader count
    sem_wait(pod_rc_sem);
    p->read_cnt++;
    if(p->read_cnt == 1)
        sem_wait(pod_sem);

    sem_post(pod_rc_sem);

    char * copy = NULL;

    // size is either the size of the array
    // or after the last written key (if the array is not full)
    // whichever is lesser
    int size = (int) num_pairs;
    if(*pairs[p->write_ptr].key == '\0') {
        size = p->write_ptr;
    }

    // Don't bother searching if empty
    if(size > 0) {

        //Want to stop one index before start
        //If negative, wrap around to last value
        int stop = p->read_ptr;

        int i = p->read_ptr;
        do {
            if (strcmp(pairs[i].key, key) == 0) {
                copy = strdup(pairs[i].value);
                p->read_ptr = (i + 1) % size;
                break;
            }
            i = (i + 1) % size;
        } while(i != stop);
    }

    // decrement reader count
    sem_wait(pod_rc_sem);
    p->read_cnt--;
    if(p->read_cnt == 0)
        sem_post(pod_sem);

    sem_post(pod_rc_sem);

    //-------------End Critical Section-------------------

    free(pod_sem_name);
    free(pod_read_cnt_name);

    return copy;
}

/**
 * Searches the kv_store for all key-value pair matching key. If found, it returns a pointer
 * to an array containing the results. The array is terminated with a null pointer.
 *
 * @param key
 * @return All values of the matching key, else returns NULL.
 */
char **kv_store_read_all(char *key) {

    if(key == NULL || *key == '\0')
        return NULL;

    size_t key_size = strlen(key);

    if(key_size > SIZE_OF(kv_pair, key)) // truncate key if necessary
        key_size = SIZE_OF(kv_pair, key);

    int pod_index = hash_index(key, key_size);

    size_t num_pairs = SIZE_OF(pod, kv_pairs) / sizeof(kv_pair);

    char * pod_sem_name = calloc(strlen(kvStore->name)*2, sizeof(char));
    sprintf(pod_sem_name, "/%s_pod%d", kvStore->name, pod_index);

    char * pod_read_cnt_name = calloc(strlen(kvStore->name)*2, sizeof(char));
    sprintf(pod_read_cnt_name, "/%s_pod_rc%d", kvStore->name, pod_index);

    //-------------Begin Critical Section-----------------

    sem_t * pod_sem = sem_open(pod_sem_name, O_CREAT | O_RDWR, S_IRWXU, 1);
    sem_t * pod_rc_sem = sem_open(pod_read_cnt_name, O_CREAT | O_RDWR, S_IRWXU, 1);

    pod *p = & kvStore->pods[pod_index];
    kv_pair *pairs = p->kv_pairs;

    // increment reader count
    sem_wait(pod_rc_sem);
    p->read_cnt++;
    if(p->read_cnt == 1)
        sem_wait(pod_sem);

    sem_post(pod_rc_sem);

    // stop index is either the size of the array
    // or after the last written key (if the array is not full)
    // whichever is lesser
    int stop_read = (int) num_pairs;
    if(*pairs[p->write_ptr].key == '\0')
        stop_read = p->write_ptr;

    // temporary array for results
    char ** copy = (char **) malloc((num_pairs+1) * sizeof(char*));
    int num_results = 0;

    //Cycle through pod to find key
    for(int i = 0; i < stop_read; i++) {
        if(strcmp(pairs[i].key, key) == 0) {
            copy[num_results] = strdup(pairs[i].value);
            num_results++;
        }
    }

    // decrement reader count
    sem_wait(pod_rc_sem);
    p->read_cnt--;
    if(p->read_cnt == 0)
        sem_post(pod_sem);

    sem_post(pod_rc_sem);

    //-------------End Critical Section-------------------
    free(pod_sem_name);
    free(pod_read_cnt_name);

    if(num_results == 0) {
        return NULL;
    }

    copy[num_results] = NULL;
    char ** truncated_copy = realloc(copy, (num_results+1) * sizeof(char*));
    free(copy);

    return truncated_copy;
}

/**
 * Deletes the kv_store database and all associated semaphores
 *
 * @return 0 if successful, -1 if unsucessful
 */
int kv_delete_db() {

    char * pod_sem_name = calloc(strlen(kvStore->name)*2, sizeof(char));

    for(int i = 0; i < SIZE_OF(kv_store, pods) / sizeof(pod); i++) {
        sprintf(pod_sem_name, "/%s_pod%d", kvStore->name, i);
        sem_unlink(pod_sem_name);
    }

    free(pod_sem_name);
    shm_unlink(kvStore->name);

    return 0;
}