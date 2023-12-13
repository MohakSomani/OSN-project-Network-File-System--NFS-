#include "NS_header.h"

// Global Variables
pthread_t server_thread_pool[MAX_SERVER_THREADS];
Server_Handle Server_Handle_Array[MAX_SERVER_THREADS];
pthread_mutex_t server_thread_pool_mutex;
int Server_index;
int Server_Counter;

Client_Handle *Client_List;
pthread_mutex_t Client_List_mutex;
int Client_Counter;

TrieNode *MountPaths;
pthread_mutex_t MountPaths_mutex;

LRUCache *Cache;
pthread_mutex_t Cache_mutex;

FILE *BookKeeping;
pthread_mutex_t Bookkeeping_mutex; // for multiple atommic logs
struct timespec boot_time;

double TimeStamp()
{
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    // Calculate the time difference in seconds
    double elapsed_time = (current_time.tv_sec - boot_time.tv_sec) +
                          (current_time.tv_nsec - boot_time.tv_nsec) * 1e-9;

    return elapsed_time;
}

pthread_t *Get_Server_Thread()
{
    pthread_mutex_lock(&server_thread_pool_mutex);
    // find for a free thread
    int i;
    for (i = 0; i < MAX_SERVER_THREADS; i++)
    {
        if (pthread_equal(server_thread_pool[i], pthread_self()))
        {
            pthread_mutex_unlock(&server_thread_pool_mutex);
            printf(RED "[-]Get_Server_Thread: Thread %ld already in use.\n" CRESET, pthread_self());
            fprintf(BookKeeping, "[-]Get_Server_Thread: Thread %ld already in use (TIME-STAMP: %lf)\n", pthread_self(), TimeStamp());
            return NULL;
        }
        else if (pthread_equal(server_thread_pool[i], 0))
        {
            pthread_mutex_unlock(&server_thread_pool_mutex);
            Server_index = i;
            return &server_thread_pool[i];
        }
    }
    pthread_mutex_unlock(&server_thread_pool_mutex);
    printf(RED "[-]Get_Server_Thread: No Free Threads for Server assingment\n" CRESET);
    fprintf(BookKeeping, "[-]Get_Server_Thread: No Free Threads for Server assingment (TIME-STAMP: %lf)\n", TimeStamp());
    return NULL;
}

void Free_Server_Thread()
{
    pthread_mutex_lock(&server_thread_pool_mutex);

    // find the thread to be freed
    int i;
    for (i = 0; i < MAX_SERVER_THREADS; i++)
    {
        if (pthread_equal(server_thread_pool[i], pthread_self()))
        {
            printf(GRN "[+]Free_Server_Thread: Thread %ld freed.\n" CRESET, pthread_self());
            fprintf(BookKeeping, "[+]Free_Server_Thread: Thread %ld freed (TIME-STAMP: %lf)\n", pthread_self(), TimeStamp());
            server_thread_pool[i] = 0;
            Server_Counter--;
            pthread_mutex_unlock(&server_thread_pool_mutex);
            return;
        }
    }

    pthread_mutex_unlock(&server_thread_pool_mutex);
    printf(RED "[+]Free_Server_Thread: Thread %ld Not In Thread Pool .\n" CRESET, pthread_self());
    fprintf(BookKeeping, "[+]Free_Server_Thread: Thread %ld Not Found (TIME-STAMP: %lf)\n", pthread_self(), TimeStamp());
}

Server_Handle *Get_Random_Server()
{
    srand(time(NULL));
    int random_index = rand() % MAX_SERVER_THREADS;
    while (pthread_equal(server_thread_pool[random_index], 0) || (!pthread_equal(pthread_self(), server_thread_pool[random_index]))) // spin until a hit
        random_index = rand() % MAX_SERVER_THREADS;

    // printf(GRN"[+]Get_Random_Server: Server found at index %d\n"CRESET, random_index);
    // fprintf(BookKeeping, "[+]Get_Random_Server: Server found at index %d (TIME-STAMP: %lf)\n", random_index,TimeStamp());
    return &Server_Handle_Array[random_index];
}

int IncClientACK(int Client_ID)
{
    Client_Handle *Client_Handle_ptr = Client_List;
    while (Client_Handle_ptr != NULL)
    {
        if (Client_Handle_ptr->Client_ID == Client_ID)
        {
            Client_Handle_ptr->ACK_flag++;
            printf(BLU "[+]IncClientACK: ACK flag incremented for client %d\n" CRESET, Client_ID);
            return Client_Handle_ptr->ACK_flag;
        }
        Client_Handle_ptr = Client_Handle_ptr->next;
    }
    printf(RED "[-]IncClientACK: Client %d not found\n" CRESET, Client_ID);
    return 69;
}

int DecClientACK(int Client_ID)
{
    pthread_mutex_lock(&Client_List_mutex);
    Client_Handle *Client_Handle_ptr = Client_List;
    while (Client_Handle_ptr != NULL)
    {
        if (Client_Handle_ptr->Client_ID == Client_ID)
        {
            Client_Handle_ptr->ACK_flag--;
            printf(BLU "[+]DecClientACK: ACK flag decremented for client %d\n" CRESET, Client_ID);
            pthread_mutex_unlock(&Client_List_mutex);
            return Client_Handle_ptr->ACK_flag;
        }
        Client_Handle_ptr = Client_Handle_ptr->next;
    }
    pthread_mutex_unlock(&Client_List_mutex);
    printf(RED "[-]DecClientACK: Client %d not found\n" CRESET, Client_ID);
    return 69;
}

