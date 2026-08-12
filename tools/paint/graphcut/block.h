/* block.h */
/*
	Template classes Block and DBlock
	Implement adding and deleting items of the same type in blocks.

	If there there are many items then using Block or DBlock
	is more efficient than using 'new' and 'delete' both in terms
	of memory and time since
	(1) On some systems there is some minimum amount of memory
	    that 'new' can allocate (e.g., 64), so if items are
	    small that a lot of memory is wasted.
	(2) 'new' and 'delete' are designed for items of varying size.
	    If all items has the same size, then an algorithm for
	    adding and deleting can be made more efficient.
	(3) All Block and DBlock functions are inline, so there are
	    no extra function calls.

	Differences between Block and DBlock:
	(1) DBlock allows both adding and deleting items,
	    whereas Block allows only adding items.
	(2) Block has an additional operation of scanning
	    items added so far (in the order in which they were added).
	(3) Block allows to allocate several consecutive
	    items at a time, whereas DBlock can add only a single item.

	Note that no constructors or destructors are called for items.

	Example usage for items of type 'MyType':

	///////////////////////////////////////////////////
	#include "block.h"
	#define BLOCK_SIZE 1024
	typedef struct { int a, b; } MyType;
	MyType *ptr, *array[10000];

	...

	Block<MyType> *block = new Block<MyType>(BLOCK_SIZE);

	// adding items
	for (int i=0; i<sizeof(array); i++)
	{
		ptr = block -> New();
		ptr -> a = ptr -> b = rand();
	}

	// reading items
	for (ptr=block->ScanFirst(); ptr; ptr=block->ScanNext())
	{
		printf("%d %d\n", ptr->a, ptr->b);
	}

	delete block;

	...

	DBlock<MyType> *dblock = new DBlock<MyType>(BLOCK_SIZE);
	
	// adding items
	for (int i=0; i<sizeof(array); i++)
	{
		array[i] = dblock -> New();
	}

	// deleting items
	for (int i=0; i<sizeof(array); i+=2)
	{
		dblock -> Delete(array[i]);
	}

	// adding items
	for (int i=0; i<sizeof(array); i++)
	{
		array[i] = dblock -> New();
	}

	delete dblock;

	///////////////////////////////////////////////////

	Note that DBlock deletes items by marking them as
	empty (i.e., by adding them to the list of free items),
	so that this memory could be used for subsequently
	added items. Thus, at each moment the memory allocated
	is determined by the maximum number of items allocated
	simultaneously at earlier moments. All memory is
	deallocated only when the destructor is called.
*/

#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <cstddef>
#include <limits>
#include <new>
#include <stdlib.h>
#include <stdexcept>

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

