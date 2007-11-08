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
 * DESCR   :	mem_push.C - Pushdown storage object
 *
 * DETAILS :	Two "current" offsets are kept into the set of allocated
 *              memory to keep track of the bottom and top of the pushdown 
 *              store.  The "top" grows downward in memory, so at any time
 *              the "top" of the store should have an offset value less than
 *              that of the "bottom".  If the "top" is greater than the
 *              "bottom", then the objects in the store have "wrapped around"
 *              the available storage so that the top of the store resides at
 *              the end of the allocated memory and the bottom is at the
 *              beginning.
 *
 *
 *              Blocks   0     1     2     3     4     5     6 ...
 *                    *-----*-----*-----*-----*-----*-----*-----*-----*-----*
 *                            ######################################
 *                    *-----*-^---*-----*-----*-----*-----*-----*---^-*-----*
 *                            Top                                   Bottom
 *
 *              Blocks   0     1     2     3     4     5     6 ...
 *                    *-----*-----*-----*-----*-----*-----*-----*-----*-----*
 *                    ####################                    ###############
 *                    *-----*-----*-----*-^---*-----*-----*---^-*-----*-----*
 *                                        Bottom              Top
 *
 *              ### = used by store
 *
 * Author: crb (Chris R. Brown)
 * ------------------------------------------------------------------------- */

#include "std/support.H"
#include "mem_push.H"
#define MIN(A,B) (A < B ? A :  B)
static const int MEM_PUSHDOWN_BLOCK_COUNT  = 4096;

/* -----------------------  Private Methods  ------------------------------- */


/* -------------------------------------------------------------------------
 * DESCR   :	Increases the number of allocated blocks to accomodate
 * 		incoming data.
 * ------------------------------------------------------------------------- */
void
mem_push::increase_mem (
   size_t amount
   )
{
   size_t mem_avail  = num_blocks * block_size;
   size_t mem_in_use = num_objects * obj_size;
   size_t i;

   /* Reallocation is tricky, but fairly infrequent */
   if (amount + mem_in_use >= mem_avail 
	   || block_index(bottom + amount - 1) > num_blocks - 1)
   {
      size_t new_size = (amount + mem_in_use > mem_avail) ?
      	 amount + mem_in_use : bottom + amount;
      size_t new_num_blocks = (new_size - 1) / block_size + 1;
      const int num_b = num_blocks;
      static const int ptr_size = sizeof(char **);
      blocks = (char **) realloc(blocks, new_num_blocks * ptr_size);
      for (i = num_b; i < new_num_blocks; i++)
         blocks[i] = NULL;

      /*
       * If our pushdown 'wraps around', we need to copy the "top" portion
       * (the part that physically lies at the end of allocated memory) upward
       */
      if (top > bottom)
      {
         size_t  old_index, blocks_to_move, new_index, new_top;
         int flag;

         flag = (block_index (top) == block_index (bottom));

         old_index      = block_index (top) + (flag ? 1 : 0);
         blocks_to_move = num_blocks - old_index;
         new_index      = new_num_blocks - blocks_to_move;
         new_top        = top + blocks_to_move * block_size;

         /* move whole blocks indirectly */
         memmove ((char *)(blocks + new_index),
                  (char *)(blocks + old_index),
                   blocks_to_move  * sizeof(UGAptr));

         for (i = old_index; i < new_index; i++)
             blocks[i] = (UGAptr)malloc(block_size);

         /* finally, copy the "top-most" portion */
         if (flag)
            memmove((char *)(block_addr (new_top)),
                    (char *)(block_addr (top)),
                    block_left (top));
         top = new_top;
      }
      else {
         for (i = num_b; i < new_num_blocks; i++)
            blocks[i] = (UGAptr)malloc(block_size);
      }

      num_blocks = new_num_blocks;
   }
}


/* -------------------------------------------------------------------------
 * DESCR   :	Decreases the allocated blocks to allow for objects that
 * 		have been removed.
 * ------------------------------------------------------------------------- */
void
mem_push::decrease_mem (
   size_t                       /* unused */
   )
{
   if (num_objects == 0 && blocks)
   {
#ifdef later
      for (int i = 0; i < num_blocks; i++)
          free(blocks[i]);
      if (blocks) free(blocks);
      num_blocks = 0;
#endif

      top = bottom = 0;
   }
}


/* ----------------------- Protected Methods ------------------------------- */


/* -------------------------------------------------------------------------
 * DESCR   :	Inserts new objects at the top of the store.  If @obj_count@
 * 		> 1,  then objects are copied into the store starting from
 * 	        the leftmost object in the array pointed to by the @objects@
 * 		parameter. 
 * ------------------------------------------------------------------------- */
