/** \file printbuffer.c
 * The implementation of the printbuffer APIs.
 */
#include <asm-generic/errno-base.h>
#include<stdlib.h>
#include<string.h>

#include"wrappers.h"
#include "iobuffer.h"

iobuffer* create_iobuffer(FILE* file, void* content, size_t content_size, double timestamp, int file_position, iobuf_operation_request operation){
	//sanity checks
	int res;
	if(file<0 || timestamp<0 || (content==NULL && content_size!=0) || (content!=NULL && content_size==0) || file_position<0){
		return NULL;
	}
	iobuffer* buf=malloc(sizeof(iobuffer));
	if(buf==NULL){
		return NULL;
	}
	memset(buf,0,sizeof(iobuffer));
	//now we populate it
	buf->operation=operation;
	buf->timestamp=timestamp;
	buf->file_position=file_position;
	buf->buffer_size=content_size;
	buf->file=file;
	buf->buffer=content;
	buf->next=NULL;

	return buf;
}

///Destroying a iobuf list element will not empty the buffer
void destroy_iobuffer(iobuffer* iobuf){
	if(iobuf==NULL){
		return;
	}
	//free and close everything related to the current buffer
	free(iobuf->buffer); //TODO check if this free is necessary
	free(iobuf);
}

int iobuffer_write(iobuffer *iobuf){
	if(iobuf==NULL){
		return ENOENT;
	}
	//we print everything using the fwrite, flushing the buffer
	int res=0;
	if(iobuf->buffer_size!=0){
		if(iobuf->file_position>=0){
			fseek(iobuf->file,iobuf->file_position,SEEK_SET);
		}
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
