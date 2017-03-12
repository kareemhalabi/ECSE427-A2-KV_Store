//
// Created by KareemHalabi on 3/10/2017.
//

#include "a2_lib.h"

int main() {
    kv_store_create("testKV");
    printf("Create success\n");
    kv_store_write("test key1", "test value");
    printf("Write success\n");
    kv_store_write("test key1", "test value 2");
    printf("%s\n", kv_store_read("test key1"));
    printf("%s\n", kv_store_read("test key1"));
    printf("%s\n", kv_store_read("test key1"));
    kv_delete_db();
    printf("Delete sucess\n");
}