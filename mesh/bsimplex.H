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
 * bsimplex.H
 *****************************************************************/
#ifndef BSIMPLEX_H_IS_INCLUDED
#define BSIMPLEX_H_IS_INCLUDED

#include "simplex_data.H"

class Bsimplex;
class Bvert;
class Bedge;
class Bface;

typedef const Bsimplex CBsimplex;
typedef const Bvert CBvert;
typedef const Bedge CBedge;
typedef const Bface CBface;

class BMESH;
class Bsimplex_list;
/*****************************************************************
 * SimplexFilter:
 *
 *      Accepts or rejects a simplex based on some criteria.
 *      Other filters are defined in bsimplex.H and bedge.H
 *      (and elsewhere).
 *****************************************************************/
class SimplexFilter {
 public:
   //******** MANAGERS ********
   SimplexFilter() {}
   virtual ~SimplexFilter() {}

   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const { return s != 0; }
};
typedef const SimplexFilter CSimplexFilter;

/*****************************************************************
 * Bsimplex:
 *
 *      Base class for BMESH vertices, edges and faces.
 *****************************************************************/
class Bsimplex;
typedef const Bsimplex CBsimplex;
class Bsimplex {
 public:

   //******** FLAG BITS ********

   // For accessing bits in member variable _flag.
   // First 2 bits are used in graph searches; remaining
   // bits are available for recording boolean states.
   enum {
      FLAG_BITS = 2,            // first 2 bits store flag value
      SELECTED_BIT,             // simplex is currently selected
      NEXT_AVAILABLE_BIT        // next available bit for derived classes
   };

   //******** MANAGERS ********

   Bsimplex() : _key(0), _flag(0), _mesh(0), _data_list(0) {}
   virtual ~Bsimplex();

   //******** ACCESSORS ********

   void   set_mesh(BMESH* mesh)         { _mesh = mesh; }
   BMESH* mesh()                const   { return _mesh; }

   //******** KEY/LOOKUP ********

   uint  key() const { return _key ? _key : ((Bsimplex*)this)->generate_key();}
   static Bsimplex* lookup(uint k) {
      return (k<(uint)_table.num()) ? _table[k] : 0;
   }

   //******** DIMENSION ********
   //   vertex: 0
   //     edge: 1
   //     face: 2
   virtual int dim() const = 0;

   //******** INDEX ********

   // index in BMESH Bvert, Bedge, or Bface array:
   virtual int  index() const = 0;
   
   //******** FLAGS/BITS ********

   // "flag" value that can store values 0..3:
   uint flag()                  const   { return _flag & FLAG_MASK; }
   void clear_flag()                    { _flag &= ~FLAG_MASK; }
   void set_flag(uchar b=1)             { clear_flag(); _flag |= b; }

   // increment the flag value (mod 4):
   void inc_flag(uint i) {
      set_flag(uchar((flag() + i) % (1 << FLAG_BITS)));
   }

   bool is_set(uint b)          const   { return (_flag & mask(b)) ? 1 : 0; }
   bool is_clear(uint b)        const   { return !is_set(b); }

   void clear_bit(uint b)               { _flag &= ~mask(b); }
   void set_bit(uint b, int x=1) {
      clear_bit(b); if(x) _flag |= mask(b);
   }

   //******** SELECTION ********

   //  To change the selection status of a Bsimplex, use
   //  MeshGlobal::select() and MeshGlobal::deselect().

   // Is it selected?
   bool is_selected()   const   { return is_set(SELECTED_BIT); }

   //******** SIMPLEX DATA ********

   SimplexData* find_data(uint key) const {
      return _data_list ? _data_list->get_item(key) : 0;
   }

   SimplexData* find_data(Cstr_ptr& s) const { return find_data((uint)**s);}
   SimplexData* find_data(void *key)   const { return find_data((uint)key);}
   
   void add_simplex_data(SimplexData* sd);
   void rem_simplex_data(SimplexData* sd) {
      if (_data_list)
         _data_list->rem(sd);
   }

   // For debugging only
   const SimplexDataList* data_list() const { return _data_list;  }

   //******** NOTIFICATIONS ********

   virtual void notify_split(Bsimplex *new_simp);
   virtual void notify_xform(CWtransf& xf);
   virtual void geometry_changed(); // vert moved, edge changed length, etc
   virtual void normal_changed();   // a vert's neighbor moved, etc

   //******** PROJECTIONS ********

   virtual void project_barycentric(CWpt &loc, mlib::Wvec &bc) const = 0;
   virtual void bc2pos(CWvec& bc, mlib::Wpt& pos) const = 0;

   Wpt bc2pos(mlib::CWvec& bc) const {
      Wpt ret;
      bc2pos(bc, ret);
      return ret;
   }

   Wpt& project_to_simplex(mlib::CWpt &pos, mlib::Wpt &ret) {
      Wvec bc;
      project_barycentric(pos, bc);
      clamp_barycentric(bc);
      bc2pos(bc, ret);
      return ret;
   }

