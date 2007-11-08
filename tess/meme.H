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
 * meme.H
 *****************************************************************/
#ifndef MEME_H_IS_INCLUDED
#define MEME_H_IS_INCLUDED

#include "mesh/lface.H"

class Bbase;    // controller for a region of mesh -- owns memes
class VertMeme; // meme controlling a vertex
class EdgeMeme; // meme controlling an edge
class FaceMeme; // meme controlling a face

/*****************************************************************
 * Meme
 *
 *   Base class for an agent stored on a Bsimplex by a Bbase.
 *   (I.e., by a Bpoint, Bcurve or Bsurface.)
 *
 *   Gets simplex notifications and can pass them to the
 *   Bbase. Examples are when the simplex is changed, deleted,
 *   or generates subdivision elements.
 *
 *   The three main types are vert memes, edge memes, and face
 *   memes. All types can be used to claim ownership of a
 *   simplex by a Bbase (for picking), and also to ensure that
 *   subdivision memes are propagated and that subdivision
 *   locations are computed by the subdivision Bbase instead
 *   of by the default subdivision rules.
 *
 *   In addition, vert memes have data and some procedure for
 *   recomputing the postion of a vertex.
 *****************************************************************/
class Meme : public SimplexData {
 public:

   //******** MANAGERS ********
   Meme(Bbase* base, Bsimplex* s, bool force_boss=false);

   virtual ~Meme() {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Meme", Meme*, SimplexData, CSimplexData*);

   //******** ACCESSORS ********
   Bbase* bbase()       const   { return _owner; }
   BMESH* mesh()        const   { return _simplex->mesh(); }

   int bnode_num()      const;

   // tells if the meme is in charge of its simplex
   // (it may not be if another meme is doing it):
   bool is_boss()       const;

   // "Dimension" of the Meme:
   //     VertMeme: 0
   //     EdgeMeme: 1
   //     FaceMeme: 2
   //  Q: Why not return _simplex->dim() ?
   //  A: Has to work even after the simplex has been deleted.
   virtual int dim() const = 0;

   // Acquiring / relinquishing boss status:
   // return true if a change occurred
   virtual bool take_charge();
   virtual bool get_demoted();

   virtual void gen_subdiv_memes() const = 0;

   //******** SimplexData NOTIFICATION METHODS ********

   // VertMeme, EdgeMeme and FaceMeme all over-ride these:
   virtual void notify_simplex_deleted()        = 0;
   virtual void notify_subdiv_gen()             = 0;
   virtual bool handle_subdiv_calc()            = 0;

   virtual void notify_split(Bsimplex* new_simp);

 protected:
   Bbase*    _owner;        // Bbase that owns the Meme
};

/*****************************************************************
 * VertMeme
 *
 *   Meme specific to vertices. It's responsible for
 *   updating the vertex position using whatever data and
 *   procedure is supplied in the derived type.
 *****************************************************************/
class VertMeme : public Meme {
 protected:
   static CWpt&  get_loc (CBvert* v) {
      return v ? v->loc() : Wpt::Origin();
   }
   static CWvec& get_norm(CBvert* v) {
      return v ? v->norm() : Wvec::null();
   }

 public:
   //******** MANAGERS ********
   VertMeme(Bbase* b, Lvert* v, bool force_boss=false);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("VertMeme", VertMeme*, Meme, CSimplexData*);

   //******** ACCESSORS ********

   Lvert* vert()       const { return (Lvert*) _simplex; }
   CWpt&  loc()  const { return get_loc (vert()); }
   CWvec& norm() const { return get_norm(vert()); }

   bool do_debug()              const   { return _debug; }
   void set_do_debug(bool b=true)       { _debug = b; }

   // Subdiv element:
   Lvert* subdiv_vert() const { return vert()->subdiv_vertex(); }

   // Subdiv parent simplex
   Bsimplex* parent_simplex() const { return vert()->parent(); }
   Lvert*    parent_vert()    const {
      Bsimplex* s = parent_simplex();
      return is_vert(s) ? ((Lvert*)s) : 0;
   }

   // the meme is requested to move to the given location
   // (or at least near it). it can answer true if it "succeeded."
   virtual bool move_to(CWpt&);

   //******** ADJACENCY ********

   // Lookup adjacent vert memes from same Bbase:
   VertMeme* nbr(int k)                         const;
   void get_nbrs(ARRAY<VertMeme*>& ret)    const;

   // Return adjacent EdgeMemes that have the same owner as this.
   void get_nbrs(ARRAY<EdgeMeme*>& ret)    const;

