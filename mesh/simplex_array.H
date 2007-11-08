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
#ifndef SIMPLEX_ARRAY_H_IS_INCLUDED
#define SIMPLEX_ARRAY_H_IS_INCLUDED

#include "bsimplex.H"
#include "simplex_filter.H"

class SimplexFilter;
typedef const SimplexFilter CSimplexFilter;
/*****************************************************************
 * SimplexArray:
 *
 *      Convenience array for Bsimplex derived types.
 *      L is the list type itself; T is Bsimplex*,
 *      Bvert*, Bedge* or Bface* (or derived types).
 *
 *      Note: null pointers should never be added to the list.
 *
 *****************************************************************/
template <class L, class T>
class SimplexArray : public ARRAY<T> {
 public:

   //******** MANAGERS ********

   SimplexArray(const ARRAY<T>& list)  : ARRAY<T>(list) {}
   explicit SimplexArray(int n=0)      : ARRAY<T>(n)    {}
   explicit SimplexArray(T s)          : ARRAY<T>()     { add(s); }

   virtual ~SimplexArray() {
      if (_do_index)
         end_index();
   }

   //******** CONVENIENCE ********

   // Clear all the flags
   void clear_flags() const {
      for (int i=0; i<num(); i++)
         (*this)[i]->clear_flag();
   }

   // Set all the flags
   void set_flags(uchar b=1) const {
      for (int i=0; i<num(); i++)
         (*this)[i]->set_flag(b);
   }

   // Increment all the flags
   void inc_flags(uchar b=1) const {
      for (int i=0; i<num(); i++)
         (*this)[i]->inc_flag(b);
   }

   // Clear a particular bit on all the simplices:
   void clear_bits(uint b) const {
      for (int i=0; i<num(); i++)
         (*this)[i]->clear_bit(b);
   }

   // Set a particular bit on all the simplices:
   void set_bits(uint b, int x=1) const {
      for (int i=0; i<num(); i++)
         (*this)[i]->set_bit(b, x);
   }

   // If whole list is from same mesh, returns the mesh:
   BMESH* mesh() const {
      if (empty()) return 0;
      BMESH* ret = (*this)[0]->mesh();
      for (int i=1; i<num(); i++)
         if (ret != (*this)[i]->mesh())
            return 0;            
      return ret;         
   }

   // Returns true if the list is empty or they all belong to
   // the same mesh
   bool same_mesh() const { return empty() || mesh() != NULL; }
   
   ARRAY<BMESH*> get_meshes() const {
     ARRAY<BMESH*> ret;
     ret.set_unique();
     for (int i=0; i<num(); i++)
       ret += (*this)[i]->mesh();

     return ret;
   }

   // delete all elements:
   void delete_all() {
      bool indexing_was_on = _do_index;
      if (_do_index)
         end_index();
      while (!empty())
         delete this->pop();    // g++ on macosx requires this->pop()
      if (indexing_was_on)      //  instead of just pop()
         begin_index();
   }

   //******** SET OPERATIONS ********

   // Return true if every element of list is also an element of this
   // (evaluates to true if list is empty):
   bool contains_all(const L& list) const {
      list.clear_flags();
      set_flags(1);
      for (int i=0; i<list.num(); i++)
         if (list[i]->flag() == 0)
            return false;
      return true;
   }

   // Return true if any element of list is also an element of this
   // (evaluates to false if list is empty):
   bool contains_any(const L& list) const {
      list.clear_flags();
      set_flags(1);
      for (int i=0; i<list.num(); i++)
         if (list[i]->flag() == 1)
            return true;
      return false;
   }

   // Return true if list has exactly the same elements as this:
   bool same_elements(const L& list) const {
      return contains_all(list) && list.contains_all(*this);
   }

   // Return true if any element occurs more than once:
   bool has_duplicates() const {
      clear_flags();
      for (int i=0; i<num(); i++) {
         if ((*this)[i]->flag())
            return true;
         (*this)[i]->set_flag(1);
      }
      return false;
   }

   // Return the elements of this list, without duplicates
   L unique_elements() const {
      L ret(num());
      clear_flags();
      for (int i=0; i<num(); i++) {
         if (!(*this)[i]->flag()) {
            ret += (*this)[i];
            (*this)[i]->set_flag(1);
         }
      }
      return ret;
   }

   // Return elements common to this and list:
   L intersect(const L& list) const {
      L ret(min(num(), list.num()));
      list.set_flags(1);
      clear_flags();
      for (int i=0; i<list.num(); i++) {
         if (list[i]->flag() == 0) {
            ret += list[i];
            list[i]->set_flag(1);
         }
      }
      return ret;
   }

   // Return union of this and list
   // (result does not contain duplicates):
   L union_no_duplicates(const L& list) const {
      return (*this + list).unique_elements();
   }