void
mem_push::insert_top (
   UGAptr       objects,
   size_t       obj_count
   )
{
   if (objects && obj_count)
   {
      register size_t obj_mem = obj_count * obj_size;
      size_t avail_mem;

      increase_mem (obj_mem);
      avail_mem = num_blocks * block_size;

      /* Copy data into top of store, left to right */
      while (obj_mem > 0)
      {
         register size_t temp_count = MIN (block_offset (top), obj_mem);
         temp_count = MIN (temp_count, obj_size);

         int signed_top = top - temp_count;
         if (signed_top < 0)
            top = top + avail_mem - temp_count;
         else
            top -= temp_count;

         memmove(block_addr (top), objects, temp_count);

         objects   += temp_count;
         obj_mem   -= temp_count;
      }
      num_objects += obj_count;
   }
}


/* -------------------------------------------------------------------------
 * DESCR :	Inserts new objects in the bottom of the store.  If
 * 		@obj_count@ > 1, then the @objects@ parameter is
 * 		interpreted as an array of objects to be inserted, starting
 * 		with the first element in the array.
 * ------------------------------------------------------------------------- */
void
mem_push::insert_bottom (
   const char *objects,
   size_t      obj_count
   )
{
   if (objects && obj_count)
   {
      register size_t obj_mem = obj_count * obj_size;

      increase_mem (obj_mem);

      /* copy objects into list, left to right */
      while (obj_mem > 0)
      {
         register size_t temp_count = MIN (block_left (bottom), obj_mem);

         memmove(block_addr(bottom), objects, temp_count);

         bottom    += temp_count;

         objects   += temp_count;
         obj_mem   -= temp_count;
      }
      num_objects += obj_count;
   }
}


/* -------------------------------------------------------------------------
 * DESCR :	Removes objects from the top of the pushdown.  If the
 * 		@obj_count@ parameter is larger than 1, objects will be
 * 		removed and copied into the array pointed to by @objects@
 * 		starting at the left of the array.
 *
 * RETURNS :    The number of objects actually removed from the store.
 * ------------------------------------------------------------------------- */
size_t
mem_push::remove_top (
   UGAptr objects,
   size_t obj_count
   )
{
   size_t return_val = MIN (obj_count, num_objects);

   peek_top (objects, return_val);

   num_objects -= return_val;
   top += return_val;

// bcz: I think these lines are bogus as long as decrease_mem has
//      all of its contents ifdef'd out.
//   size_t avail_mem = num_blocks * block_size;
//   if (top >= avail_mem)
//      top -= avail_mem;

   decrease_mem (obj_count * obj_size);

   return (return_val);
}


/* -------------------------------------------------------------------------
 * DESCR   :	Extracts objects from the top of the pushdown store without
 * 		removing them, as the @pop@ method describes above.
 *
 * RETURNS :    The number of objects actually transferred to the data buffer.
 * ------------------------------------------------------------------------- */
size_t
mem_push::peek_top (
   UGAptr objects,
   size_t obj_count
   ) const
{
   size_t          return_val = 0;

   if (objects)
   {
      size_t          temp_top = top;
      register size_t obj_mem;

      return_val = MIN (obj_count, num_objects);
      obj_mem = return_val * obj_size;

      /* Copy data from store into the array, left to right */   
      while (obj_mem > 0)
      {
         register size_t temp_count = MIN (block_left (temp_top), obj_mem);

         memmove (objects, block_addr (temp_top), temp_count);

         temp_top  += temp_count;

         obj_mem   -= temp_count;
         objects   += temp_count;
      }
   }

   return (return_val);
}


/* -----------------------  Public Methods   ------------------------------- */

/* -------------------------------------------------------------------------
 * DESCR   :	Constructor.  If no object size is specified, then the
 * 		storage deals with bytes and the application is responsible
 * 		for tracking the objects placed on the pushdown store.
 * ------------------------------------------------------------------------- */
mem_push::mem_push (
   size_t object_size                 /* size of a single data object */
   ) :
   obj_size (object_size)
{
   blocks     = NULL;
   num_blocks = num_objects = 0;
   top = bottom = 0;

   /* intelligently choose an allocation block size based on the data size */
   block_size = obj_size * MEM_PUSHDOWN_BLOCK_COUNT;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Destructor.
 * ------------------------------------------------------------------------- */
mem_push::~mem_push (void)
{
   for (size_t i = 0; i < num_blocks; i++)
      free(blocks[i]);
   if (blocks) free(blocks);
}