   // Return adjacent FaceMemes that have the same owner as this.
   void get_nbrs(ARRAY<FaceMeme*>& ret)    const;

   //******** BOSS STATUS ********

   // tracks_boss():
   //
   //   Returns true if:
   //     - this meme is not the boss, and
   //     - there is a boss meme on this vertex, and
   //     - this meme agrees with the current boss
   //       about where to put the vertex.
   bool tracks_boss();

   // Returns true if this meme is the boss,
   // or acts the same as the boss:
   virtual bool is_boss_like() {
      return is_boss() || tracks_boss();
   }

   // Called when boss-status changes
   // (XXX - not sure why! -lem 12/2002)
   void mark_parent_dirty() const;

   // Meme virtual methods:
   //   Acquiring / relinquishing boss status.
   //   Returns true if a change occurred:
   virtual bool take_charge();
   virtual bool get_demoted();

   //******** CHILD MANAGEMENT ********

   // The "child" meme lives on the subdivision vertex;
   // it is put there by this meme, and inherits
   // properties from this meme (e.g. uv-coords).

   // Return the "parent" meme if it exists:
   VertMeme* parent() const;

   // Return the "child" meme if it exists:
   VertMeme* child() const;

   // Similar to above, but creates it if needed:
   virtual VertMeme* get_child() const;

   // Update the child meme -- creating it if needed.
   // (E.g., can be called when attributes of this meme have changed):
   virtual VertMeme* update_child()     const;

   // Update this vert meme by getting attributes from the nearby
   // vert meme(s) in the next mesh up in the hierarchy:
   virtual void update_from_parent();

   // A sterile VertMeme can be a boss but can't generate children.
   // A non-boss VertMeme can't change the vert or generate children.
   // A potent VertMeme is both a boss and is not sterile.
   void sterilize();
   void unsterilize();
   bool is_sterile()    const;
   bool is_potent()     const;

   // is_epotent():
   //
   //   'e' refers to 'edge'; tells whether the vert meme
   //   can help generate an active (boss) child meme on the
   //   subdivision vertex of an adjacent edge:
   bool is_epotent() {
      return (!is_sterile() && is_boss_like());
   }

   //******** COMPUTING ********

   // A pinned meme won't move during smoothing
   bool is_pinned() const { return _is_pinned; }
   void pin();  // make it pinned
   void unpin();// unpin it

   static uchar max_cold_count();
   void   set_hot();
   void   set_cold()            { _cold_count = max_cold_count(); }
   bool   is_hot()      const   { return _cold_count == 0; }
   bool   is_warm()     const   { return _cold_count < max_cold_count(); }
   bool   is_cold()     const   { return !is_warm(); }
   double heat()  const {
      return 1.0 - min(1.0, double(_cold_count)/max_cold_count());
   }
   bool tick() {
      if (_cold_count < max_cold_count())
         _cold_count++;
      return is_warm();
   }

   // Target location for being more "relaxed":
   virtual Wpt  smooth_target() const;

   // Displacement to the target from current location:
   Wvec target_delt() const;    

   double lookup_rest_length(CBedge* e) const;

   // Compute where to move to a more "relaxed" position.
   // Starts with the "target delt" (above), but subclasses
   // can add in some constraint (e.g. to stick to a shape
   // while moving toward the target as much as possible):
   virtual bool compute_delt()  { return false; }

   // Move to the previously computed relaxed position:
   virtual bool apply_delt()    { return false; }

   // E.g., say it's a UVmeme clinging to a surface via a Map2D3D;
   // the following can be used to repostion the vertex after
   // the Map2D3D has changed. (Sub-classes should override):
   virtual CWpt& compute_update() { return (_update = loc()); }

   // Slave's day off:
   bool compute_boss_update() {
      if (is_boss()) {
         compute_update();
         return 1;
      }
      return 0;
   }
         
   // Apply the computed location to the vertex:
   virtual bool apply_update(double thresh = 0.01);

   // Compute and apply a new position.
   // The given threshold is multipled by the average edge
   // length around the vertex. If the new position is not
   // at least that far away from the old position, the update
   // is rejected.
   bool do_update(double thresh = 0.01) {
      compute_update();
      return apply_update(thresh);
   }

   // Called when the vertex location changed but this VertMeme
   // didn't cause it:
   virtual void vert_changed_externally();

   // For retriangulation
   // XXX - not currently used. lem 12/2002
   virtual double target_length() const { return 0; }

   //******** STATICS ********

