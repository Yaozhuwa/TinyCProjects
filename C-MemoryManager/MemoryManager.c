#include <stdio.h>
#include <stdlib.h>
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})
#define MAX_NODE_NUM 5

typedef struct _node{
    struct _node *pre;
    struct _node *next;
}Node;

typedef struct _mem{
    void *addr;
    unsigned int size;
    Node node;
}ManagerNode;

typedef struct _list{
    unsigned int length;
    Node *head;
    Node *tail;
}List;

List ListFree = {0, NULL, NULL};
List ListUnAlloc = {0, NULL, NULL};
List ListAlloc = {0, NULL, NULL};
void *UsrMemAddr = NULL;

void initNode2Null(Node * node){
    node->pre = NULL;
    node->next = NULL;
}
void initList(List *list){
    list->length = 0;
    list->head = NULL;
    list->tail = NULL;
}
void addNode2Tail(List * mList, Node *mNode){
    initNode2Null(mNode);
    if(NULL==mList->head){
        mList->head = mNode;
        mList->tail = mNode;
    }else{
        mList->tail->next = mNode;
        mNode->pre = mList->tail;
        mList->tail = mNode;
    }
    mList->length ++;
}
void addNode2Head(List *mList, Node *mNode){
    initNode2Null(mNode);
    if(NULL==mList->head){
        mList->head = mNode;
        mList->tail = mNode;
    }else{
        mNode->next = mList->head;
        mList->head->pre = mNode;
        mList->head = mNode;
    }
    mList->length ++;
}

// 从List中剥离一个已知节点
void detachNode(List *mList, Node *mNode){
    if (mNode == mList->head){
        if (mNode == mList->tail){
            mList->head = NULL;
            mList->tail = NULL;
        }else{
            mList->head = mNode->next;
            mList->head->pre = NULL;
        }
    }else{
        if (mNode == mList->tail){
            mList->tail = mNode->pre;
            mList->tail->next = NULL;
        }else{
            mNode->pre->next = mNode->next;
            mNode->next->pre = mNode->pre;
        }
    }
    mList->length --;
}

void addNodeBeforeNode(List *mList, Node *insertNode, Node *positionNode){
    insertNode->pre = positionNode->pre;
    insertNode->next = positionNode;
    positionNode->pre->next = insertNode;
    positionNode->pre = insertNode;
    mList->length ++;
}

void mergeNode2UnAlloc(Node *mNode){
    Node * pNode=NULL;
    ManagerNode *pManagerNode=container_of(mNode, ManagerNode, node);
    ManagerNode *pTempMNode=NULL, *pTempMNodePre=NULL;
    void *addr = pManagerNode->addr;
    unsigned int size = pManagerNode->size;


    for(pNode=ListUnAlloc.head; pNode!=NULL;pNode=pNode->next){
        pTempMNode = container_of(pNode, ManagerNode, node);
        if ((char *)addr < (char *)(pTempMNode->addr)){
            // 1. 如果插入点在头节点前 2. 插入点在头节点到尾节点之间
            if (pNode == ListUnAlloc.head){
                // 1. 如果可以合并 2. 不能合并
                if ((char *)addr + size == (char *)(pTempMNode->addr)){
                    pTempMNode->addr = addr;
                    pTempMNode->size += size;
                    addNode2Tail(&ListFree, mNode);
                }
                else{
                    addNode2Head(&ListUnAlloc, mNode);
                }
            }
            else{
                pTempMNodePre = container_of(pNode->pre, ManagerNode, node);
                if((char *)(pTempMNodePre->addr)+pTempMNodePre->size == addr && \
                (char *)addr+size == pTempMNode->addr) {
                    pTempMNodePre->size += size + pTempMNode->size;
                    pTempMNode->size = 0;
                    pTempMNode->addr = NULL;
                    detachNode(&ListUnAlloc, pNode);
                    addNode2Tail(&ListFree, pNode);
                    addNode2Tail(&ListFree, mNode);
                }
                else if((char *)(pTempMNodePre->addr)+pTempMNodePre->size != addr&& \
                (char *)addr+size != pTempMNode->addr){
                    addNodeBeforeNode(&ListUnAlloc, mNode, pNode);
                }
                else if((char *)(pTempMNodePre->addr)+pTempMNodePre->size == addr&& \
                (char *)addr+size != pTempMNode->addr){
                    pTempMNodePre->size += size;
                    addNode2Tail(&ListFree, mNode);
                }
                else if((char *)(pTempMNodePre->addr)+pTempMNodePre->size != addr&& \
                (char *)addr+size == pTempMNode->addr){
                    pTempMNode->addr = addr;
                    pTempMNode->size += size;
                    addNode2Tail(&ListFree, mNode);
                }
            }
            return;
        }
    }
    // 插入点在尾节点后
    pNode = ListUnAlloc.tail;
    pTempMNode = container_of(pNode, ManagerNode, node);
    // 1. 可以和尾节点合并 2. 不能和尾节点合并
    if ((char *)pTempMNode->addr + pTempMNode->size == addr){
        pTempMNode->size += size;
        addNode2Tail(&ListFree, mNode);
    }else{
        addNode2Tail(&ListUnAlloc, mNode);
    }
    return;
}

