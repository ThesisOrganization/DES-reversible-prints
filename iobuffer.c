/** \file printbuffer.c
 * The implementation of the printbuffer APIs.
 */
#include "iobuffer.h"
#include <asm-generic/errno-base.h>
#include <stdlib.h>
#include <string.h>
#include"wrappers.h"

iobuffer* create_iobuffer(FILE* file, void* content, size_t content_size, simtime_t timestamp){
	//sanity checks
	int res;
	if(file<0 || timestamp<0 || (content==NULL && content_size!=0) || (content!=NULL && content_size==0)){
		return NULL;
	}
	iobuffer* buf=malloc(sizeof(iobuffer));
	if(buf==NULL){
		return NULL;
	}
	memset(buf,0,sizeof(iobuffer));
	//now we populate it
	buf->fstat=FCLOSE_NOT_REQUESTED;
	buf->timestamp=timestamp;
	buf->file_position=ftell(file);
	if(buf->file_position<0 && buf->file_position!=ESPIPE){
		res=buf->file_position;
		free(buf);
		return NULL;
	}
	buf->buffer_size=content_size;
	buf->file=file;
	buf->buffer=content;

	return buf;
}

int add_iobuffer(iobuf_list* list, iobuffer* buffer){
	if(buffer==NULL || list==NULL){
		return ENOENT;
	}
	iobuf_list_elem* it=list->first;
	//we allocate the new iobuffer
	iobuf_list_elem *elem=malloc(sizeof(iobuf_list_elem));
	if(elem==NULL){
		return ENOMEM;
	}
	elem->iobuf=buffer;
	//we need to insert the first element of the list
	if(it==NULL){
		list->first=elem;
		list->last=elem;
	} else {
		//the old last is now connected to the new iobuffer
		list->last->next=elem;
		//the new iobuffer becomes the last element
		list->last=elem;
	}
	return IOBUF_OP_SUCCESS;
}

///Destroying a iobuf list element will not empty the buffer
void destroy_iobuf_list_elem(iobuf_list_elem* elem){
	if(elem==NULL){
		return;
	}
	//free and close everything related to the current buffer
	free(elem->iobuf->buffer);
	if(elem->iobuf->fstat!=FCLOSE_DONE){
		__real_fclose(elem->iobuf->file);
	}
	free(elem->iobuf);
	free(elem);
}

///destroying the list will not empty the printbuffers
void destroy_iobuf_list(iobuf_list** list){
	if(*list==NULL){
		return;
	}
	iobuf_list_elem *it=(*list)->first,*tmp;
	while(it!=NULL){
		//save the next element in a temp location
		tmp=it->next;
		destroy_iobuf_list_elem(it);
		it=tmp;
	}
	*list=NULL;
}

int iobuffer_write(iobuffer *iobuf){
	if(iobuf==NULL){
		return ENOENT;
	}
	//we print everything using the fwrite, flushing the buffer
	int res=0;
	if(iobuf->buffer_size!=0){
		res=__real_fwrite(iobuf->buffer,iobuf->buffer_size,1,iobuf->file);
		if(res<0){
			return res;
		}
		fflush(iobuf->file);
	}
	//if needed we close the associated file
	if(iobuf->fstat==FCLOSE_REQUESTED){
		res=__real_fclose(iobuf->file);
		if(res<0){
			return res;
		}
	}
	return IOBUF_OP_SUCCESS;
}

iobuffer* iobuf_list_pop(iobuf_list* list){
	if(list==NULL){
		return NULL;
	}
	iobuf_list_elem* elem;
	iobuffer* buffer=NULL;
	elem=list->first;
	if(elem==NULL){
		return NULL;
	}
	//we update the list;
	list->first=elem->next;
	//we can now free the list element and return only the iobuffer
	buffer=elem->iobuf;
	free(elem);
	return buffer;
}