   // The following static methods generate a child vert meme on
   // the subdivision vertex of an edge. The child receives
   // attributes that are an "average" of those from this vert
   // meme and neighboring ones of the same type as this.
   //
   // The first deals with a "strong" edge (not a quad diagonal),
   // computing the average from 2 vert memes.  The second deals
   // with a weak edge (quad diagonal), computing the average
   // from the 4 vert memes located at the corners of the quad:
   static VertMeme* gen_child(Lvert*, VertMeme*, VertMeme*);
   static VertMeme* gen_child(Lvert*, VertMeme*, VertMeme*,
                                      VertMeme*, VertMeme*);

   //******** Meme VIRTUAL METHODS ********
   virtual int dim() const { return 0; }

   virtual void gen_subdiv_memes() const;

   //******** SimplexData NOTIFICATION METHODS ********

   virtual void notify_normal_changed();
   virtual void notify_simplex_deleted();
   virtual void notify_subdiv_gen();

   // VertMemes do something special with this:
   virtual void notify_simplex_changed();

   virtual bool handle_subdiv_calc();

   //************************************************************
 protected:
   Wpt    _update;     // computed position to be applied to the vertex
   bool         _is_sterile; // if true, cannot generate children
   bool         _is_pinned;  // if true, don't move during smoothing
   uchar        _cold_count; // number of times compute_delt() had no effect
   bool         _debug;      // if true, print diagnostic stuff

   // Flag that is true while a VertMeme is setting a vertex
   // location, to suppress simplex_changed() notifications
   // from being forwarded needlessly to the Bbase:
   bool _suspend_change_notification;

   // Subclasses override the following virtual methods to
   // generate a child that mixes properties of this vert
   // meme and the given ones, after screening by the
   // VertMeme base class to ensure they're all the same
   // kind as this one. The three methods correspond to 3
   // cases:
   //   (1) A vert meme generating its own subdiv vert meme;
   //   (2) A vert meme generating the subdiv vert for one of
   //       its edges, where it will do averaging with the vert
   //       meme at the other end of the edge; and
   //   (3) A vert meme doing averaging with the other 3 vert
   //       memes of a quad, where the subdiv vert to be generated
   //       is associated with the interior edge of the quad.
   virtual VertMeme* _gen_child(Lvert*) const {
      return 0;
   }
   virtual VertMeme* _gen_child(Lvert*, VertMeme*) const {
      return 0;
   }
   virtual VertMeme* _gen_child(
      Lvert*, VertMeme*, VertMeme*, VertMeme*) const {
      return 0;
   }

   // Subclasses can override the following virtual
   // methods to copy attributes into this vert meme
   // from nearby vert memes in the next mesh up in the
   // hierarchy.  The appropriate method is chosen in
   // the public method update_from_parent(), depending
   // on whether the parent simplex is a:
   //   (1) vertex
   //   (2) "strong" edge
   //   (3) "weak" edge (quad diagonal)
   virtual void copy_attribs_v(VertMeme*) {}
   virtual void copy_attribs_e(VertMeme*, VertMeme*) {}
   virtual void copy_attribs_q(VertMeme*, VertMeme*, VertMeme*, VertMeme*) {}
};

/*****************************************************************
 * EdgeMeme
 *
 *   A meme on an edge -- gets simplex notifications.
 *   Derived types may record info governing edge operations
 *   (swap, split, or collapse).
 *****************************************************************/
class EdgeMeme : public Meme {
 public:
   //******** MANAGERS ********
   EdgeMeme(Bbase* b, Ledge* e, bool force_boss=false);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("EdgeMeme", EdgeMeme*, Meme, CSimplexData*);

   //******** MESH ELEMENTS ********

   // The edge:
   Ledge* edge()  const { return (Ledge*) _simplex; }

   // VertMemes of the same Bbase at the two endpoints:
   VertMeme* v1() const;
   VertMeme* v2() const;

   // VertMemes of the same Bbase at the opposite verts on
   // each face adjacent to the edge:
   VertMeme* vf1() const;
   VertMeme* vf2() const;

   // Subdiv elements:
   Lvert* subdiv_vert() const { return edge()->subdiv_vertex(); }
   Ledge* subdiv_e1()   const { return edge()->subdiv_edge1(); }
   Ledge* subdiv_e2()   const { return edge()->subdiv_edge2(); }

   VertMeme* child()            const;
   VertMeme* get_child()        const;
   VertMeme* update_child()     const;

   EdgeMeme* child_e1()         const;
   EdgeMeme* child_e2()         const;

   //******** RETRIANGULATION METHODS ********

   // XXX - not used (but worked in the past). lem 12/2002

   virtual bool try_swap()       { return 0; }
   virtual bool try_split()      { return 0; }
   virtual bool try_collapse()   { return 0; }