ManagerNode * createManagerNode(){
    ManagerNode * mNode = (ManagerNode *)malloc(sizeof(ManagerNode));
    if (NULL == mNode){
        return NULL;
    }
    mNode->addr = NULL;
    mNode->size = 0;
    initNode2Null(&(mNode->node));
    return mNode;
}

// 从空闲节点ListFree尾节点中取出一个Node节点断开并返回
Node * getNodeFromFreeList(){
    Node *p = NULL;
    if (ListFree.length == 0){
        printf("Error: 空闲节点不足，内存碎片超限！\n");
        return NULL;
    }else if(ListFree.length == 1){
        p = ListFree.head;
        ListFree.head = NULL;
        ListFree.tail = NULL;
    }else{
        p = ListFree.tail;
        ListFree.tail = p->pre;
        ListFree.tail->next = NULL;
    }
    ListFree.length --;
    return p;

}
int yammInit(unsigned int size){
    int i = 0;
    UsrMemAddr = malloc(size);
    if (NULL == UsrMemAddr){
        printf("Error: 内存管理器初始化失败: 用户内存分配失败!\n");
        return -1;
    }
    initList(&ListFree);
    initList(&ListUnAlloc);
    initList(&ListAlloc);

    ManagerNode *managerNode = createManagerNode();
    if (NULL == managerNode){
        printf("Error: 内存管理器初始化失败: 创建管理节点失败-(1)！\n");
        return -1;
    }
    managerNode->addr = UsrMemAddr;
    managerNode->size = size;
    addNode2Tail(&ListUnAlloc, &(managerNode->node));

    for(i=1;i<MAX_NODE_NUM;++i){
        managerNode = createManagerNode();
        if (NULL == managerNode){
            printf("Error: 内存管理器初始化失败: 创建管理节点失败-(2)！\n");
            return -1;
        }
        addNode2Tail(&ListFree, &(managerNode->node));
    }
    return 0;
}

void * yammAlloc(unsigned int size){
    Node *pNode=NULL, *pTempNode=NULL;
    ManagerNode *pManagerNode = NULL, *pTempManagerNode=NULL;
    if (0==ListUnAlloc.length){
        printf("yammAlloc() Failed: 没有未分配的内存了！");
        return NULL;
    }
    for (pNode=ListUnAlloc.head; pNode != NULL; pNode=pNode->next){
        pManagerNode = container_of(pNode, ManagerNode, node);
        if(size == pManagerNode->size){
            detachNode(&ListUnAlloc, pNode);
            addNode2Tail(&ListAlloc, pNode);
            return pManagerNode->addr;
        }
        else if(size < pManagerNode->size){
            pTempNode = getNodeFromFreeList();
            if (NULL==pTempNode){
                printf("Error: yammAlloc() Failed: 内存分配失败！\n");
                return NULL;
            }
            addNode2Tail(&ListAlloc, pTempNode);
            pTempManagerNode = container_of(pTempNode, ManagerNode, node);
            pTempManagerNode->addr = pManagerNode->addr;
            pTempManagerNode->size = size;

            pManagerNode->addr = (char *)(pManagerNode->addr) + size;
            pManagerNode->size -= size;

            return pTempManagerNode->addr;
        }
    }
    printf("Error: yammAlloc() Failed: 找不到满足要求大小的未分配内存块!");
    return NULL;
}

