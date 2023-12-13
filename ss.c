#include "header.h"
int PORT_SS;
int PORT_NM;

int *sock_for_NM;

pthread_mutex_t trie_read, trie_write;
pthread_mutex_t read_h, write_h;
int readers_h;
int trie_readers;
Trie *create()
{
    Trie *y = (Trie *)malloc(sizeof(Trie));
    y->next = NULL;
    Trie *head = (Trie *)malloc(sizeof(Trie));
    head->next = NULL;
    y->child = head;
    return y;
}

Blank *search(Trie *F, char *str)
{
    char *ty = (char *)malloc(sizeof(char) * MAX_BUFFER);
    strcpy(ty, str);
    pthread_mutex_lock(&trie_read);
    trie_readers++;
    if (trie_readers == 1)
    {
        pthread_mutex_lock(&trie_write);
    }
    pthread_mutex_unlock(&trie_read);
    char *token = strtok_r(ty, "/", &ty);
    Trie *T = F->child;
    Blank *b = NULL;
    while (1)
    {
        if (str == NULL)
        {
            break;
        }
        token = strtok_r(ty, "/", &ty);
        if (token == NULL)
        {
            break;
        }
        Trie *i = T->next;
        Trie *prev_i = T;
        int flag = 0;
        while (i != NULL)
        {
            if (strcmp(i->string, token) == 0)
            {
                flag = 1;
                b = i->b;
                T = i->child;
                break;
            }
            prev_i = i;
            i = i->next;
        }
        if (flag == 0)
        {
            pthread_mutex_lock(&trie_read);
            trie_readers--;
            if (trie_readers == 0)
            {
                pthread_mutex_unlock(&trie_write);
            }
            pthread_mutex_unlock(&trie_read);
            return NULL;
        }
    }
    pthread_mutex_lock(&trie_read);
    trie_readers--;
    if (trie_readers == 0)
    {
        pthread_mutex_unlock(&trie_write);
    }
    pthread_mutex_unlock(&trie_read);
    // free(ty);
    return b;
}

Trie *insert(char *str, Trie *F, Blank *b)
{
    char *ty = (char *)malloc(sizeof(char) * MAX_BUFFER);
    strcpy(ty, str);
    pthread_mutex_lock(&trie_write);
    char *token = strtok_r(ty, "/", &ty);
    Trie *T = F->child;
    Trie *prev_T = F;
    while (1)
    {
        if (str == NULL)
        {
            break;
        }
        token = strtok_r(ty, "/", &ty);
        if (token == NULL)
        {
            break;
        }
        Trie *i = T->next;
        Trie *prev_i = T;
        int flag = 0;
        while (i != NULL)
        {
            if (strcmp(i->string, token) == 0)
            {
                flag = 1;
                prev_T = T;
                T = i->child;
                break;
            }
            prev_i = i;
            i = i->next;
        }
        if (flag == 0)
        {
            Trie *y = create();
            prev_i->next = y;
            strcpy(y->string, token);
            y->b = b;
            prev_T = y;
            T = y->child;
        }
    }
    prev_T->b = b;
    pthread_mutex_unlock(&trie_write);
    // free(ty);
    return F;
}

Trie *delete(char *str, Trie *F)
{
    pthread_mutex_lock(&trie_write);
    char *ty = (char *)malloc(sizeof(char) * MAX_BUFFER);
    strcpy(ty, str);
    char *token = strtok_r(ty, "/", &ty);
    Trie *T = F->child;
    Trie *parent = F;
    Trie *prev_T = F;
    Trie *hemang = NULL;
    while (1)
    {
        if (str == NULL)
        {
            break;
        }
        token = strtok_r(ty, "/", &ty);
        if (token == NULL)
        {
            break;
        }
        Trie *i = T->next;
        Trie *prev_i = T;
        int flag = 0;
        while (i != NULL)
        {
            if (strcmp(i->string, token) == 0)
            {
                prev_T = i;
                parent = prev_i;
                T = i->child;
                // hemang = i;
                break;
            }
            prev_i = i;
            i = i->next;
        }
    }
    // T->next = NULL;
    parent->next = prev_T->next;
    pthread_mutex_unlock(&trie_write);
    // free(ty);
    return F;
}

Trie *Tree;

char *get_permissions_string(struct stat *file_stat)
{
    char perms[] = "rwxrwxrwx";
    char *permissions_str = (char *)malloc(11 * sizeof(char));

    if (!permissions_str)
    {
        perror("Memory allocation error");
        // exit(EXIT_FAILURE);
        return NULL;
    }
    permissions_str[0] = (S_ISDIR(file_stat->st_mode)) ? 'd' : '-';
    for (int i = 0; i < 9; ++i)
    {
        permissions_str[i + 1] = (file_stat->st_mode & (1 << (8 - i))) ? perms[i] : '-';
    }
    permissions_str[10] = '\0';

    return permissions_str;
}

long long get_size(const char *path)
{
    struct stat statbuf;

    if (stat(path, &statbuf) == -1)
    {
        perror("Error getting file/directory size");
        return -1;
    }

    if (S_ISDIR(statbuf.st_mode))
    {
        long long total_size = 0;
        DIR *dir = opendir(path);

        if (!dir)
        {
            perror("Error opening directory");
            return -1;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                char subpath[PATH_MAX];
                snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
                if (search(Tree, subpath) != NULL)
                    total_size += get_size(subpath);
            }
        }

        closedir(dir);
        return total_size;
    }
    else
    {
        return statbuf.st_size;
    }
}

