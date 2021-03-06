/**
*                       Copyright (C) 2008-2017 HPDCS Group
*                       http://www.dis.uniroma1.it/~hpdcs
*
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*
* @file state.c
* @brief The state module is responsible for managing LPs' simulation states.
*	In particular, it allows to take a snapshot, to restore a previous snapshot,
*	and to silently re-execute a portion of simulation events to bring
*	a LP to a partiuclar LVT value for which no simulation state
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*/


#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <core.h>
#include <timer.h>
#include <list.h>
//#include <scheduler/process.h>
//#include <scheduler/scheduler.h>
#include <state.h>
//#include <communication/communication.h>
#include <dymelor.h>
#include <statistics.h>
#include <queue.h>
#include <hpdcs_utils.h>
#include <prints.h>

#if POSTING==1
#include <posting.h>
#endif

#if CONSTANT_CHILD_INVALIDATION==1
#include <atomic_epoch_and_ts.h>
#endif

#if DEBUG==1
#include <checks.h>
#endif

#if PREEMPT_COUNTER==1
#include <preempt_counter.h>
#endif

#if HANDLE_INTERRUPT==1
#include <handle_interrupt.h>
#endif

#if REVERSIBLE_IO==1
#include <reversibleio.h>
#endif
/// Function pointer to switch between the parallel and serial version of SetState
//void (*SetState)(void *new_state);
//
//void send_antimessages(unsigned int lid, simtime_t lvt) {
//
//}
//
//void activate_LP(unsigned int lp, simtime_t lvt, msg_t *evt, void *state) {		
//	executeEvent(lp, lvt, evt->type, evt->data, evt->data_size, state, true, evt);	
//}


/**
 * Computes the checkpoint interval for the sparse state saving.
 * 
 * @author Davide Cingolani
 *
 * @param
 */
inline void checkpoint_interval_recalculate(unsigned int lid) {
	double checkpoint_time;
	double event_time;
	double avg_rollback_length;

	// // Check whether to recalculate the checkpoint interval
	// if (LPS[lid]->until_ckpt_recalc++ < CKPT_RECALC_PERIOD) {
	// 	return;
	// }
	// LPS[lid]->until_ckpt_recalc = 0;

	// // Computes the cost of a checkpoint operation overall
	// // as the sum of the time average time spent to take
	// // the snapshot and the time to coast forward.
	// // Cr = Cckp + Ccf
	// rollback_time_last = 0;
	// rollback_time = avg_time_checkpoint() + avg_time_restore();

	// // If the cost, compared to the last one, is grater than the
	// // threshold given, then increment the checkpoint interval
	// if (rollback_time > (rollback_time_last + CKPT_RECALC_THRESHOLD)) {
	// 	LPS[lid]->ckpt_period++;
	// }
	// // otherwise, decrement the checkpoint interval
	// else if (rollback_time < (rollback_time_last - CKPT_RECALC_THRESHOLD)) {
	// 	LPS[lid]->ckpt_period--;
	// }

	avg_rollback_length = (double)lp_stats[lid].events_total / (double)lp_stats[lid].counter_checkpoints;
	checkpoint_time = (double)avg_time_checkpoint(lp_stats[lid]);
	event_time = (double)avg_time_event(lp_stats[lid]);

	// Recalculate the checkpoint interval
	LPS[lid]->ckpt_period = (unsigned int)sqrt((checkpoint_time / event_time) * 2 * (avg_rollback_length + 1));

	//printlp("Checkpoint period updated: %lu\n", LPS[lid]->ckpt_period);

	// Update statistics of checkpoint recalculations
	statistics_post_lp_data(lid, STAT_CKPT_RECALC, 1);
	statistics_post_lp_data(lid, STAT_CKPT_PERIOD, (double)LPS[lid]->ckpt_period);
}

