/** \file reversibleio.h
 * This file will contain the API that will use the iobuffers and the wrappers to make I/O operations reversible */

#ifndef REVERSIBLEIO_H_INCLUDED
#define REVERSIBLEIO_H_INCLUDED

#include <events.h>

/** \brief Initializes the reversible io datastructures.
 * It initializes the non blocking queues in each LP and creates an heap to hold the forward windows of each Lp to create ordered I/O operations.
 */
void reversibleio_init();

/** \brief Collects the I/O operations from the events that are to be collected.
 * \param[in] lp the lp id from where to collect messages
 * \param[in] event_horizon The timestamp until events must be collected
 * \param[in] to_msg The message until the collection must be done
 */
void reversibleio_collect(int lp,double event_horizon,msg_t* to_msg);

/** \brief rollbacks the I/O operations for the given message.
 * \param[in] msg The message to rollback;
 */
void reversibleio_rollback(msg_t *msg);

/// \brief Executes the collected operations
void reversibleio_execute();

/// \brief Frees the unnecessary memory.
void reversibleio_clean();

///\brief Destroys all the datastructures needed
void reversibleio_destroy();

///\brief Flushes the events in the queue according to the last event horizon.
void reversibleio_flush();
#endif // REVERSIBLEIO_H_INCLUDED