void read_file_copy(char *str, char *path2, int client, char *str2)
{
    // int sock = *sock_fd;
    int y;
    Blank *b = search(Tree, str);
    char *data = (char *)calloc((CHUNK_SIZE), sizeof(char));
    pthread_mutex_lock(&b->read);
    b->readers++;
    if (b->readers == 1)
    {
        pthread_mutex_lock(&b->write);
    }
    pthread_mutex_unlock(&b->read);

    int count = 0;
    FILE *file = fopen(str, "r");
    if (file == NULL)
    {
        perror("Error opening the file");
        pthread_mutex_lock(&b->read);
        b->readers--;
        if (b->readers == 0)
        {
            pthread_mutex_unlock(&b->write);
        }
        pthread_mutex_unlock(&b->read);
        SERVER_RESPONSE *req = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
        req->done = 3;
        req->CMD = 2;
        req->FLAGS = 0;
        req->Error_Code = 302;
        req->ClientID = client;
        while (send(*sock_for_NM, &req, sizeof(SERVER_RESPONSE), 0) < 0)
        {
            y = 0;
        }
        return;
    }
    int set = 0;
    // SERVER_RESPONSE *req = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
    char *hemang = (char *)malloc(sizeof(char) * MAX_BUFFER);
    SERVER_RESPONSE req;
    while ((data[count] = fgetc(file)) != EOF)
    {
        count++;
        if (count == CHUNK_SIZE)
        {
            memset(&req, 0, sizeof(SERVER_RESPONSE));
            // char* data = (char*)malloc(sizeof(char)*MAX_BUFFER);
            req.CMD = 2;
            if (set == 0)
            {
                req.FLAGS = 0;
            }
            else
            {
                req.FLAGS = 1;
            }
            req.done = 3;
            req.Error_Code = 0;
            req.ClientID = client;
            // snprintf(req.Response_Data, MAX_BUFFER, "%s\n%s\n%s", str2, path2, data);
            // strcpy
            // // strcpy(req.Response_Data, str2);
            // // strcat(req.Response_Data , "\n");
            // // strcat(req.Response_Data , path2);
            // // strcat(req.Response_Data , "\n");
            // // strcat(req.Response_Data , data);
            strcpy(hemang, str2);
            strcat(hemang, "\n");
            strcat(hemang, path2);
            strcat(hemang, "\n");
            strcat(hemang, data);
            strcpy(req.Response_Data, hemang);
            req.Response_Data[MAX_BUFFER - 1] = '\0';
            long int n;
            while ((n = send(*sock_for_NM, &req, sizeof(SERVER_RESPONSE), 0)) < 0)
            {
                y = 0;
            }
            printf("%d\n", req.done);
            fflush(stdout);
            printf("Req Response Data: %s ", req.Response_Data);
            fflush(stdout);
            count = 0;
            set++;
            // free(req);
            memset(data, '\0', CHUNK_SIZE);
            memset(hemang, 0, MAX_BUFFER);
        }
    }
    // SERVER_RESPONSE *req = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
    if (count >= 0)
    {
        memset(&req, 0, sizeof(SERVER_RESPONSE));
        req.CMD = 2;
        req.FLAGS = 0;
        req.done = 3;
        req.Error_Code = 0;
        req.ClientID = client;
        strcpy(hemang, str2);
        strcat(hemang, "\n");
        strcat(hemang, path2);
        strcat(hemang, "\n");
        strcat(hemang, data);
        strcpy(req.Response_Data, hemang);
        // strcpy(req->path, data);
        // snprintf(req.Response_Data, MAX_BUFFER, "%s\n%s\n%s", str2, path2, data);
        send(*sock_for_NM, &req, sizeof(SERVER_RESPONSE), 0);
        // free(req);
    }
    fclose(file);

    pthread_mutex_lock(&b->read);
    b->readers--;
    if (b->readers == 0)
    {
        pthread_mutex_unlock(&b->write);
    }
    pthread_mutex_unlock(&b->read);
    return;
}

Trie *recursive_Trie1(Trie *F, char *str, char *str2, int client, char *pat)
{
    Trie *T = F->child;
    Trie *i = T->next;
    while (i != NULL)
    {
        char *cat_str = (char *)calloc(MAX_BUFFER, sizeof(char));
        char *cat_str2 = (char *)calloc(MAX_BUFFER, sizeof(char));
        snprintf(cat_str2, MAX_BUFFER, "%s/%s", str, i->string);
        struct stat file_stat;
        if (stat(cat_str2, &file_stat) == 0 && i->string[0] != '_')
        {
            if (S_ISDIR(file_stat.st_mode))
            {
                if (client == -1)
                {
                    snprintf(cat_str, MAX_BUFFER, "%s/%s", str2, i->string);
                }
                else
                {
                    snprintf(cat_str, MAX_BUFFER, "%s/%s_copy", str2, i->string);
                }
                SERVER_RESPONSE *res = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
                res->CMD = 4;
                res->FLAGS = 0;
                res->ClientID = client;
                res->Error_Code = 0;
                res->done = 3;
                // strcpy(res->Response_Data, cat_str);
                snprintf(res->Response_Data, MAX_BUFFER, "%s\n%s", pat, cat_str);
                while (send(*sock_for_NM, res, sizeof(SERVER_RESPONSE), 0) < 0)
                {
                    puts("Send failed");
                }
                recursive_Trie1(i, cat_str2, cat_str, client, pat);
            }
            // else
            // {
            //     if(client==-1)
            //     {
            //         snprintf(cat_str, MAX_BUFFER, "%s/%s", str2, i->string);
            //     }
            //     else{
            //         char *ty = (char *)malloc(sizeof(char) * MAX_BUFFER);
            //         strcpy(ty, i->string);
            //         char *tok = strtok_r(ty, ".", &ty);
            //         if (strcmp(ty, "") != 0)
            //         {
            //             snprintf(cat_str, MAX_BUFFER, "%s/%s_copy.%s", str2, tok, ty);
            //         }
            //         else
            //         {
            //             snprintf(cat_str, MAX_BUFFER, "%s/%s_copy", str2, i->string);
            //         }

            //     }
            //     // snprintf(cat_str2, MAX_BUFFER, "%s/%s", str, i->string);
            //     read_file_copy(cat_str2, cat_str, client, pat);
            // }
        }
        free(cat_str);
        free(cat_str2);
        i = i->next;
    }
}