/**
* This function is used to create a state log to be added to the LP's log chain
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param lid The Light Process Identifier
*/
bool LogState(unsigned int lid) {

	bool take_snapshot = false;
	state_t new_state; // If inserted, list API makes a copy of this

	if(is_blocked_state(LPS[lid]->state)) {
		return take_snapshot;
	}

	// Keep track of the invocations to LogState
	LPS[lid]->from_last_ckpt++;

	if(LPS[lid]->state_log_forced) {
		LPS[lid]->state_log_forced = false;
		LPS[lid]->from_last_ckpt = 0;

		take_snapshot = true;
		goto skip_switch;
	}

	// Switch on the checkpointing mode.
	switch(rootsim_config.checkpointing) {

		case COPY_STATE_SAVING:
			take_snapshot = true;
			break;

		case PERIODIC_STATE_SAVING:
			if(LPS[lid]->from_last_ckpt >= LPS[lid]->ckpt_period) {
				take_snapshot = true;
				LPS[lid]->from_last_ckpt = 0;
			}
			break;

		default:
			rootsim_error(true, "State saving mode not supported.");
	}

skip_switch:

	// Shall we take a log?
	if (take_snapshot) {

		// Take a log and set the associated LVT
		//printf("%s %d %d\n", "BANANA", lid, LPS[lid]->from_last_ckpt);
		new_state.log = log_state(lid);
		new_state.lvt = lvt(lid);
		new_state.last_event = LPS[lid]->bound;
		new_state.state = LPS[lid]->state;

		new_state.seed 					= LPS[lid]->seed 					;
		new_state.num_executed_frames	= LPS[lid]->num_executed_frames		;

		// We take as the buffer state the last one associated with a SetState() call, if any
		new_state.base_pointer = LPS[lid]->current_base_pointer;

		// list_insert() makes a copy of the payload
		(void)list_insert_tail(lid, LPS[lid]->queue_states, &new_state);

	}

	return take_snapshot;
}


unsigned long long RestoreState(unsigned int lid, state_t *restore_state) {
	log_restore(lid, restore_state);
	LPS[lid]->current_base_pointer 	= restore_state->base_pointer 			;
	LPS[lid]->state 				= restore_state->state 					;
	return restore_state->num_executed_frames	;
}

/**
* This function bring the state pointed by "state" to "final time" by re-executing all the events without sending any messages
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param lid The id of the Light Process
* @param state_buffer The simulation state to be passed to the LP
* @param pointer The pointer to the element of the input event queue from which start the re-execution
* @param final_time The time where align the re-execution
*
* @return The number of events re-processed during the silent execution
*/

#if HANDLE_INTERRUPT==1
//cleaned version of silent execution,inside it we have macro HANDLE_INTERRUPT==1 to check differencies with original version not cleaned.
unsigned int silent_execution(unsigned int lid, void *state_buffer, msg_t *evt, simtime_t until_ts, unsigned int tie_breaker) {
	unsigned int events = 0;
	unsigned short int old_state;
	
	msg_t * local_next_evt, * last_executed_event;
	// current state can be either idle READY, or ROLLBACK, so we save it and then put it back in place
	old_state = LPS[lid]->state;
	LPS[lid]->state = LP_STATE_SILENT_EXEC;

	#if DEBUG==1//not present in original version
	unsigned int last_frame_event,local_next_frame_event,last_epoch_event,local_next_epoch_event;
	check_silent_execution(old_state);
	#endif

	// Reprocess events. Outgoing messages are explicitly discarded, as this part of
	// the simulation has been already executed at least once

	last_executed_event = evt;

	#if DEBUG==1//not present in original version
	last_frame_event = evt->frame;
	last_epoch_event=evt->epoch;
	#endif

	#if HANDLE_INTERRUPT==1
	LPS[lid]->last_silent_exec_evt = evt;
	#endif

	while(1){
		local_next_evt = list_next(evt);
		
		evt = local_next_evt;
		//condition1 stop rollback:stop to point immediately previous of "ts" pair (dest_ts,dest_tb)
		if(evt == NULL || 
			evt->timestamp > until_ts || (evt->timestamp == until_ts && evt->tie_breaker >= tie_breaker)  ) 
			break;

		#if HANDLE_INTERRUPT==1
		if( (evt != NULL) && !is_valid(evt)){
			//condition2 stop rollback!!
			#if DEBUG==1
			check_stop_rollback(old_state,lid);
			#endif
			
			LPS[lid]->dummy_bound->state=ROLLBACK_ONLY;
			break;
		}
		#endif

		//evt is the next event to Process

		#if DEBUG==1//not present in original version
		local_next_frame_event=local_next_evt->frame;
		local_next_epoch_event=local_next_evt->epoch;
		check_epoch_and_frame(last_frame_event,local_next_frame_event,last_epoch_event,local_next_epoch_event);
		check_events_state(old_state,evt,local_next_evt);
		#endif

		#if HANDLE_INTERRUPT==1
		start_exposition_of_current_event(evt);
		#endif

		executeEvent(lid, evt->timestamp, evt->type, evt->data, evt->data_size, state_buffer, true, evt);

		#if HANDLE_INTERRUPT==1
		end_exposition_of_current_event(evt);
		#endif
		
		events++;

		last_executed_event = evt;

		#if DEBUG==1//not present in original version
		last_frame_event = evt->frame;
		last_epoch_event = evt->epoch;
		#endif

		#if HANDLE_INTERRUPT==1
		LPS[lid]->last_silent_exec_evt = evt;
		//update frames and from_last_ckpt
		#if RESUMABLE_ROLLBACK==1
		LPS[lid]->num_executed_frames_resumable++;
		LPS[lid]->from_last_ckpt_resumable = (LPS[lid]->from_last_ckpt_resumable+1)%CHECKPOINT_PERIOD;
		#endif

		#endif

	}
	
	#if HANDLE_INTERRUPT==1
	insert_ordered_in_list(lid,(struct rootsim_list_node*)LPS[lid]->queue_in,LPS[lid]->last_silent_exec_evt,current_msg);
	#endif

	#if HANDLE_INTERRUPT==1
	if(old_state != LP_STATE_ONGVT){//we don't change bound in LP_STATE_ONGVT
		LPS[lid]->old_valid_bound = last_executed_event;
	}
	#else
	if(old_state != LP_STATE_ONGVT){
		LPS[lid]->bound = last_executed_event;
	}
	#endif

	#if HANDLE_INTERRUPT==1
	LPS[lid]->last_silent_exec_evt = last_executed_event;
	#endif
	
	LPS[lid]->state = old_state;

	return events;
}


