#ifndef __MY_LL_H
#define __MY_LL_H

#include<stdio.h>
#include<stdlib.h>

struct linenode {
    char *line;
    struct linenode *next;
};


struct linenode *my_ll_insert_begin(struct linenode *head,char *line);   //inserts element in linked list
struct linenode *my_ll_append(struct linenode *head,char *line);    //insert at the last of linked list
char *my_ll_get_first(struct linenode *head);
void *my_ll_free(struct linenode *head);

#define my_ll_foreach(pos, head) for(pos = (head); pos != NULL; pos = pos->next)

#endif