Trie *recursive_Trie2(Trie *F, char *str, char *str2, int client, char *pat)
{
    Trie *T = F->child;
    Trie *i = T->next;
    while (i != NULL)
    {
        char *cat_str = (char *)calloc(MAX_BUFFER, sizeof(char));
        char *cat_str2 = (char *)calloc(MAX_BUFFER, sizeof(char));
        snprintf(cat_str2, MAX_BUFFER, "%s/%s", str, i->string);
        struct stat file_stat;
        if (stat(cat_str2, &file_stat) == 0 && i->string[0] != '_')
        {
            if (S_ISDIR(file_stat.st_mode))
            {
                if (client == -1)
                {
                    snprintf(cat_str, MAX_BUFFER, "%s/%s", str2, i->string);
                }
                else
                {
                    snprintf(cat_str, MAX_BUFFER, "%s/%s_copy", str2, i->string);
                }
                SERVER_RESPONSE *res = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
                res->CMD = 4;
                res->FLAGS = 0;
                res->ClientID = client;
                res->Error_Code = 0;
                res->done = 3;
                // strcpy(res->Response_Data, cat_str);
                snprintf(res->Response_Data, MAX_BUFFER, "%s\n%s", pat, cat_str);
                while (send(*sock_for_NM, res, sizeof(SERVER_RESPONSE), 0) < 0)
                {
                    puts("Send failed");
                }
                recursive_Trie2(i, cat_str2, cat_str, client, pat);
            }
            else
            {
                if (client == -1)
                {
                    snprintf(cat_str, MAX_BUFFER, "%s/%s", str2, i->string);
                }
                else
                {
                    char *ty = (char *)malloc(sizeof(char) * MAX_BUFFER);
                    strcpy(ty, i->string);
                    char *tok = strtok_r(ty, ".", &ty);
                    if (strcmp(ty, "") != 0)
                    {
                        snprintf(cat_str, MAX_BUFFER, "%s/%s_copy.%s", str2, tok, ty);
                    }
                    else
                    {
                        snprintf(cat_str, MAX_BUFFER, "%s/%s_copy", str2, i->string);
                    }
                }
                // snprintf(cat_str2, MAX_BUFFER, "%s/%s", str, i->string);
                read_file_copy(cat_str2, cat_str, client, pat);
            }
        }
        free(cat_str);
        free(cat_str2);
        i = i->next;
    }
}

void copyRes(Trie *F, char *str, char *str2, int client)
{
    // int sock = *(sock_fd);
    char *ty = (char *)malloc(sizeof(char) * MAX_BUFFER);
    strcpy(ty, str);
    // pthread_mutex_lock(&trie_write);
    pthread_mutex_lock(&trie_read);
    trie_readers++;
    if (trie_readers == 1)
    {
        pthread_mutex_lock(&trie_write);
    }
    pthread_mutex_unlock(&trie_read);
    char *token = strtok_r(ty, "/", &ty);
    Trie *T = F->child;
    Trie *prev_T = F;
    while (1)
    {
        if (str == NULL)
        {
            break;
        }
        token = strtok_r(ty, "/", &ty);
        if (token == NULL)
        {
            break;
        }
        Trie *i = T->next;
        while (i != NULL)
        {
            if (strcmp(i->string, token) == 0)
            {
                prev_T = i;
                T = i->child;
                // hemang = i;
                break;
            }
            // prev_i = i;
            i = i->next;
        }
    }
    char *cat_str = (char *)calloc(MAX_BUFFER, sizeof(char));
    char *cat_str2 = (char *)calloc(MAX_BUFFER, sizeof(char));
    struct stat file_stat;
    if (stat(str, &file_stat) == 0)
    {
        if (S_ISDIR(file_stat.st_mode))
        {
            // snprintf(cat_str, MAX_BUFFER, "%s/%s_copy", str2, T->string);
            // snprintf(cat_str2, MAX_BUFFER, "%s", str);
            // SERVER_RESPONSE *res = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
            // res->CMD = 4;
            // res->FLAGS = 0;
            // res->ClientID = client;
            // res->Error_Code = 0;
            // // strcpy(res->Response_Data, cat_str);
            // snprintf(res->Response_Data, MAX_BUFFER, "%s\n%s", str2, cat_str);
            // while (send(*sock_for_NM, res, sizeof(SERVER_RESPONSE), 0) < 0)
            // {
            //     puts("Send failed");
            // }
            // recursive_Trie(prev_T, cat_str2, cat_str, client, str2);

            // recursive_Trie1(prev_T ,  str ,str2 , client , str2);
            // SERVER_RESPONSE res;
            // // res.done = 69;
            // if(client==-1)
            // {
            //     res.done = 96;
            // }
            // else{
            //     res.done = 68;
            // }
            // res.CMD = 7;
            // res.Error_Code = 0;
            // res.FLAGS = 0;
            // res.ClientID = client;
            // while (send(*sock_for_NM, &res, sizeof(SERVER_RESPONSE), 0) < 0)
            // {
            //     puts("Send failed");
            // }
            recursive_Trie2(prev_T, str, str2, client, str2);
        }
        else
        {
            char *ty = (char *)malloc(sizeof(char) * MAX_BUFFER);
            strcpy(ty, prev_T->string);
            char *tok = strtok_r(ty, ".", &ty);
            if (strcmp(ty, "") != 0)
            {
                snprintf(cat_str, MAX_BUFFER, "%s/%s_copy.%s", str2, tok, ty);
            }
            else
            {
                snprintf(cat_str, MAX_BUFFER, "%s/%s_copy", str2, prev_T->string);
            }
            snprintf(cat_str2, MAX_BUFFER, "%s", str);
            read_file_copy(cat_str2, cat_str, client, str2);
        }
    }
    // struct stat file_stat;

    // recursive_Trie(prev_T ,  str ,str2 , client , str2);
    pthread_mutex_lock(&trie_read);
    trie_readers--;
    if (trie_readers == 0)
    {
        pthread_mutex_unlock(&trie_write);
    }
    pthread_mutex_unlock(&trie_read);
}

