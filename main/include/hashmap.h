#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 32

typedef struct Entry {
    char *key;
    int value;
    struct Entry *next;
} Entry;

typedef struct HashMap HashMap;

struct HashMap {
    Entry *buckets[TABLE_SIZE];
    void (*put)(HashMap *, const char *, int);
    int (*get)(HashMap *, const char *, int*);
};

HashMap init_hashmap();
unsigned int hash(const char *key);
void hashmap_put(HashMap *, const char *, int);
int hashmap_get(HashMap *, const char *, int *);