   virtual Bsimplex* bc2sim(CWvec& bc) const = 0;

   static void clamp_barycentric(Wvec &bc) {
      bc.set(max(bc[0],0.0), max(bc[1],0.0), max(bc[2],0.0));
      bc /= (bc[0] + bc[1] + bc[2]);
   }

   //******** INTERSECTION ********

   // Intersection w/ ray from given screen point -- returns the point
   // on the Bsimplex that is nearest to the given screen space point.
   // Note: the returned "near point" and "normal" are both
   //       transformed to world space.
   virtual bool view_intersect(
      CNDCpt&,  // Given screen point. Following are returned:
      Wpt&,     // Near point on the simplex IN WORLD SPACE (not object space)
      double&,  // Distance from camera to near point
      double&,  // Screen distance (in PIXELs) from given point
      Wvec& n   // "normal" vector at intersection point IN WORLD SPACE
      ) const = 0;

   // Convenience, when you just want the hit point:
   virtual bool near_point(CNDCpt& p, mlib::Wpt& hit) const {
      double d, d2d; Wvec n;
      return view_intersect(p, hit, d, d2d, n);
   }

   //******** LOCAL SEARCH AND NEAREST POINTS ********

   //**** NEW VERSION ****

   // Do a local search over the mesh, starting from this Bsimplex,
   // crossing to neighboring Bsimplexes as long as they are closer to
   // the target point and they are accepted by the filter. Stop when
   // reaching a Bsimplex that is locally closest (no neighbors are
   // closer), and return that one.
   Bsimplex* walk_to_target(
      CWpt& target, CSimplexFilter& filter = SimplexFilter()
      ) const;
   Bsimplex* walk_to_target(
      CWpt& target,
      Wpt&  near_pt,    // return val
      Wvec& near_bc,    // return val
      CSimplexFilter& filter = SimplexFilter()
      ) const {
      Bsimplex* ret = walk_to_target(target, filter);
      assert(ret);
      near_pt = ret->nearest_pt(target, near_bc);
      return ret;
   }

   // return a list of adjacent Bsimplexes:
   virtual Bsimplex_list neighbors() const = 0;

   // Distance from this Bsimplex to the given point:
   double dist(CWpt& p)           const { return nearest_pt(p   ).dist(p); }
   double dist(CWpt& p, mlib::Wvec& bc) const { return nearest_pt(p,bc).dist(p); }

   //**** OLD VERSION ****

   virtual bool local_search(
      Bsimplex *&end, Wvec &final_bc,
      CWpt &target, mlib::Wpt &reached,
      Bsimplex *repeater = 0, int iters = 30
      ) = 0;

   virtual NDCpt nearest_pt_ndc(mlib::CNDCpt&, mlib::Wvec&, int&)               const = 0;
   virtual Wpt   nearest_pt(mlib::CWpt& p, mlib::Wvec &bc, bool &is_on_simplex) const = 0;
   virtual Wpt   nearest_pt(mlib::CWpt& p, mlib::Wvec& bc) const {
      bool b;
      return nearest_pt(p,bc,b);
   }
   virtual Wpt nearest_pt(mlib::CWpt& p) const {
      Wvec bc;
      return nearest_pt(p,bc);
   }

   // probably should be replaced:
   virtual bool   on_face(const Bface* f)       const = 0;
   virtual Bface* get_face()                    const = 0;

 //*******************************************************
 // PROTECTED
 //*******************************************************
 protected:
   uint              _key;       // unique id for looking up this simplex
   uint              _flag;      // for graph searching and boolean states
   BMESH*            _mesh;      // mesh containing this simplex
   SimplexDataList*  _data_list; // optional simplex data list

   //******** INTERNAL METHODS ********

   // called once (first time _key is accessed):
   uint generate_key();

   // Table for looking up a Bsimplex from its "key" value. First slot
   // contains a null item, so index of 0 always looks up a null item.
 public:
   class IDtable : public ARRAY<Bsimplex*> {
    public:
      IDtable(int max) : ARRAY<Bsimplex*>(max) { *this += (Bsimplex*)0; }
   };
 protected:
   static IDtable       _table;

   enum { FLAG_MASK = ((1 << FLAG_BITS) - 1) };

   // convert a bit number to corresponding mask:
   static uint mask(uint b) { return (1 << (b - 1)); }
};

inline bool
is_vert(CBsimplex* s)
{
   return (s && (s->dim() == 0));
}

inline bool
is_edge(CBsimplex* s)
{
   return (s && (s->dim() == 1));
}

inline bool
is_face(CBsimplex* s)
{
   return (s && (s->dim() == 2));
}

inline BMESH*
get_mesh(CBsimplex* s) 
{
   return s ? s->mesh() : (BMESH*)0;
}

inline Bface*
get_bface(CBsimplex* s) 
{
   return s ? s->get_face() : (Bface*)0;
}

#endif // BSIMPLEX_H_IS_INCLUDED

// end of file bsimplex.H
