```C
//Client Side
typedef struct {
    int CMD;
    int FLAGS;    

}REQUEST;
// client -> Naming Server

typedef struct {
    int FLAGS;  // 1st bit LRU Hit, 2nd 
    int CMD;    
    int Storage_IP;
    int Storage_Port;
    int Error_Code;
    char Response_Data[4096];
}RESPONSE;
 // Naming Server -> client

```

```c
//Storage Server
typedef struct SS_init{
    char access_paths[MAX_BUFFER]; 
    char ss_directory[MAX_BUFFER];
    char ip[100];
    int port_client;
    int port_nm;
}SS_init;
```

```c
//LRU API Calls

LRUInit();

LRUGet();

LRUPut();
```