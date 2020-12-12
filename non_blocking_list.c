/** \file non_blocking_list.c
 * Implementation of a non blocking linked list.
 */

#include "non_blocking_list.h"
#include<stdlib.h>
#include<string.h>
#include <asm-generic/errno-base.h>
#include "dymelor.h"
#include "wrappers.h"

int nblist_init(nblist* list){
	if(list!=NULL){
		//now we need to create a dummy element, to which will allow us to use the list without locks
		list->head=rsalloc(sizeof(nblist_elem));
		if(list->head==NULL){
			return ENOMEM;
		}
		list->tail=list->head;
		list->old=list->head;
		memset(list->head,0,sizeof(nblist_elem));
		list->head->type=NBLIST_DUMMY;
		list->head->key=0;
		list->epoch=0;
	}
	return NBLIST_OP_SUCCESS;
}

///This will move only the tail pointer
int nblist_add(nblist* list, void* content, double key,nblist_elem_type type){
	if(content==NULL || list==NULL){
		return ENOENT;
	}
	//we allocate the new list element
	nblist_elem *elem=rsalloc(sizeof(nblist_elem));
	if(elem==NULL){
		return ENOMEM;
	}
	elem->type=type;
	elem->content=content;
	elem->key=key;
	elem->next=NULL;
	//the old last is now connected to the new element
	list->tail->next=elem;
	//the new element becomes the last element
	list->tail=elem;
	return NBLIST_OP_SUCCESS;
}

///destroying the list will not empty the printbuffers
void nblist_clean(nblist* list,void (*dealloc)(void*)){
	if(list==NULL || list->old==list->head){
		return;
	}
	nblist_elem *elem;
	while(list->old!=list->head){
		//save the next element in a temp location
		elem=list->old;
		list->old=elem->next;
		dealloc(elem->content);
		rsfree(elem);
	}
}

void* nblist_pop(nblist* list){
	if(list==NULL || list->head==NULL || list->head == list->tail){
		return NULL;
	}
	nblist_elem* elem=NULL;
	//we skip the dummy nodes
	int found_valid_elem=0;
	while(!found_valid_elem && list->head != list->tail){
		elem=list->head;
		//we update the list;
		list->head=elem->next;
		if(elem->type!=NBLIST_DUMMY){
			found_valid_elem=1;
		}
	}
	return elem->content;
}

void nblist_merge(nblist *dest,nblist *source){
	if(dest==NULL || dest->tail==NULL || source== NULL){
		return;
	}
	dest->tail->next=source->head;
	dest->tail=source->tail;
	nblist_init(source);
}

void nblist_destroy(nblist* list,void (*dealloc)(void*)){
	if(list==NULL){
		return;
	}
	nblist_elem* elem=NULL;
	while(list->old!=NULL){
		elem=list->old;
		list->old=elem->next;
		if(elem->content!=NULL){
			dealloc(elem->content);
		}
		rsfree(elem);
	}
	list->head=NULL;
	list->old=NULL;
	list->tail=NULL;
}

void nblist_set_epoch(nblist* list, unsigned int epoch){
	list->epoch=epoch;
}

nblist_elem* nblist_peek(nblist* list){
	if(list==NULL || list->head==NULL){
		return NULL;
	}
	nblist_elem* elem=list->head;
	while(elem->next!=NULL && elem->type==NBLIST_DUMMY){
		elem=elem->next;
	}
	if(elem->type!=NBLIST_DUMMY){
		return elem;
	}else{
		return NULL;
	}
}
void nblist_print(nblist* list){
	nblist_elem* elem=list->old;
	int head,old=1,tail;
	__real_printf("OLD ->  ");
	while(elem!=NULL){
		if(elem==list->head){
			head=1;
			__real_printf("HEAD -> ");
		}
		if(elem==list->tail){
			tail=1;
			__real_printf("TAIL -> ");
		}
		if(!head && !old && !tail){
			__real_printf("        ");
		}
		__real_printf("key=%f, type:%d, next=%p, content:%p\n",elem->key,elem->type,elem->next,elem->content);
		elem=elem->next;
		old=0;
		head=0;
		tail=0;
	}
	__real_printf("LIST ENDED\n");
}