#else
unsigned int silent_execution(unsigned int lid, void *state_buffer, msg_t *evt, simtime_t until_ts, unsigned int tie_breaker) {
	unsigned int events = 0;
	unsigned short int old_state;
	//LP_state *lp_ptr = LPS[lid];
	msg_t * local_next_evt, * last_executed_event;
	// current state can be either idle READY, or ROLLBACK, so we save it and then put it back in place
	old_state = LPS[lid]->state;
	LPS[lid]->state = LP_STATE_SILENT_EXEC;
	bool stop_silent = false;

	#if DEBUG==1//not present in original version
	unsigned int last_frame_event,local_next_frame_event,last_epoch_event,local_next_epoch_event;
	check_silent_execution(old_state);
	#endif

	// Reprocess events. Outgoing messages are explicitly discarded, as this part of
	// the simulation has been already executed at least once

	last_executed_event = evt;

	#if DEBUG==1//not present in original version
	last_frame_event = evt->frame;
	last_epoch_event=evt->epoch;
	#endif

	#if HANDLE_INTERRUPT==1
	LPS[lid]->last_silent_exec_evt = evt;
	#endif

	while(1){
		local_next_evt = list_next(evt);

		//while(local_next_evt != NULL && !is_valid(local_next_evt) && local_next_evt != LPS[lid]->bound){
		//	list_extract_given_node(lid, lp_ptr->queue_in, local_next_evt);
		//	//list_place_after_given_node_by_content(TID, to_remove_local_evts, 
		//	//									local_next_evt, ((rootsim_list *)to_remove_local_evts)->head->data);
		//	__sync_or_and_fetch(&local_next_evt->state, ELIMINATED);//local_next_evt->state = ANTI_MSG; //DEBUG									
		//	local_next_evt = list_next(evt);
		//	stop_silent = true;
		//	// TODO: valutare cancellazione da coda globale
		//}
		//if(local_next_evt != NULL && !is_valid(local_next_evt)){
		//	stop_silent = true;
		//	break;
		//}
		
		evt = local_next_evt;
		if(old_state != LP_STATE_ONGVT){
			//if(evt == NULL | evt->node != 0x5A7E)
			if(stop_silent || evt == NULL || 
				evt->timestamp > until_ts || (evt->timestamp == until_ts && evt->tie_breaker >= tie_breaker)  ) 
				break;
		}
		else{
			if(stop_silent || evt == NULL || 
			evt->timestamp > until_ts || (evt->timestamp == until_ts && evt->tie_breaker > tie_breaker)  ) 
				break;
		}
		#if HANDLE_INTERRUPT==1
		if( (evt != NULL) && !is_valid(evt)){
			//stop rollback!!
			#if DEBUG==1
			check_stop_rollback(old_state,lid);
			#endif
			
			LPS[lid]->dummy_bound->state=ROLLBACK_ONLY;
			break;
		}
		#endif

		//evt is the next event to Process

		#if DEBUG==1//not present in original version
		local_next_frame_event=local_next_evt->frame;
		local_next_epoch_event=local_next_evt->epoch;
		check_epoch_and_frame(last_frame_event,local_next_frame_event,last_epoch_event,local_next_epoch_event);
		check_events_state(old_state,evt,local_next_evt);
		#endif

		executeEvent(lid, evt->timestamp, evt->type, evt->data, evt->data_size, state_buffer, true, evt);
		#if HANDLE_INTERRUPT==1
		change_dest_ts(lid,&until_ts,&tie_breaker);//if ScheduleNeWEvent viewed priority_message it changed the bound with priority_msg but doesn't chagne dest_ts 
		#endif
		events++;

		last_executed_event = evt;

		#if DEBUG==1//not present in original version
		last_frame_event = evt->frame;
		last_epoch_event = evt->epoch;
		#endif

		#if HANDLE_INTERRUPT==1
		LPS[lid]->last_silent_exec_evt = evt;
		#endif

	}
	
	#if HANDLE_INTERRUPT==1
	if(current_msg!=NULL){
		insert_ordered_in_list(lid,(struct rootsim_list_node*)LPS[lid]->queue_in,LPS[lid]->last_silent_exec_evt,current_msg);
	}
	#endif

#if DEBUG==1
		if(old_state == LP_STATE_ONGVT){
			if(stop_silent){
				if(local_next_evt->timestamp < until_ts || 
				(local_next_evt->timestamp == until_ts && local_next_evt->tie_breaker <= tie_breaker)){ 
					printf(RED("Ho beccato un evento non valido durante On_GVT\n")); 
					print_event(local_next_evt);
					gdb_abort;
				}
			}
		}
#endif

	#if HANDLE_INTERRUPT==1
	if(old_state != LP_STATE_ONGVT){
		LPS[lid]->old_valid_bound = last_executed_event;
	}
	#else
	if(old_state != LP_STATE_ONGVT){
		LPS[lid]->bound = last_executed_event;
	}
	#endif

	#if HANDLE_INTERRUPT==1
	LPS[lid]->last_silent_exec_evt = last_executed_event;
	#endif
	
	LPS[lid]->state = old_state;
	return events;
}
#endif



