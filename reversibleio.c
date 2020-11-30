/** \file reversibleio.c
 * This file will contain the API that will use the iobuffers and the wrappers to make I/O operations reversible */

#include "iobuffer.h"
#include "wrappers.h"
#include "io_heap.h"
#include "core.h"
#include "list.h"
#include "dymelor.h"

///We have one heap for the unseekable
io_heap *io_h;
double* per_lp_horizon;

void reversibleio_init(){
	int i;
	//we create the heap
	per_lp_horizon=rsalloc(sizeof(double)*n_prc_tot);
	memset(per_lp_horizon,-1,sizeof(double)*n_prc_tot);
	io_h=io_heap_new(MIN_HEAP,n_prc_tot);
	//we initialize the windows in each LP.
	for(i=0;i<n_prc_tot;i++){
		nblist_init(&LPS[i]->io_forward_window);
		nblist_init(&LPS[i]->io_reverse_window);
		//during the initialization we populate the heap with the forward windows
		io_heap_add(io_h,&LPS[i]->io_forward_window);
	}
}

void reversibleio_collect(int lp,double event_horizon){
	//we save the new event horizon for the current lp
	per_lp_horizon[lp]=event_horizon;
	//we start reading the event list to get the events inside the global window.
	msg_t *msg=list_head(LPS[lp]->queue_in);
	//now we extract the I/O operations from the heap according to their timestamp and execute them
	while(msg->timestamp<event_horizon){
		nblist_merge(&LPS[lp]->io_forward_window,&msg->io_forward_window);
		msg=list_next(LPS[lp]->queue_in);
	}
}

void reversibleio_rollback(){

}

void reversibleio_execute(){
	//we need to compute the minimum event horizon
	int i,event_horizon;
	event_horizon=per_lp_horizon[0];
	for(i=0;i<n_prc_tot;i++){
		if(per_lp_horizon[i]<event_horizon){
			event_horizon=per_lp_horizon[i];
		}
	}
	double timestamp=io_heap_peek(io_h);
	iobuffer* buf;
	while(timestamp<event_horizon){
		buf=(iobuffer*) io_heap_poll(io_h)->content;
		switch(buf->operation){
			case IOBUF_FWRITE:
				fseek(buf->file,buf->file_position,SEEK_SET);
				__real_fwrite(buf->buffer,buf->buffer_size,1,buf->file);
				break;
			case IOBUF_FCLOSE:
				__real_fclose(buf->file);
				break;
		}
	}
}

///To flush all the queues we insert a dummy node in each of the and we retry extracting
void reversibleio_flush(){
	int i;
	double max_timestamp=0,tmp;
	for(i=0;i<n_prc_tot;i++){
		//we need to insert a dummy an element that is at least later than the current tail
		tmp=LPS[i]->io_forward_window.tail->key;
		if(tmp>max_timestamp){
			max_timestamp=tmp+1;
		}
		nblist_add(&LPS[i]->io_forward_window,NULL,max_timestamp,NBLIST_DUMMY);
	}
	//then we execute these events, so if the horizon is correct we will get the I/O operation executed
	reversibleio_execute();
}

void reversibleio_clean(){
	int i;
	for(i=0;i<n_prc_tot;i++){
		nblist_clean(&LPS[i]->io_forward_window,destroy_iobuffer);
		//nblist_clean(&LPS[i]->io_reverse_window,destroy_iobuffer);
	}
}

void reversibleio_destroy(){
	int i=0;
	for(i=0;i<n_prc_tot;i++){
		nblist_destroy(&LPS[i]->io_forward_window,destroy_iobuffer);
	}
	io_heap_delete(io_h);
}
