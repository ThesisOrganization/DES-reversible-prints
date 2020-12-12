/** \file wrappers.c
 * This file contains the wrappers for the printf and the fprintf, which will allow us to realize the reversible output in the simulation.
 * But GCC is a very funny compiler and instead of using printf and fprints it uses puts and fwrite -.-.
 */

#include <stdarg.h>
#include <asm-generic/errno-base.h>
#include <stdio.h>
#include <errno.h>
#include <iobuffer.h>
#include <wrappers.h>
#include <queue.h>
#include <ROOT-Sim.h>
#include <events.h>
#include <core.h>
#include <dymelor.h>
#include <non_blocking_list.h>

/** \brief Initializes a window based on the message epoch.
 * \param[in] msg The message from which we get the epoch.
 * \param[in] list The list to initialize.
 * This function will initialize the window and set its epoch if it is NULL or if the window epoch does not match the message epoch.
 */
void init_window(msg_t* msg,nblist* list){
	if(list!=NULL){
		if(list->head==NULL){
			nblist_init(list);
			nblist_set_epoch(list,msg->epoch);
		}else{
			if(list->epoch!=msg->epoch){
				nblist_destroy(list,destroy_iobuffer);
				nblist_init(list);
				nblist_set_epoch(list,msg->epoch);
			}
		}
	}
}

/** \brief small utility which helps to select the nblist according to the ftell result. Additionally it will init the list according to the epoch of the message.
 * \param[in] msg event in which we must add the I/O operation.
 * \param[in] fpos return value from the ftell.
 * \param[in] err_code errno value after ftell.
 * \returns The list to be used to store the I/O operation
 */
nblist* select_and_init_window(msg_t* msg,int fpos,int err_code){
	nblist* list;
	if(fpos<0 && err_code==ESPIPE){
		list=&current_msg->io_forward_window;
	}else{
		list=&current_msg->io_reverse_window;
	}
	init_window(msg,list);
	return list;
}


/** \brief wraps the fwrite, so the I/O operation will become reversible (so it also wraps the fprintf since gcc replaces it with the fwrite)
 * Takes all the parameters of the fwrite and has the same return values of the fwrite.
 * If the file pointer is seekable, the operation is stored in a buffer and delayed until the vent collection; otherwise the operation will be executed but a backup a the overwritten portion is taken so we can restore it in case of rollback.
 */
size_t __wrap_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream){
	if(LPS[current_lp]->state==LP_STATE_ROLLBACK){
		return size*nmemb;
	}
	int op_res,res;
	int fpos=0;
	void* tmp=NULL;
	iobuffer* buf;
	nblist* list=NULL;
	//we check if the file is seekable
	fpos=ftell(stream);
	list=select_and_init_window(current_msg,fpos,errno);
	if(fpos>=0){/*
		///If ftell is successful then we take a backup (to be restored in case of rollback).
		tmp=rsalloc(sizeof(size*nmemb));
		memset(tmp,'\0',size*nmemb);
		if(tmp==NULL){
			errno=ENOMEM;
			return 0;
		}
		fread(tmp,size,nmemb,stream);
		//we ignore the eof error
		res=feof(stream);
		if(res==0){
			//but we can't ignore a generic error
			res=ferror(stream);
			if(res!=0){
				errno=res;
				return 0;
			}
		}
		//we restore the original file position
		res=fseek(stream,fpos,SEEK_SET);
		if(res<0){
			return 0;
		}
		//we do the fwrite
		op_res=__real_fwrite(ptr,size,nmemb,stream);*/
		//to temporarily avoid buffers creation for seekable files
		fpos=-1;
	}else{
		//we create the iobuffer and add it to the list
		tmp=ptr;
		op_res=size*nmemb;
	}
	buf=create_iobuffer(stream,tmp,size,nmemb,current_lvt,fpos,IOBUF_FWRITE);
	if(buf==NULL){
		errno=ENOMEM;
		return 0;
	}
	res=nblist_add(list,buf,current_lvt,NBLIST_ELEM);
	if(res!=NBLIST_OP_SUCCESS){
		errno=res;
		return 0;
	}
	return op_res;
}

/** \brief This wrapper wraps the puts to out (and the printfs since gcc replaces them with puts) and redirects them to fwrite wrapper.
 * Behaves like the stdlib puts.
 */
int __wrap_puts(const char *s){
	int res;
	//we get the number of chars that should be written
	int size=snprintf(NULL,0,"%s\n",s)+1;
	if(LPS[current_lp]->state==LP_STATE_ROLLBACK){
		return size;
	}
	res=__wrap_fwrite(s,sizeof(char),size,stdout);
	return res;
}


int __wrap_printf(const char * format, ...){
	int res;
	va_list args;
	int len=0;
	char* string=NULL;
	va_start(args,format);
	len=vsnprintf(NULL,0,format,args)+1;
	va_end(args);
	if(LPS[current_lp]->state==LP_STATE_ROLLBACK){
		return len;
	}
	string=rsalloc(sizeof(char)*len);
	memset(string,'\0',len);
	va_start(args,format);
	vsnprintf(string,len,format,args);
	va_end(args);
	res=__wrap_fwrite(string,sizeof(char),len,stdout);
	return res;
}

/** We need to wrap the fclose since the model cannot close the file in an event that could be discarded.
 * Regardless of the file type the operation will go in the event io forward window.
 * Behaves like the fclose.
*/
int __wrap_fclose(FILE* stream){
	if(LPS[current_lp]->state==LP_STATE_ROLLBACK){
		return 0;
	}
	init_window(current_msg,&current_msg->io_forward_window);
	int res;
	iobuffer* buf=create_iobuffer(stream,NULL,0,0,current_lvt,0,IOBUF_FCLOSE);
	if(buf!=NULL){
		errno=ENOMEM;
		return EOF;
	}
	res=nblist_add(&current_msg->io_forward_window,buf,buf->timestamp,NBLIST_ELEM);
	if(res!=NBLIST_OP_SUCCESS){
		errno=res;
		return EOF;
	}
	return 0;
}