/**
* This function rolls back the execution of a certain LP. The point where the
* execution is rolled back is identified by the timestamp <destination_time,tie_breaker>.
* For a rollback operation to take place, that pointer must be set before calling
* this function.
* If the ts is in the future it restore the state in the future (if available).
* If the ts is INFTY it restore the last event executed (LP->bound) (if available).
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param lid The Logical Process Id
*/
void rollback(unsigned int lid, simtime_t destination_time, unsigned int tie_breaker) {
	state_t *restore_state, *s;
	msg_t *last_restored_event;
	msg_t * bound_next;
	unsigned int reprocessed_events;
	unsigned int rollback_lenght;
	//unsigned long long starting_frame;
	//unsigned int avg_events_btw_rollback;

	clock_timer rollback_timer;

	// Sanity check
	if(LPS[lid]->state != LP_STATE_ROLLBACK && LPS[lid]->state != LP_STATE_ONGVT) {
		rootsim_error(false, "I'm asked to roll back LP %d's execution, but rollback_bound is not set. Ignoring...\n", lid);
		return;
	}
	#if DEBUG==1//this is not present in original version
	//ProcessEvent requires that current_lp and current_msg are corrected set(e.g. ProcessEvent internally call Random that use current_lp)
	//this means that LP and current_lp must be equals.
	//LP and current_lp can be different in case we have lock on lp x and we called this function with value lp y.
	//this scenario can happen for example if we lock lp x and we don't reset current_lp with instruction current_lp=x
	if(current_lp!=lid){
		printf("current_lp different from LP_idx function argument\n");
		gdb_abort;
	}
	#endif
	//printf("ROLLBACK of LP%lu on time %.5f, %lu\n", lid, destination_time, tie_breaker);

	if(destination_time == INFTY){
		bound_next = list_next(LPS[lid]->bound);
		if(bound_next != NULL){
			destination_time = bound_next->timestamp;
			tie_breaker = bound_next->tie_breaker;
		}
	}

	clock_timer_start(rollback_timer);

	// Find the state to be restored, and prune the wrongly computed states
	restore_state = list_tail(LPS[lid]->queue_states);
	
	assert(restore_state!=NULL);
	// It's >= rather than > because we have NOT taken into account simultaneous events YET
	while (restore_state != NULL && 
		( restore_state->lvt >= destination_time || !is_valid(restore_state->last_event) ) ) { //TODO: aggiungere tie_breaker e mettere solo >
		s = restore_state;
		restore_state = list_prev(restore_state);
		if(LPS[lid]->state == LP_STATE_ROLLBACK){
			log_delete(s->log);
			s->last_event = (void *)0xBABEBEEF;
			list_delete_by_content(lid, LPS[lid]->queue_states, s);
		}
	}
	assert(restore_state!=NULL);
	
//	if(restore_state->lvt != restore_state->last_event->timestamp){ //DEBUG
//		printf("Il checkpoint è sputtanato!\n");
//		gdb_abort;
//	}
	// Restore the simulation state and correct the state base pointer
	log_restore(lid, restore_state);
	LPS[lid]->current_base_pointer 	= restore_state->base_pointer;
	
	last_restored_event = restore_state->last_event;
	#if DEBUG==1//not present in original version
	if(last_restored_event->frame!=restore_state->num_executed_frames-1){
		printf("invalid frame number after restore state,frame_event=%d,frame_LP=%d\n",last_restored_event->frame,LPS[lid]->num_executed_frames);
		gdb_abort;
	}
	#endif

	#if RESUMABLE_ROLLBACK==1
	//we have restore state with checkpoint,this LP_state is actually RESUMABLE,respect of last_silent_exec,so promotes INVALID state to RESUMABLE state
	if(LPS[lid]->LP_simulation_state==INVALID)
		LPS[lid]->LP_simulation_state=RESUMABLE;
	LPS[lid]->num_executed_frames_resumable = restore_state->num_executed_frames;
	LPS[lid]->from_last_ckpt_resumable=0;
	#endif
	
	reprocessed_events = silent_execution(lid, LPS[lid]->current_base_pointer, last_restored_event, destination_time, tie_breaker);

	// THE BOUND HAS BEEN RESTORED BY THE SILENT EXECUTION
	statistics_post_lp_data(lid, STAT_EVENT_SILENT, (double)reprocessed_events);
	//TODO
	//con questo check si vogliono tenere in conto sia gli eventi silent in mode ONGVT per riportarsi ad una simulazione commit,
	// sia gli eventi after mode ONGVT ossia LP_STATE_ROLLBACK necessaria per riportarsi nel punto in cui si era della simulazione
	//ma destination time==INFTY potrebbe essere sovrascritto col list_next(bound), quindi sfalsa la statistica
	if(LPS[lid]->state == LP_STATE_ONGVT || destination_time == INFTY)
		statistics_post_lp_data(lid, STAT_EVENT_SILENT_FOR_GVT, (double)reprocessed_events);

	//The bound variable is set in silent_execution.
	if(LPS[lid]->state == LP_STATE_ROLLBACK){
		rollback_lenght = LPS[lid]->num_executed_frames; 
		LPS[lid]->num_executed_frames = restore_state->num_executed_frames + reprocessed_events;
		rollback_lenght -= LPS[lid]->num_executed_frames;
		
		#if CONSTANT_CHILD_INVALIDATION==1
		double new_epoch=get_epoch_of_LP(lid)+1;
		atomic_epoch_and_ts temp;
		set_epoch(&temp,new_epoch);
		set_timestamp(&temp,destination_time);
		atomic_store_epoch_and_ts(&(LPS[lid]->atomic_epoch_and_ts),temp);
		#else
		LPS[lid]->epoch++;
		#endif
		
		LPS[lid]->from_last_ckpt = reprocessed_events%CHECKPOINT_PERIOD; // TODO
		// TODO
		//LPS[lid]->from_last_ckpt = ??;

		//statistics_post_lp_data(lid, STAT_ROLLBACK, 1);

		//TODO
		//con questo check si vogliono tenere in conto il numero di eventi in un rollback che non sia quello forzato tramite la modalità ONGVT
		//ma destination time potrebbe essere sovrascritto col list_next(bound), quindi sfalsa la statistica tenendo in considerazione anche quelli nella modalità ONGVT
		if(destination_time < INFTY)
			statistics_post_lp_data(lid, STAT_ROLLBACK_LENGTH,(double)rollback_lenght);

		statistics_post_lp_data(lid, STAT_CLOCK_ROLLBACK, (double)clock_timer_value(rollback_timer));
	}

	LPS[lid]->state = restore_state->state;
}

