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
#ifndef HASH_NODE_INCLUDED
#define HASH_NODE_INCLUDED


class hash_node {
    private:
	long       _key;
	void      *_data;
	hash_node *_next;
    public:
        hash_node(long key, void *value, hash_node *next)
	    {_key = key; _data = value; _next = next;}
	hash_node(const hash_node &old)
	    {_key = old._key; _data = old._data;
	     _next = old._next ? new hash_node(*old._next) : 0;}
        hash_node() {_key = 0; _data = 0; _next = 0;}
    	hash_node *next() const       {return _next;}
    	void       next(hash_node *n) {_next = n;}
    	void      *data() const       {return _data;}
    	void      *&data_ptr()        {return _data;}
    	long       key()  const       {return _key;}
    	void       data(void *data)   {_data = data;}
};

#define CHASH const HASH
template <class T>
class ARRAY;

class HASH {
    private:
	int         _size;
        hash_node **_table;
	int         _mask;
	int         _seq_val;
	hash_node **table() const {return _table;}
	int         _lastval;
	int  hash(const long key) const;
	long hash(const char *) const;
        // Returns 0 if done
	int    next_seq(long &key, void *&data, hash_node *&seq_elt,
                        int &seq_val) const;
    public:
    	HASH(int size);
	HASH(const HASH &hash_table);
	~HASH();
	void clear();
	// Add - returns 0 if doesn't already exist, 1 if it does
	int    add(long key, void *data);
	int    add(const char *key, void *data) {char *tmp; return add(key, data, tmp, 0);}
	int    add(const char *key, void *data, char *&loc, int create_new=1);
	int    add(const char *key, char *&loc) {return add(key, (void *) _lastval++, loc);}
	int    del(long key);
	void  *find(long key) const {void **data = find_addr(key); return data ? *data : 0;}
	void  *find(char *key) const;
	void **find_addr(long key) const;
	int    bfind(long key, void *&data) const;

        void   get_items(ARRAY<long> &keys, ARRAY<void *> &data) const;

        double load_factor() const; // avg number of elements per slot
};
#endif
