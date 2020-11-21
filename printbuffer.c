/** \file printbuffer.c
 * The implementation of the printbuffer APIs.
 */
#include "printbuffer.h"
#include "wrappers.h"
#include <asm-generic/errno-base.h>
#include <stdlib.h>
#include <string.h>

int add_printbuffer(pbuf_list **list, FILE* file, pbuf_policy policy){
	//sanity checks
	if(file<0 || list==NULL){
		return ENOENT;
	}
	pbuf_list* head=*list;
	//we allocate the new printbuffer
	pbuf_list *elem=malloc(sizeof(pbuf_list));
	memset(elem,0,sizeof(pbuf_list));
	//now we populate it
	elem->pbuf.fstat=FCLOSE_NOT_REQUESTED;
	elem->pbuf.buffer_size=PBUF_LEN;
	elem->pbuf.file=file;
	elem->pbuf.policy=policy;
	elem->pbuf.buffer=malloc(sizeof(char)*PBUF_LEN);
	if(elem->pbuf.buffer==NULL){
		return ENOMEM;
	}
	elem->pbuf.buf_written_num=0;
	memset(elem->pbuf.buffer,'\0',sizeof(char)*PBUF_LEN);
	//we need to insert the first element of the list
	if(head==NULL){
		*list=elem;
	} else {
		// since the list is circular we just need to check the prev component.
		if(head->prev==NULL){
			//we need to insert the 2nd element
			elem->next=head;
			elem->prev=head;
			head->prev=elem;
			head->next=elem;
		}else{
			//the last elem becomes the prev of the new last.
			elem->prev=head->prev;
			//the next of the new last is the head
			elem->next=head;
			//the next of the old last is the new last
			head->prev->next=elem;
			//the prev of the head is the last elem
			head->prev=elem;
		}
	}
	return PBUF_OP_SUCCESS;
}

///Destroying a pbuf list element will not empty the buffer
void destroy_pbuf_list_elem(pbuf_list* elem){
	if(elem==NULL){
		return;
	}
	//free and close everything related to the current buffer
	free(elem->pbuf.buffer);
	if(elem->pbuf.fstat!=FCLOSE_DONE){
		fclose(elem->pbuf.file);
	}
	free(elem);
}

///destroying the list will not empty the printbuffers
void destroy_pbuf_list(pbuf_list** list){
	if(list==NULL){
		return;
	}
	pbuf_list *head=*list,*it=(*list)->next,*tmp;
	while(it->next!=head){
		//save the next element in a temp location
		tmp=it->next;
		destroy_pbuf_list_elem(it);
		it=tmp;
	}
}


printbuffer* search_pbuf_list(FILE* file, pbuf_list *list){
	pbuf_list* it=list;
	//we navigate the list
	while(it->next!=list){
		//if we have found the matching file we return the printbuffer
		if(it->pbuf.file==file){
			return &(it->pbuf);
		}
		it=it->next;
	}
	return NULL;
}


int printbuffer_grow(printbuffer* pbuf){
	if(pbuf==NULL){
		return ENOENT;
	}
	//we need choose what to do based on the policy
	switch(pbuf->policy){
		case PBUF_GROW:
			pbuf->buffer=realloc(pbuf->buffer,sizeof(char)*(PBUF_GROW_LEN+pbuf->buffer_size));
			if(pbuf->buffer==NULL){
				return ENOMEM;
			}
			//we set to '\0' the new portion of the buffer
			memset((pbuf->buffer)+pbuf->buffer_size+1,'\0',PBUF_GROW_LEN);
			break;
		case PBUF_OVERWRITE:
			pbuf->buf_written_num=0;
			break;
		default:
			return EPERM;
	}
	return PBUF_OP_SUCCESS;
}


int prinbuffer_empty(printbuffer *pbuf){
	if(pbuf==NULL){
		return ENOENT;
	}
	//we print everything using the fprintf, flushing the buffer
	int res=0;
	res=fprintf(pbuf->file,"%s",pbuf->buffer);
	if(res<0){
		return res;
	}
	fflush(pbuf->file);
	//we reset the buffer to '\0'
	memset(pbuf->buffer,'\0',pbuf->buffer_size);
	//then we reset the buffer pointer
	pbuf->buf_written_num=0;
	return PBUF_OP_SUCCESS;
}

int printbuffer_write(printbuffer* pbuf,const char* format,...){
	//we use a little trick to get the length of the formatted string
	int res;
	va_list args;
	va_start(args,format);
	size_t len=snprintf(NULL,0,format,args);

	//we check if the string that we must write can fit into the buffer
	while(len>pbuf->buffer_size - pbuf->buf_written_num){
		//we grow the buffer until the string fits
		res=printbuffer_grow(pbuf);
		if(res<PBUF_OP_SUCCESS){
			return res;
		}
	}
	pbuf->buf_written_num+= snprintf(pbuf->buffer,pbuf->buffer_size-pbuf->buf_written_num,format,args);
	va_end(args);
	return PBUF_OP_SUCCESS;
}
