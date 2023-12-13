#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>
#include <fcntl.h>
#include <math.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <dirent.h>
#include <semaphore.h>
#include "./naming_server/color.h"

#define NS_PORT 8081                  //Naming Server with Client                                                                 // Port where CLients connect
#define SS_NS 8080
#define IP_ADDRESS "127.0.0.1"                                                                  // IP address of the host
#define MAX_BUFFER 4096                                                                       //  maximunm size of any string unless defined specifically
#define LRU_CAPACITY 3
#define MAX_CLIENT 10
#define CHUNK_SIZE 3072

typedef struct Packet{
    char message[MAX_BUFFER+1];                                                                 // string to store the message
    int file_struct;                                                                            // defined to check whether the message is transmitted from storage server to the naming server which contain the file structure of that storage structure
    int type_of_message;                                                                        //0 for data, 1 for ack , 2 for ERROR, 3 for LRU.
    int error_code;                                                                               // -1 if no error
    int command_code;                                                                           // -1 for no command , 0 for read , 1 for write , 2 for get , 3 for create , 4 for delete , 5 for copy 
    int size_message;                                                                           // size of the message string                           
}Packet;

typedef struct SS_init{
    char access_paths[MAX_BUFFER];
    char ss_directory[MAX_BUFFER];
    char ip[100];
    int port_client;
    int port_nm;
}SS_init;

typedef struct {
    int CMD;
    int FLAGS;    
    char path[MAX_BUFFER];
    char Request_Data[MAX_BUFFER];
    int done;
}REQUEST;
// client -> Naming Server
// Server request structure
// Storage server sends this to naming server
typedef struct {
    int CMD;
    int FLAGS;    
    char path[MAX_BUFFER];
    int done;
    int ClientID;
}SERVER_REQUEST;

typedef struct {
    int FLAGS;  
    int CMD;    
    char Storage_IP[MAX_BUFFER];
    int Storage_Port;
    int Error_Code;
    char Response_Data[MAX_BUFFER];
    int done;
}RESPONSE;
 // Naming Server -> client

typedef struct {
    char feedback[100];
    int err_code;
    int cmd;
}ACK;

struct LLnode
{
    char key[MAX_BUFFER];
    char data[MAX_BUFFER];
    struct LLnode *next;
    struct LLnode *prev;
};   

typedef struct LLnode* LL;
struct LRU
{   //LRU_CAPACITY defined in header.h
    LL head;
    LL tail;
    int num_nodes;
};

typedef struct LRU* LRU;
typedef struct Blank{
    pthread_mutex_t read;
    pthread_mutex_t write;
    int readers;
}Blank;

typedef struct trie{
    char string[100];
    struct trie* next;
    struct trie* child;
    Blank* b;
}Trie;

typedef struct FUN{
    int* sockfd;
    SERVER_REQUEST req;
}FUN;

// Client ACK structure
// Client sends this to naming server to acknowledge the response
typedef struct {
    int Error_Code;
    int done;
}CLIENT_ACK;

// Server response structure
// Naming server sends this to storage server
typedef struct{
    int FLAGS;  
    int CMD;    
    char Storage_IP[MAX_BUFFER];
    int Storage_Port;
    int Error_Code;
    char Response_Data[MAX_BUFFER];
    int done;
    int ClientID;
}SERVER_RESPONSE;
