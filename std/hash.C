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
/*************************************************************************
 *    NAME: Loring Holden
 *    USER: lsh
 *    FILE: hash.C
 *    DATE: Wed Aug 27 12:53:19 US/Eastern 1997
 *************************************************************************/
#include <cstring>
#include <cassert>
#include "support.H"

ThreadMutex mutex;

static const int RIGHT_BITS_TO_DROP = 3;

inline
int
HASH::hash(const long key) const
{
	return (key & _mask) >> RIGHT_BITS_TO_DROP;
}

/*************************************************************************
 * Function Name: HASH::HASH
 * Parameters: int size
 * Effects: 
 *************************************************************************/
HASH::HASH(int size)
{
   if (size == 0) {
      _size    = 0;
      _mask    = 0;
      _lastval = 0;
      _table   = 0;
      return;
   }
   int 	    realsize;
   // size of 2 gives an actual _size of 1, which doesn't work w/ the
   // hash function
   if (size < 4) size = 4;

   /* Round up to next power of 2 */
   for (--size, realsize = 1; size; size >>= 1) realsize <<= 1;

   _table = new hash_node*[realsize];
   for (int i = 0; i < realsize; i++) _table[i] = 0;

   _mask = (realsize - 1) << RIGHT_BITS_TO_DROP;
   _size = (realsize - 1);

   _lastval = 0;
}

/*************************************************************************
 * Function Name: HASH::HASH
 * Parameters: const HASH &hash_table
 * Effects: Copy constructor
 *************************************************************************/

HASH::HASH(const HASH &old)
   : _size(old._size),
     _mask(old._mask),
     _lastval(0)
{
   _table = new hash_node*[_size + 1];

   for (int i =  0; i < _size + 1; i++) {
      if (old.table()[i]) {
	 // Recursively copies buckets
	 table()[i] = new hash_node(*old.table()[i]);
      }
   }
}


/*************************************************************************
 * Function Name: HASH::~HASH
 * Parameters: 
 * Effects: 
 *************************************************************************/

HASH::~HASH()
{
   clear();
   delete [] _table;
}


/*************************************************************************
 * Function Name: HASH::add
 * Parameters: long key, void *data
 * Returns: int
 * Effects: 
 * DESCR   : Add a new node to the hash table, associating the data pointer
 *           with the key value.  Any previous data associated with the key
 *           is overwritten.
 *
 * RETURNS : +1 if there was previous data assiciated with key.
 *           -1 if the allocation of the new HASH node failed.
 *            0 otherwise.
 *************************************************************************/
int
HASH::add(long key, void *data)
{
   CriticalSection cs(&mutex);
   assert(table());
   hash_node *e, *new_node, *prev, **list = &table()[hash(key)];

   for (e = *list, prev=0; e != 0; prev = e, e = e->next()) {
      if (e->key() == key) {
	 e->data(data);
	 return  1;
      }
      else if (e->key() > key) break;
   }

   new_node  = new hash_node(key, data, e);
   if (prev == 0) *list = new_node;
   else 	   prev->next(new_node);

   return 0;
}


/*************************************************************************
 * Function Name: HASH::del
 * Parameters: long key
 * Returns: int
 * Effects: 
 *************************************************************************/
int
HASH::del(long key)
{
   hash_node *e, *prev, **list = &table()[hash(key)];

   for (e = *list, prev = 0; e != 0; prev = e, e=e->next()) {
      if (e->key() == key) {
	 if (prev == 0)
	    *list = e->next();
	 else
	    prev->next(e->next());
	 delete e;
	 return 0;
      }
      else if (e->key() > key) break;
   }

   return 1;
}


/*************************************************************************
 * Function Name: HASH::find_addr
 * Parameters: long key
 * Returns: void **
 * Effects: 
 * DESCR   : Looks up the data associated with a key.
 *
 * RETURNS : A pointer to the data associated with the key.
 *           0 if the key was not in the table.
 *************************************************************************/
void **
HASH::find_addr(long key) const
{
   CriticalSection cs(&mutex);
   hash_node **list, *e;

   list = &table()[hash(key)];

   for (e = *list; e != 0; e = e->next())
      if (e->key() == key)     return &e->data_ptr();
      else if (e->key() > key) break;

   return 0;
}