#if RESUMABLE_ROLLBACK==1
void rollback_forward(unsigned int lid, simtime_t destination_time, unsigned int tie_breaker){
	msg_t *last_restored_event;
	unsigned int rollback_forward_length;

	clock_timer rollback_forward_timer;

	// Sanity check
	if(LPS[lid]->state != LP_STATE_ROLLBACK && LPS[lid]->state != LP_STATE_ONGVT) {
		rootsim_error(false, "I'm asked to roll back LP %d's execution, but rollback_bound is not set. Ignoring...\n", lid);
		return;
	}
	#if DEBUG==1//this is not present in original version
	//ProcessEvent requires that current_lp and current_msg are corrected set(e.g. ProcessEvent internally call Random that use current_lp)
	//this means that LP and current_lp must be equals.
	//LP and current_lp can be different in case we have lock on lp x and we called this function with value lp y.
	//this scenario can happen for example if we lock lp x and we don't reset current_lp with instruction current_lp=x
	if(current_lp!=lid){
		printf("current_lp different from LP_idx\n");
		gdb_abort;
	}
	#endif

	clock_timer_start(rollback_forward_timer);
	
	last_restored_event = LPS[lid]->last_silent_exec_evt;

	#if DEBUG==1
	if(LPS[lid]->last_silent_exec_evt->frame!=LPS[lid]->num_executed_frames_resumable-1){
		printf("error in frame last\n");
		gdb_abort;
	}
	#endif

	rollback_forward_length = silent_execution(lid, LPS[lid]->current_base_pointer, last_restored_event, destination_time, tie_breaker);
	// THE BOUND HAS BEEN RESTORED BY THE SILENT EXECUTION
	#if REPORT==1
	statistics_post_lp_data(lid, STAT_CLOCK_RESUME_ROLLBACK_LP, (double)clock_timer_value(rollback_forward_timer));
	statistics_post_lp_data(lid, STAT_EVENT_RESUME_ROLLBACK_LP, (double)rollback_forward_length);
	statistics_post_lp_data(lid,STAT_COUNT_RESUME_ROLLBACK_LP,1);
	#else
	(void)rollback_forward_length;
	(void)rollback_forward_timer;
	#endif

	//update epoch of LP and restore frames and last_checkpoint
	#if CONSTANT_CHILD_INVALIDATION==1
	double new_epoch=get_epoch_of_LP(current_lp)+1;
	atomic_epoch_and_ts temp;
	set_epoch(&temp,new_epoch);
	set_timestamp(&temp,destination_time);
	atomic_store_epoch_and_ts(&(LPS[lid]->atomic_epoch_and_ts),temp);
	#else
	LPS[lid]->epoch++;
	#endif
	LPS[lid]->num_executed_frames=LPS[lid]->num_executed_frames_resumable;
	LPS[lid]->from_last_ckpt = LPS[lid]->from_last_ckpt_resumable;
}
#endif

