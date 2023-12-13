#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <stdint.h>

// Define the maximum size of the cache
#define CACHE_SIZE 17

typedef struct Node {
    char key[50];
    void* value;
    struct Node* next;
    struct Node* prev;
} Node;

typedef struct LRUCache {
    Node* head;
    Node* tail;
    Node* hashmap[CACHE_SIZE];
} LRUCache;

// Function prototypes
//Global
//Initializes Cache
LRUCache* createCache(); 
//Insert a value with a key into cache
void put(LRUCache* cache, const char* key, void* value); 
//Retreive a value from a key in Hash
const void* get(LRUCache* cache, const char* key); 
//Frees the Cache
void freeCache(LRUCache* cache);
//Print all value , key tuple in cache
void printCache(LRUCache* cache); 
//Flushes the cache
void flushCache(LRUCache* cache);


//Local Helpers
Node* createNode(const char* key, void* value);
void removeFromList(LRUCache* cache, Node* node);
void moveToHead(LRUCache* cache, Node* node);
int cacheSize(LRUCache* cache);
int hashFunction(const char* key);

#endif /* LRU_CACHE_H */