template <class Type> class Block
{
public:
	/* Constructor. Arguments are the block size and
	   (optionally) the pointer to the function which
	   will be called if allocation failed; the message
	   passed to this function is "Not enough memory!" */
	Block(int size, void (*err_function)(char *) = NULL)
	{
		if (size <= 0)
			throw std::invalid_argument("Block size must be positive");
		first = last = NULL;
		block_size = size;
		error_function = err_function;
	}

	/* Destructor. Deallocates all items added so far */
	~Block() { while (first) { block *next = first -> next; delete [] reinterpret_cast<char *>(first); first = next; } }

	/* Allocates 'num' consecutive items; returns pointer
	   to the first item. 'num' cannot be greater than the
	   block size since items must fit in one block */
	Type *New(int num = 1)
	{
		Type *t;
		if (num <= 0 || num > block_size)
			throw std::invalid_argument("Block allocation count is out of range");

		if (!last || last->current + num > last->last)
		{
			if (last && last->next) last = last -> next;
			else
			{
				const std::size_t item_count = static_cast<std::size_t>(block_size-1);
				if (item_count > (std::numeric_limits<std::size_t>::max()-sizeof(block))/sizeof(Type))
					throw std::length_error("Block allocation size overflow");
				const std::size_t bytes = sizeof(block) + item_count*sizeof(Type);
				block *next = reinterpret_cast<block *>(new (std::nothrow) char [bytes]);
				if (!next)
				{
					if (error_function) (*error_function)(const_cast<char *>("Not enough memory!"));
					throw std::bad_alloc();
				}
				if (last) last -> next = next;
				else first = next;
				last = next;
				last -> current = & ( last -> data[0] );
				last -> last = last -> current + block_size;
				last -> next = NULL;
			}
		}

		t = last -> current;
		last -> current += num;
		return t;
	}

	/* Returns the first item (or NULL, if no items were added) */
	Type *ScanFirst()
	{
		for (scan_current_block=first; scan_current_block; scan_current_block = scan_current_block->next)
		{
			scan_current_data = & ( scan_current_block -> data[0] );
			if (scan_current_data < scan_current_block -> current) return scan_current_data ++;
		}
		return NULL;
	}

	/* Returns the next item (or NULL, if all items have been read)
	   Can be called only if previous ScanFirst() or ScanNext()
	   call returned not NULL. */
	Type *ScanNext()
	{
		while (scan_current_data >= scan_current_block -> current)
		{
			scan_current_block = scan_current_block -> next;
			if (!scan_current_block) return NULL;
			scan_current_data = & ( scan_current_block -> data[0] );
		}
		return scan_current_data ++;
	}

	/* Marks all elements as empty */
	void Reset()
	{
		block *b;
		if (!first) return;
		for (b=first; ; b=b->next)
		{
			b -> current = & ( b -> data[0] );
			if (b == last) break;
		}
		last = first;
	}

/***********************************************************************/

private:

	typedef struct block_st
	{
		Type					*current, *last;
		struct block_st			*next;
		Type					data[1];
	} block;

	int		block_size;
	block	*first;
	block	*last;

	block	*scan_current_block;
	Type	*scan_current_data;

	void	(*error_function)(char *);
};

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

template <class Type> class DBlock
{
public:
	/* Constructor. Arguments are the block size and
	   (optionally) the pointer to the function which
	   will be called if allocation failed; the message
	   passed to this function is "Not enough memory!" */
	DBlock(int size, void (*err_function)(char *) = NULL)
	{
		if (size <= 0)
			throw std::invalid_argument("DBlock size must be positive");
		first = NULL;
		first_free = NULL;
		block_size = size;
		error_function = err_function;
	}

	/* Destructor. Deallocates all items added so far */
	~DBlock() { while (first) { block *next = first -> next; delete [] reinterpret_cast<char *>(first); first = next; } }

	/* Allocates one item */
	Type *New()
	{
		block_item *item;

		if (!first_free)
		{
			block *next = first;
			const std::size_t item_count = static_cast<std::size_t>(block_size-1);
			if (item_count > (std::numeric_limits<std::size_t>::max()-sizeof(block))/sizeof(block_item))
				throw std::length_error("DBlock allocation size overflow");
			const std::size_t bytes = sizeof(block) + item_count*sizeof(block_item);
			first = reinterpret_cast<block *>(new (std::nothrow) char [bytes]);
			if (!first)
			{
				first = next;
				if (error_function) (*error_function)(const_cast<char *>("Not enough memory!"));
				throw std::bad_alloc();
			}
			first_free = & (first -> data[0] );
			for (item=first_free; item<first_free+block_size-1; item++)
				item -> next_free = item + 1;
			item -> next_free = NULL;
			first -> next = next;
		}

		item = first_free;
		first_free = item -> next_free;
		return (Type *) item;
	}

	/* Deletes an item allocated previously */
	void Delete(Type *t)
	{
		((block_item *) t) -> next_free = first_free;
		first_free = (block_item *) t;
	}

/***********************************************************************/

private:

	typedef union block_item_st
	{
		Type			t;
		block_item_st	*next_free;
	} block_item;

	typedef struct block_st
	{
		struct block_st			*next;
		block_item				data[1];
	} block;

	int			block_size;
	block		*first;
	block_item	*first_free;

	void	(*error_function)(char *);
};


#endif

