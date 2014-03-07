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

#include <set>
#include <vector>

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
class SimplexArray : public vector<T> {
 public:

   //******** MANAGERS ********

   SimplexArray(const vector<T>& list) : vector<T>(list) {}
   explicit SimplexArray(int n=0)      : vector<T>()     { vector<T>::reserve(n); }
   explicit SimplexArray(T s)          : vector<T>()     { this->push_back(s); }

   virtual ~SimplexArray() {}

   //******** CONVENIENCE ********

   // Clear all the flags
   void clear_flags() const {
      for (size_t i=0; i<size(); i++)
         (*this)[i]->clear_flag();
   }

   // Set all the flags
   void set_flags(uchar b=1) const {
      for (size_t i=0; i<size(); i++)
         (*this)[i]->set_flag(b);
   }

   // Increment all the flags
   void inc_flags(uchar b=1) const {
      for (size_t i=0; i<size(); i++)
         (*this)[i]->inc_flag(b);
   }

   // Clear a particular bit on all the simplices:
   void clear_bits(uint b) const {
      for (size_t i=0; i<size(); i++)
         (*this)[i]->clear_bit(b);
   }

   // Set a particular bit on all the simplices:
   void set_bits(uint b, int x=1) const {
      for (size_t i=0; i<size(); i++)
         (*this)[i]->set_bit(b, x);
   }

   // If whole list is from same mesh, returns the mesh:
   BMESH* mesh() const {
      if (empty()) return nullptr;
      BMESH* ret = (*this)[0]->mesh();
      for (size_t i=1; i<size(); i++)
         if (ret != (*this)[i]->mesh())
            return nullptr;
      return ret;         
   }

   // Returns true if the list is empty or they all belong to
   // the same mesh
   bool same_mesh() const { return empty() || mesh() != nullptr; }
   
   vector<BMESH*> get_meshes() const {
     vector<BMESH*> ret;
     set<BMESH*> unique;
     for (size_t i=0; i<size(); i++) {
       pair<set<BMESH*>::iterator,bool> result;
       result = unique.insert((*this)[i]->mesh());
       if (result.second)
          ret.push_back(*result.first);
     }

     return ret;
   }

   // delete all elements:
   void delete_all() {
      while (!empty()) {
         delete this->back();
         this->pop_back();
      }
   }

   //******** SET OPERATIONS ********

   // Return true if every element of list is also an element of this
   // (evaluates to true if list is empty):
   bool contains_all(const L& list) const {
      list.clear_flags();
      set_flags(1);
      for (size_t i=0; i<list.size(); i++)
         if (list[i]->flag() == 0)
            return false;
      return true;
   }

   // Return true if any element of list is also an element of this
   // (evaluates to false if list is empty):
   bool contains_any(const L& list) const {
      list.clear_flags();
      set_flags(1);
      for (size_t i=0; i<list.size(); i++)
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
      for (size_t i=0; i<size(); i++) {
         if ((*this)[i]->flag())
            return true;
         (*this)[i]->set_flag(1);
      }
      return false;
   }

   // Return the elements of this list, without duplicates
   L unique_elements() const {
      L ret(size());
      clear_flags();
      for (size_t i=0; i<size(); i++) {
         if (!(*this)[i]->flag()) {
            ret.push_back((*this)[i]);
            (*this)[i]->set_flag(1);
         }
      }
      return ret;
   }

   // Return elements common to this and list:
   L intersect(const L& list) const {
      L ret(min(size(), list.size()));
      list.set_flags(1);
      clear_flags();
      for (size_t i=0; i<list.size(); i++) {
         if (list[i]->flag() == 0) {
            ret.push_back(list[i]);
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
      L ret(size());
      clear_flags();
      list.set_flags(1);
      for (size_t i=0; i<size(); i++) {
         if ((*this)[i]->flag() == 0) {
            ret.push_back((*this)[i]);
            (*this)[i]->set_flag(1);
         }
      }
      return ret;
   }

   //******** SimplexFilter METHODS ********

   L filter(CSimplexFilter& f) const {
      L ret;
      for (size_t i=0; i<size(); i++)
         if (f.accept((*this)[i]))
           ret.push_back((*this)[i]);
      return ret;
   }

   // Returns true if all simplices satisfy the given filter
   // (if empty, returns true)
   bool all_satisfy(CSimplexFilter& f) const {
      for (size_t i=0; i<size(); i++)
         if (!f.accept((*this)[i]))
            return false;
      return true;
   }

   // Returns true if any simplices satisfy the given filter
   // (if empty, returns false)
   bool any_satisfy(CSimplexFilter& f) const {
      for (size_t i=0; i<size(); i++)
         if (f.accept((*this)[i]))
            return true;
      return false;
   }

   int num_satisfy(CSimplexFilter& f) const {
      int ret = 0;
      for (size_t j=0; j<size(); j++)
         if (f.accept((*this)[j]))
            ret++;
      return ret;
   }

   // Returns the first simplex satisfied by the filter:
   T first_satisfies(CSimplexFilter& f) const {
      for (size_t i=0; i<size(); i++)
         if (f.accept((*this)[i]))
            return (*this)[i];
      return nullptr;
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
         this->reserve(size() + list.size());
         for (size_t i=0; i<list.size(); i++)
            this->push_back(list[i]);
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
      typename vector<T>::const_iterator it;
      it = std::find(begin(), end(), s);
      if (it != end())
         return it - begin();
      else
         return -1;
   }

   using vector<T>::size;
   using vector<T>::empty;
   using vector<T>::clear;
   using vector<T>::begin;
   using vector<T>::end;

 protected:

   //******** ARRAY VIRTUAL METHODS ********

   virtual void  append_ele(const T& s) {
      // Don't permit nullptr pointers to be inserted
      if (s) {
         vector<T>::push_back(s);
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
   Bsimplex_list(const vector<Bsimplex*>& list) :
      SimplexArray<Bsimplex_list,Bsimplex*>(list) {}
};

#endif
