#ifndef __NS_H__
#define __NS_H__

#include <pthread.h>
#include <semaphore.h>
#include "Trie.h"
#include "LRU.h"
#include "External.h"
#define MAX_CONNECTION_Q 5
#define GLOBAL_SERVER_PORT 8080
#define GLOBAL_CLIENT_PORT 8081
#define MAX_BUFFER 4096
#define SLEEP_TIME 5
#define PING_TIMEOUT 5000 // 5 seconds
#define FLUSH_TIMEOUT 10 // 10 seconds

#define MAX_SERVER_THREADS 25
#define BACKUP_SERVERS 1   
#define MAX_SERVER_REQUESTS 10

// Global variables

// struct for server handle containing all the information about the server connection
typedef struct Server_Handle{
    int Server_ID; // ID of the server
    int Server_Down; // Flag for server down
    int Server_BackUp; // counter for backups(from other server) in server
    // int CMD;
    char paths[MAX_BUFFER]; // Paths of the exposed directories structure
    sem_t req; // Condition variable for setting MAX REQUEST for a server
    char* Client_IP; // IP of the storage_server exposed to client
    int Client_Port; // Port of the storage_server exposed to client
    char* Naming_IP; // IP of the storage_server exposed to naming_server
    int Naming_Port; // Port of the storage_server exposed to naming_server for listening to data
    int SS_sockfd; // Socket file descriptor of the storage_server which is use for sending data
    int ssl;
    // int Client_sockfd; // Socket file descriptor of the client
    struct Server_Handle* BackUp[BACKUP_SERVERS]; // Backup servers
}Server_Handle;
extern Server_Handle Server_Handle_Array[MAX_SERVER_THREADS];
// Thread pool for connections
extern pthread_t server_thread_pool[MAX_SERVER_THREADS];
// Mutex for thread pool
extern pthread_mutex_t server_thread_pool_mutex;
// index for thread pool
extern int Server_index;
// counter for servers
extern int Server_Counter;

// struct for client handle containing all the information about the client connection
typedef struct Client_Handle{
    int Client_ID; // ID of the client
    int Client_sockfd; // Socket file descriptor of the client
    char* Client_IP; // IP of the client
    int Client_Port; // Port of the client
    int ACK_flag; // Flag for ACK
    pthread_t client_thread; // Thread for client
    struct Client_Handle* prev; // Pointer to previous client
    struct Client_Handle* next; // Pointer to next client
}Client_Handle;
// Head of the client linked list
extern Client_Handle* Client_List;
// Mutex for Client_List
extern pthread_mutex_t Client_List_mutex;
// counter for clients
extern int Client_Counter;

// Trie for storing the paths from all Storage Servers
extern TrieNode* MountPaths;
// Mutex for MountPaths
extern pthread_mutex_t MountPaths_mutex;

// LRU cache for paths lookups
extern LRUCache* Cache;
// Mutex for Cache
extern pthread_mutex_t Cache_mutex;

pthread_t* GetServerThread();
pthread_t* FreeThread();
CLIENT_RESPONSE* ProcessRequest(CLIENT_REQUEST* request);
double TimeStamp();

void* Client_Acceptor_Thread(void* arg);
void* Client_Handler_Thread(void* arg);

void* Server_Acceptor_Thread(void* arg);
void* Server_Handler_Thread(void* arg);


#endif
