/*****************************************************************
 * This file is part of jot-lib (or "jot" for short):
 *   <http://code.google.com/p/jot-lib/>
 * 
 * jot-lib is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * jot-lib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with jot-lib.  If not, see <http://www.gnu.org/licenses/>.`
 *****************************************************************/
/* Copyright 1992, Brown Computer Graphics Group.  All Rights Reserved. */

/* -------------------------------------------------------------------------
 *
 *                <     File description here    >
 *
 * ------------------------------------------------------------------------- */

#ifndef STD_MEM_HAS_BEEN_INCLUDED
#define STD_MEM_HAS_BEEN_INCLUDED

/* --------------------------    Constants     ----------------------------- */

/* --------------------------      Types       ----------------------------- */

typedef char   	    UGAgeneric;
typedef UGAgeneric *UGAptr;
typedef enum {
     STD_FALSE = 0,
     STD_TRUE  = 1
} STDbool;

/* --------------------------     Classes      ----------------------------- */

/*
 * TITLE :	Stack/Queue Storage Class
 *
 * DESCR :	This private class provides simple queue and/or stack
 * 		functionality for data objects.  Means of data access are
 * 		not restricted to LIFO or FIFO; enforcement of stack/queue
 * 		semantics is left up to child objects.
 *
 * DETAILS :	In order to make data i/o as efficient as possible with a
 * 		minimum of reallocations, the data is stored in a series of
 * 		rotating buffers, with byte offsets used to mark the
 * 		current push and pop positions.
 */
class mem_push {
 public:
    mem_push(size_t datasize = sizeof(UGAgeneric));
    virtual ~mem_push();

   STDbool is_empty () const { return num_objects == 0 ? STD_TRUE : STD_FALSE; }
   
   void remove_all() {remove_top(0, count());}

 protected:
   /*
    * DESCR   :	Basic pushoperations, providing double-ended data
    * 		access. 
    *
    * DETAILS :	Note to class authors: in the current implementation,
    * 		the methods @insert_bottom@ and @remove_top@ are more
    * 		efficient than their counterparts and should be
    * 		preferred. 
    */
   void   insert_top    (UGAptr, size_t count = 1);
   void   insert_bottom (const char *, size_t count = 1);
   size_t remove_top    (UGAptr = NULL, size_t count = 1);
   size_t peek_top      (UGAptr = NULL, size_t count = 1) const;
   
   size_t count () const { return num_objects; }

 private:
   void   increase_mem (size_t);
   void   decrease_mem (size_t);

   size_t block_index  (size_t offset) const  { return offset / block_size; }
   size_t block_offset (size_t offset) const  { return offset % block_size; }
   size_t block_left   (size_t offset) const  { return block_size - block_offset (offset); }
   UGAptr block_addr   (size_t offset) const  { return blocks[block_index(offset)] + 
                                                       block_offset (offset); }

   UGAptr   *blocks;
   size_t    num_blocks;
   size_t    num_objects;
   size_t    obj_size, block_size;
   size_t    top, bottom;
};


/*
 * TITLE   :	Queue memory storage
 */
class STDmem_queue : public mem_push {
 public:
   STDmem_queue (size_t datasize = sizeof(UGAgeneric)):mem_push(datasize) {}

   void   put  (const char *data, size_t count = 1)       { insert_bottom(data, count); }
   size_t get  (UGAptr data=NULL, size_t count = 1)       { return remove_top(data,count);}
   size_t peek (UGAptr data=NULL, size_t count = 1) const { return peek_top(data,count); }
   size_t count()                                   const { return mem_push::count(); }
};


/*
 * TITLE   :	Stack memory storage
 */
class STDmem_stack : public mem_push {
 public:
   STDmem_stack (size_t datasize = sizeof(UGAgeneric)):mem_push(datasize) {}

   void   push (UGAptr data,        size_t count = 1)       { insert_top(data,count); }
   size_t pop  (UGAptr data = NULL, size_t count = 1)       { return remove_top(data,count); }
   size_t peek (UGAptr data = NULL, size_t count = 1) const { return peek_top(data,count); }
   size_t count()                                     const { return mem_push::count(); }
};


/*
 * TITLE   :	Double-ended queue storage
 */
class STDmem_dequeue : public mem_push {
 public:
   STDmem_dequeue (size_t datasize = sizeof(UGAgeneric)):mem_push(datasize) {}

   void   push (UGAptr data,        size_t count = 1)       { insert_top(data,count); }
   void   put  (UGAptr data,        size_t count = 1)       { insert_bottom(data,count); }
   size_t pop  (UGAptr data = NULL, size_t count = 1)       { return remove_top(data,count); }
   size_t peek (UGAptr data = NULL, size_t count = 1) const { return peek_top(data,count); }
   size_t count()                                     const { return mem_push::count(); }
};

#endif  /* STD_MEM_HAS_BEEN_INCLUDED */