char *read_file(char *str, int *sock_fd)
{
    int sock = *sock_fd;
    Blank *b = search(Tree, str);
    char *data = (char *)calloc((CHUNK_SIZE) + 1, sizeof(char));
    pthread_mutex_lock(&b->read);
    b->readers++;
    if (b->readers == 1)
    {
        pthread_mutex_lock(&b->write);
    }
    pthread_mutex_unlock(&b->read);

    int count = 0;
    FILE *file = fopen(str, "r");
    if (file == NULL)
    {
        perror("Error opening the file");
        pthread_mutex_lock(&b->read);
        b->readers--;
        if (b->readers == 0)
        {
            pthread_mutex_unlock(&b->write);
        }
        pthread_mutex_unlock(&b->read);
        RESPONSE *res = (RESPONSE *)malloc(sizeof(RESPONSE));
        res->Error_Code = 302;
        res->done = 1;
        // res->ClientID = client;
        send(sock, res, sizeof(RESPONSE), 0);
        return NULL;
    }
    // printf("Hemang\n");
    while ((data[count] = fgetc(file)) != EOF)
    {
        count++;
        if (count == CHUNK_SIZE)
        {
            RESPONSE *res = (RESPONSE *)malloc(sizeof(RESPONSE));
            // char* data = (char*)malloc(sizeof(char)*MAX_BUFFER);
            res->Error_Code = 0;
            strcpy(res->Response_Data, data);
            // res->ClientID = client;
            // printf("%s\n",res->Response_Data);
            send(sock, res, sizeof(RESPONSE), 0);
            count = 0;
            memset(data, '\0', CHUNK_SIZE);
            free(res);
        }
    }
    if (count >= 0)
    {
        RESPONSE *res = (RESPONSE *)malloc(sizeof(RESPONSE));
        // char* data = (char*)malloc(sizeof(char)*MAX_BUFFER);
        res->Error_Code = 0;
        res->done = 1;
        // res->ClientID = client;
        strcpy(res->Response_Data, data);
        send(sock, res, sizeof(RESPONSE), 0);
        free(res);
        printf("Hemang\n");
    }
    fclose(file);

    pthread_mutex_lock(&b->read);
    b->readers--;
    if (b->readers == 0)
    {
        pthread_mutex_unlock(&b->write);
    }
    pthread_mutex_unlock(&b->read);
    return data;
}

char *get_file_info(char *str)
{
    Blank *b = search(Tree, str);
    pthread_mutex_lock(&b->read);
    b->readers++;
    if (b->readers == 1)
    {
        pthread_mutex_lock(&b->write);
    }
    pthread_mutex_unlock(&b->read);
    struct stat file_info;
    if (stat(str, &file_info) == 0)
    {
        char *buff = (char *)malloc(sizeof(char) * 2 * MAX_BUFFER);
        // snprintf(buff, 2 * MAX_BUFFER, "File Size: %ld bytes\nFile Permissions: %o\n", file_info.st_size, file_info.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
        snprintf(buff, 2 * MAX_BUFFER, "File Size: %lld bytes\nFile Permissions: %s\n", get_size(str), get_permissions_string(&file_info));
        pthread_mutex_lock(&b->read);
        b->readers--;
        if (b->readers == 0)
        {
            pthread_mutex_unlock(&b->write);
        }
        pthread_mutex_unlock(&b->read);
        // return data;
        return buff;
    }
    else
    {
        perror("No such file exits");
        pthread_mutex_lock(&b->read);
        b->readers--;
        if (b->readers == 0)
        {
            pthread_mutex_unlock(&b->write);
        }
        pthread_mutex_unlock(&b->read);
        return NULL;
    }
}

