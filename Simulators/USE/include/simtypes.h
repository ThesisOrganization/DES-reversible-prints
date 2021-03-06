#pragma once
#ifndef __SIMTYPES_H
#define __SIMTYPES_H

#include <float.h>
#include <atomic.h>
#include <list.h>
#include <events.h>
#include <state.h>
#include <numerical.h>
#include <nb_calqueue.h>

#if CONSTANT_CHILD_INVALIDATION==1
#include <atomic_epoch_and_ts.h>
#endif

#if REVERSIBLE_IO==1
#include "non_blocking_list.h"
#endif

/// Infinite timestamp: this is the highest timestamp in a simulation run
#define INFTY DBL_MAX

#define current_LP_lvt bound->timestamp //used to retrieve the current LVT TODO: usare questo metodo o inserire il campo nella struttura?

#ifndef CLEAN_CKP_INTERVAL
#define CLEAN_CKP_INTERVAL 1000
#endif

/// This defines the type with whom timestamps are represented
typedef double simtime_t;

typedef struct _LP_state {

/* UNUSED VARIABLES */


//#ifdef ENABLE_ULT
//	/// LP execution state. This MUST be the first declared field in the struct
//	LP_context_t	context;
//
//	/// LP execution state when blocked during the execution of an event
//	LP_context_t	default_context;
//
//	/// Process' stack
//	void 		*stack;
//#endif /* ENABLE_ULT */
//
//	/// Logical Process lock, used to serialize accesses to concurrent data structures
//	spinlock_t	lock;
//	/// ID of the worker thread towards which the LP is bound
//	unsigned int	worker_thread;
//	/// Processed rendezvous queue
//	list(msg_t)	rendezvous_queue;
//
//	#ifdef HAVE_CROSS_STATE
//	unsigned int ECS_synch_table[MAX_CROSS_STATE_DEPENDENCIES];
//	unsigned int ECS_index;
//	#endif
//
//	unsigned long long	wait_on_rendezvous;
//	unsigned int		wait_on_object;
//	/// Processed rendezvous queue
//	list(msg_t)	rendezvous_queue;
//	/// Bottom halves queue
//	list(msg_t)	bottom_halves;
//	/// Buffer used by KLTs for buffering outgoing messages during the execution of an event
//	outgoing_t outgoing_buffer;
//	/// Output messages queue
//	list(msg_hdr_t)	queue_out;


	/// Local ID of the thread (used to translate from bound LPs to local id)
	unsigned int 	lid;

	/// Seed to generate pseudo-random values
	seed_type	seed;

	/// Current execution state of the LP
	short unsigned int state;

	/// This variable mainains the current checkpointing interval for the LP
	unsigned int	ckpt_period;

	/// Counts how many events executed from the last checkpoint (to support PSS)
	unsigned int	from_last_ckpt;

	/// If this variable is set, the next invocation to LogState() takes a new state log, independently of the checkpointing interval
	bool		state_log_forced;

	/// The current state base pointer (updated by SetState())
	void 		* current_base_pointer;

	/// Input messages queue
	list(msg_t)	queue_in;

	/// Pointer to the last correctly elaborated event
	/*volatile*/ msg_t		*bound;


	/// Saved states queue
	list(state_t)	queue_states;

	/// Unique identifier within the LP
	unsigned long long	mark;

	/* ADDED FOR PADS 2018 */
	unsigned int num_executed_frames;

	#if CONSTANT_CHILD_INVALIDATION==1
	atomic_epoch_and_ts atomic_epoch_and_ts;
	#else
	/*volatile*/ unsigned int epoch;
	#endif
	
	unsigned int until_ongvt;
	unsigned int until_ckpt_recalc;
		
#if DEBUG == 1
	msg_t* 	last_rollback_event;
#endif

	simtime_t commit_horizon_ts;
	unsigned long long commit_horizon_tb;
	
	unsigned int until_clean_ckp;

#if HANDLE_INTERRUPT==1
	msg_t* last_silent_exec_evt;//is private if we have lock on LP
	msg_t* old_valid_bound;//is private if we have lock on LP
	msg_t* dummy_bound;//is private if we have lock on LP
	unsigned int  LP_simulation_state;//is private if we have lock on LP
	msg_t* last_interrupted_evt;//is private if we have lock on LP. No need to reset, it is overwritten at each interruption 
	msg_t* msg_curr_executed;//this is NULL if there is no event in execution,any threads need of access concurrently at this field 
#endif

#if IPI_DECISION_MODEL==1
	void*ipi_statistics;//used to keep track of per-LP event's statistics,any threads need of access concurrently at this field
#endif

#if RESUMABLE_ROLLBACK==1
	unsigned int num_executed_frames_resumable;
	unsigned int	from_last_ckpt_resumable;
#endif
	
#if POSTING==1
	msg_t* priority_message __attribute__ ((aligned (CACHE_LINE_SIZE)));//used to write priority message,any threads need of access concurrently at this field
#endif

#if REVERSIBLE_IO==1
	nblist io_forward_window,io_reverse_window;
#endif

} LP_state;


#endif