/*************************************************************************
 * Function Name: HASH::bfind
 * Parameters: long key, void *&data
 * Returns: int
 * Effects: 
 * DESCR   : Looks up the data associated with the key.  This "better" find
 *           will allow the user to distinguish between an unsuccessful
 *           search and a successful search for a key with 0 data.
 *
 * RETURNS : STD_1  - If the key was found in the table.
 *                       The data is also returned in the data parameter.
 *           STD_0 - If the key was not found.
 *************************************************************************/
int
HASH::bfind(long key, void *&data) const
{
   hash_node **list, *e;

   list = &table()[hash(key)];

   for (e = *list; e != 0; e = e->next()) {
      if (e->key() == key) {
	 data = e->data();
	 return 1;
      }
      else if (e->key() > key) break;
   }
   return 0;
}


/*************************************************************************
 * Function Name: HASH::addn
 * Parameters: int num, char *key, void *data
 * Returns: int
 * Effects: 
 * DESCR   : Add a node to the table, where the key is n bytes long.
 *
 * RETURNS : +1 if there was previous data assiciated with key.
 *           -1 if the allocation of the new HASH node failed.
 *            0 otherwise.
 *************************************************************************/
int
HASH::add(const char *key, void *data, char *&loc, int create_new)
{
   CriticalSection cs(&mutex);
   assert(table());
   hash_node *e, *prev, **list = &table()[hash(key)];

   for (e = *list, prev = 0; e != 0; prev = e, e=e->next())
      if (!strcmp((char *) e->key(), key)) {
         loc = (char *) e->key();
	 e->data(data);
	 return 0;
      }

   loc = create_new ? strdup(key) : (char *) key;
   hash_node *new_node = new hash_node((long) loc, data, e);

   if (prev == 0) *list = new_node;
   else 	   prev->next(new_node);
   return(0);
}


/*************************************************************************
 * Function Name: HASH::findn
 * Parameters: int num_bytes, char *key
 * Returns: void  *
 * Effects: 
 * DESCR   : Looks up the data associated with a key of n bytes.
 *
 * RETURNS : Data associated with the key.
 *           0 if the key was not in the table.
 *************************************************************************/
void  *
HASH::find(char *key) const
{
   CriticalSection cs(&mutex);
   assert(table());
   hash_node *e, **list = &table()[hash(key)];

   for (e = *list; e != 0; e = e->next())
      if (!strcmp((char *) e->key(), key))
         return e->data();

   return(0);
}

void
HASH::get_items(ARRAY<long> &keys, ARRAY<void *> &items) const
{
   hash_node *seq_elt = 0;
   int        seq_val = -1;
   long       key;
   void      *data;
   keys.clear();
   items.clear();
   while (next_seq(key, data, seq_elt, seq_val)) {
       items += data;
       keys  += key;
   }
}

/*************************************************************************
 * Function Name: HASH::next_seq
 * Parameters: long &key, void *&data
 * Returns: int
 * Effects: 
 *************************************************************************/
int
HASH::next_seq(long &key, void *&data, hash_node *&seq_elt, int &seq_val) const
{
   // If no items, return 0
   if (!table()) return 0;

   while (seq_elt == 0) {
      if (seq_val == _size) 
         return 0;
      seq_elt = table()[++seq_val];
   }

   key  = seq_elt->key();
   data = seq_elt->data();

   seq_elt = seq_elt->next();

   return 1;
}

/*************************************************************************
 * Function Name: HASH::hash
 * Parameters: char *
 * Returns: long
 * Effects: 
 *************************************************************************/
long
HASH::hash(const char *key) const
{
   /* first value should be 0 */
   static int list[] = { 0, 4, 8, 12, 11, 5, 1, 9, 4, 10, 7, 1 };
   long i, k = 0;

   if (key == 0 || *key == '\0') return 0;

   for(i=0; key[i] != '\0'; ++i)  k += (long)(key[i]) << list[i % 12];
   return hash(k);
}

/***********************************************************************
 * Method    : HASH::clear
 * Parameters: 
 * Returns   : void
 * Effects   : 
 ***********************************************************************/
void
HASH::clear() 
{
   hash_node **list, *e, *next;
   int 		   i;

   for (i = _size, list = &_table[i]; i--; --list) {
      for (e = *list; e; e = next) {
	 next = e->next();
	 delete e;
      }
      _table[i] = 0;
   }
}

double 
HASH::load_factor() const
{
   // return avg number of elements per slot

   if (_size < 1)
      return 0;

   int count = 0;
   for (int i = 0; i < _size; i++)
      for (hash_node* e = _table[i]; e; e = e->next())
         count++;

   return double(count)/_size;
}
