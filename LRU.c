#include "header.h"

LRU LRU_init()
{
    LRU lru = (LRU)malloc(sizeof(struct LRU));
    lru->head = (LL)malloc(sizeof(struct LLnode));
    lru->tail = (LL)malloc(sizeof(struct LLnode));
    lru->num_nodes = 0;
    lru->head->next = lru->tail;
    lru->tail->prev = lru->head;
    lru->head->prev = NULL;
    lru->tail->next = NULL;
    lru->head->key[0] = '\0';
    lru->tail->key[0] = '\0';
    return lru;
}

char* LRU_get(LRU L, char* key)
{
    LL node = L->head->next;
    while(node != L->tail)
    {
        if(strcmp(node->key, key) == 0)
        {
            //move node to front
            node->prev->next = node->next;
            node->next->prev = node->prev;
            node->next = L->head->next;
            node->prev = L->head;
            L->head->next->prev = node;
            L->head->next = node;
            return node->data;
        }
        node = node->next;
    }
    return NULL;
}

void LRU_put(LRU L, char* key, char* data)
{
    char* tmp = LRU_get(L, key);
    if(tmp!=NULL)
    return;
    
    //node not found
    LL new_node = (LL)malloc(sizeof(struct LLnode));
    strcpy(new_node->key, key);
    strcpy(new_node->data, data);
    new_node->next = L->head->next;
    new_node->prev = L->head;
    L->head->next->prev = new_node;
    L->head->next = new_node;
    L->num_nodes++;
    if(L->num_nodes > LRU_CAPACITY)
    {
        LL temp = L->tail->prev;
        temp->prev->next = L->tail;
        L->tail->prev = temp->prev;
        free(temp);
        L->num_nodes--;
    }
}

void LRU_print(LRU L)
{
    LL node = L->head->next;
    while(node != L->tail)
    {
        printf("  %s->%s  ", node->key, node->data);
        node = node->next;
    }
    printf("\n");
}

void LRU_free(LRU L)
{
    LL node = L->head->next;
    while(node != L->tail)
    {
        LL temp = node;
        node = node->next;
        free(temp);
    }
    free(L->head);
    free(L->tail);
    free(L);
}

int main()
{
    char a[10] = "aa";
    char b[10] = "bb";
    char c[10] = "cc";
    char d[10] = "dd";
    char e[10] = "ee";
    char f[10] = "ff";
    char g[10] = "gg";

    LRU lru = LRU_init();
    LRU_put(lru, a, b);
    LRU_print(lru);
    LRU_put(lru, c, d);
    LRU_put(lru, e, f);
    LRU_print(lru);

    char* tmp = LRU_get(lru, a);
    if(tmp==NULL)
    {
        printf("DNE\n");
    }else
    {
        printf("%s\n", tmp);
    }
    LRU_print(lru);

    LRU_put(lru, g, b);

    LRU_print(lru);
}