void yammFree(void *ptr){
    Node *pNode=NULL;
    ManagerNode *pManagerNode=NULL;
    unsigned int size=0;

    for (pNode=ListAlloc.head; pNode!=NULL; pNode=pNode->next){
        pManagerNode = container_of(pNode, ManagerNode, node);
        if (ptr == pManagerNode->addr){
            detachNode(&ListAlloc, pNode);
            mergeNode2UnAlloc(pNode);
            return;
        }
    }
    // 没有找到分配该地址的内存块
    printf("Error: yammFree() Failed, 没有找到分配地址 %p 的内存块\n", ptr);
    return;
}

void destroyList(List *mList){
    Node *pNode=mList->head;
    ManagerNode *pMNode=NULL;
    if (mList->length == 0){
        return;
    }else if(mList->length == 1){
        pMNode = container_of(pNode, ManagerNode, node);
        free(pMNode);
        mList->length --;
        return;
    }
    pNode = pNode->next;
    while(pNode){
        pMNode = container_of(pNode->pre, ManagerNode, node);
        free(pMNode);
        pMNode = NULL;
        mList->length --;
        if(pNode == mList->tail){
            pMNode = container_of(pNode, ManagerNode, node);
            free(pMNode);
            mList->head = NULL;
            mList->tail = NULL;
            mList->length --;
            if(mList->length != 0){
                printf("Error: destroyList() Failed: XXX!\n");
            }
            return;
        }
        pNode = pNode->next;
    }
}

void yammDestroy(void){
    if (NULL == UsrMemAddr){
        printf("Error: yammDestroy() Failed! yamm 还未初始化！\n");
        return;
    }
    destroyList(&ListAlloc);
    destroyList(&ListUnAlloc);
    destroyList(&ListFree);
    free(UsrMemAddr);
    UsrMemAddr = NULL;

}

void yammdump(){
    Node *p = NULL;
    ManagerNode *pMNode = NULL;
    if(NULL==UsrMemAddr){
        printf("Error: yammdump() Failed! yamm 还未初始化！\n");
        return;
    }
    printf("usedMemBlocks: %d blocks\n", ListAlloc.length);
    for(p=ListAlloc.head; p!=NULL; p=p->next){
        pMNode = container_of(p, ManagerNode, node);
        printf("addr: %p, size: %#X\n", pMNode->addr, pMNode->size);
    }
    printf("------------------------------------------------------------\n");
    printf("freeMemBlocks: %d blocks\n", ListUnAlloc.length);
    for(p=ListUnAlloc.head; p!=NULL; p=p->next){
        pMNode = container_of(p, ManagerNode, node);
        printf("addr: %p, size: %#X\n", pMNode->addr, pMNode->size);
    }
    printf("\n");

}
int main() {
    void *Mem[5]={0};
    yammInit(0x10000);
    yammdump();

    Mem[0] = yammAlloc(0x100);
    Mem[1] = yammAlloc(0x100);
    Mem[2] = yammAlloc(0x100);
    Mem[3] = yammAlloc(0x100);
    Mem[4] = yammAlloc(0x100);
    yammdump();
    yammFree(Mem[1]);
    yammFree(Mem[2]);
    Mem[2] = yammAlloc(0x300);
    yammdump();
    yammDestroy();
    return 0;
}
