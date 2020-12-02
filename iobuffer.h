/** \file iobuffer.h
 * This file contains the datastructures necessary to make prints reversible
 */

#ifndef IOBUFFER_H_INCLUDED
#define IOBUFFER_H_INCLUDED

#include <stdio.h>
/// Success code
#define IOBUF_OP_SUCCESS 0

///This enum is used to check if the model has requested an fclose.
typedef enum _iobuf_operation_request{
	IOBUF_FWRITE=0, ///< fwrite has been issued.
	IOBUF_FCLOSE ///< fclose has been issued.
} iobuf_operation_request;

///An iobuffer, will hold the buffer of chars to be written and the file pointer which specified where these chars must be written. Additionally it will hold the request to close the file.
typedef struct _iobuffer{
	FILE* file; ///< The file where the chars need to be printed.
	long file_position; ///< The file position indicator befor the I/O operation.
	double timestamp; ///< The timestamp where the operation has been issued.
	iobuf_operation_request operation; ///< If an fclose for the associated file has been requested.
	void* buffer; ///< The buffer where the chars will be stored until they are printed.
	size_t buffer_size; ///< The actual size of the buffer
} iobuffer;

/** \brief Creates a new iobuffer.
 * \param[in] file The file pointer associated with the iobuffer.
 * \param[in] content The content of the buffer.
 * \param[in] content_size The size of the content.
 * \param[in] timestamp The timestamp of the io operation.
 * \param[in] file_position The position inside the file.
 * \returns the iobuffer on success, otherwise NULL.
*/
iobuffer* create_iobuffer(FILE* file, void* content, size_t content_size, double timestamp,int file_position,iobuf_operation_request operation);

/** \brief Deallocates a iobuffer list element, destroying the list in the process.
 * \param[in] elem The list element to be destroyed.
 */
void destroy_iobuffer(void* iobuf);

/** \brief Writes the content associated with the iobuffer on the associated file. If requested issues the fclose.
 * \param buffer The buffer to empty.
 * \return ::PBUF_OP_SUCCESS or an error code
 */
int iobuffer_write(iobuffer *iobuf);

#endif // PRINTBUFFER_H_INCLUDED
