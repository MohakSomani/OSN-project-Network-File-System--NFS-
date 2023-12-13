#ifndef __EXTERNAL_H__
#define __EXTERNAL_H__

#define MAX_BUFFER 4096

// External API Codes
#define READ 1
#define WRITE 2
#define INFO 3
#define CREATE 4
#define DELETE 5
#define COPY 6 
#define LIST 8

// Server Forward Codes
#define PRIV_CMD_ACK 1
#define COPY_TOK_FWD 3
#define COPY_EOT 69
#define COPY_TOK_CONFIRM 4
#define BACKUP_CMD_ACK 420
#define CREATE_EOT 68
#define BACKUP_ACK 96

//Storage Server initialization structure
typedef struct{
    char access_paths[MAX_BUFFER]; 
    char ss_directory[MAX_BUFFER]; //redundant
    char ip[100];
    int port_client;
    int port_nm;
}SS_init;

//Client response structure
// Naming server sends this to client
typedef struct{
    int FLAGS;  
    int CMD;    
    char Storage_IP[MAX_BUFFER];
    int Storage_Port;
    int Error_Code;
    char Response_Data[MAX_BUFFER];
    int done;
}CLIENT_RESPONSE;

//Client request structure
// Client sends this to naming server
typedef struct {
    int CMD;
    int FLAGS;    
    char path[MAX_BUFFER];
    char Request_Data[MAX_BUFFER];
    int done;
}CLIENT_REQUEST;


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

// Server request structure
// Storage server sends this to naming server
typedef struct {
    int CMD;
    int FLAGS;    
    char path[MAX_BUFFER];
    int done;
    int ClientID;
}SERVER_REQUEST;

#endif