/**
* This function computes the time barrier, namely the first state snapshot
* which is associated with a simulation time <= that the passed simtime
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param lid The light Process Id
* @param simtime The simulation time to be associated with a state barrier
* @return A pointer to the state that represents the time barrier
*/
state_t *find_time_barrier(int lid, simtime_t simtime) {

	state_t *barrier_state;

	if(D_EQUAL(simtime, 0.0)) {
		return list_head(LPS[lid]->queue_states);
	}

	barrier_state = list_tail(LPS[lid]->queue_states);

	// Must point to the state with lvt immediately before the GVT
	while (barrier_state != NULL && barrier_state->lvt >= simtime) {
		barrier_state = list_prev(barrier_state);
  	}
  	if(barrier_state == NULL) {
		barrier_state = list_head(LPS[lid]->queue_states);
	}

/*
	// TODO Search for the first full log before the gvt
	while(true) {
		if(is_incremental(current->log) == false)
			break;
	  	current = list_prev(current);
	}
*/

	return barrier_state;
}

/**
* This function sets the buffer of the current LP's state
*
* @author Francesco Quaglia
*
* @param new_state The new buffer
*
* @todo malloc wrapper
*/
void ParallelSetState(void *new_state) {
	LPS[current_lp]->current_base_pointer = new_state;
}







