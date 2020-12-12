/** \file non_blocking_list.h
 * A non blocking liked list without the usage of atomic instructions, works only if there exactly one thread that inserts elements and one thread that removes elements.
 */

#ifndef NON_BLOCKING_LIST_H_INCLUDED
#define NON_BLOCKING_LIST_H_INCLUDED

#define NBLIST_OP_SUCCESS 0

typedef enum _nblist_elem_type{
	NBLIST_DUMMY=0,
	NBLIST_ELEM
} nblist_elem_type;

///An element of the list.
typedef struct _nblist_elem{
	double key; ///< The key that can be used to order the elements.
	nblist_elem_type type; ///< The list struct will not use concurrency, but to do so we need a dummy element to avoid having the list empty.
	void *content; ///< The content.
	struct _nblist_elem *next; ///< The next element in the list.
} nblist_elem;

//The non blocking list.
typedef struct _nblist{
	nblist_elem* head; ///< The first element in the list.
	nblist_elem* tail; ///< The last element in the list.
	nblist_elem* old; ///< The first element that can be removed.
	unsigned int epoch; ///< The epoch of the last operation on the list
} nblist;

/** \brief Initializes the non blocking list.
 * \param[in] list The list to initialize
 * \return ::NBLIST_OP_SUCCESS or an error code.
 */
int nblist_init(nblist* list);

/** \brief Adds an element to the given list.
 * \param[in,out] list The list  where a new element must be added.
 * \param[in] buffer The buffer to add to the list.
 * \returns ::NBLIST_OP_SUCCESS on success, otherwise the error code.
 */
int nblist_add(nblist* list, void* content,double key,nblist_elem_type type);

/** \brief Deallocates the whole nblist.
 * \param[in] list The list to be destroyed.
 */
void nblist_clean(nblist* list,void (*dealloc)(void*));

/** \brief returns the first nblist element removing it from the list.
 * \param[in] list The list where the first element must be removed.
 * \returns NULL on error or the first element in the list.
 */
void* nblist_pop(nblist* list);

/** \brief merges two nblists.
 * In detail the last element of the dset nblist will be connected to the first element to the source nblist.
 * Then the tail of the dest nblist will be the tail of the source nblist.
 * Finally the source nblist will be reinitialized.
 * This method requires locking.
 */
void nblist_merge(nblist *dest,nblist *source);

/** \brief destroys an nblist
 * \param[in] list The list to destroy.
 * \param[in] dealloc The function to deallocate the payloads
 */
void nblist_destroy(nblist* list,void (*dealloc)(void*));

/** \brief updates the Epoch parameter in the list
 * \param[in] list The list where the epoch must be set
 * \param[in] epoch The epoch value.
 */
void nblist_set_epoch(nblist* list,unsigned int epoch);

/** \brief prints the given nblist, for debug use only
 * \param[in] list The list to print
 */
void nblist_print(nblist* list);
/** \brief get the first valid element (skipping dummies) without advancing the head of the list, the list tail is not excluded.
 * \param[in] list The list in where we wish to peek.
 * \returns The first valid element NULL otherwise.
 */
nblist_elem* nblist_peek(nblist* list);
#endif // NON_BLOCKING_LIST_H_INCLUDED
