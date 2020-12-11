/** \file reversibleio.c
 * This file will contain the API that will use the iobuffers and the wrappers to make I/O operations reversible */

#include "iobuffer.h"
#include "wrappers.h"
#include "io_heap.h"
#include "core.h"
#include "list.h"
#include "dymelor.h"
#include "events.h"

#include "reversibleio.h"

///We have one heap for the unseekable
io_heap *io_h;
double* per_lp_horizon;

void reversibleio_init(){
	unsigned i;
	//we create the heap
	per_lp_horizon=rsalloc(sizeof(double)*n_prc_tot);
	memset(per_lp_horizon,-1,sizeof(double)*n_prc_tot);
	io_h=io_heap_new(MIN_HEAP,n_prc_tot);
	//we initialize the windows in each LP.
	for(i=0;i<n_prc_tot;i++){
		LPS[i]->io_forward_window=rsalloc(sizeof(nblist));
		nblist_init(LPS[i]->io_forward_window);
		//nblist_init(LPS[i]->io_reverse_window);
		//during the initialization we populate the heap with the forward windows
		io_heap_add(io_h,LPS[i]->io_forward_window);
	}
}

void reversibleio_collect(int lp,double event_horizon){
	//we save the new event horizon for the current lp
	per_lp_horizon[lp]=event_horizon;
	//we start reading the event list to get the events inside the global window.
	msg_t *msg=list_head(LPS[lp]->queue_in);
	while(msg!=NULL && msg->timestamp<event_horizon){
		///for the forward window we extract the I/O operations from the heap according to their timestamp and execute them
		nblist_merge(LPS[lp]->io_forward_window,msg->io_forward_window);
		nblist_destroy(msg->io_forward_window,destroy_iobuffer);
		msg->io_forward_window=NULL;
		///for the reverse window we destroy the nblist since we do not need to roll the I/O operations back
		//nblist_destroy(LPS[lp]->io_reverse_window,destroy_iobuffer);
		msg=list_next(LPS[lp]->queue_in);
	}
}

void reversibleio_rollback(msg_t *msg){
	if(msg==NULL){
		return;
	}
	////For stream files we simply discard the forward window
	if(msg->io_forward_window!=NULL){
		nblist_destroy(msg->io_forward_window,destroy_iobuffer);
		rsfree(msg->io_forward_window);
		msg->io_forward_window=NULL;
	}
	///For seekable files we need to restore them using the backups in the reverse window
}

void reversibleio_execute(){
	//we need to compute the minimum event horizon
	unsigned int i;
	double event_horizon;
	event_horizon=-1;
	for(i=0;i<n_prc_tot;i++){
		if(per_lp_horizon[i]<event_horizon || event_horizon<0){
			event_horizon=per_lp_horizon[i];
		}
	}
	double timestamp=0;
	iobuffer* buf=NULL;
	while(timestamp<event_horizon){
		buf=(iobuffer*)io_heap_poll(io_h);
		if(buf!= NULL){
			timestamp=buf->timestamp;
			iobuffer_write(buf);
		} else {
			break;
		}
	}
}

///To flush all the queues we insert a dummy node in each of the and we retry extracting
void reversibleio_flush(){
	unsigned int i;
	double max_timestamp=0,tmp;
	for(i=0;i<n_prc_tot;i++){
		//we need to insert a dummy an element that is at least later than the current tail
		tmp=LPS[i]->io_forward_window->tail->key;
		if(tmp>max_timestamp){
			max_timestamp=tmp+1;
		}
		nblist_add(LPS[i]->io_forward_window,NULL,max_timestamp,NBLIST_DUMMY);
	}
	//then we execute these events, so if the horizon is correct we will get the I/O operation executed
	reversibleio_execute();
}

void reversibleio_clean(){
	unsigned int i;
	for(i=0;i<n_prc_tot;i++){
		nblist_clean(LPS[i]->io_forward_window,destroy_iobuffer);
		// TODO: can we keep these buffers?
		nblist_clean(LPS[i]->io_reverse_window,destroy_iobuffer);
	}
}

void reversibleio_destroy(){
	unsigned int i=0;
	for(i=0;i<n_prc_tot;i++){
		nblist_destroy(LPS[i]->io_forward_window,destroy_iobuffer);
		rsfree(LPS[i]->io_forward_window);
		LPS[i]->io_forward_window=NULL;
	}
	io_heap_delete(io_h);
}
