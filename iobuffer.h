/** \file iobuffer.h
 * This file contains the datastructures necessary to make prints reversible
 */

#ifndef IOBUFFER_H_INCLUDED
#define IOBUFFER_H_INCLUDED

#include <stdio.h>
#include "ROOT-Sim.h"
/// Success code
#define IOBUF_OP_SUCCESS 0

/// The policy to be used when the buffer is full.
typedef enum _iobuf_policy{
	IOBUF_FORWARD_WINDOW=0, ///< The buffer will hold a single operation, which will be flushed on state commit
	IOBUF_REVERSE_WINDOW ///< The buffer will hold a backup of the file portion that a write operation has overwritten
} iobuf_policy;

///This enum is used to check if the model has requested an fclose.
typedef enum _iobuf_fclose_request {
	FCLOSE_NOT_REQUESTED=0, ///< no flcose has been issued.
	FCLOSE_REQUESTED, ///< an fclose ha been issued.
	FCLOSE_DONE ///< The fclose has been already done.
} iobuf_fclose_request;

///An iobuffer, will hold the buffer of chars to be written and the file pointer which specified where these chars must be written. Additionally it will hold the request to close the file.
typedef struct _iobuffer{
	FILE* file; ///< The file where the chars need to be printed.
	long file_position; ///< The file position indicator befor the I/O operation.
	simtime_t timestamp; ///< The timestamp where the operation has been issued.
 	iobuf_fclose_request fstat; ///< If an fclose for the associated file has been requested.
	void* buffer; ///< The buffer where the chars will be stored until they are printed.
	size_t buffer_size; ///< The actual size of the bufferd
} iobuffer;

///An element of the list of iobuffers associated with each event.
typedef struct _iobuf_list_elem{
	iobuffer *iobuf; ///< The printbuffer.
	struct _iobuf_list_elem *next; ///< The next printbuffers in the list.
} iobuf_list_elem;

//The list of iobuffers associated with the event.
typedef struct _iobuf_list{
	iobuf_policy policy; ///< The policy applied to ensure reversibility.
	iobuf_list_elem* first; ///< The first element in the list.
	iobuf_list_elem* last; ///< The last element in the list.
} iobuf_list;

/** \brief Creates a new iobuffer.
 * \param[in] file The file pointer associated with the iobuffer.
 * \param[in] content The content of the buffer.
 * \param[in] content_size The size of the content.
 * \param[in] timestamp The timestamp of the io operation.
 * \returns the iobuffer on success, otherwise NULL.
*/
iobuffer* create_iobuffer(FILE* file, void* content, size_t content_size, simtime_t timestamp);

/** \brief Adds an iobuffer to the given list.
 * \param[in,out] list The list of iobuffers where a new buffer must be added.
 * \param[in] buffer The buffer to add to the list.
 * \returns ::PBUF_OP_SUCCESS on success, otherwise the error code.
 */
int add_iobuffer(iobuf_list* list, iobuffer* buffer);

/** \brief Deallocates the whole iobuffer list.
 * \param[in] list The list to be destroyed.
 */
void destroy_iobuf_list(iobuf_list** list);

/** \brief Deallocates a iobuffer list element, destroying the list in the process.
 * \param[in] elem The list element to be destroyed.
 */
void destroy_iobuf_list_elem(iobuf_list_elem* elem);

/** \brief returns the first iobuffer list element removing it from the list.
 * \param[in] list The list where the first element must be removed.
 * \returns NULL on error or the first iobuffer in the list.
 */
iobuffer* iobuf_list_pop(iobuf_list* list);

/** \brief Writes the content associated with the iobuffer on the associated file. If requested issues the fclose.
 * \param buffer The buffer to empty.
 * \return ::PBUF_OP_SUCCESS or an error code
 */
int iobuffer_write(iobuffer *iobuf);

#endif // PRINTBUFFER_H_INCLUDED