   // Returns a vertex with the same owner as this edge:
   virtual Bvert* get_collapsable_vert() {
      return v1() ? edge()->v1() : v2() ? edge()->v2() : (Bvert*)0;
   }

   // The desired length for edges:
   virtual double target_length() const { return 0; }

   // The ratio of actual length to desired length:
   double length_ratio() const {
      double t = target_length();
      return (t>0) ? (edge()->length()/t) : 1.0;
   }

   // Used in ARRAY::sort to order EdgeMemes by decreasing
   // length ratio:
   static int length_ratio_decreasing(const void* a, const void* b);

   //******** Meme VIRTUAL METHODS ********
   virtual int dim() const { return 1; }

   virtual void gen_subdiv_memes() const;

   //******** SimplexData NOTIFICATION METHODS ********
   virtual void notify_simplex_deleted();
   virtual void notify_subdiv_gen();
   virtual bool handle_subdiv_calc();

   //******** REST LENGTH ********

   // used for the spring force in mesh relaxation

   static double lookup_rest_length(CBedge*);

   void   set_rest_length(double len);
   void   clear_rest_length()           { set_rest_length(0); }
   bool   has_rest_length()     const   { return _rest_length > 0; }

   // Pass on the given rest length to both child edges
   void propagate_length(double len) const;

   // returns the cached length, or computes it on the fly:
   double rest_length() const {
      return has_rest_length() ? _rest_length : local_length(edge());
   }

   // cache current local length:
   void   freeze_rest_length() { set_rest_length(local_length(edge())); }

   // like freeze_rest_length(), but skip if it's set already:
   void   acquire_rest_length() {
      if (!has_rest_length())
         freeze_rest_length();
   }

 protected:
   double       _rest_length;

   // compute a value for rest length from current
   // conditions near the edge:
   static double local_length(CBedge*);
};

/*****************************************************************
 * FaceMeme
 *
 *   A meme on a face -- gets simplex notifications.
 *****************************************************************/
class FaceMeme : public Meme {
 public:
   //******** MANAGERS ********
   FaceMeme(Bbase* b, Lface* v);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("FaceMeme", FaceMeme*, Meme, CSimplexData*);

   //******** MESH ELEMENTS ********

   // The face:
   Lface* face()  const { return (Lface*)    _simplex; }

   // VertMemes of the same Bbase at the 3 face vertices:
   VertMeme* v1() const;
   VertMeme* v2() const;
   VertMeme* v3() const;

   // If the face is a quad, returns the vert meme for the
   // 4th vertex (NULL otherwise):
   VertMeme* vq() const;

   // Subdiv elements
   Ledge* subdiv_e1()   const { return face()->subdiv_edge1(); }
   Ledge* subdiv_e2()   const { return face()->subdiv_edge2(); }
   Ledge* subdiv_e3()   const { return face()->subdiv_edge3(); }

   Lface* subdiv_f1()   const { return face()->subdiv_face1(); }
   Lface* subdiv_f2()   const { return face()->subdiv_face2(); }
   Lface* subdiv_f3()   const { return face()->subdiv_face3(); }
   Lface* subdiv_fc()   const { return face()->subdiv_face_center(); }

   //******** Meme VIRTUAL METHODS ********
   virtual int dim() const { return 2; }

   virtual void gen_subdiv_memes() const;

   //******** SimplexData NOTIFICATION METHODS ********
   virtual void notify_simplex_deleted();
   virtual void notify_subdiv_gen();