   // Returns elements of this that are not also in list
   L minus(const L& list) const {
      L ret(num());
      clear_flags();
      list.set_flags(1);
      for (int i=0; i<num(); i++) {
         if ((*this)[i]->flag() == 0) {
            ret += (*this)[i];
            (*this)[i]->set_flag(1);
         }
      }
      return ret;
   }

   //******** SimplexFilter METHODS ********

   L filter(CSimplexFilter& f) const {
      L ret;
      for (int i=0; i<num(); i++)
         if (f.accept((*this)[i]))
           ret += (*this)[i];
      return ret;
   }

   // Returns true if all simplices satisfy the given filter
   // (if empty, returns true)
   bool all_satisfy(CSimplexFilter& f) const {
      for (int i=0; i<num(); i++)
         if (!f.accept((*this)[i]))
            return false;
      return true;
   }

   // Returns true if any simplices satisfy the given filter
   // (if empty, returns false)
   bool any_satisfy(CSimplexFilter& f) const {
      for (int i=0; i<num(); i++)
         if (f.accept((*this)[i]))
            return true;
      return false;
   }

   int num_satisfy(CSimplexFilter& f) const {
      int ret = 0;
      for (int j=0; j<num(); j++)
         if (f.accept((*this)[j]))
            ret++;
      return ret;
   }

   // Returns the first simplex satisfied by the filter:
   T first_satisfies(CSimplexFilter& f) const {
      for (int i=0; i<num(); i++)
         if (f.accept((*this)[i]))
            return (*this)[i];
      return 0;
   }

   //******** SELECTED ELEMENTS ********

   // Convenience: get the selected or unselected elements
   L selected_elements() {
      return filter(BitSetSimplexFilter(Bsimplex::SELECTED_BIT));
   }
   L unselected_elements() {
      return filter(BitClearSimplexFilter(Bsimplex::SELECTED_BIT));
   }

   //******** ARRAY OPERATORS ********

   // Append elements of list to this one:
   void append(const L& list) {
      if (!list.empty()) {
         this->realloc(num() + list.num());
         for (int i=0; i<list.num(); i++)
            *this += list[i];
      }
   }

   // Concatenate this list with another, producing a new list:
   L operator+(const L& list) const {
      L ret = *this;
      ret.append(list);
      return ret;
   }

   //******** ARRAY VIRTUAL METHODS ********

   virtual int get_index(const T& s) const {
      if (_do_index) {
         IndexData* d = lookup_data(s);
         return d ? d->index() : -1;
      }
      return ARRAY<T>::get_index(s);
   }

   using ARRAY<T>::num;
   using ARRAY<T>::empty;
   using ARRAY<T>::clear;
   using ARRAY<T>::begin_index;
   using ARRAY<T>::end_index;
   
 protected:

   using ARRAY<T>::_do_index;

   virtual void set_index(const T& el, int i) const {
      if (_do_index)
         put_data(el)->set_index(i);
   }

   virtual void clear_index(const T& el) const {
      if (_do_index)
         delete lookup_data(el);
   }

   /*****************************************************************
    * IndexData:
    *
    *    Used to store element indices WRT a particular SimplexArray.
    *
    *****************************************************************/
   class IndexData : public SimplexData {
    public:

      //******** MANAGERS ********

      IndexData(uint key, Bsimplex* s) :
         SimplexData(key, s), _index(-1) {}

      //******** ACCESSORS ********

      int  index()                 const   { return _index; }
      void set_index(int i)                { _index = i; }

    protected:
      int  _index;
   };

   //******** ARRAY INDEXING ********

   // On the given simplex, find the data owned by this list:
   IndexData* lookup_data(const T& s) const {
      return s ? (IndexData*)s->find_data((uint)this) : 0;
   }

   // Similar to lookup_data() but creates the data if needed
   IndexData* put_data(const T& s) const {
      IndexData* ret = lookup_data(s);
      return ret ? ret : new IndexData((uint)this, (T)s);
   }

   //******** ARRAY VIRTUAL METHODS ********

   virtual void  append_ele(const T& s) {
      // Don't permit NULL pointers to be inserted
      if (s) {
         ARRAY<T>::append_ele(s);
      } else {
         err_msg("SimplexArray::append_ele: warning: ignoring NULL Simplex*");
      }
   }
};

// Actual classes based on above template:
class Bsimplex_list;    // defined below
class Bvert_list;       // defined in bedge.H
class Bedge_list;       // defined in bvert.H
class Bface_list;       // defined in Bface.H

/************************************************************
 * Bsimplex_list
 ************************************************************/
typedef const Bsimplex_list CBsimplex_list;
class Bsimplex_list : public SimplexArray<Bsimplex_list,Bsimplex*> {
 public:
   //******** MANAGERS ********
   Bsimplex_list(int n=0) :
      SimplexArray<Bsimplex_list,Bsimplex*>(n)    {}
   Bsimplex_list(CARRAY<Bsimplex*>& list) :
      SimplexArray<Bsimplex_list,Bsimplex*>(list) {}
};

#endif
