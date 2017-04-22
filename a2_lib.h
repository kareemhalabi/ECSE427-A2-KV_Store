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
    char last_key[key_size];\
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

DEF_KV_STORE(128,128,32,32); // Define kv_store params here
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