void *Server_Acceptor_Thread(void *arg)
{
    printf(UGRN "[+]Server_Acceptor_Thread started.\n" CRESET);
    fprintf(BookKeeping, "[+]Server_Acceptor_Thread started (TIME-STAMP: %lf)\n", TimeStamp());

    // printf("[+]Server_Acceptor_Thread waiting for connection.\n");
    int port = GLOBAL_SERVER_PORT;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    CheckError(sockfd, "socket()");

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    CheckError(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)), "bind()");
    // printf("[+]Server_Acceptor_Thread binded to port %d\n",port);

    // set socket options to reusable to avoid bind error while restarting server
    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    CheckError(listen(sockfd, MAX_CONNECTION_Q), "listen()");
    printf(WHT "[+]Server_Acceptor_Thread waiting for connection on port %d\n" CRESET, port);
    fprintf(BookKeeping, "[+]Server_Acceptor_Thread waiting for connection on port %d (TIME-STAMP: %lf)\n", port, TimeStamp());

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd;
    while (client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len))
    {
        CheckError(client_sockfd, "accept()");
        printf(YEL "[+]Server_Acceptor_Thread accepted connection from %s:%d\n" CRESET, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        fprintf(BookKeeping, "[+]Server_Acceptor_Thread accepted connection from %s:%d (TIME-STAMP: %lf)\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), TimeStamp());

        pthread_t *thread = Get_Server_Thread();
        if (thread == NULL)
        {
            printf(RED "[-]Server_Acceptor_Thread: GetThread failed.\n" CRESET);
            fprintf(BookKeeping, "[-]Server_Acceptor_Thread: GetThread failed (TIME-STAMP: %lf)\n", TimeStamp());
            CheckError(close(client_sockfd), "close()");
            sleep(SLEEP_TIME);
            continue;
        }

        // Populate Handle
        Server_Handle_Array[Server_index].Server_ID = Server_index;
        Server_Handle_Array[Server_index].SS_sockfd = client_sockfd;
        sem_init(&Server_Handle_Array[Server_index].req, 0, MAX_SERVER_REQUESTS);

        // int* client_sockfd_ptr = (int*)malloc(sizeof(int));
        // *client_sockfd_ptr = client_sockfd;

        CheckError(pthread_create(thread, NULL, Server_Handler_Thread, (void *)&Server_Handle_Array[Server_index]), "pthread_create()");

        printf(BYEL "[+]Server_Acceptor_Thread created thread %ld\n" CRESET, *thread);
        fprintf(BookKeeping, "[+]Server_Acceptor_Thread created thread %ld (TIME-STAMP: %lf)\n", *thread, TimeStamp());
    }

    CheckError(client_sockfd, "accept()");

    return NULL;
}

void *Server_Handler_Thread(void *arg) // thread for listening to a server
{
    // int client_sockfd = *(int*)arg;
    // free(arg);
    Server_Handle *Server_Handle_ptr = (Server_Handle *)arg;
    printf(BLU "[+]Server_Handler_Thread started for Server ID: %d.\n" CRESET, Server_Handle_ptr->Server_ID);
    fprintf(BookKeeping, "[+]Server_Handler_Thread started for Server ID: %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());

    // Unpack socket fd from Server_Handle
    int client_sockfd = Server_Handle_ptr->SS_sockfd;

    SS_init *Init_Packet = (SS_init *)malloc(sizeof(SS_init));
    int bytes = recv(client_sockfd, Init_Packet, sizeof(SS_init), 0);
    if (bytes == 0)
    {
        printf(REDB "[-]Server_Handler_Thread: server %d closed connection unexpectedly.\n" CRESET, Server_Handle_ptr->Server_ID);
        fprintf(BookKeeping, "[-]Server_Handler_Thread: server %d closed connection unexpectedly (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());
        CheckError(close(client_sockfd), "close()");
        fflush(stdout);
        return NULL;
    }
    else if (bytes < 0)
    {
        printf(RED "[-]Server_Handler_Thread: recv() failed.\n" CRESET);
        fprintf(BookKeeping, "[-]Server_Handler_Thread: recv() failed (TIME-STAMP: %lf)\n", TimeStamp());
        CheckError(close(client_sockfd), "close()");
        fflush(stdout);
        return NULL;
    }

    // unpack information from Init_Packet
    Server_Handle_ptr->Client_Port = Init_Packet->port_client;
    Server_Handle_ptr->Client_IP = Init_Packet->ip;
    Server_Handle_ptr->Naming_Port = Init_Packet->port_nm;
    Server_Handle_ptr->Naming_IP = Init_Packet->ip;
    Server_Handle_ptr->Server_BackUp = 0;

    printf("->Client Port Provided: %d\n", Init_Packet->port_client); // debug
    printf("->Client IP Provided: %s\n", Init_Packet->ip);            // debug
    printf("->Naming Port Provided: %d\n", Init_Packet->port_nm);     // debug
    printf("->Naming IP Provided: %s\n", Init_Packet->ip);            // debug

    // insert list of all files in the server onto the mount point
    // printf("%s\n", Init_Packet->access_paths); //debug
    strcpy(Server_Handle_ptr->paths, Init_Packet->access_paths);
    char *access_paths_copy = strdup(Init_Packet->access_paths);
    char *path = strtok_r(access_paths_copy, "\n", &access_paths_copy);
    while (path != NULL)
    {
        // printf("%s\n", path); //debug
        path = path + 2; // remove the first 2 characters './' (dot-slash)
        pthread_mutex_lock(&MountPaths_mutex);
        Insert_Path(MountPaths, path, Server_Handle_ptr);
        pthread_mutex_unlock(&MountPaths_mutex);
        path = strtok_r(access_paths_copy, "\n", &access_paths_copy);
    }
    // free(access_paths_copy);

    printf(BWHT "[+]Server_Handler_Thread: Server Handle Initiated: %p\n" CRESET, Server_Handle_ptr);
    printf(BGRN "[+]Storage_Server %d mounted \n" CRESET, Server_Handle_ptr->Server_ID);
    fprintf(BookKeeping, "[+]Storage_Server %d mounted (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());

    printf(CYN);                          // debug
    printf("Current Mount Directory:\n"); // debug
    Print_Trie(MountPaths, 0);            // debug
    printf(CRESET);                       // debug

    // set up listner for server
    int lsockfd;
    CheckError((lsockfd = socket(AF_INET, SOCK_STREAM, 0)), "socket()");
    struct sockaddr_in lserver_addr;
    lserver_addr.sin_family = AF_INET;
    lserver_addr.sin_port = htons(Server_Handle_ptr->Naming_Port);
    lserver_addr.sin_addr.s_addr = inet_addr(Server_Handle_ptr->Naming_IP);

    CheckError(connect(lsockfd, (struct sockaddr *)&lserver_addr, sizeof(lserver_addr)), "connect()");
    printf(BGRN "[+]Server_Handler_Thread: Connected to server %d for listening\n" CRESET, Server_Handle_ptr->Server_ID);
    fprintf(BookKeeping, "[+]Server_Handler_Thread: Connected to server %d for listening (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());

    Server_Handle_ptr->ssl = lsockfd;

    pthread_mutex_lock(&server_thread_pool_mutex);
    Server_Counter++;
    pthread_mutex_unlock(&server_thread_pool_mutex);
    // BACK UP
    if (Server_Counter < BACKUP_SERVERS + 1)
    {
        printf("Waiting for More Servers to come online\n");
        while (Server_Counter < BACKUP_SERVERS + 1)
            sleep(SLEEP_TIME);
    }
    printf("Servers Online\n");

    // assign backup server
    printf("Assigning Backup Servers\n");

    pthread_mutex_lock(&server_thread_pool_mutex);
    int BackUpCount = 0;
    while (BackUpCount < BACKUP_SERVERS)
    {
        // Server with curent min load
        Server_Handle *MIN = &Server_Handle_Array[0];
        for (int i = 1; i < Server_index + 1; i++)
        {
            Server_Handle *SERVER_HANDLE_PTR = &Server_Handle_Array[i];
            if((SERVER_HANDLE_PTR->Server_ID == Server_Handle_ptr->Server_ID)||(SERVER_HANDLE_PTR->Server_Down))
                continue;
            else if(BackUpCount>0 && SERVER_HANDLE_PTR->BackUp[BackUpCount-1]->Server_ID == Server_Handle_ptr->Server_ID)
                continue;
            if (SERVER_HANDLE_PTR->Server_BackUp <= MIN->Server_BackUp)
                MIN = SERVER_HANDLE_PTR;
        }
        Server_Handle_ptr->BackUp[BackUpCount++] = MIN;
        MIN->Server_BackUp++;
    }
    pthread_mutex_unlock(&server_thread_pool_mutex);

    // Create Backup Directories
    for (int i = 0; i < BACKUP_SERVERS; i++)
    {
        int sockfd = Server_Handle_ptr->BackUp[i]->SS_sockfd;

        SERVER_REQUEST *request = (SERVER_REQUEST *)malloc(sizeof(SERVER_REQUEST));
        request->CMD = CREATE;
        request->FLAGS = 0; // directory
        request->ClientID = -1;
        request->done = 3;
        snprintf(request->path, MAX_BUFFER, "./_backup_%d_%d", Server_Handle_ptr->Server_ID, i + 1);

        while (send(sockfd, request, sizeof(SERVER_REQUEST), 0) != sizeof(SERVER_REQUEST))
        {
            free(request);
            printf(BRED "[-]Server_Handler_Thread: send() failed for Server %d\n" CRESET, Server_Handle_ptr->BackUp[i]->Server_ID);
            fprintf(BookKeeping, "[-]Server_Handler_Thread: send() failed for Server %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->BackUp[i]->Server_ID, TimeStamp());
            // CheckError(close(sockfd), "close()");
            printf("Trying again..");
            sleep(SLEEP_TIME);
        }

        int backl=Server_Handle_ptr->BackUp[i]->ssl;
        SERVER_RESPONSE *response = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
        if ((recv(backl, response, sizeof(SERVER_RESPONSE), 0) == 0))
        {
            printf(REDB "[-]Server_Handler_Thread: server %d closed connection unexpectedly.\n" CRESET, Server_Handle_ptr->BackUp[i]->Server_ID);
            fprintf(BookKeeping, "[-]Server_Handler_Thread: server %d closed connection unexpectedly (TIME-STAMP: %lf)\n", Server_Handle_ptr->BackUp[i]->Server_ID, TimeStamp());
            // CheckError(close(client_sockfd), "close()");

            free(response);

            printf(YEL "Redirecting to Backups\n" CRESET);
            fprintf(BookKeeping, YEL "Redirecting to Backups (TIME-STAMP: %lf)\n" CRESET, TimeStamp());

            // redirect to backups
            Server_Handle_ptr->Server_Down = 1;
            return NULL;
        }
        // print response
        printf(BMAG);
        printf("Response received from server %d\n", Server_Handle_ptr->BackUp[i]->Server_ID);
        printf("Response Cmd: %d\n", response->CMD);
        printf("Response Flags: %d\n", response->FLAGS);
        printf("Response Fwd Flag: %d\n", response->done);
        printf("Response Error Code: %d\n", response->Error_Code);
        printf("Response Data: %s\n", response->Response_Data);
        printf(CRESET);
        // Update Mount Point
        char* path=request->path;
        strtok_r(path, "/", &path);                         // extract first token into path for resolution
        Insert_Path(MountPaths, path, Server_Handle_ptr->BackUp[i]);
        free(request);
            // if (response->Error_Code == SUCCESS && (response->CMD == CREATE || response->CMD == WRITE))
            // {
            //     // Insert path into trie
            //     char *path = response->Response_Data;
            //     strtok_r(path, "/", &path);                         // extract first token into path for resolution
            //     printf("Inserting Path \"%s\" into mount\n", path); // debug
            //     pthread_mutex_lock(&MountPaths_mutex);
            //     if (Insert_Path(MountPaths, " ", Server_Handle_ptr) == -1)
            //         printf("Insert Invalid Arg\n");
            //     pthread_mutex_unlock(&MountPaths_mutex);
            // }
            free(response);
    }

    // sleep(1); // Sync Requests
    // Copy Information from Server to Backup Servers
    for (int i = 0; i < BACKUP_SERVERS; i++)
    {
        int sockfd = Server_Handle_ptr->BackUp[i]->SS_sockfd;

        SERVER_REQUEST *request = (SERVER_REQUEST *)malloc(sizeof(SERVER_REQUEST));

        request->CMD = COPY;
        request->FLAGS = 0; // directory
        request->ClientID = -1;
        request->done = 3;
        snprintf(request->path, MAX_BUFFER, "./ ./_backup_%d_%d", Server_Handle_ptr->Server_ID, i + 1);

        if (send(Server_Handle_ptr->SS_sockfd, request, sizeof(SERVER_REQUEST), 0) != sizeof(SERVER_REQUEST))
        {
            free(request);
            printf(BRED "[-]Server_Handler_Thread: send() failed for Server %d\n" CRESET, Server_Handle_ptr->Server_ID);
            fprintf(BookKeeping, "[-]Server_Handler_Thread: send() failed for Server %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());
            // CheckError(close(sockfd), "close()");
            return NULL;
        }

        free(request);
    }

    printf("[+]Server_Handle_Thread: Backup Servers Assigned for Server %d\n", Server_Handle_ptr->Server_ID);
    printf("Backup Servers: ");
    for (int i = 0; i < BACKUP_SERVERS; i++)
        printf("%d ", Server_Handle_ptr->BackUp[i]->Server_ID);

    printf("\n");
    fprintf(BookKeeping, "[+]Server_Handle_Thread: Backup Servers Assigned for Server %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());
    while (1) // listen to server
    {
        SERVER_RESPONSE *response = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
        if ((recv(lsockfd, response, sizeof(SERVER_RESPONSE), 0) == 0))
        {
            printf(REDB "[-]Server_Handler_Thread: server %d closed connection unexpectedly.\n" CRESET, Server_Handle_ptr->Server_ID);
            fprintf(BookKeeping, "[-]Server_Handler_Thread: server %d closed connection unexpectedly (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());
            // CheckError(close(client_sockfd), "close()");

            free(response);

            printf(YEL "Redirecting to Backups\n" CRESET);
            fprintf(BookKeeping, YEL "Redirecting to Backups (TIME-STAMP: %lf)\n" CRESET, TimeStamp());

            // redirect to backups
            Server_Handle_ptr->Server_Down = 1;
            return NULL;
        }

        printf(BMAG);
        printf("Response received from server %d\n", Server_Handle_ptr->Server_ID);
        printf("Response Cmd: %d\n", response->CMD);
        printf("Response Flags: %d\n", response->FLAGS);
        printf("Response Fwd Flag: %d\n", response->done);
        printf("Response Error Code: %d\n", response->Error_Code);
        printf("Response Data: %s\n", response->Response_Data);
        printf(CRESET);

        pthread_mutex_lock(&Bookkeeping_mutex);
        fprintf(BookKeeping, "Response received from server %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());
        fprintf(BookKeeping, "Response Cmd: %d\n", response->CMD);
        fprintf(BookKeeping, "Response Flags: %d\n", response->FLAGS);
        fprintf(BookKeeping, "Response Fwd Flag: %d\n", response->done);
        fprintf(BookKeeping, "Response Error Code: %d\n", response->Error_Code);
        fprintf(BookKeeping, "Response Data: %s\n", response->Response_Data);
        pthread_mutex_unlock(&Bookkeeping_mutex);

        // detect server disconnection requests
        if (response->done == 0 && response->FLAGS == -1 && response->Error_Code == -1)
        {
            free(response);
            break;
        }

        // handle valid responses from server
        int fwd_flag = response->done;
        int OP = response->CMD;

        switch (fwd_flag)
        {
        case PRIV_CMD_ACK: // Fwd Response Packet to client for privilaged cmd
        {
            sem_post(&Server_Handle_ptr->req);
            printf(BYEL "Recieved Sever Response packet from %d Forwarding response packet to client %d\n" CRESET, Server_Handle_ptr->Server_ID, response->ClientID);
            fprintf(BookKeeping, "Recieved Sever Response packet from %d Forwarding response packet to client %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, response->ClientID, TimeStamp());

            // Extract path from response

            char *path = response->Response_Data;
            strtok_r(path, "/", &path);          // extract first token into path for resolution
            if (response->Error_Code == SUCCESS) // Update mount point
            {
                printf("Server %d returned success code for client %d on Operation %d\n", Server_Handle_ptr->Server_ID, response->ClientID, response->CMD);
                fprintf(BookKeeping, "Server %d returned success code for client %d on Operation %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, response->ClientID, response->CMD, TimeStamp());

                printf(BWHT "Updating mount point\n" CRESET);
                if (response->CMD == CREATE) // Create response
                {
                    // Insert path into mount point
                    printf("Inserting Path \"%s\" into mount\n", path); // debug
                    pthread_mutex_lock(&MountPaths_mutex);
                    if (Insert_Path(MountPaths, path, Server_Handle_ptr) == -1)
                        printf("Insert Invalid Arg\n");
                    pthread_mutex_unlock(&MountPaths_mutex);
                }
                else if (response->CMD == DELETE) // Delete response
                {
                    // Delete path from mount point
                    printf("Deleting Path \"%s\" into mount\n", path); // debug
                    pthread_mutex_lock(&MountPaths_mutex);
                    if (Delete_Path(MountPaths, path) == -1)
                        printf("Invalid Path Delete\n");
                    pthread_mutex_unlock(&MountPaths_mutex);

                    pthread_mutex_lock(&Cache_mutex);
                    // flush the Cache to prevent stale data
                    flushCache(Cache);
                    pthread_mutex_unlock(&Cache_mutex);
                }
                printf(BWHT "Mount point updated\n" CRESET);
                printf(CYN);                          // debug
                printf("Current Mount Directory:\n"); // debug
                Print_Trie(MountPaths, 0);            // debug
                printf(CRESET);                       // debug
            }
            else
            {
                printf(RED "Server %d returned error code %d for client %d on Operation %d\n" CRESET, Server_Handle_ptr->Server_ID, response->Error_Code, response->ClientID, response->CMD);
                fprintf(BookKeeping, "Server %d returned error code %d for client %d on Operation %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, response->Error_Code, response->ClientID, response->CMD, TimeStamp());
            }
            printf("Forwarding response to client %d\n", response->ClientID); // debug
            int client_sockfd = response->ClientID;

            CLIENT_RESPONSE *client_response = (CLIENT_RESPONSE *)malloc(sizeof(CLIENT_RESPONSE));
            client_response->CMD = response->CMD;
            client_response->FLAGS = response->FLAGS; // 1 for file 0 for directory
            client_response->Error_Code = response->Error_Code;
            strcpy(client_response->Response_Data, "Operation Successfull\n");
            client_response->done = 1;

            free(response);
            if (send(client_sockfd, client_response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
            {
                free(client_response);
                printf(BRED "[-]Server_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                fprintf(BookKeeping, "[-]Server_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                CheckError(close(client_sockfd), "close()");
            }
            free(client_response);
            break;
        }
        case COPY_TOK_FWD: //  Copy packet token from SS; fwd to Corresponding SS
        {
            sem_post(&Server_Handle_ptr->req);
            // do a path resolution
            char *path;
            SERVER_REQUEST *request = (SERVER_REQUEST *)malloc(sizeof(SERVER_REQUEST));
            char *req_path;
            path = strtok_r(response->Response_Data, "\n", &req_path); // extract first token into path for resolution
            strcpy(request->path, req_path);

            Server_Handle *Target_Server_ptr;

            if ((Target_Server_ptr = (Server_Handle *)get(Cache, path)))
            {
                // printf("Cache hit\n"); //debug
            }
            else
            {
                // printf("Cache Miss\n"); //debug
                Target_Server_ptr = Get_Server(MountPaths, path);
                if (Target_Server_ptr == NULL)
                {
                    printf("Path not found\n"); // debug
                    // discard the packet
                    free(response);
                    free(request);
                    printf(BYEL "Recieved Invalid Sever Response packet (Server Forward Request) from %d (Discarding Packet)\n" CRESET, Server_Handle_ptr->Server_ID);
                    fprintf(BookKeeping, "Recieved Invalid Sever Response packet (Server Forward Request) from %d (Discarding Packet) (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());
                    break;
                }
            }

            int Target_SS_sockfd = Target_Server_ptr->SS_sockfd;
            printf(BYEL "Recieved Sever Response packet from %d Forwarding response packet to Server %d\n" CRESET, Server_Handle_ptr->Server_ID, Target_Server_ptr->Server_ID);
            fprintf(BookKeeping, "Recieved Sever Response packet from %d Forwarding response packet to Server %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, Target_Server_ptr->Server_ID, TimeStamp());

            request->CMD = response->CMD;
            request->FLAGS = response->FLAGS;
            request->done = 3;
            request->ClientID = response->ClientID;

            free(response);
            // send request to target server
            sem_wait(&Target_Server_ptr->req);
            if (send(Target_SS_sockfd, request, sizeof(SERVER_REQUEST), 0) != sizeof(SERVER_REQUEST))
            {
                free(request);
                printf(BRED "[-]Server_Handler_Thread: send() failed for Server %d\n" CRESET, Server_Handle_ptr->Server_ID);
                fprintf(BookKeeping, "[-]Server_Handler_Thread: send() failed for Server %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());
                CheckError(close(Target_SS_sockfd), "close()");
                break;
            }

            IncClientACK(request->ClientID);

            free(request);
            break;
        }
        case COPY_EOT: // Copy Token EOT packet
        {
            sem_post(&Server_Handle_ptr->req);
            printf(UCYN "END of Copy Token Transmission Signaled from Server %d\n" CRESET, Server_Handle_ptr->Server_ID);
            fprintf(BookKeeping, "END of Copy Token Transmission Signaled from Server %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());

            if (DecClientACK(response->ClientID) == -1)
            {
                IncClientACK(response->ClientID);
                // send a copy confirmation to the client
                int client_sockfd = response->ClientID;
                CLIENT_RESPONSE *client_response = (CLIENT_RESPONSE *)malloc(sizeof(CLIENT_RESPONSE));
                client_response->CMD = COPY;
                client_response->FLAGS = 0;
                client_response->Error_Code = SUCCESS;
                strcpy(client_response->Response_Data, "Copy Operation Successfull\n");
                client_response->done = 1;

                if (send(client_sockfd, client_response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                {
                    free(client_response);
                    printf(BRED "[-]Server_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                    fprintf(BookKeeping, "[-]Server_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                    CheckError(close(client_sockfd), "close()");
                }
                free(client_response);

                printf(BYEL "Recieved Copy(token) Confirmation Sever Response packet from %d Forwarding response packet to client %d\n" CRESET, Server_Handle_ptr->Server_ID, response->ClientID);
            }
            break;
        }
        case COPY_TOK_CONFIRM: // Copy(token) Confirmation packet from SS
        {
            sem_post(&Server_Handle_ptr->req);
            printf(BHWHT "Recieved Copy(token) Confirmation Sever Response packet from %d\n" CRESET, Server_Handle_ptr->Server_ID);
            fprintf(BookKeeping, "Recieved Copy(token) Confirmation Sever Response packet from %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());

            // token identification
            printf(UYEL);
            printf("Token Type: %d\n", response->CMD);
            printf("Token Status: %d\n", response->Error_Code);
            printf(CRESET);

            fprintf(BookKeeping, "Token Type: %d\n", response->CMD);
            fprintf(BookKeeping, "Token Status: %d\n", response->Error_Code);

            // Update Mount Point
            if (response->Error_Code == SUCCESS && (response->CMD == CREATE || response->CMD == WRITE))
            {
                // Insert path into trie
                char *path = response->Response_Data;
                strtok_r(path, "/", &path);                         // extract first token into path for resolution
                printf("Inserting Path \"%s\" into mount\n", path); // debug
                pthread_mutex_lock(&MountPaths_mutex);
                if (Insert_Path(MountPaths, path, Server_Handle_ptr) == -1)
                    printf("Insert Invalid Arg\n");
                pthread_mutex_unlock(&MountPaths_mutex);
            }

            if (DecClientACK(response->ClientID) == -1)
            {
                IncClientACK(response->ClientID);
                // send a copy confirmation to the client
                int client_sockfd = response->ClientID;
                CLIENT_RESPONSE *client_response = (CLIENT_RESPONSE *)malloc(sizeof(CLIENT_RESPONSE));
                client_response->CMD = COPY;
                client_response->FLAGS = 0;
                client_response->Error_Code = SUCCESS;
                strcpy(client_response->Response_Data, "Copy Operation Successfull\n");
                client_response->done = 1;

                if (send(client_sockfd, client_response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                {
                    free(client_response);
                    printf(BRED "[-]Server_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                    fprintf(BookKeeping, "[-]Server_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                    CheckError(close(client_sockfd), "close()");
                }
                free(client_response);

                printf(BYEL "Recieved Copy(token) Confirmation Sever Response packet from %d Forwarding response packet to client %d\n" CRESET, Server_Handle_ptr->Server_ID, response->ClientID);
            }
            break;
        }
        case BACKUP_CMD_ACK: // Fwd Response Packet to client for backup cmd
        {
            sem_post(&Server_Handle_ptr->req);
            printf("Recieved Sever Response packet from %d(Backup Initiated)\n" CRESET, Server_Handle_ptr->Server_ID);
            fprintf(BookKeeping, "Recieved Sever Response packet from %d(Backup Initiated) (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());

            // token identification
            printf(UYEL);
            printf("Token Type: %d\n", response->CMD);
            printf("Token Status: %d\n", response->Error_Code);
            printf("Token Path: %s\n", response->Response_Data);
            printf(CRESET);

            char *path = response->Response_Data;
            strtok_r(path, "/", &path); // extract first token into path for resolution

            if (response->CMD == CREATE)
            {
                // Insert path into trie
                printf("Inserting Path \"%s\" into mount\n", path); // debug
                pthread_mutex_lock(&MountPaths_mutex);
                if (Insert_Path(MountPaths, path, Server_Handle_ptr) == -1)
                    printf("Insert Invalid Arg\n");
                pthread_mutex_unlock(&MountPaths_mutex);
            }

            break;
        }
        case CREATE_EOT:
        {
            sem_post(&Server_Handle_ptr->req);
            printf("All Corresponding Directories have been Created and primed for Writing.\n");
            fprintf(BookKeeping, "All Corresponding Directories have been Created and primed for Writing. (TIME-STAMP: %lf)\n", TimeStamp());
            sleep(1); // sleep for sync
            break;
        }
        case BACKUP_ACK:
        {
            sem_post(&Server_Handle_ptr->req);
            printf("Recieved Sever Response packet from %d(Backup Acknowledegment)\n" CRESET, Server_Handle_ptr->Server_ID);
            fprintf(BookKeeping, "Recieved Sever Response packet from %d(Backup Acknowledegment) (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());
            break;
        }
        default:
        {
            printf(BRED "[-]Server_Handler_Thread: Invalid Packet[Forward Flag Unrecognized] ACTION:DISCARDED \n" CRESET);
            fprintf(BookKeeping, "[-]Server_Handler_Thread: Invalid Packet[Forward Flag Unrecognized] ACTION:DISCARDED (TIME-STAMP: %lf)\n", TimeStamp());
            break;
        }
        }
    }

    // handle server disconnection
    /*
    // Free_Server_Handle(Server_Handle_ptr);
    memset(Server_Handle_ptr, 0, sizeof(Server_Handle));

    // free the thread slot for another server
    Free_Server_Thread(Server_Handle_ptr->Server_ID);

    printf(BHMAG"[+]Server_Handle_Thread: Server %d Disconnected."CRESET,Server_Handle_ptr->Server_ID);
    fprintf(BookKeeping, "[+]Server_Handle_Thread: Server %d Disconnected (TIME-STAMP: %lf)\n",Server_Handle_ptr->Server_ID,TimeStamp());
    return NULL;
    */

    printf(BBLU "EXIT request received from server %d\n" CRESET, Server_Handle_ptr->Server_ID);
    fprintf(BookKeeping, "EXIT request received from server %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());
    fflush(stdout); // debug

    // Terminate the server
    Free_Server_Thread(Server_Handle_ptr->Server_ID);
    close(client_sockfd);

    // remove server from mount point
    char *access_paths = Server_Handle_ptr->paths;
    path = strtok_r(access_paths, "\n", &access_paths);
    while (path != NULL)
    {
        // printf("%s\n", path); //debug
        path = path + 2; // remove the first 2 characters './' (dot-slash)
        pthread_mutex_lock(&MountPaths_mutex);
        Delete_Path(MountPaths, path);
        pthread_mutex_unlock(&MountPaths_mutex);
        path = strtok_r(access_paths, "\n", &access_paths);
    }

    printf("Server %d removed from mount point\n", Server_Handle_ptr->Server_ID);
    fprintf(BookKeeping, "Server %d removed from mount point (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());
    fflush(stdout); // debug

    memset(Server_Handle_ptr, 0, sizeof(Server_Handle));
    // flush the Cache to prevent stale data
    pthread_mutex_lock(&Cache_mutex);
    flushCache(Cache);
    pthread_mutex_unlock(&Cache_mutex);

    printf(HRED "[+]Server_Handle_Thread: Server %d Disconnected.\n" CRESET, Server_Handle_ptr->Server_ID);
    fprintf(BookKeeping, "[+]Server_Handle_Thread: Server %d Disconnected (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());

    fflush(stdout);

    return NULL;
}

void *Client_Acceptor_Thread(void *arg)
{
    printf(UGRN "[+]Client_Acceptor_Thread started.\n" CRESET);
    fprintf(BookKeeping, "[+]Client_Acceptor_Thread started (TIME-STAMP: %lf)\n", TimeStamp());

    // printf("[+]Client_Acceptor_Thread waiting for connection.\n");
    int port = GLOBAL_CLIENT_PORT;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    CheckError(sockfd, "socket()");

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    CheckError(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)), "bind()");
    // printf("[+]Client_Acceptor_Thread binded to port %d\n",port);

    // set socket options to reusable to avoid bind error while restarting server
    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    CheckError(listen(sockfd, MAX_CONNECTION_Q), "listen()");
    printf(WHT "[+]Client_Acceptor_Thread waiting for connection on port %d\n" CRESET, port);
    fprintf(BookKeeping, "[+]Client_Acceptor_Thread waiting for connection on port %d (TIME-STAMP: %lf)\n", port, TimeStamp());

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd;
    while (client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len))
    {
        CheckError(client_sockfd, "accept()");
        printf(YEL "[+]Client_Acceptor_Thread accepted connection from %s:%d\n" CRESET, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        fprintf(BookKeeping, "[+]Client_Acceptor_Thread accepted connection from %s:%d (TIME-STAMP: %lf)\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), TimeStamp());

        pthread_t thread;
        Client_Handle *client_sockfd_ptr = (Client_Handle *)malloc(sizeof(Client_Handle));

        // Initialize The Client Handle
        Client_Handle *Client_Handle_ptr = (Client_Handle *)malloc(sizeof(Client_Handle));
        Client_Handle_ptr->Client_ID = client_sockfd;
        Client_Handle_ptr->Client_sockfd = client_sockfd;
        Client_Handle_ptr->Client_IP = inet_ntoa(client_addr.sin_addr);
        Client_Handle_ptr->Client_Port = ntohs(client_addr.sin_port);
        Client_Handle_ptr->ACK_flag = 0;
        Client_Handle_ptr->client_thread = thread;

        pthread_mutex_lock(&Client_List_mutex);
        // Add Client to Client List
        Client_Handle_ptr->prev = NULL;
        Client_Handle_ptr->next = Client_List;
        if (Client_List != NULL)
            Client_List->prev = Client_Handle_ptr;
        Client_List = Client_Handle_ptr;
        Client_Counter++;
        pthread_mutex_unlock(&Client_List_mutex);

        pthread_create(&thread, NULL, Client_Handler_Thread, (void *)Client_Handle_ptr);
        printf(BYEL "[+]Client_Acceptor_Thread created thread %ld\n" CRESET, thread);
        fprintf(BookKeeping, "[+]Client_Acceptor_Thread created thread %ld (TIME-STAMP: %lf)\n", thread, TimeStamp());
    }

    CheckError(client_sockfd, "accept()");

    return NULL;
}

void *Client_Handler_Thread(void *arg)
{
    Client_Handle *Client_Handle_ptr = (Client_Handle *)arg;
    int client_sockfd = Client_Handle_ptr->Client_sockfd;

    int CLIENT_ID = client_sockfd; // each client has a unique socket fd
    printf(BLU "[+]Client_Handler_Thread started for Client ID: %d.\n" CRESET, CLIENT_ID);
    fprintf(BookKeeping, "[+]Client_Handler_Thread started for Client ID: %d (TIME-STAMP: %lf)\n", CLIENT_ID, TimeStamp());

    while (1) // Use IsSocketConnectedR() to check if client is still connected
    {
        /*
        printf("testing\n");
        sleep(4);
        if (IsSocketConnected(client_sockfd))
            printf("connection alive\n");
        else
        {
            printf("connection dead\n");
            break;
        }
        */
        CLIENT_REQUEST *request = (CLIENT_REQUEST *)malloc(sizeof(CLIENT_REQUEST));
        /*
        struct pollfd mypoll = { client_sockfd, POLLIN|POLLPRI };

        if(poll(&mypoll, 1, PING_TIMEOUT))
        {
            printf("Client %d sent a request\n",client_sockfd);
        }
        else
        {
            printf("Client %d did not send a request.\n",client_sockfd);
            printf("Listening again...\n");
            continue;
        }
        */

        ssize_t bytes_read = recv(client_sockfd, request, sizeof(CLIENT_REQUEST), 0);
        // printf("bytes_read: %ld\n", bytes_read); //debug

        if (bytes_read == 0)
            break;
        else if (bytes_read < 0)
        {
            printf(RED "[-]Client_Handler_Thread: recv() failed for Client %d.\n" CRESET, CLIENT_ID);
            perror("recv()");
            fprintf(BookKeeping, "[-]Client_Handler_Thread: recv() failed for Client %d (TIME-STAMP: %lf)\n", CLIENT_ID, TimeStamp());
            // CheckError(close(client_sockfd), "close()");
            return NULL;
        }

        printf(BMAG "Request received from client %d\n", client_sockfd);
        printf("Request Cmd: %d\n", request->CMD);
        printf("Request Path: %s\n", request->path);
        printf("Request Flags: %d\n" CRESET, request->FLAGS);

        pthread_mutex_lock(&Bookkeeping_mutex);
        fprintf(BookKeeping, "Request received from client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
        fprintf(BookKeeping, "Request Cmd: %d\n", request->CMD);
        fprintf(BookKeeping, "Request Path: %s\n", request->path);
        fprintf(BookKeeping, "Request Flags: %d\n", request->FLAGS);
        pthread_mutex_unlock(&Bookkeeping_mutex);

        // send an ACK to client to indicate request received
        // CLIENT_ACK* ack = (CLIENT_ACK*)malloc(sizeof(CLIENT_ACK));
        // ack->done = 1;
        // ack->Error_Code = SUCCESS;
        // if(send(client_sockfd, ack, sizeof(CLIENT_ACK), 0) != sizeof(CLIENT_ACK))
        // {
        //     printf(BRED"[-]Client_Handler_Thread: send() failed for Client %d\n"CRESET,client_sockfd);
        //     fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n",client_sockfd,TimeStamp());
        //     // CheckError(close(client_sockfd), "close()");
        //     return NULL;
        // }
        // free(ack);

        // process the request ( and generate response)
        // CLIENT_RESPONSE* response = ProcessRequest(request);

        int OP = request->CMD;
        CLIENT_RESPONSE *response = (CLIENT_RESPONSE *)malloc(sizeof(CLIENT_RESPONSE));
        memset(response, 0, sizeof(CLIENT_RESPONSE));

        response->CMD = OP;
        switch (OP)
        {
        // Handling all direct communication commands
        case READ:  // READ
        case WRITE: // WRITE
        case INFO:  // INFO
        {
            printf("Client requests a Direct OP\n"); // debug
            Server_Handle *Server_Handle_ptr;

            if ((Server_Handle_ptr = (Server_Handle *)get(Cache, request->path)))
            {
                printf("Cache hit\n"); // debug
            }
            else
            {
                printf("Cache Miss\n"); // debug
                Server_Handle_ptr = Get_Server(MountPaths, request->path);
                if (Server_Handle_ptr == NULL)
                {
                    printf("Path not found\n"); // debug
                    response->Error_Code = PATH_NOT_FOUND;

                    // send Response
                    if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                    {
                        printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                        fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                        CheckError(close(client_sockfd), "close()");
                        break;
                    }
                    free(response);
                    break;
                }
                put(Cache, request->path, (void *)Server_Handle_ptr);
                printCache(Cache);
            }
            free(request);

            response->Error_Code = SUCCESS;
            strcpy(response->Response_Data, "Direct OP Response Data");
            strcpy(response->Storage_IP, Server_Handle_ptr->Client_IP);
            response->Storage_Port = Server_Handle_ptr->Client_Port;

            // print response
            printf(BMAG);
            printf("Response generated for client %d \n", client_sockfd);
            printf("Response Cmd: %d\n", response->CMD);
            printf("Response Flags: %d\n", response->FLAGS);
            printf("Response Error Code: %d\n", response->Error_Code);
            printf("Response Data: %s\n", response->Response_Data);
            printf("Response Server IP: %s\n", response->Storage_IP);
            printf("Response Server Port: %d\n", response->Storage_Port);
            printf(CRESET);

            pthread_mutex_lock(&Bookkeeping_mutex);
            fprintf(BookKeeping, "Response generated for client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
            fprintf(BookKeeping, "Response Cmd: %d\n", response->CMD);
            fprintf(BookKeeping, "Response Flags: %d\n", response->FLAGS);
            fprintf(BookKeeping, "Response Error Code: %d\n", response->Error_Code);
            fprintf(BookKeeping, "Response Data: %s\n", response->Response_Data);
            fprintf(BookKeeping, "Response Server IP: %s\n", response->Storage_IP);
            fprintf(BookKeeping, "Response Server Port: %d\n", response->Storage_Port);
            pthread_mutex_unlock(&Bookkeeping_mutex);

            // send Response
            if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
            {
                printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                // CheckError(close(client_sockfd), "close()");
                return NULL;
            }
            free(response);

            // wait for ACK from client
            CLIENT_ACK *ack = (CLIENT_ACK *)malloc(sizeof(CLIENT_ACK));
            if (recv(client_sockfd, ack, sizeof(CLIENT_ACK), 0) != sizeof(CLIENT_ACK))
            {
                printf(BRED "[-]Client_Handler_Thread: recv() failed for Client %d\n" CRESET, CLIENT_ID);
                fprintf(BookKeeping, "[-]Client_Handler_Thread: recv() failed for Client %d (TIME-STAMP: %lf)\n", CLIENT_ID, TimeStamp());
                // CheckError(close(client_sockfd), "close()");
                return NULL;
            }
            else if (ack->done == 1)
            {
                printf(BGRN "[+]Client_Handler_Thread: Client %d ACK received set for CMD: %d with Error Code %d\n" CRESET, CLIENT_ID, OP, ack->Error_Code);
                fprintf(BookKeeping, "[+]Client_Handler_Thread: Client %d ACK received with Error Code %d (TIME-STAMP: %lf)\n", CLIENT_ID, ack->Error_Code, TimeStamp());
            }
            else
            {
                printf(BRED "[-]Client_Handler_Thread: Client %d ACK received not set for CMD: %d with Error Code %d\n" CRESET, CLIENT_ID, OP, ack->Error_Code);
                fprintf(BookKeeping, "[-]Client_Handler_Thread: Client %d ACK not set with Error Code %d (TIME-STAMP: %lf)\n", CLIENT_ID, ack->Error_Code, TimeStamp());
            }
            free(ack);
            break;
        }
        case LIST: // LIST
        {
            printf("Client requests a LIST OP\n"); // debug
            char *Directory_Tree;
            if ((Directory_Tree = Get_Directory_Tree(MountPaths, request->path))) // if path exists
            {
               if(request!=NULL)
               {
                free(request);
                request=NULL;
                }
                char *free_ptr = Directory_Tree;
                printf("Path found\n");                                      // debug
                printf(YEL "Directory Tree:\n %s\n" CRESET, Directory_Tree); // debug

                // break the directory tree into response packets
                while (strlen(Directory_Tree) > 0)
                {
                    CLIENT_RESPONSE *response = (CLIENT_RESPONSE *)malloc(sizeof(CLIENT_RESPONSE));
                    memset(response, 0, sizeof(CLIENT_RESPONSE));

                    response->CMD = OP;
                    response->Error_Code = TRNSFR_IN_PROGRESS;
                    strncpy(response->Response_Data, Directory_Tree, MAX_BUFFER - 1);
                    Directory_Tree = Directory_Tree + MAX_BUFFER - 1;

                    if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                    {
                        // free(free_ptr);
                        printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                        fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());

                        // send error response
                        CLIENT_RESPONSE *response = (CLIENT_RESPONSE *)malloc(sizeof(CLIENT_RESPONSE));
                        memset(response, 0, sizeof(CLIENT_RESPONSE));
                        response->CMD = OP;
                        response->Error_Code = TRNSFR_ERROR;
                        strcpy(response->Response_Data, "Incomplete Transfer");
                        send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0);
                        free(response);

                        CheckError(close(client_sockfd), "close()");
                        break;
                    }
                    free(response);
                }
                if(free_ptr!=NULL)
                free(free_ptr);
                printf("End Of Transmission\n"); // debug

                CLIENT_RESPONSE *response = (CLIENT_RESPONSE *)malloc(sizeof(CLIENT_RESPONSE));
                memset(response, 0, sizeof(CLIENT_RESPONSE));
                response->CMD = OP;
                response->Error_Code = SUCCESS;
                strcpy(response->Response_Data, "END");
                response->done = 1;

                printf(YEL "Sending Final ACK packet to Client for LIST CMD%d\n" CRESET, client_sockfd);
                fprintf(BookKeeping, "Sending Final ACK packet to Client %d for LIST CMD (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());

                if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                {
                    printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                    fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());

                    // send error response
                    CLIENT_RESPONSE *response = (CLIENT_RESPONSE *)malloc(sizeof(CLIENT_RESPONSE));
                    memset(response, 0, sizeof(CLIENT_RESPONSE));
                    response->CMD = OP;
                    response->Error_Code = TRNSFR_ERROR;
                    strcpy(response->Response_Data, "Incomplete Transfer");
                    send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0);

                    // CheckError(close(client_sockfd), "close()");
                    return NULL;
                }
                free(response);
            }
            else // if path does not exist
            {
                free(request);
                printf("Path not found\n"); // debug

                CLIENT_RESPONSE *response = (CLIENT_RESPONSE *)malloc(sizeof(CLIENT_RESPONSE));
                memset(response, 0, sizeof(CLIENT_RESPONSE));
                response->CMD = OP;
                response->Error_Code = PATH_NOT_FOUND;
                strcpy(response->Response_Data, "Error: Path not found in exposed directory tree");

                if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                {
                    free(response);
                    printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                    fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());

                    // send error response
                    CLIENT_RESPONSE *response = (CLIENT_RESPONSE *)malloc(sizeof(CLIENT_RESPONSE));
                    memset(response, 0, sizeof(CLIENT_RESPONSE));
                    response->CMD = OP;
                    response->Error_Code = TRNSFR_ERROR;
                    strcpy(response->Response_Data, "Incomplete Transfer");
                    send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0);
                    free(response);

                    CheckError(close(client_sockfd), "close()");
                    break;
                }
                free(response);
            }
            break;
        }
        case CREATE: // CREATE
        case DELETE: // DELETE
        {
            if ((strcmp(request->path, "./") == 0) || (strcmp(request->path, "mount/") == 0))
            {
                printf("Client requests a CREATE/DELETE OP on root directory\n"); // debug

                if (OP == 4)
                {
                    // Choose a random server to create the file
                    Server_Handle *Server_Handle_ptr = Get_Random_Server();
                    sem_wait(&Server_Handle_ptr->req);

                    SERVER_REQUEST *server_request = (SERVER_REQUEST *)malloc(sizeof(SERVER_REQUEST));
                    server_request->CMD = OP;
                    strcpy(server_request->path, request->path);
                    strcat(server_request->path, request->Request_Data); // for create operation, request data contains the file name provide complete path
                    server_request->FLAGS = request->FLAGS;
                    server_request->done = 0;
                    server_request->ClientID = CLIENT_ID;

                    free(request);
                    int sever_sockfd = Server_Handle_ptr->SS_sockfd;

                    printf("Forwarding request %d to server %d (Privilaged OP)[ROOT CREATE]\n", OP, Server_Handle_ptr->Server_ID);
                    fprintf(BookKeeping, "Forwarding request %d to server %d (Privilaged OP)[ROOT CREATE] (TIME-STAMP: %lf)\n", OP, Server_Handle_ptr->Server_ID, TimeStamp());

                    if (send(sever_sockfd, server_request, sizeof(SERVER_REQUEST), 0) != sizeof(SERVER_REQUEST))
                    {
                        free(server_request);
                        printf(BRED "[-]Client_Handler_Thread: error while forwarding request to server %d\n" CRESET, Server_Handle_ptr->Server_ID);
                        fprintf(BookKeeping, "[-]Client_Handler_Thread: error while forwarding request to server %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());

                        response->Error_Code = REQ_FWD_ERROR;
                        strcpy(response->Response_Data, "Error: Request could not be forwarded to server");
                        response->CMD = OP;
                        response->FLAGS = request->FLAGS;
                        response->done = 1;

                        if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                        {
                            printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                            fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                            // CheckError(close(client_sockfd), "close()");
                            return NULL;
                        }
                        break;
                    }
                    free(server_request);
                    break;
                }
                else
                {
                    // Send Error response
                    response->Error_Code = MOUNT_DEL;
                    strcpy(response->Response_Data, "Error: Deleting mount directory is not allowed");
                    response->CMD = OP;
                    response->FLAGS = request->FLAGS;
                    response->done = -1;

                    if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                    {
                        printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                        fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                        CheckError(close(client_sockfd), "close()");
                        return NULL;
                    }
                    free(response);
                    break;
                }
            }
            else
            {
                printf("Client requests a CREATE/DELETE OP \n"); // debug
                Server_Handle *Server_Handle_ptr;
                if ((Server_Handle_ptr = (Server_Handle *)get(Cache, request->path)))
                {
                    printf("Cache hit\n"); // debug
                }
                else
                {
                    printf("Cache Miss\n"); // debug
                    Server_Handle_ptr = Get_Server(MountPaths, request->path);
                    if (Server_Handle_ptr == NULL)
                    {
                        printf("Path not found\n"); // debug
                        response->Error_Code = PATH_NOT_FOUND;

                        // send Response
                        if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                        {
                            printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                            fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                            CheckError(close(client_sockfd), "close()");
                            break;
                        }
                        free(response);
                        break;
                    }
                    put(Cache, request->path, (void *)Server_Handle_ptr);
                    printCache(Cache);
                }

                sem_wait(&Server_Handle_ptr->req);

                SERVER_REQUEST *server_request = (SERVER_REQUEST *)malloc(sizeof(SERVER_REQUEST));
                server_request->CMD = OP;
                strcpy(server_request->path, request->path);
                if (OP == CREATE)
                {
                    strcat(server_request->path, "/");
                    strcat(server_request->path, request->Request_Data); // for create operation, request data contains the file name provide complete path
                }

                server_request->FLAGS = request->FLAGS;
                server_request->done = 0;
                server_request->ClientID = CLIENT_ID;

                int sever_sockfd = Server_Handle_ptr->SS_sockfd;

                free(request);
                printf("Forwarding request %d to server %d (Privilaged OP)\n", OP, Server_Handle_ptr->Server_ID);
                fprintf(BookKeeping, "Forwarding request %d to server %d (Privilaged OP) (TIME-STAMP: %lf)\n", OP, Server_Handle_ptr->Server_ID, TimeStamp());

                if (send(sever_sockfd, server_request, sizeof(SERVER_REQUEST), 0) != sizeof(SERVER_REQUEST))
                {
                    free(server_request);
                    printf(BRED "[-]Client_Handler_Thread: error while forwarding request to server %d\n" CRESET, Server_Handle_ptr->Server_ID);
                    fprintf(BookKeeping, "[-]Client_Handler_Thread: error while forwarding request to server %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());

                    response->Error_Code = REQ_FWD_ERROR;
                    strcpy(response->Response_Data, "Error: Request could not be forwarded to server");
                    response->CMD = OP;
                    response->FLAGS = request->FLAGS;
                    response->done = 1;

                    if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                    {
                        printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                        fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                        // CheckError(close(client_sockfd), "close()");
                        return NULL;
                    }
                    break;
                }
                free(server_request);
                break;
            }
        }
        case COPY: // COPY
        {
            printf("Client requests a COPY OP\n");
            Server_Handle *Server_Handle_ptr;

            if ((Server_Handle_ptr = (Server_Handle *)get(Cache, request->path)))
            {
                printf("Cache hit\n"); // debug
            }
            else
            {
                printf("Cache Miss\n"); // debug
                Server_Handle_ptr = Get_Server(MountPaths, request->path);
                if (Server_Handle_ptr == NULL)
                {
                    printf("Path not found\n"); // debug
                    response->Error_Code = PATH_NOT_FOUND;

                    // send Response
                    if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                    {
                        free(response);
                        printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                        fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                        // CheckError(close(client_sockfd), "close()");
                        return NULL;
                    }
                    free(response);
                    break;
                }
                put(Cache, request->path, (void *)Server_Handle_ptr);
                printCache(Cache); // debug
            }

            sem_wait(&Server_Handle_ptr->req);

            // free(request);
            SERVER_REQUEST *server_request = (SERVER_REQUEST *)malloc(sizeof(SERVER_REQUEST));
            server_request->CMD = OP;
            strcpy(server_request->path, request->path);
            strcat(server_request->path, " ");
            strcat(server_request->path, request->Request_Data);

            server_request->FLAGS = request->FLAGS;
            server_request->done = 0;
            server_request->ClientID = CLIENT_ID;

            int sever_sockfd = Server_Handle_ptr->SS_sockfd;

            printf("Forwarding request %d to server %d (Privilaged OP)\n", OP, Server_Handle_ptr->Server_ID);
            fprintf(BookKeeping, "Forwarding request %d to server %d (Privilaged OP) (TIME-STAMP: %lf)\n", OP, Server_Handle_ptr->Server_ID, TimeStamp());

            if (send(sever_sockfd, server_request, sizeof(SERVER_REQUEST), 0) != sizeof(SERVER_REQUEST))
            {
                free(server_request);
                printf(BRED "[-]Client_Handler_Thread: error while forwarding request to server %d\n" CRESET, Server_Handle_ptr->Server_ID);
                fprintf(BookKeeping, "[-]Client_Handler_Thread: error while forwarding request to server %d (TIME-STAMP: %lf)\n", Server_Handle_ptr->Server_ID, TimeStamp());

                response->Error_Code = REQ_FWD_ERROR;
                strcpy(response->Response_Data, "Error: Request could not be forwarded to server");
                response->CMD = OP;
                response->FLAGS = request->FLAGS;
                response->done = 1;

                if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
                {
                    printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                    fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                    CheckError(close(client_sockfd), "close()");
                    break;
                }
                break;
            }
            free(server_request);
            break;
        }
        default:
        {
            response->Error_Code = CMD_NOT_FOUND;
            strcpy(response->Response_Data, "Error: Command not found");
            response->CMD = OP;
            response->FLAGS = request->FLAGS;
            response->done = 0;

            printf(BRED "[-]Client_Handler_Thread: Invalid Command %d\n" CRESET, OP);
            fprintf(BookKeeping, "[-]Client_Handler_Thread: Invalid Command %d (TIME-STAMP: %lf)\n", OP, TimeStamp());

            if (send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
            {
                printf(BRED "[-]Client_Handler_Thread: send() failed for Client %d\n" CRESET, client_sockfd);
                fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n", client_sockfd, TimeStamp());
                CheckError(close(client_sockfd), "close()");
                break;
            }
            free(response);
        }
        }

        /*
        // print response
        printf(BMAG);
        printf("Response generated for client %d \n",client_sockfd);
        printf("Response Cmd: %d\n", response->CMD);
        printf("Response Flags: %d\n", response->FLAGS);
        printf("Response Error Code: %d\n", response->Error_Code);
        printf("Response Data: %s\n", response->Response_Data);
        printf("Response Server IP: %s\n", response->Storage_IP);
        printf("Response Server Port: %d\n", response->Storage_Port);
        printf(CRESET);

        pthread_mutex_lock(&Bookkeeping_mutex);
        fprintf(BookKeeping, "Response generated for client %d (TIME-STAMP: %lf)\n",client_sockfd,TimeStamp());
        fprintf(BookKeeping, "Response Cmd: %d\n", response->CMD);
        fprintf(BookKeeping, "Response Flags: %d\n", response->FLAGS);
        fprintf(BookKeeping, "Response Error Code: %d\n", response->Error_Code);
        fprintf(BookKeeping, "Response Data: %s\n", response->Response_Data);
        fprintf(BookKeeping, "Response Server IP: %s\n", response->Storage_IP);
        fprintf(BookKeeping, "Response Server Port: %d\n", response->Storage_Port);
        pthread_mutex_unlock(&Bookkeeping_mutex);


        //send Response
        if(send(client_sockfd, response, sizeof(CLIENT_RESPONSE), 0) != sizeof(CLIENT_RESPONSE))
        {
            printf(BRED"[-]Client_Handler_Thread: send() failed for Client %d\n"CRESET,client_sockfd);
            fprintf(BookKeeping, "[-]Client_Handler_Thread: send() failed for Client %d (TIME-STAMP: %lf)\n",client_sockfd,TimeStamp());
            CheckError(close(client_sockfd), "close()");
            break;
        }
        */
    }

    // Remove Client from linked list
    pthread_mutex_lock(&Client_List_mutex);
    if (Client_Handle_ptr->prev == NULL)
        Client_List = Client_Handle_ptr->next;
    else
        Client_Handle_ptr->prev->next = Client_Handle_ptr->next;

    if (Client_Handle_ptr->next != NULL)
        Client_Handle_ptr->next->prev = Client_Handle_ptr->prev;

    Client_Counter--;
    free(Client_Handle_ptr);
    pthread_mutex_unlock(&Client_List_mutex);

    printf(MAG "[+]Client_Handler_Thread: Client %d Disconnected\n" CRESET, CLIENT_ID);
    fprintf(BookKeeping, "[+]Client_Handler_Thread: Client %d Disconnected (TIME-STAMP: %lf)\n", CLIENT_ID, TimeStamp());
    fflush(stdout);
    return NULL;
}

void *FileFlusher(void *)
{
    while (1)
    {
        sleep(FLUSH_TIMEOUT);
        printf(BBLK "Flushing Logs\n" CRESET);
        // char *MountStructure = Get_Directory_Tree(MountPaths, "mount/");
        fprintf(BookKeeping, "\n------------------------Logs Update Start (TIME-STAMP: %lf)------------------------\n\n", TimeStamp());
        fprintf(BookKeeping, "Global Server Count: %d\n", Server_Counter);
        fprintf(BookKeeping, "Global Client Count: %d\n", Client_Counter);
        // fprintf(BookKeeping, "Global Mount for clients:\n");
        // fprintf(BookKeeping, "%s", MountStructure);
        fprintf(BookKeeping, "\n------------------------Logs Update End(TIME-STAMP: %lf)------------------------\n\n", TimeStamp());

        fflush(BookKeeping);
    }
}

int main()
{
    // file for bookkeeping
    BookKeeping = fopen("bookkeeping.log", "w");
    pthread_mutex_init(&Bookkeeping_mutex, NULL);
    if (BookKeeping == NULL)
    {
        printf("[-]Error creating bookkeeping logs\n");
        exit(EXIT_FAILURE);
    }

    // Thread for flushing logs
    pthread_t Flusher;
    pthread_create(&Flusher, NULL, FileFlusher, NULL);

    clock_gettime(CLOCK_MONOTONIC, &boot_time);
    printf("\n");
    printf(GRNB BHWHT "[+]Naming Server started.\n" CRESET);
    fprintf(BookKeeping, "[+]Naming Server started (BOOT TIME: %lf)\n", TimeStamp());

    printf("-------------------------------------------------------------\n");
    fprintf(BookKeeping, "-------------------------------------------------------------\n");

    // initialize global variables
    Server_index = 0;
    Server_Counter = 0;
    Client_Counter = 0;
    Client_List = NULL;
    memset(Server_Handle_Array, 0, sizeof(Server_Handle_Array));
    pthread_mutex_init(&server_thread_pool_mutex, NULL);

    MountPaths = Init_Trie();
    strcpy(MountPaths->path_token, "mount");
    MountPaths->Server_Handle = NULL;
    memset(MountPaths->children, 0, sizeof(MountPaths->children));

    // Insert_Path(MountPaths,"./",NULL);
    pthread_mutex_init(&MountPaths_mutex, NULL);

    Cache = createCache();
    pthread_mutex_init(&Cache_mutex, NULL);

    pthread_t Client_Acceptor;
    pthread_t Server_Acceptor;

    pthread_create(&Client_Acceptor, NULL, Client_Acceptor_Thread, NULL);
    pthread_create(&Server_Acceptor, NULL, Server_Acceptor_Thread, NULL);

    pthread_join(Client_Acceptor, NULL);
    pthread_join(Server_Acceptor, NULL);

    fclose(BookKeeping);
    return 0;
}