/**
* This function sets the checkpoint mode
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param ckpt_mode The new checkpoint mode
*/
void set_checkpoint_mode(int ckpt_mode) {
	rootsim_config.checkpointing = ckpt_mode;
}




/**
* This function sets the checkpoint period
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param lid The Logical Process Id
* @param period The new checkpoint period
*/
void set_checkpoint_period(unsigned int lid, int period) {
	LPS[lid]->ckpt_period = period;
}


/**
* This function tells the logging subsystem to take a LP state log
* upon the next invocation to <LogState>(), independently of the current
* checkpointing period
*
* @author Alessandro Pellegrini
*
* @param lid The Logical Process Id
* @param period The new checkpoint period
*/
void force_LP_checkpoint(unsigned int lid) {
	LPS[lid]->state_log_forced = true;
}





void clean_checkpoint(unsigned int lid, simtime_t commit_horizon) {
	state_t *to_state, *tmp_state=NULL;
	msg_t *from_msg, *to_msg=NULL, *tmp_msg=NULL;

	(void) tmp_msg;
	
	from_msg = list_head(LPS[lid]->queue_in);
	to_state = list_tail(LPS[lid]->queue_states);


	assert(to_state!=NULL);

	while (to_state != NULL && (to_state->last_event->timestamp) >= commit_horizon){
		to_state = list_prev(to_state);
	}//to_state è l'ultimo checkpoint da tenere
	
	if(to_state != NULL){//dobbiamo prendere uno stato in più per l'OnGvt
		to_state = list_prev(to_state);
	}
	
	if(to_state != NULL){
		to_msg = list_prev(to_state->last_event); //ultimo evento da buttare
	
		///Rimozione msg dalla vecchia lista
		((struct rootsim_list*)LPS[lid]->queue_in)->head = list_container_of(to_state->last_event);
		list_container_of(to_state->last_event)->prev = NULL; 
		
	
		to_state = list_prev(to_state); //ultimo checkpoint da buttare
		//if(to_state != NULL)
		//	to_msg = to_state->last_event;
	}
	#if REVERSIBLE_IO==1
	reversibleio_collect(current_lp,LPS[current_lp]->commit_horizon_ts,to_msg);
	#endif

	//Rimozione Stati
	while (to_state != NULL){ //TODO: aggiungere tie_breaker e mettere solo >
		tmp_state = list_prev(to_state);
		log_delete(to_state->log);
		to_state->last_event = (void *)0xBABEBEEF;
		list_delete_by_content(lid, LPS[lid]->queue_states, to_state); //<-migliorabile
		to_state = tmp_state;
	}
	
	//while(to_msg != NULL){
	//	tmp_msg = list_prev(to_msg);
	//	list_extract_given_node(tid, ((struct rootsim_list*)LPS[lid]->queue_in), to_msg);
	//	list_node_clean_by_content(to_msg); //NON DOVREBBE SERVIRE
	//	list_insert_tail_by_content(to_remove_local_evts, to_msg);
	//	to_msg = tmp_msg;
	//}
	
	
	
	//Aggancio alla nuova lista
	if(to_msg != NULL){
		//LA SIZE VA A QUEL PAESE
		list_container_of(to_msg)->next = NULL;
		if(((struct rootsim_list*)to_remove_local_evts)->tail != NULL){//ovvero la coda è vuota
			((struct rootsim_list*)to_remove_local_evts)->tail->next = list_container_of(from_msg);			
		}
		else{
			((struct rootsim_list*)to_remove_local_evts)->head = list_container_of(from_msg);
		}
		
		list_container_of(from_msg)->prev = ((struct rootsim_list*)to_remove_local_evts)->tail;
		((struct rootsim_list*)to_remove_local_evts)->tail = list_container_of(to_msg);
	}
	
	
	assert(list_tail(LPS[lid]->queue_states)!=NULL);
	
}