   // Faces don't compute subdiv locations so this is never called:
   virtual bool handle_subdiv_calc() { assert(0); return 0; }
};

// Used by VertMeme list types:
template <class T>
void _compute_update(const T& L)
{
   for (int i=0; i<L.num(); i++)
      L[i]->compute_update();
}
template <class T>
bool _compute_boss_update(const T& L)
{
   bool ret = false;
   for (int i=0; i<L.num(); i++)
      ret = L[i]->compute_boss_update() || ret;
   return ret;
}
template <class T>
bool _apply_update(const T& L, double thresh)
{
   bool ret = false;
   for (int i=0; i<L.num(); i++)
      ret = L[i]->apply_update(thresh) || ret;
   return ret;
}
template <class T>
bool _compute_delt(const T& L)
{
   bool ret = false;
   for (int i=0; i<L.num(); i++)
      ret = L[i]->compute_delt() || ret;
   return ret;
}
template <class T>
bool _apply_delt(const T& L)
{
   bool ret = false;
   for (int i=0; i<L.num(); i++)
      ret = L[i]->apply_delt() || ret;
   return ret;
}
template <class T>
bool _tick(const T& L)
{
   bool ret = false;
   for (int i=0; i<L.num(); i++)
      ret = L[i]->tick() || ret;
   return ret;
}
template <class T>
bool _do_relax(const T& L)
{
   _tick(L);
   _compute_delt(L);
   return _apply_delt(L);
}
template <class T>
void _sterilize(const T& L)
{
   for (int i=0; i<L.num(); i++)
      L[i]->sterilize();
}
template <class T>
void _unsterilize(const T& L)
{
   for (int i=0; i<L.num(); i++)
      L[i]->unsterilize();
}
template <class T>
void _pin(const T& L)
{
   for (int i=0; i<L.num(); i++)
      L[i]->pin();
}
template <class T>
void _unpin(const T& L)
{
   for (int i=0; i<L.num(); i++)
      L[i]->unpin();
}
template <class T>
void _set_hot(const T& L)
{
   for (int i=0; i<L.num(); i++)
      L[i]->set_hot();
}
template <class T>
bool _is_any_warm(const T& L)
{
   for (int i=0; i<L.num(); i++)
      if (L[i]->is_warm())
         return true;
   return false;
}

/*****************************************************************
 * MemeList:
 *
 *   List of Memes with convenience methods.
 *****************************************************************/
template <class T>
class MemeList : public ARRAY<T*> {
 public:

   //******** MANAGERS ********

   MemeList(int n=0)              : ARRAY<T*>(n)    {}
   MemeList(const MemeList& list) : ARRAY<T*>(list) {}

   //******** CONVENIENCE ********

   void delete_all() {
      for (int k=0; k<num(); k++)
         delete (*this)[k];
      clear();
   }

   void notify_subdiv_gen() const {
      for (int k=0; k<num(); k++)
         (*this)[k]->notify_subdiv_gen();
   }

   using ARRAY<T*>::num;
   using ARRAY<T*>::clear;
};

/*****************************************************************
 * VertMemeList:
 *
 *   List of VertMemes with convenience methods.
 *****************************************************************/
class VertMemeList : public MemeList<VertMeme> {
 public:

   //******** MANAGERS ********

   VertMemeList(int n=0)                  : MemeList<VertMeme>(n)    {}
   VertMemeList(const VertMemeList& list) : MemeList<VertMeme>(list) {}

   //******** CONVENIENCE ********

   void compute_update()          const { _compute_update(*this); }
   bool compute_boss_update()     const { return _compute_boss_update(*this); }
   bool apply_update(double t=.01)const { return _apply_update(*this, t); }

   bool compute_delt()            const { return _compute_delt(*this); }
   bool apply_delt()              const { return _apply_delt(*this); }
   bool do_relax()                const { return _do_relax(*this); }

   void sterilize()               const { _sterilize(*this);   }
   void unsterilize()             const { _unsterilize(*this); }

   void pin()                     const { _pin(*this);   }
   void unpin()                   const { _unpin(*this); }

   void set_hot()                 const { _set_hot(*this); }

   bool is_any_warm()             const { return _is_any_warm(*this); }

   // Bvert_list methods
   Bvert_list verts() const {
      Bvert_list ret(num());
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->vert();
      return ret;
   }
   Wpt centroid()  const { return verts().center(); }

   //******** DIAGNOSTIC ********

   // report the number of memes of the given type
   int meme_count(Cstr_ptr& class_name);

   // print debugging info about what kinds of memes are here:
   // (optionaly print 'iter' value if >= 0)
   void print_debug(int iter = -1);

};
typedef const VertMemeList CVertMemeList;

/*****************************************************************
 * EdgeMemeList:
 *****************************************************************/
class EdgeMemeList : public MemeList<EdgeMeme> {
 public:

   //******** MANAGERS ********

   EdgeMemeList(int n=0)                  : MemeList<EdgeMeme>(n)    {}
   EdgeMemeList(const EdgeMemeList& list) : MemeList<EdgeMeme>(list) {}
};
typedef const EdgeMemeList CEdgeMemeList;

/*****************************************************************
 * FaceMemeList:
 *****************************************************************/
class FaceMemeList : public MemeList<FaceMeme> {
 public:

   //******** MANAGERS ********

   FaceMemeList(int n=0)                  : MemeList<FaceMeme>(n)    {}
   FaceMemeList(const FaceMemeList& list) : MemeList<FaceMeme>(list) {}
};
typedef const FaceMemeList CFaceMemeList;

#endif // MEME_H_IS_INCLUDED

// end of file meme.H
