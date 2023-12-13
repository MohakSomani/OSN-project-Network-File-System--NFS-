// connects to NS
// while loop starts
// Takes in buffer
// Structures Sending Package string
// Sends it to NS
// Checks LRU for the buffer's entry                                         (in NS)
// If entry found returns a string package starting with LRU with the data  (in NS)
// Else                                                                     (in NS)
// If CREATE DELETE or COPY client waits for the output
// If the output comes with LRU in start then just print it
// Else If READ WRITE or LIST then client waits for IP and PORT
// If the output comes with LRU in start then just print it
// for the SS and connect to it in a thread and send the package and wait for the output
// disconnect from SS and close the thread

#include "header.h"
fd_set readfds;
struct timeval timeout;

int main()
{
    time_t strt, curr;
    char *CMDS[9];
    CMDS[1] = (char *)malloc(sizeof(char) * 10);
    strcpy(CMDS[1], "READ");
    CMDS[2] = (char *)malloc(sizeof(char) * 10);
    strcpy(CMDS[2], "WRITE");
    CMDS[3] = (char *)malloc(sizeof(char) * 10);
    strcpy(CMDS[3], "INFO");
    CMDS[4] = (char *)malloc(sizeof(char) * 10);
    strcpy(CMDS[4], "CREATE");
    CMDS[5] = (char *)malloc(sizeof(char) * 10);
    strcpy(CMDS[5], "DELETE");
    CMDS[7] = (char *)malloc(sizeof(char) * 10);
    strcpy(CMDS[7], "COPY");
    CMDS[8] = (char *)malloc(sizeof(char) * 10);
    strcpy(CMDS[8], "LIST");
    int sock;
    struct sockaddr_in addr;
    socklen_t addr_size;
    int n;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror(RED "101 : [-]Socket error" reset);
        exit(1);
    }
    printf(BLU "[+]TCP server socket created.\n" reset);
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(NS_PORT);
    addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    while (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror(RED "101 : [-]Connection error" reset);
        printf(BLU "Trying to connect again in 3 seconds.\n" reset);
        sleep(3);
    }
    printf(BLU "Connected to the Naming Server.\n" reset);

    fd_set readfds;
    struct timeval timeout;

    printf(GRNB "\n=================- Client Started -=================" reset);

    char *buffer = (char *)malloc(MAX_BUFFER * sizeof(char));
    char *delim = "   \r\t";
    bzero(buffer, MAX_BUFFER);
    while (1)
    {
        free(buffer);
        buffer = (char *)malloc(MAX_BUFFER * sizeof(char));
        bzero(buffer, MAX_BUFFER);
        char *saveptr;
        size_t size = MAX_BUFFER;
        size_t length = 0;
        int c;
        // scanf("%s", buffer);
        printf(BHCYN "\nEnter Command: " reset);
        while ((c = getchar()) != '\n' && c != EOF)
        {
            // Resize the buffer if necessary
            if (length == size - 1)
            {
                size *= 2;
                char *newBuffer = (char *)realloc(buffer, size * sizeof(char));
                if (newBuffer == NULL)
                {
                    fprintf(stderr, RED "Memory reallocation failed\n" reset);
                    free(buffer);
                    return 1;
                }
                buffer = newBuffer;
            }

            // Store the character in the buffer
            buffer[length++] = (char)c;
        }

        // Null-terminate the string
        buffer[length] = '\0';
        if (strcmp(buffer, "exit") == 0)
        {
            break;
        }
        else if (strcmp(buffer, "HELP") == 0)
        {
            printf(GRNB "\n====================== HELP ======================\n" reset
                       BGRN "Welcome to the Custom Network File System Commands!\n"
                        "Explore the power of our networked file system with these commands:\n\n"
                        "1. READ <Path>\n"
                        "   - Read and display the contents of a file at the specified path.\n\n"

                        "2. WRITE <A/O> <Path> <Data>\n"
                        "   - Append (A) or overwrite (O) data to the file at the given path.\n\n"

                        "3. INFO <Path>\n"
                        "   - Retrieve information about the file or directory at the specified path.\n\n"

                        "4. CREATE <F/D> <Name> <Path>\n"
                        "   - Create a new file (F) or directory (D) with the given name at the specified path.\n\n"

                        "5. DELETE <F/D> <Path>\n"
                        "   - Delete the file or directory at the specified path.\n\n"

                        "6. COPY <Source Path> <Destination Path>\n"
                        "   - Copy a file from the source path to the destination path.\n\n"

                        "7. LIST [Path]\n"
                        "   - List the contents of the specified directory. If no path is given, it lists the MOUNT folder.\n\n"

                        "Now, go ahead and navigate your custom network file system! Enjoy exploring and managing your files.\n"
                        "===================================================\n" reset);
        }
        else
        {
            REQUEST req;
            char *buffer_copy = strdup(buffer);
            char *tok = strtok_r(buffer_copy, delim, &saveptr);
            if (strcmp(tok, "READ") == 0)
            {
                req.CMD = 1;
                if (saveptr == NULL)
                {
                    printf(RED "103 : Invalid Command Format.\nType 'HELP' for more input formats.\n" reset);
                    continue;
                }
                strcpy(req.path, saveptr);
            }
            else if (strcmp(tok, "WRITE") == 0)
            {
                req.CMD = 2;
                tok = strtok_r(saveptr, delim, &saveptr);
                if (tok == NULL || strlen(tok) == 0)
                {
                    printf(RED "103 : Invalid Command Format.\nType 'HELP' for more input formats.\n" reset);
                    continue;
                }
                if (tok[0] == 'A')
                {
                    req.FLAGS = 0;
                }
                else if (tok[0] == 'O')
                {
                    req.FLAGS = 1;
                }
                else
                {
                    printf(RED "103 : Invalid Flag\n" reset);
                    continue;
                }
                if (saveptr == NULL || strlen(saveptr) == 0)
                {
                    printf(RED "103 : Invalid Command Format.\nType 'HELP' for more input formats.\n" reset);
                    continue;
                }
                tok = strtok_r(saveptr, " \r\t\n", &saveptr);
                // if (saveptr == NULL || strlen(saveptr)==0)
                // {
                //     scanf("%s", saveptr);
                // }
                strcpy(req.path, tok); // path  =  path
                req.path[strlen(req.path)] = '\0';
            }
            else if (strcmp(tok, "INFO") == 0)
            {
                req.CMD = 3;
                if (saveptr == NULL || strlen(saveptr) == 0)
                {
                    printf(RED "103 : Invalid Command Format.\nType 'HELP' for more input formats.\n" reset);
                    continue;
                }
                strcpy(req.path, saveptr);
            }
            else if (strcmp(tok, "CREATE") == 0)
            {
                req.CMD = 4;
                tok = strtok_r(saveptr, delim, &saveptr);
                if (tok == NULL || strlen(tok) == 0)
                {
                    printf(RED "103 : Invalid Command Format.\nType 'HELP' for more input formats.\n" reset);
                    continue;
                }
                if (tok[0] == 'F')
                {
                    req.FLAGS = 1;
                }
                else if (tok[0] == 'D')
                {
                    req.FLAGS = 0;
                }
                else
                {
                    printf(RED "103 : Invalid Flag\n" reset);
                    continue;
                }
                tok = strtok_r(saveptr, delim, &saveptr);
                if (tok == NULL || strlen(tok) == 0 || saveptr == NULL || strlen(saveptr) == 0)
                {
                    printf(RED "103 : Invalid Command Format.\nType 'HELP' for more input formats.\n" reset);
                    continue;
                }
                // printf("Name: %s \n Path: %s\n", tok, saveptr);
                strcpy(req.Request_Data, tok); // name
                strcpy(req.path, saveptr);     // path
                if (strncmp(req.path, "mount", 5) == 0)
                {
                    char buffer[MAX_BUFFER];
                    strcpy(buffer, ".");
                    strcat(buffer, req.path + 5);
                    // printf("Path sent to SS %s\n", buffer);
                    strcpy(req.path, buffer);
                }
            }
            else if (strcmp(tok, "DELETE") == 0)
            {
                req.CMD = 5;
                strcpy(req.path, saveptr);
            }
            else if (strcmp(tok, "RMDIR") == 0)
            {
                req.CMD = 6;
                strcpy(req.path, saveptr);
            }
            else if (strcmp(tok, "COPY") == 0)
            {
                req.CMD = 6;
                tok = strtok_r(saveptr, delim, &saveptr);
                if (tok == NULL || strlen(tok) == 0 || saveptr == NULL || strlen(saveptr) == 0)
                {
                    printf(RED "103 : Invalid Command Format.\nType 'HELP' for more input formats.\n" reset);
                    continue;
                }
                strcpy(req.path, tok);             // path  =  source_path
                strcpy(req.Request_Data, saveptr); // Data = dest_path
                if (strncmp(req.Request_Data, "mount", 5) == 0)
                {
                    char buffer[MAX_BUFFER];
                    strcpy(buffer, ".");
                    strcat(buffer, req.Request_Data + 5);
                    // printf("Path sent to SS %s\n", buffer);
                    strcpy(req.Request_Data, buffer);
                }
            }
            else if (strcmp(tok, "LIST") == 0)
            {
                req.CMD = 8;
                strcpy(req.path, saveptr);
            }
            else
            {
                printf(RED "104 : Invalid Command\n" reset);
                continue;
            }
            req.done = 1;
            // printf("Size snettt %ld", sizeof(req));
            if (strncmp(req.path, "mount", 5) == 0)
            {
                char buffer[MAX_BUFFER];
                strcpy(buffer, ".");
                strcat(buffer, req.path + 5);
                // printf("Path sent to SS %s\n", buffer);
                strcpy(req.path, buffer);
            }
            if (send(sock, (void *)&req, sizeof(req), 0) != sizeof(req))
            {

                printf(REDHB "Server Disconnected\n" reset);
                exit(1);
            }

            printf(BLU "Request Package sent to NS.\n" reset);

            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);
            timeout.tv_sec = 2;
            timeout.tv_usec = 0;

            int selectResult = select(sock + 1, &readfds, NULL, NULL, &timeout);

            if (selectResult == -1)
            {
                perror(RED "Error in select" reset);
                exit(1);
            }
            else if (selectResult == 0)
            {
                // Timeout occurred
                printf(RED "105: No response from NS within 2 seconds.\n" reset);
                close(sock);
                sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0)
                {
                    perror(RED "101 : [-]Socket error" reset);
                    exit(1);
                }
                // printf(BLU "[+]TCP server socket created.\n" reset);
                memset(&addr, '\0', sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(NS_PORT);
                addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
                time(&strt);
                while (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
                {
                    time(&curr);
                    if (curr - strt > 7)
                    {
                        printf(REDHB "105: SERVER DISCONNECTED\n" reset);
                        exit(1);
                    }
                    perror(RED "101 : [-]Connection error" reset);
                    sleep(2);
                }
                continue;
            }

            // CLIENT_ACK ack;
            // struct pollfd mypoll = {sock, POLLIN | POLLPRI};

            // if (poll(&mypoll, 1, PING_TIMEOUT))
            // {
            //     printf(BLU"Receive ACK package from NS\n"reset, sock);
            //     int r=recv(sock,&ack,sizeof(CLIENT_ACK),0);
            //     printf("%d ACK RECEIVEDDD\n",r);
            //     if (ack.Error_Code != 0)
            //     {
            //         printf(RED "Error Code: %d\n" reset, ack.Error_Code);
            //         continue;
            //     }
            // }
            // else
            // {
            //     printf(RED"NS did not send ACK package\n"reset, sock);
            //     printf(BLU"Listening again...\n"reset);
            //     continue;
            // }

            if (strncmp(req.path, "mount", 5) == 0)
            {
                char buffer[MAX_BUFFER];
                strcpy(buffer, ".");
                strcat(buffer, req.path + 5);
                // printf("Path sent to SS %s\n", buffer);
                strcpy(req.path, buffer);
            }

            RESPONSE resp;
            int r;
            while (1)
            {
                r = recv(sock, &resp, sizeof(RESPONSE), 0);
                if (r > 0)
                    break;
                if (r == 0)
                {
                    printf(REDHB "Server Disconnected\n" reset);
                    exit(1);
                }
            }
            printf(BLU "Response Package Received from NS.\n" reset);
            // printf("Received %d %ld: CMD : %d \ndone: %d \n err code : %d \n",r,sizeof(RESPONSE),resp.CMD,resp.done,resp.Error_Code);
            if (resp.Error_Code != 0)
            {
                printf(RED "Error Code: %d\n" reset, resp.Error_Code);
                continue;
            }
            else if (resp.CMD == 1 || resp.CMD == 2 || resp.CMD == 3)
            {
                char *ip = resp.Storage_IP;
                int port = resp.Storage_Port;
                int sock_ss;
                struct sockaddr_in addr_ss;
                socklen_t addr_size_ss;
                char buffer_ss[1024];
                int n_ss;

                // printf("Received Data: %s\n", resp.Response_Data);

                sock_ss = socket(AF_INET, SOCK_STREAM, 0);
                if (sock_ss < 0)
                {
                    perror(RED "102 : [-]Socket error" reset);
                    exit(1);
                }
                printf(BLU "[+]TCP server socket created.\n" reset);

                memset(&addr_ss, '\0', sizeof(addr_ss));
                addr_ss.sin_family = AF_INET;
                addr_ss.sin_port = htons(port);
                addr_ss.sin_addr.s_addr = inet_addr(ip);

                time(&strt);
                while (connect(sock_ss, (struct sockaddr *)&addr_ss, sizeof(addr_ss)) < 0)
                {
                    time(&curr);
                    if (curr - strt > 7)
                    {
                        printf(RED "105: Could not connect to the Storage Server\n" reset);
                        close(sock_ss);
                        close(sock);
                        sock = socket(AF_INET, SOCK_STREAM, 0);
                        if (sock < 0)
                        {
                            perror(RED "101 : [-]Socket error" reset);
                            exit(1);
                        }
                        printf(BLU "[+]TCP server socket created.\n" reset);
                        memset(&addr, '\0', sizeof(addr));
                        addr.sin_family = AF_INET;
                        addr.sin_port = htons(NS_PORT);
                        addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
                        time(&strt);
                        while (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
                        {
                            time(&curr);
                            if (curr - strt > 7)
                            {
                                printf(REDHB "105: SERVER DISCONNECTED\n" reset);
                                exit(1);
                            }
                            perror(RED "101 : [-]Connection error" reset);
                            sleep(2);
                        }
                        continue;
                    }
                    perror(RED "102 : [-]Connection error" reset);
                    printf(BLU "Port: %d  Trying to connect again in 2 seconds.\n" reset, port);
                    sleep(2);
                }
                printf(BLU "Connected to the Storage Server.\n" reset);

                if (resp.CMD == 2)
                {
                    // strcat(req.path, " ");
                    RESPONSE resp1;
                    req.done = 0;
                    printf(BLU "Request Package sent again to SS.\n" reset);

                    send(sock_ss, (void *)&req, sizeof(req), 0);
                    int remaining = strlen(saveptr);
                    int offset = 0;

                    while (remaining > 0)
                    {

                        strcpy(req.path, saveptr + offset);
                        if (remaining < CHUNK_SIZE)
                        {
                            req.done = 1;
                            strcat(req.path, "\0");
                        }
                        else
                        {
                            req.done = 0;
                        }
                        int sent = send(sock_ss, (void *)&req, sizeof(req), 0);
                        sent = strlen(req.path);
                        // printf("%d chunk Package sent to SS.\n%d Remaining\n", sent, remaining);

                        remaining -= sent;
                        offset += sent;
                    }
                    recv(sock_ss, &resp1, sizeof(resp1), 0);
                    if (resp1.Error_Code != 0)
                    {
                        printf(RED "Error Code: %d\n" reset, resp1.Error_Code);
                        continue;
                    }
                    else
                    {
                        printf(BWHT "%s\n" reset, resp1.Response_Data);
                    }
                    // printf("hee\n");
                }
                else
                {
                    printf(BLU "Request Package sent again to SS.\n" reset);
                    send(sock_ss, (void *)&req, sizeof(req), 0);
                }
                // printf("hee\n");

                // recv(sock, &resp, sizeof(resp), 0);
                if (resp.CMD != 2)
                {
                    char *finalString = NULL;
                    size_t totalReceived = 0;
                    size_t bufferSize = 0;
                    RESPONSE *resp1;
                    while (1)
                    {
                        resp1 = (RESPONSE *)malloc(sizeof(RESPONSE));
                        ssize_t received;
                        while (1)
                        {
                            r = recv(sock_ss, resp1, sizeof(RESPONSE), 0);
                            if (r > 0)
                                break;
                            if (r == 0)
                            {
                                printf(REDHB "Server Disconnected\n" reset);
                                exit(1);
                            }
                        }
                        received = r;
                        // printf("%ld Response Package Received from SS.\n", received);
                        // printf("ERR CODE : %d \nDONE : %d \nData : %s\n", resp1->Error_Code, resp1->done, resp1->Response_Data);
                        char chunk[CHUNK_SIZE];
                        // ssize_t received = recv(sock, (void *)&resp, sizeof(resp), 0);
                        received = strlen(resp1->Response_Data);
                        if (received <= 0)
                        {
                            break; // Connection closed or error
                        }
                        received = strlen(resp1->Response_Data);
                        // Resize the buffer if necessary
                        if (totalReceived + received > bufferSize)
                        {
                            bufferSize = totalReceived + received;
                            finalString = realloc(finalString, bufferSize);
                            if (finalString == NULL)
                            {
                                perror(RED "Memory reallocation failed" reset);
                                break;
                            }
                        }

                        // Concatenate the chunk to the final string
                        memcpy(finalString + totalReceived, resp1->Response_Data, received);
                        totalReceived += received;

                        // printf("Received %ld bytes\n", totalReceived);

                        if (resp1->done == 1)
                        {
                            // printf("aisdias\n");
                            break;
                        }
                    }

                    if (resp1->Error_Code != 0)
                    {
                        printf(RED "Error Code: %d\n" reset, resp1->Error_Code);
                        continue;
                    }
                    else
                    {
                        // printf("%s\n", resp.finalString);
                        // Null-terminate the final string
                        if (finalString != NULL)
                        {
                            finalString[totalReceived - 1] = '\0';

                            // Print the final string
                            printf(BWHT "Data read from the file: \n%s\n====================\n" reset, finalString);

                            // Free the dynamically allocated memory
                            free(finalString);
                        }
                    }
                }
                // printf("hee\n");
                close(sock_ss);
                printf(BLU "Disconnected from the storage server.\n" reset);

                CLIENT_ACK *ack = (CLIENT_ACK *)malloc(sizeof(CLIENT_ACK));
                ack->done = 1;
                ack->Error_Code = 0;
                if (send(sock, (void *)ack, sizeof(CLIENT_ACK), 0) != sizeof(CLIENT_ACK))
                {
                    printf(REDHB "Server Disconnected\n" reset);
                    exit(1);
                }
            }
            else if (resp.CMD == 8)
            {
                printf("%s\n", resp.Response_Data);
                RESPONSE *resp1;
                while (1)
                {
                    resp1 = (RESPONSE *)malloc(sizeof(RESPONSE));
                    ssize_t received;
                    while (1)
                    {
                        r = recv(sock, resp1, sizeof(RESPONSE), 0);
                        if (r > 0)
                            break;
                        if (r == 0)
                        {
                            printf(REDHB "Server Disconnected\n" reset);
                            exit(1);
                        }
                    }
                    // printf("Receive : done: %d \n err code : %d \n",resp1->done,resp1->Error_Code);
                    received = r;
                    if (resp1->done == 1)
                    {
                        break;
                    }
                    if (resp1->Error_Code != 0)
                    {
                        printf(RED "Error Code: %d\n" reset, resp1->Error_Code);
                        continue;
                    }
                    printf(BWHT "%s" reset, resp1->Response_Data);
                }
            }
            // else if (resp.CMD == 4 || resp.CMD == 5 || resp.CMD == 7)
            // {
            //     ACK ack;
            //     while (1)
            //     {
            //         r = recv(sock, &ack, sizeof(ack), 0);
            //         if (r > 0)
            //             break;
            //         if (r == 0)
            //         {
            //             printf("Server Disconnected\n");
            //             exit(1);
            //         }
            //     }
            //     if (ack.err_code == 0)
            //     {
            //         printf("%s Executed Succesfully \n", CMDS[ack.cmd]);
            //     }
            //     else
            //     {
            //         printf("Error Code : %d\n", ack.err_code);
            //     }
            // }
            else
            {
                if (resp.Error_Code != 0)
                {
                    printf(RED "Error Code : %d\n" reset, resp.Error_Code);
                }
                else
                    printf(BWHT "%s\n" reset, resp.Response_Data);
            }
        }
    }

    close(sock);
    printf(BLU "Disconnected from the server.\n" reset);

    return 0;
}
