#include "header.h"

Trie* create()
{
    Trie* y = (Trie*)malloc(sizeof(Trie));
    y->next = NULL;
    Trie*head =(Trie*)malloc(sizeof(Trie));
    head->next = NULL;
    y->child = head;
    return y;
}


Blank* search (Trie*F, char*str)
{
    Trie*T = F->child;
    Blank*b = NULL;
    while(1)
    {
        char* token = strtok_r(str , "/" , &str);
        if(token==NULL)
        {
            break;
        }
        Trie* i = T->next;
        Trie* prev_i = T;
        int flag = 0;
        while(i!=NULL)
        {
            if(strcmp(i->string , token)==0)
            {
                flag=1;
                b = i->b;
                T = i->child;
            }
            prev_i=i;
            i=i->next;
        }
        if(flag==0)
        {
            return NULL;
        }
    }
    return b;
}

Trie* insert(char* str , Trie*F , Blank*b)
{
    Trie*T = F->child;
    while(1)
    {
        if(str==NULL)
        {
            return NULL;
        }
        char* token = strtok_r(str , "/" , &str);
        if(token==NULL)
        {
            break;
        }
        Trie* i = T->next;
        Trie* prev_i = T;
        int flag = 0;
        while(i!=NULL)
        {
            if(strcmp(i->string , token)==0)
            {
                flag=1;
                T = i->child;
                break;
            }
            prev_i=i;
            i=i->next;
        }
        if(flag==0)
        {
            Trie* y  = create();
            prev_i->next = y;
            strcpy(y->string , token);
            y->b = b;
            T = y->child;
        }
    }
    return F;
}

int main()
{
    Trie*T = create();
    Blank*b = (Blank*)malloc(sizeof(Blank));
    char str[] = "mount/hemang/mohak/prakhar";
    char* h = (char*)malloc(sizeof(char)*100);
    strcpy(h , str);
    T = insert(str , T , b);
    printf("%d" , search(T , str));
}