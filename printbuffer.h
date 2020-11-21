/** \file printbuffer.h
 * This file contains the datastructures necessary to make prints reversible
 */

#ifndef PRINTBUFFER_H_INCLUDED
#define PRINTBUFFER_H_INCLUDED

#include <stdio.h>

///The initial size of every printbuffer
#define PBUF_LEN 512

///The amount of bytes that eah buffer will grow if full
#define PBUF_GROW_LEN 128

/// Success code
#define PBUF_OP_SUCCESS 0

/// The policy to be used when the buffer is full.
typedef enum _pbuf_policy{
		PBUF_GROW=0, ///< The printbuffer will grow, using a realloc, by a fixed rate of chars, defined in ::PBUF_GROW_SIZE.
		PBUF_REJECT, ///< The print operation will be rejected.
		PBUF_OVERWRITE ///< The buffer will be overwritten for the oldest print (at the beginning of the buffer).
} pbuf_policy;

//Enum which contains the status of the fclose for the current file.
typedef enum _pbuf_fclose_request {
	FCLOSE_NOT_REQUESTED=0, ///< no flcose has been issued
	FCLOSE_REQUESTED, ///< an fclose ha been issued
	FCLOSE_DONE
} pbuf_fclose_request;

///A printbuffer, will hold the buffer of chars to be printed and the file pointer which specified where these chars must be printed.
typedef struct _printbuffer{
	FILE* file; ///< The file where the chars need to be printed.
 	pbuf_fclose_request fstat; ///< If an fclose for the associated file has been requested.
	char* buffer; ///< The buffer where the chars will be stored until they are printed.
	pbuf_policy policy; ///< The policy applied by the printbuffer when it is full and a print operation is received.
	size_t buffer_size; ///< The actual size of the buffer
	size_t buf_written_num; ///< The number of characters written in the buffer.
} printbuffer;

///The circular list of printbuffers associated with each LP.
typedef struct _pbuf_list{
	printbuffer pbuf; ///< The printbuffer.
	struct _pbuf_list *next; ///< The next printbuffers in the list.
	struct _pbuf_list *prev; ///< The previous element in the list, or the last element if the current list element if the first.
} pbuf_list;

/** \brief Adds a new printbuffer to the given list.
 * \param[in,out] list The list of printbuffers where a new buffer must be added.
 * \param[in] file The file pointer associated with the printbuffer.
 * \param[in] policy The policy to use on the new buffer.
 * \returns ::PBUF_OP_SUCCESS on succes, otherwise the error code.
*/
int add_printbuffer(pbuf_list **list, FILE* file,pbuf_policy policy);

/** \brief Deallocates the whole printbuffer list.
 * \param[in] list The list to be destroyed.
 */
void destroy_pbuf_list(pbuf_list** list);

/** \brief Deallocates a printbuffer list element.
 * \param[in] elem The list element to be destroyed.
 */
void destroy_pbuf_list_elem(pbuf_list* elem);

/** \brief Searches for a specific buffer identified by the file pointer.
 * \param[in] file The file used as a search term.
 * \param[in] list the list where the search is performed
 * \return a pointer to a ::printbuffer or NULL.
 */
printbuffer* search_pbuf_list(FILE* file, pbuf_list *list);

/** \brief Grows the printbuffer according to the policy.
 * \param[in] buffer The printbuffer to grow.

 */
int printbuffer_grow(printbuffer *pbuf);

/** \brief Empties the printbuffer, writing on the associated file.
 * \param buffer The buffer to empty.
 * \return ::PBUF_OP_SUCCESS or an error code
 */
int prinbuffer_empty(printbuffer *pbuf);

/** \brief Writes into the given printbuffer
 * \param[in] pbuf The print buffer where we must write
 * \param[in] format The format string to write, followed by the format arguments
 * \return ::PBUF_OP_SUCCESS or an error code.
 */
int printbuffer_write(printbuffer* pbuf,const char* format,...);

#endif // PRINTBUFFER_H_INCLUDED
