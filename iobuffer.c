/** \file printbuffer.c
 * The implementation of the printbuffer APIs.
 */
#include <asm-generic/errno-base.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>

#include "iobuffer.h"
#include "dymelor.h"
#include "wrappers.h"

iobuffer* create_iobuffer(FILE* file, void* content, size_t element_size,size_t element_num, double timestamp, int file_position, iobuf_operation_request operation){
	//sanity checks
	if(file==NULL || timestamp<0 || (content==NULL && (element_num!=0 || element_size!=0)) || (content==NULL && (element_num==0 || element_size==0))){
		return NULL;
	}
	iobuffer* buf=rsalloc(sizeof(iobuffer));
	if(buf==NULL){
		return NULL;
	}
	memset(buf,0,sizeof(iobuffer));
	//now we populate it
	buf->operation=operation;
	buf->timestamp=timestamp;
	buf->file_position=file_position;
	buf->buffer_elements_size=element_size;
	buf->buffer_elements_num=element_num;
	buf->file=file;
	buf->buffer=content;

	return buf;
}

///Destroying a iobuf list element will not empty the buffer
void destroy_iobuffer(void* iobuf){
	if(iobuf==NULL){
		return;
	}
	iobuffer* buf=(iobuffer*) iobuf;
	//free and close everything related to the current buffer
	rsfree(buf->buffer); //TODO check if this free is necessary
	rsfree(iobuf);
}

int iobuffer_write(iobuffer *iobuf){
	if(iobuf==NULL){
		return ENOENT;
	}
	//we print everything using the fwrite, flushing the buffer
	int res=0;
	if(iobuf->buffer_elements_num>0 && iobuf->buffer_elements_size>0 && iobuf->operation==IOBUF_FWRITE){
		if(iobuf->file_position>=0){
			fseek(iobuf->file,iobuf->file_position,SEEK_SET);
		}
		res=__real_fwrite(iobuf->buffer,iobuf->buffer_elements_size,iobuf->buffer_elements_num,iobuf->file);
		if(res<0){
			return res;
		}
		fflush(iobuf->file);
	}
	//if needed we close the associated file
	if(iobuf->operation==IOBUF_FCLOSE){
		res=__real_fclose(iobuf->file);
		if(res<0){
			return res;
		}
	}
	return IOBUF_OP_SUCCESS;
}
