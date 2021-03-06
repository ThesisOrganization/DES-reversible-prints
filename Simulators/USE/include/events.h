#pragma once
#ifndef __EVENTS_H
#define __EVENTS_H

#include <timer.h>

#if REVERSIBLE_IO==1
#include <non_blocking_list.h>
#endif

#define MAX_DATA_SIZE		128

#define NEW_EVT 	0x0
#define EXTRACTED 	0x1
#define ELIMINATED	0x2
#define ANTI_MSG	0x3

struct event_processing_info{
	//execution_mode and evt_start_time are read/writed in 2 near statements,put them in same cache line
	unsigned int execution_mode;//e.g. LP_STATE_READY,LP_STATE_SILENT_EXEC ecc,any threads need of access concurrently at this field
	clock_timer evt_start_time;//set to the event starting time when executed, 0 otherwise,any threads need of access concurrently at this field
}__attribute__((aligned (CACHE_LINE_SIZE)));

typedef struct __msg_t
{
	/* event's attributes */
	unsigned short sender_id;//unsigned int sender_id;						//MAI				//Sednder LP
	unsigned short receiver_id;//unsigned int receiver_id;					//P-F				//Receiver LP

	simtime_t timestamp;						//P-F				//Timestamp execution of the event

	unsigned short tie_breaker;//unsigned long long tie_breaker; 			//F					//maximum timestamp of produced events used for garbage collection of msg_t due to lazy invalidation

	char type;//int type;									//P					//Type of event (e.g. INIT_STATE)
	char data_size;//unsigned int data_size;									//size of the payload of the vent

	/*volatile*/ unsigned int epoch;			//F					//LP's epoch at executing time
	unsigned int frame; 						//F					//debug//order of execution of the event in the tymeling

	void * monitor;//unsigned int monitor;//								//F


	/* validity attributes */

	struct __msg_t * father;					//F					//address of the father event

	simtime_t max_outgoing_ts; 					//ALTRO				//maximum timestamp of produced events used for garbage collection of msg_t due to lazy invalidation

	unsigned int fatherFrame;	 				//F					//order of execution of the father in the tymeling
	/*volatile*/ unsigned int fatherEpoch;		//ALTRO			//father LP's epoch at executing time



	/*volatile*/ unsigned int state;				//F			//state of the node (EXTRACTED, ELIMINATED OR ANTI-EVENT)

	unsigned char data[MAX_DATA_SIZE];						//payload of the event



#if DEBUG==1 //REVERSIBLE
	/* Support to undo event mechanism */
	unsigned int previous_seed;	//seed to generate random number taken before the execution
#endif
	revwin_t *revwin;			//reverse window to rollback
#if DEBUG == 1
	struct __bucket_node * node;					//address of the belonging node

	unsigned int roll_epoch;	//DEBUG

	struct __bucket_node * del_node;

	unsigned long long creation_time;
	unsigned long long execution_time;
	unsigned long long rollback_time;
	unsigned long long deletion_time;

	struct __msg_t * local_next;	//address of the next event executed on the relative LP //occhio, potrebbe non servire
	struct __msg_t * local_previous;	//address of the previous event executed on the relative LP ////occhio, potrebbe non servire
	struct __msg_t * commit_bound;	//address of the previous event executed on the relative LP ////occhio, potrebbe non servire

	simtime_t gvt_on_commit;
	struct __msg_t * event_on_gvt_on_commit;
#endif

#if HANDLE_INTERRUPT==1
	struct event_processing_info processing_info;
#endif

#if POSTING==1
	bool collectionable;//used for garbage collection

	unsigned long long posted __attribute__ ((aligned (CACHE_LINE_SIZE)));//any threads need of access concurrently at this field,this field is accessed frequently in read/write manner,put this in 1 cache line
#endif

#if REVERSIBLE_IO==1
	nblist io_forward_window,io_reverse_window;
#endif

} msg_t;


typedef struct _msg_hdr_t {
	// Kernel's information
	unsigned int   		sender;
	unsigned int   		receiver;
	// TODO: non serve davvero, togliere
	int   			type;
	unsigned long long	rendezvous_mark;	/// Unique identifier of the message, used for rendez-vous event
	// TODO: fine togliere
	simtime_t		timestamp;
	simtime_t		send_time;
	unsigned long long	mark;
} msg_hdr_t;

typedef struct _outgoing_t {
	msg_t **outgoing_msgs;
	unsigned int size;
	unsigned int max_size;
	simtime_t *min_in_transit;
} outgoing_t;

#endif
