#pragma once
#include "common.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

typedef struct {
    ObjString* key;
    Value value;
} Entry;

// hashmap which does not use separate chaining (i.e. a linked list per bucket)
// instead, open addressing (also called closed hashing)
// the simplest form of which is called linear probing
// this just means if there's a collision, move ahead until a valid slot is found
typedef struct {
    size_t count;
    size_t capacity;
    Entry* entries;
} Table;

void initTable( Table* table );
void freeTable( Table* table );
void tableAddAll( Table* from, Table* to );
bool tableSet( Table* table, ObjString* key, Value value );
bool tableGet( Table* table, ObjString* key, Value* value );
