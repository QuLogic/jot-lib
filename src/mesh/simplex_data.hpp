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
/*****************************************************************
 * simplex_data.H
 *****************************************************************/
#ifndef SIMPLEX_DATA_H_IS_INCLUDED
#define SIMPLEX_DATA_H_IS_INCLUDED

#include "net/rtti.H"
#include "mlib/points.H"

#include <vector>

using namespace mlib;

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

class Bsimplex;
/*****************************************************************
 * SimplexData:
 *
 *      Base class for data attached to a Bsimplex; a Bsimplex is a
 *      mesh element such as a vertex, edge, or face.
 *****************************************************************/
class SimplexData;
typedef const SimplexData CSimplexData;
class SimplexData {
 public:
   //******** MANAGERS ********

   // Store the data on the simplex with a given "owner ID":
   void set(uintptr_t id, Bsimplex* s);
   void set(const string& str, Bsimplex* s) { set((uintptr_t)str.c_str(), s); }

   SimplexData(uintptr_t key, Bsimplex* s): _id(0), _simplex(nullptr) { set(key, s);}
   SimplexData(const string& str, Bsimplex* s): _id(0), _simplex(nullptr) { set(str, s);}
  
   virtual ~SimplexData();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS_BASE("SimplexData", CSimplexData*);

   //******** ACCESSORS ********
   uintptr_t  id()                      const   { return _id; }
   Bsimplex*  simplex()                 const   { return _simplex; }

   //******** VIRTUAL METHODS ********

   // Notification that the simplex changed, meaning:
   //
   //   For a vertex: It moved.
   //
   //   For an edge or face:
   //      Its shape changed. I.e. one of its vertices moved.
   //
   virtual void notify_simplex_changed()  {}

   // Notification that the normal of the simplex changed, meaning:
   //
   //   For a face: One of its vertices moved.
   //
   //   For an edge or vertex:
   //      An adjacent face changed shape, or was added or removed.
   //
   virtual void notify_normal_changed()   {}

   // Notification that simplex was transformed -- i.e. the
   // simplex is a vertex and its location was changed by being
   // multiplied by the given Wtransf. This notification will
   // follow a call to notify_simplex_changed():
   virtual void notify_simplex_xformed(CWtransf&) {}

   // Notification that the simplex has been deleted:
   virtual void notify_simplex_deleted()  { _simplex = nullptr; delete this; }

   // Notification that the simplex was split, generating the
   // given simplex.  E.g., when an edge is split, it generates a
   // new vertex at the middle of the edge; the edge is redefined
   // to run from one of the original vertices to the new vertex,
   // and a new edge is generated running from the new vertex to
   // the other original vertex. So notify split will be called
   // twice, once with the new vertex and once with the new edge:
   virtual void notify_split(Bsimplex*)   {}

   // Notification that the simplex generated its subdiv elements:
   virtual void notify_subdiv_gen() {}

   // Query issued to data on edges and faces:
   // Does the data want to handle assigning the subdivision location?
   virtual bool handle_subdiv_calc()  { return false; }

 //*****************************************************************
 protected:
   uintptr_t _id;       // unique ID of the owner of this data
   Bsimplex* _simplex;  // the simplex this is attached to.
};

/*****************************************************************
 * SimplexDataList:
 *
 *      Convenience class -- an array of SimplexData pointers.
 *      Can lookup a SimplexData item from its owner's unique ID,
 *      or "key."
 *****************************************************************/
class SimplexDataList : public vector<SimplexData*> {
 public:
   //******** MANAGERS ********
   SimplexDataList(int n=0) : vector<SimplexData*>() { reserve(n); }
   ~SimplexDataList();

   //******** LOOKUP ********
   SimplexData* get_item(uintptr_t key) const {
      for (vector<SimplexData*>::size_type k=0; k<size(); k++)
         if (at(k)->id() == key)
            return at(k);
      return nullptr;
   }

   //******** CONVENIENCE METHODS ********
   void notify_split(Bsimplex* new_simp) const {
      for (vector<SimplexData*>::size_type k=0; k<size(); k++)
         at(k)->notify_split(new_simp);
   }
   void notify_simplex_changed() const {
      for (vector<SimplexData*>::size_type k=0; k<size(); k++)
         at(k)->notify_simplex_changed();
   }
   void notify_normal_changed() const {
      for (vector<SimplexData*>::size_type k=0; k<size(); k++)
         at(k)->notify_normal_changed();
   }
   void notify_simplex_xformed(CWtransf& xf) const {
      for (vector<SimplexData*>::size_type k=0; k<size(); k++)
         at(k)->notify_simplex_xformed(xf);
   }
   void notify_simplex_deleted() const {
      for (vector<SimplexData*>::size_type k=0; k<size(); k++)
         at(k)->notify_simplex_deleted();
   }
   void notify_subdiv_gen() const {
      for (vector<SimplexData*>::size_type k=0; k<size(); k++)
         at(k)->notify_subdiv_gen();
   }
   bool  handle_subdiv_calc() {
      // They all get a chance...
      // return true if one or more take it:
      bool ret = false;
      for (vector<SimplexData*>::size_type k=0; k<size(); k++)
         ret = at(k)->handle_subdiv_calc() || ret;
      return ret;
   }
};

#endif // SIMPLEX_DATA_H_IS_INCLUDED

// end of file simplex_data.H
