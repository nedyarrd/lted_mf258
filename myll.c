#include "myll.h"

struct linenode *my_ll_insert_begin(struct linenode *head,char *line)   //inserts element in linked list
{
    struct linenode *new;
    new=(struct linenode*)malloc(sizeof(struct linenode));    // make room for new node
    new->line=line;       // insert value in new node
    new->next=head;        // add adres of previously new node to first element, can be NULL
    head=new;              // make our node first node
    return head;
}


struct linenode *my_ll_append(struct linenode *head,char *line)    //insert at the last of linked list
{
    struct linenode *new, *tmp;
    if (head == NULL) 
	return my_ll_insert_begin(head,line);

    new = (struct linenode*)malloc(sizeof(struct linenode));
    if(new == NULL){
        syslog(LOG_MAKEPRI(LOG_DAEMON,LOG_ERR),"Unable to allocate memory.");
        exit(-1);
    }
    else
    {
        new->line = line;
        new->next = NULL;
        tmp = head;
        while(tmp->next != NULL)
            tmp = tmp->next;
        tmp->next = new;
    }
    return head;
}

char *my_ll_get_first(struct linenode *head)
    {
    return head->line;
    }

char *my_ll_get_index(struct linenode *head,int index)
	{
	int i = index;
	struct linenode *tmp = head;
	while(i>0)
		if (tmp->next != NULL)
			{
			tmp = tmp->next;
			i--;
			}
		else 
			{
				return NULL;
			}
	return tmp->line;
			
	}

void *my_ll_free(struct linenode *head)
    {
    struct linenode *tmp = head;
    struct linenode *tmp2;
    if (head == NULL)
	{
	free(head);
	return NULL;
	}
    while(tmp->next != NULL)
	{
	tmp2 = tmp->next;
	free(tmp);
	tmp = tmp2;
	}
    free(tmp);
    }