char *create_file(char *str)
{
    // umask(0);
    int file = open(str, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (file != -1)
    {
        char *return_value = (char *)malloc(sizeof(char) * MAX_BUFFER);
        snprintf(return_value, MAX_BUFFER, "%s", str);
        Blank *b = (Blank *)malloc(sizeof(Blank));
        pthread_mutex_init(&b->read, NULL);
        pthread_mutex_init(&b->write, NULL);
        b->readers = 0;
        Tree = insert(str, Tree, b);
        return return_value;
    }
    else
    {
        return NULL;
    }
}

char *create_dir(char *str)
{
    printf("str received : %s\n", str);
    if (mkdir(str, S_IRWXU | S_IRWXG | S_IRWXO) == 0)
    {
        char *return_value = (char *)malloc(sizeof(char) * MAX_BUFFER);
        // return_value = "Directory created succecssfully.";
        snprintf(return_value, MAX_BUFFER, "%s", str);
        Blank *b = (Blank *)malloc(sizeof(Blank));
        Tree = insert(str, Tree, b);
        return return_value;
    }
    else
    {
        perror("Error creating directory");
        return NULL;
    }
}

char *write_file(char *path, int *sockfd)
{
    int sock = *sockfd;
    Blank *b = search(Tree, path);
    pthread_mutex_lock(&b->write);
    // char*path = strtok_r(str , "\n" ,&str);
    FILE *file = fopen(path, "w");
    if (file == NULL)
    {
        while (1)
        {
            REQUEST *req = (REQUEST *)malloc(sizeof(REQUEST));
            recv(sock, req, sizeof(REQUEST), 0);
            if (req->done == 1)
                break;
        }
        perror("Error opening file");
        pthread_mutex_unlock(&b->write);
        // printf("adijw\n");
        return NULL;
    }
    while (1)
    {
        REQUEST *req = (REQUEST *)malloc(sizeof(REQUEST));
        recv(sock, req, sizeof(REQUEST), 0);
        fprintf(file, "%s", req->path);
        fflush(file);
        if (req->done = 1)
        {
            char *return_value = (char *)malloc(sizeof(char) * MAX_BUFFER);
            return_value = "Data written successfully on the file.";
            fclose(file);
            pthread_mutex_unlock(&b->write);
            return return_value;
        }
    }
}

char *append_file(char *path, int *sockfd)
{

    int sock = *sockfd;
    Blank *b = search(Tree, path);
    pthread_mutex_lock(&b->write);
    // char*path = strtok_r(str , "\n" ,&str);
    FILE *file = fopen(path, "a");
    if (file == NULL)
    {
        while (1)
        {
            REQUEST *req = (REQUEST *)malloc(sizeof(REQUEST));
            recv(sock, req, sizeof(REQUEST), 0);
            if (req->done == 1)
                break;
        }
        perror("Error opening file");
        pthread_mutex_unlock(&b->write);
        return NULL;
    }
    while (1)
    {
        REQUEST *req = (REQUEST *)malloc(sizeof(REQUEST));
        recv(sock, req, sizeof(REQUEST), 0);
        fprintf(file, "%s", req->path);
        fflush(file);
        if (req->done = 1)
        {
            char *return_value = (char *)malloc(sizeof(char) * MAX_BUFFER);
            return_value = "Data written successfully on the file.";
            pthread_mutex_unlock(&b->write);
            // printf("Hemang\n");
            return return_value;
        }
    }
}

void deleteDirectory(const char *path)
{
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;

    dir = opendir(path);
    if (dir == NULL)
    {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
        // return NULL;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char full_path[MAX_BUFFER];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        if (stat(full_path, &statbuf) == -1)
        {
            perror("Error getting file/directory information");
            exit(EXIT_FAILURE);
        }

        if (S_ISDIR(statbuf.st_mode))
        {
            deleteDirectory(full_path);
        }
        else
        {
            if (remove(full_path) != 0)
            {
                perror("Error deleting file");
                exit(EXIT_FAILURE);
            }
            printf("Deleted file: %s\n", full_path);
        }
    }

    closedir(dir);

    if (rmdir(path) != 0)
    {
        perror("Error deleting directory");
        exit(EXIT_FAILURE);
    }

    printf("Deleted directory: %s\n", path);
}

char *delete_file(char *str)
{

    struct stat file_stat;
    if (stat(str, &file_stat) == 0 && search(Tree, str) != NULL)
    {
        if (S_ISDIR(file_stat.st_mode) != 0)
        {
            deleteDirectory(str);
            char *return_value = (char *)malloc(sizeof(char) * MAX_BUFFER);
            // return_value = "Directory deleted successfully";
            snprintf(return_value, MAX_BUFFER, "%s", str);
            Blank *b = search(Tree, str);
            Tree = delete (str, Tree);
            return return_value;
        }
        else
        {
            printf("strr %s\n", str);
            if (remove(str) == 0)
            {
                char *return_value = (char *)malloc(sizeof(char) * MAX_BUFFER);
                // return_value = "File deleted successfully";
                snprintf(return_value, MAX_BUFFER, "%s", str);
                return return_value;
            }
            else
            {
                perror("Error deleting file");
                return NULL;
            }
        }
    }
    else
    {

        return NULL;
    }
}

// void *helper_copy(char *str, int *sock, char *path2)
// {
//     struct dirent **namelist;
//     int n = scandir(str, &namelist, NULL, alphasort);
//     for (int i = 0; i < n; i++)
//     {
//         if (namelist[i]->d_name[0] == '.')
//         {
//             char concatenated_string[4097];
//             snprintf(concatenated_string, sizeof(concatenated_string), "%s/%s", str, namelist[i]->d_name);
//             char final[4097];
//             snprintf(final, sizeof(final), "%s/%s", path2, namelist[i]->d_name);
//             struct stat file_stat;
//             if (stat(concatenated_string, &file_stat) == -1)
//             {
//                 continue;
//             }
//             if (S_ISDIR(file_stat.st_mode))
//             {
//                 SERVER_REQUEST *req = (SERVER_REQUEST *)malloc(sizeof(SERVER_REQUEST));
//                 req->CMD = 4;
//                 req->FLAGS = 0;
//                 strcpy(req->path, path2);
//                 while (send(*sock, req, sizeof(SERVER_REQUEST), 0) < 0)
//                 {
//                     puts("Send failed");
//                 }
//                 helper_copy(concatenated_string, sock, final);
//             }
//             else
//             {
//                 SERVER_REQUEST *req = (SERVER_REQUEST *)malloc(sizeof(SERVER_REQUEST));
//                 req->CMD = 2;
//                 req->FLAGS = 0;
//                 req->done = 0;
//                 strcpy(req->path, path2);
//                 while (send(*sock, req, sizeof(SERVER_REQUEST), 0) < 0)
//                 {
//                     puts("Send failed");
//                 }
//                 read_file_copy(concatenated_string, sock, path2);
//             }
//         }
//     }
// }

char *write_file2(char *str)
{
    char *ty; //= (char*)malloc(sizeof(char)*MAX_BUFFER);
    // s//trcpy(ty , str);
    char *token = strtok_r(str, "\n", &ty);
    FILE *file = fopen(token, "w");
    if (file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }
    fprintf(file, "%s", ty);
    fflush(file);
    // Blank*b = search(Tree , token);
    Blank *b = (Blank *)malloc(sizeof(Blank));
    pthread_mutex_init(&b->read, NULL);
    pthread_mutex_init(&b->write, NULL);
    b->readers = 0;
    Tree = insert(token, Tree, b);
    fclose(file);
    return token;
}

void append_file3(char *str, char *abc)
{

    char *ty; //= (char*)malloc(sizeof(char)*MAX_BUFFER);
    // // strcpy(ty , str);
    char *token = strtok_r(str, "\n", &ty);
    FILE *file = fopen(token, "a");
    if (file == NULL)
    {
        perror("Error opening file");
        // return NULL;
        strcpy(abc, "NULL");
        return;
    }
    fprintf(file, "%s", ty);
    fflush(file);
    fclose(file);
    if (search(Tree, token) == NULL)
    {
        if (chmod(token, S_IRWXU | S_IRWXG | S_IRWXO) != 0)
        {
            perror("Error setting permissions");
            remove(token);
            // return NULL;
            strcpy(abc, "NULL");
            return;
        }
        Blank *b = (Blank *)malloc(sizeof(Blank));
        pthread_mutex_init(&b->read, NULL);
        pthread_mutex_init(&b->write, NULL);
        b->readers = 0;
        Tree = insert(token, Tree, b);
    }
    // free(ty);
    // free(token);
    // return token;
    strcpy(abc, token);
    return;
}

char *append_file2(char *str)
{
    char *ty; //= (char*)malloc(sizeof(char)*MAX_BUFFER);
    // // strcpy(ty , str);
    char *token = strtok_r(str, "\n", &ty);
    FILE *file = fopen(token, "a");
    if (file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }
    fprintf(file, "%s", ty);
    fflush(file);
    fclose(file);
    if (search(Tree, token) == NULL)
    {
        if (chmod(token, S_IRWXU | S_IRWXG | S_IRWXO) != 0)
        {
            perror("Error setting permissions");
            remove(token);
            return NULL;
        }
        Blank *b = (Blank *)malloc(sizeof(Blank));
        pthread_mutex_init(&b->read, NULL);
        pthread_mutex_init(&b->write, NULL);
        b->readers = 0;
        Tree = insert(token, Tree, b);
    }
    // free(ty);
    // free(token);
    return token;
}

void *handle_nm_req(void *arg)
{
    // lock
    FUN for_function = *(FUN *)arg;
    int sock_fd = *(for_function.sockfd);
    SERVER_REQUEST req = for_function.req;
    // char data[MAX_BUFFER];
    // char *data = (char *)malloc(sizeof(char) * MAX_BUFFER);
    int client = req.ClientID;
    if (req.CMD == 4 && req.done == 3)
    {
        pthread_mutex_lock(&write_h);
        // SERVER_RESPONSE *res = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
        SERVER_RESPONSE res;
        char *data = (char *)malloc(sizeof(char) * MAX_BUFFER);
        // strcpy(data ,create_dir(req.path));
        data = create_dir(req.path);
        if (data == NULL)
        {
            res.Error_Code = 304;
            res.FLAGS = 0;
            res.ClientID = client;
            res.CMD = 4;
            res.done = 4;
            // if(client==-1)
            // {
            //     res.done = 421;
            // }
            // else{
            //     res.done = 4;
            // }
        }
        else
        {
            res.Error_Code = 0;
            res.ClientID = client;
            res.CMD = 4;
            res.FLAGS = 0;
            res.done = 4;
            // if(client==-1)
            // {
            //     res.done = 421;
            // }
            // else{
            //     res.done = 4;
            // }
            strcpy(res.Response_Data, data);
        }
        while (send(*sock_for_NM, &res, sizeof(SERVER_RESPONSE), 0) < 0)
        {
            puts("Send failed");
        }
        pthread_mutex_unlock(&write_h);
        free(data);
    }
    else if (req.CMD == 2 && req.done == 3)
    {
        pthread_mutex_lock(&read_h);
        readers_h++;
        if (readers_h == 1)
        {
            pthread_mutex_lock(&write_h);
        }
        pthread_mutex_unlock(&read_h);

        if (req.FLAGS == 0)
        {
            // SERVER_RESPONSE *res = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
            SERVER_RESPONSE res;
            // char *data = (char *)malloc(sizeof(char) * MAX_BUFFER);
            // printf("%s" , req.path);
            // fflush(stdout);
            // strcpy(data,append_file2(req.path));
            char data[MAX_BUFFER];
            append_file3(req.path, data);

            // if (data == NULL)
            if (strncmp(data, "NULL", 4) == 0)
            {
                res.Error_Code = 304;
                res.FLAGS = 0;
                res.ClientID = client;
                res.CMD = 2;
                res.done = 4;
                // if(client==-1)
                // {
                //     res.done = 421;
                // }
                // else{
                //     res.done = 4;
                // }
            }
            else
            {
                res.Error_Code = 0;
                res.ClientID = client;
                res.CMD = 2;
                res.FLAGS = 0;
                res.done = 4;
                // if(client==-1)
                // {
                //     res.done = 421;
                // }
                // else{
                //     res.done = 4;
                // }
                strcpy(res.Response_Data, data);
            }
            while (send(*sock_for_NM, &res, sizeof(SERVER_RESPONSE), 0) < 0)
            {
                puts("Send failed");
            }
        }
        else
        {
            // SERVER_RESPONSE *res = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
            SERVER_RESPONSE res;
            char data[MAX_BUFFER];
            append_file3(req.path, data);
            // data = append_file2(req.path);
            // if (data == NULL)
            if (strncmp(data, "NULL", 4) == 0)
            {
                res.Error_Code = 305;
                res.FLAGS = 1;
                res.ClientID = client;
                res.CMD = 2;
                res.done = 4;
                // if(client==-1)
                // {
                //     res.done = 421;
                // }
                // else{
                //     res.done = 4;
                // }
            }
            else
            {
                res.Error_Code = 0;
                res.ClientID = client;
                res.CMD = 2;
                res.FLAGS = 1;
                res.done = 4;
                // if(client==-1)
                // {
                //     res.done = 421;
                // }
                // else{
                //     res.done = 4;
                // }
                strcpy(res.Response_Data, data);
            }
            while (send(*sock_for_NM, &res, sizeof(SERVER_RESPONSE), 0) < 0)
            {
                puts("Send failed");
            }
            // free(res.
        }

        pthread_mutex_lock(&read_h);
        readers_h--;
        if (readers_h == 0)
        {
            pthread_mutex_unlock(&write_h);
        }
        pthread_mutex_unlock(&read_h);
    }
    else if (req.CMD == 4)
    {
        pthread_mutex_lock(&write_h);
        if (req.FLAGS == 1)
        {
            // SERVER_RESPONSE *res = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
            SERVER_RESPONSE res;
            char *data = (char *)malloc(sizeof(char) * MAX_BUFFER);
            data = create_file(req.path);
            // strcpy(data ,create_file(req.path));
            if (data == NULL)
            {
                res.Error_Code = 303;
                res.CMD = 4;
                res.FLAGS = 1;
                res.ClientID = client;
                // res.done = 1;
                if (client == -1)
                {
                    res.done = 420;
                }
                else
                {
                    res.done = 1;
                }
            }
            else
            {
                res.Error_Code = 0;
                strcpy(res.Response_Data, data);
                res.CMD = 4;
                res.FLAGS = 1;
                res.ClientID = client;
                // res.done = 1;
                if (client == -1)
                {
                    res.done = 420;
                }
                else
                {
                    res.done = 1;
                }
            }
            while (send(*sock_for_NM, &res, sizeof(SERVER_RESPONSE), 0) < 0)
            {
                puts("Send failed");
                // return 0;
            }
        }
        else
        {
            // SERVER_RESPONSE *res = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
            SERVER_RESPONSE res;
            char *data = (char *)malloc(sizeof(char) * MAX_BUFFER);
            data = create_dir(req.path);
            // strcpy(data ,create_dir(req.path));
            if (data == NULL)
            {
                res.Error_Code = 304;
                res.CMD = 4;
                res.FLAGS = 0;
                res.ClientID = client;
                // res.done = 1;
                if (client == -1)
                {
                    res.done = 420;
                }
                else
                {
                    res.done = 1;
                }
            }
            else
            {
                res.Error_Code = 0;
                res.ClientID = client;
                res.CMD = 4;
                res.FLAGS = 0;
                // res.done = 1;
                if (client == -1)
                {
                    res.done = 420;
                }
                else
                {
                    res.done = 1;
                }
                strcpy(res.Response_Data, data);
            }
            printf("Hemang\n");
            while (send(*sock_for_NM, &res, sizeof(SERVER_RESPONSE), 0) < 0)
            {
                puts("Send failed");
            }
        }
        // sleep(1);
        pthread_mutex_unlock(&write_h);
    }
    else if (req.CMD == 5)
    {
        pthread_mutex_lock(&write_h);
        // SERVER_RESPONSE *res = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
        SERVER_RESPONSE res;
        char *data = (char *)malloc(sizeof(char) * MAX_BUFFER);
        // printf("stHHHrr %s\n", r.->path);
        // strcpy(data , delete_file(req.path));
        data = delete_file(req.path);
        if (data == NULL)
        {
            res.CMD = req.CMD;
            res.FLAGS = req.FLAGS;
            strcpy(res.Response_Data, "File to be deleted does not exist");
            res.Error_Code = 301;
            res.ClientID = client;
            // res.done = 1;
            if (client == -1)
            {
                res.done = 420;
            }
            else
            {
                res.done = 1;
            }
        }
        else
        {
            res.CMD = req.CMD;
            res.FLAGS = req.FLAGS;
            res.Error_Code = 0;
            res.ClientID = client;
            // res.done =1;
            if (client == -1)
            {
                res.done = 420;
            }
            else
            {
                res.done = 1;
            }
            strcpy(res.Response_Data, data);
        }
        while (send(*sock_for_NM, &res, sizeof(SERVER_RESPONSE), 0) < 0)
        {
            puts("Send failed");
        }

        pthread_mutex_unlock(&write_h);
    }
    else if (req.CMD == 6)
    {
        pthread_mutex_lock(&read_h);
        readers_h++;
        if (readers_h == 1)
        {
            pthread_mutex_lock(&write_h);
        }
        pthread_mutex_unlock(&read_h);

        char *ty = (char *)malloc(sizeof(char) * MAX_BUFFER);
        strcpy(ty, req.path);
        char *token = strtok_r(ty, " ", &ty);
        copyRes(Tree, token, ty, client);
        // SERVER_RESPONSE *res = (SERVER_RESPONSE *)malloc(sizeof(SERVER_RESPONSE));
        SERVER_RESPONSE res;
        // res.done = 69;
        if (client == -1)
        {
            res.done = 96;
        }
        else
        {
            res.done = 69;
        }
        res.CMD = 7;
        res.Error_Code = 0;
        res.FLAGS = 0;
        res.ClientID = client;
        while (send(*sock_for_NM, &res, sizeof(SERVER_RESPONSE), 0) < 0)
        {
            puts("Send failed");
        }

        pthread_mutex_lock(&read_h);
        readers_h--;
        if (readers_h == 0)
        {
            pthread_mutex_unlock(&write_h);
        }
        pthread_mutex_unlock(&read_h);
    }
    // free(data);
    // free(req);
    // free(for_function);
}

void *client_request(void *arg)
{
    int sock = *(int *)arg;
    char buffer[MAX_BUFFER];
    int read_size;

    while (1)
    {

        REQUEST *req = (REQUEST *)malloc(sizeof(REQUEST));
        // int client = req->ClientID;
        // printf("MMMMKKK\n");
        read_size = recv(sock, req, sizeof(REQUEST), 0);
        // printf("MMMMKKK%d\n", read_size);
        if (read_size <= 0)
        {
            printf("Client disconnected\n");
            return NULL;
        }
        if (req->CMD == 1)
        {
            read_file(req->path, &sock);
        }
        else if (req->CMD == 2)
        {
            if (req->FLAGS == 1)
            {
                RESPONSE *res = (RESPONSE *)malloc(sizeof(RESPONSE));
                char *data = (char *)malloc(sizeof(char) * MAX_BUFFER);
                data = write_file(req->path, &sock);
                if (data == NULL)
                {
                    res->Error_Code = 302;
                    // res->ClientID = client;
                }
                else
                {
                    res->Error_Code = 0;
                    // res->ClientID = client;
                    strcpy(res->Response_Data, data);
                }
                while (send(sock, res, sizeof(RESPONSE), 0) < 0)
                {
                    puts("Send failed");
                    // return 0;
                }
            }
            else
            {
                RESPONSE *res = (RESPONSE *)malloc(sizeof(RESPONSE));
                char *data = (char *)malloc(sizeof(char) * MAX_BUFFER);
                data = append_file(req->path, &sock);
                if (data == NULL)
                {
                    res->Error_Code = 302;
                    // res->ClientID = client;
                }
                else
                {
                    res->Error_Code = 0;
                    // res->ClientID = client;
                    strcpy(res->Response_Data, data);
                }
                while (send(sock, res, sizeof(RESPONSE), 0) < 0)
                {
                    puts("Send failed");
                    // return 0;
                }
            }
        }
        else if (req->CMD == 3)
        {
            RESPONSE *res = (RESPONSE *)malloc(sizeof(RESPONSE));
            char *data = (char *)malloc(sizeof(char) * 2 * MAX_BUFFER);
            data = get_file_info(req->path);
            if (data == NULL)
            {
                res->Error_Code = 302;
                // res->ClientID = client;
            }
            else
            {
                res->Error_Code = 0;
                // res->ClientID = client;
                strcpy(res->Response_Data, data);
                res->done = 1;
            }
            while (send(sock, res, sizeof(RESPONSE), 0) < 0)
            {
                puts("Send failed");
                // return 0;
            }
        }
    }
}

void *nm_server(void *arg) // function which is called by the thread which handles the connection of storage server with the naming server
{
    int sock_fd = *(int *)arg; // Taking the argument and converting it back to socket
    SS_init *ss = (SS_init *)malloc(sizeof(SS_init));
    strcpy(ss->ip, IP_ADDRESS);
    ss->port_client = PORT_SS;
    ss->port_nm = PORT_NM;
    char *ss_dir = (char *)malloc(sizeof(char) * MAX_BUFFER);
    strcpy(ss->ss_directory, getcwd(ss_dir, MAX_BUFFER));
    int n;
    scanf("%d\n", &n);
    char *final = (char *)malloc(sizeof(char) * 10 * MAX_BUFFER);
    memset(final, '\0', 10 * MAX_BUFFER);
    for (int i = 0; i < n; i++)
    {
        char *buff = (char *)malloc(sizeof(char) * MAX_BUFFER);
        fgets(buff, MAX_BUFFER, stdin);
        buff[strlen(buff) - 1] = '\0';
        char *buf_copy = (char *)malloc(sizeof(char) * MAX_BUFFER);
        strcpy(buf_copy, buff);

        // printf("%s\n" , buf_copy);

        Blank *b = (Blank *)malloc(sizeof(Blank));
        struct stat file_stat;
        if (stat(buff, &file_stat) != -1)
        {
            if (!S_ISDIR(file_stat.st_mode))
            {
                pthread_mutex_init(&b->read, NULL);
                pthread_mutex_init(&b->write, NULL);
                b->readers = 0;
            }
        }
        Tree = insert(buff, Tree, b);
        strcat(buf_copy, "\n");
        strcat(final, buf_copy);
        // printf("%s\n" , final);
    }
    // strcat(final , "\n");
    strcpy(ss->access_paths, final);
    printf("Sending Init Packet\n");
    printf("IP:(%s)Port:(%d)\n", ss->ip, ss->port_client);
    if (send(sock_fd, ss, sizeof(SS_init), 0) < 0)
    {
        printf("Send failed");
        return 0;
    }
    FILE *file_u = fopen("log.txt", "a");
    // FUN *for_function = (FUN *)malloc(sizeof(FUN));
    FUN for_function;
    // SERVER_REQUEST*req = (SERVER_REQUEST *)malloc(sizeof(SERVER_REQUEST));
    for_function.sockfd = &sock_fd;
    SERVER_REQUEST req;
    while (1)
    {
        memset(&req, 0, sizeof(SERVER_REQUEST));
        int read_size = recv(sock_fd, &req, sizeof(SERVER_REQUEST), 0);
        for_function.req = req;
        fprintf(file_u, "CMD = %d\n FLAG = %d \n DONE = %d \n DATA = %s\n ", req.CMD, req.FLAGS, req.done, req.path);
        fflush(file_u);
        // if(req.CMD == 4)
        // {
        //     pthread_mutex_lock(&write_h);
        // }
        printf("CMD = %d\n FLAG = %d \n DONE = %d \n DATA = %s\n ", req.CMD, req.FLAGS, req.done, req.path);
        fflush(stdout);

        pthread_t thread_for_multiple_req;
        pthread_create(&thread_for_multiple_req, NULL, handle_nm_req, (void *)&for_function);
        // usleep(10);
        if (req.done == 3)
        {
            pthread_join(thread_for_multiple_req, NULL);
        }
    }
}

int main(int argc, char *argv[])
{
    PORT_SS = atoi(argv[1]);
    // PORT_SS = 9070;
    Tree = create();
    trie_readers = 0;
    readers_h = 0;
    pthread_mutex_init(&trie_read, NULL);
    pthread_mutex_init(&trie_write, NULL);
    pthread_mutex_init(&read_h, NULL);
    pthread_mutex_init(&write_h, NULL);
    PORT_NM = atoi(argv[2]); // argument which the port in which the storage server will ask the clients to connect                                                                // argument which the port in which the storage server will ask the clients to connect
    // PORT_NM = 9075;
    int sock_for_nm, sock_for_client, client_sock, *new_sock, sock, ns_sock;
    struct sockaddr_in storage_server, connect_client, client, connect_ns, ns;
    pthread_t thread_for_nm;

    if ((sock_for_nm = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Could not create socket");
    }

    storage_server.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    storage_server.sin_family = AF_INET;
    storage_server.sin_port = htons(SS_NS);

    if ((sock_for_client = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Could not create socket");
    }

    connect_client.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    connect_client.sin_family = AF_INET;
    connect_client.sin_port = htons(PORT_SS);

    while (connect(sock_for_nm, (struct sockaddr *)&storage_server, sizeof(storage_server)) < 0)
    {
        perror("connect failed. Error");
        printf("Retrying...\n");
        sleep(3);
    }

    printf("Storage server has been connected to the Naming server.\n");

    if (pthread_create(&thread_for_nm, NULL, nm_server, (void *)&sock_for_nm) < 0)
    {
        perror("could not create thread");
        return 1;
    }
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Could not create socket");
    }

    connect_ns.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    connect_ns.sin_family = AF_INET;
    connect_ns.sin_port = htons(PORT_NM);

    if (bind(sock, (struct sockaddr *)&connect_ns, sizeof(connect_ns)) < 0)
    {
        perror("bind failed");
        return 1;
    }
    listen(sock, MAX_CLIENT);
    int d = sizeof(struct sockaddr_in);
    ns_sock = accept(sock, (struct sockaddr *)&client, (socklen_t *)&d);
    sock_for_NM = (int *)malloc(sizeof(int));
    *sock_for_NM = ns_sock;

    if (bind(sock_for_client, (struct sockaddr *)&connect_client, sizeof(connect_client)) < 0)
    {
        perror("bind failed");
        return 1;
    }
    // printf("Listening for Clients\n");
    listen(sock_for_client, MAX_CLIENT);

    int c = sizeof(struct sockaddr_in);
    // printf("Port waiting for client connection : %d\n",sock_for_client);
    while ((client_sock = accept(sock_for_client, (struct sockaddr *)&client, (socklen_t *)&c)))
    {
        printf("Connection accepted Port : %d\n", sock_for_client);
        pthread_t client_thread;
        new_sock = (int *)malloc(sizeof(int));
        *new_sock = client_sock;

        if (pthread_create(&client_thread, NULL, client_request, (void *)new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
    }
    pthread_join(thread_for_nm, NULL);
}