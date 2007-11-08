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
 * bbase.H
 *****************************************************************/
#ifndef BBASE_H_IS_INCLUDED
#define BBASE_H_IS_INCLUDED

#include "map3d/bnode.H"
#include "mesh/lmesh.H"
#include "std/stop_watch.H"
#include "std/config.H"

#include "meme.H"

class TEXBODY;
class Bpoint;
class Bcurve;
class Bsurface;
class Bpoint_list;
class Bcurve_list;
class Bsurface_list;
class Action;
/*****************************************************************
 * Bbase:
 *
 *      Convenience base class for Bpoint, Bcurve and Bsurface.
 *****************************************************************/
class Bbase_list;
class Bbase;
typedef const Bbase CBbase;
class Bbase : public Bnode, public RefImageClient, public BMESHobs {
 public:

   //******** MANAGERS ********

   // Create a top-level Bbase:
   Bbase(CLMESHptr& m=0);

   virtual ~Bbase();

   // Delete all mesh elements controlled by this, 
   // and all memes (boss or not).
   virtual void delete_elements();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Bbase", Bbase*, Bnode, CBnode*);

   //******** MESH ACCESSORS ********

   // The mesh operated on by this Bbase:
   LMESHptr   mesh()                    { return _mesh; }
   CLMESHptr& mesh()            const   { return _mesh; }

   // The control mesh in charge of the subdivision hierarchy::
   LMESH* ctrl_mesh()   const   { return _mesh ? _mesh->control_mesh() : 0; }

   // The currently displayed mesh in the subdivision hierarchy:
   LMESH* cur_mesh()    const   { return _mesh ? _mesh->cur_mesh() : 0; }

   // The level of the currently displayed subdivision mesh:
   int cur_level()      const   { return _mesh ? _mesh->cur_level() : 0; }

   // Same as above, but relative to the level of this mesh:
   int rel_cur_level()  const   { return _mesh ? _mesh->rel_cur_level() : 0; }

   // The level of this mesh in the LMESH hierarchy:
   int subdiv_level()   const   { return _mesh ? _mesh->subdiv_level() : 0; }

   // The geom that owns the whole hierarchy of subdivision meshes:
   GEOM*  geom()        const   { return _mesh ? _mesh->geom() : 0; }

   TEXBODY* texbody()   const;

   // Return true if the Bbase's mesh is in a TEXBODY and
   // is classified as a skeleton mesh there
   bool is_skel()       const;

   // Like above, but not covered by an outer surface:
   bool is_inner_skel() const;

   // Return a mesh suitable to use for building surfaces defined
   // by inflating this Bbase:
   LMESHptr get_inflate_mesh()  const;

   // virtual method to apply a transform. 
   // returns true if status is okay; false if there was a problem.
   // e.g. to transform a piece of mesh made of a uv-surface defined
   // via its boundary curves, the uv surface and boundary curves and
   // points would call the transform() method on their respective
   // maps...
   virtual bool apply_xf(CWtransf&, CMOD&) { return true; }
   bool translate(CWvec& delt, CMOD& m) {
      return apply_xf(Wtransf::translation(delt),m);
   }

   //******** Actions ********

   Action* get_action() const { return _action; }
   void    set_action(Action* a);

   //******** Meme FINDING ********

   // Each Meme is stored on its Bsimplex using either the Bbase class
   // name as the lookup key, or using the address of its Bbase owner.
   // The first method is used when the Meme is the "boss" of the
   // Bsimplex, which means it alone (among Memes) can modify the
   // Bsimplex. The other Memes are there just for informational
   // purposes (i.e. to store info).

   // Bbase class name converted to unsigned int,
   // for SimplexData lookup:
   static uint key();

   // Returns the boss meme (if any) on the given simplex:
   static Meme* find_boss_meme(CBsimplex* s) {
      return s ? (Meme*)s->find_data(key()) : 0;
   }

   // Convenience -- does upcasts:
   static VertMeme* find_boss_vmeme(CBvert* v) {
      return (VertMeme*)find_boss_meme(v);
   }
   static EdgeMeme* find_boss_ememe(CBedge* e) {
      return (EdgeMeme*)find_boss_meme(e);
   }
   static FaceMeme* find_boss_fmeme(CBface* f) {
      return (FaceMeme*)find_boss_meme(f);
   }

   // Convenience: lookup boss memes for a whole list of simplices
   static VertMemeList find_boss_vmemes(CBvert_list& verts);
   static EdgeMemeList find_boss_ememes(CBedge_list& edges);

   // More convenience: get all the boss vertex memes for a given mesh:
   static VertMemeList find_boss_vmemes(CLMESHptr& mesh) {
      return find_boss_vmemes(mesh ? mesh->verts() : Bvert_list());
   }

   static bool has_boss(CBsimplex* s) { return find_boss_meme(s) != 0; }

   // Return true if every vertex is under meme control
   static bool is_covered(CBvert_list& verts) {
      return find_boss_vmemes(verts).num() == verts.num();
   }

   // Returns the Bbase that owns the boss meme on the simplex:
   static Bbase* find_owner(CBsimplex* s) {
      Meme* m = find_boss_meme(s);
      return m ? m->bbase() : 0;
   }

   // Calls Bbase::find_owner() on each element of the list,
   // and returns the set of Bbases found:
   static Bbase_list find_owners(CBvert_list& verts);
   static Bbase_list find_owners(CBedge_list& edges);
   static Bbase_list find_owners(CBface_list& faces);
   static Bbase_list find_owners(CLMESHptr& mesh);

   // If all the elements are owned by a single Bbase, return it:
   static Bbase* find_owner(CBvert_list& verts);
   static Bbase* find_owner(CBedge_list& edges);
   static Bbase* find_owner(CBface_list& faces);

   // Find the Bbase owner of this simplex, or if there is none,
   // find the Bbase owner of its subdivision parent (if any),
   // continuing up the subdivision hierarchy until we find a
   // Bbase owner or hit the top trying.
   static Bbase*     find_controller (CBsimplex* s);
   static Bbase_list find_controllers(CBvert_list& verts);
   static Bbase_list find_controllers(CBedge_list& edges);
   static Bbase_list find_controllers(CBface_list& faces);

   //******** NON-STATIC FINDING METHODS ********

   // Finds the Meme (if any) put on the simplex by *this* Bbase:
   virtual Meme* find_meme(CBsimplex* s) const;

   // Convenience -- does upcasts:
   VertMeme* find_vert_meme(CBvert* v) const { return (VertMeme*)find_meme(v);}
   EdgeMeme* find_edge_meme(CBedge* e) const { return (EdgeMeme*)find_meme(e);}
   FaceMeme* find_face_meme(CBface* f) const { return (FaceMeme*)find_meme(f);}

   const VertMemeList& vmemes() const { return _vmemes; }

   // tells whether this Bbase owns the given simplex:
   bool owns(CBsimplex* s) const { return find_owner(s) == this; }

   //******** Meme MANAGEMENT ********

   // Adding

   // Vert memes generally need specific parameters so there is
   // no generic method for generating new vert memes. But once
   // created they can be added to the vert meme list easily
   // enough:
   virtual VertMeme* add_vert_meme(VertMeme* v);

   // Bcurve and Bsurface over-ride the following to create
   // generic EdgeMemes or FaceMemes on the given edges or
   // faces, respectively.
   virtual EdgeMeme* add_edge_meme(Ledge*) { assert(0); return 0; }
   virtual FaceMeme* add_face_meme(Lface*) { assert(0); return 0; }

   virtual EdgeMeme* add_edge_meme(EdgeMeme*) { assert(0); return 0; }
   virtual FaceMeme* add_face_meme(FaceMeme*) { assert(0); return 0; }

   // Convenience for adding a lot of faces/edges at once:
   void add_face_memes(CBface_list& faces);
   void add_edge_memes(CBedge_list& edges);

   // Removal
   virtual void rem_vert_meme(VertMeme* v);
   virtual void rem_edge_meme(EdgeMeme*  ) { assert(0); }
   virtual void rem_face_meme(FaceMeme*  ) { assert(0); }

   // Subclasses using hot lists should over-ride these:
   virtual void compute_update() const { _vmemes.compute_boss_update(); }
   virtual bool apply_update();
   bool do_update() {
      compute_update();
      return apply_update();
   }

   //******** Meme NOTIFICATION ********

   // Sub-classes that expect this should over-ride:
   virtual void notify_simplex_split(Meme*, Bsimplex*) {
      err_msg("Bbase::notify_split: not implemented");
   }

   // The vertex moved -- subclasses may care:
   virtual void notify_vert_changed(VertMeme*) {}
   
   //******** PICKING ********

   // Given screen-space point p and search radius (in pixels),
   // return the Bbase closest to p within the search radius (if
   // any). When successful, also fills in the Wpt that was hit,
   // and the hit simplex (if requested).
   //
   // Note: the simplex may not belong directly to the base if it's
   // from a subdivision level not controlled directly by a Bbase.
   //
   static Bbase* hit_bbase(CNDCpt& p, double radius, mlib::Wpt& hit,
                                Bsimplex** s=0);
   static Bbase* hit_bbase(CNDCpt& p, double radius) {
      Wpt hit;
      return hit_bbase(p, radius, hit);
   }

   // Same as above, but returns the control-level Bbase:
   static Bbase* hit_ctrl_bbase(CNDCpt& p, double rad, mlib::Wpt& hit,
                                     Bsimplex** s=0){
      Bbase* ret = hit_bbase(p, rad, hit, s);
      return ret ? ret->control() : 0;
   }
   static Bbase* hit_ctrl_bbase(CNDCpt& p, double rad) {
      Wpt hit;
      return hit_ctrl_bbase(p, rad, hit);
   }

   // most recently created:
   static Bbase* last() { return _last; }

   // returns true if it is the most recent one
   // (if the ctrl bbase of _last is also the ctrl bbase of this):
   bool is_last() const;

   //******** TIMEOUT / SELECTION / COLOR ********

   // How many seconds does the visual fadeout last when
   // selection wears off?
   virtual double fade_time() const { return 1; }

   // Weight that varies from 0 to 1 over the fade interval:
   double fade_weight() const {
      return 1.0 - clamp(_fade_timer.remaining()/fade_time(), 0.0, 1.0);
   }

   // Globally postpone timeout for another dur seconds:
   static void hold(double dur=1e6) { _fade_timer.reset(dur); }

   // Selection status:
   bool is_selected() const { return (_selected && fade_weight() < 1); }
   virtual void set_selected();
   virtual void unselect();

   // XXX - Useful to some subclasses?
   virtual void selection_changed() {}

   // Color to use when selected:
   virtual CCOLOR& selection_color() const;

   // Color to use when not selected:
   virtual CCOLOR& regular_color()   const;

   // Return the color to use for drawing based on whether this
   // is selected, or if the selection already wore off or is
   // wearing off:
   COLOR draw_color();

   //******** SELECTION LIST ********

   static Bbase_list selected_bbases();

   static void deselect_all();

   //******** SHOW/HIDE ********

   // Hide means make the Bbase disappear; show means make it reappear.
   virtual bool can_hide()      const   { return true; }
   virtual bool can_show()      const   { return true; }

   virtual void hide()                  { _is_shown = false; }
   virtual void show()                  { _is_shown = true; }

  bool is_hidden() const { return !_is_shown; }

   //******** SUBDIVISION BBASES ********

   // Bbase at top of hierarchy:
   Bbase* control() const {
      return (_parent ? _parent->control() : (Bbase*)this);
   }
   bool   is_control()  const   { return (_parent == NULL); }

   Bbase* parent()      const   { return _parent; }
   Bbase* child()       const   { return _child; }

   // Set the parent, which also sets the mesh:
   virtual void set_parent(Bbase* p);

   // Return child k levels down from this one.
   // Positive k corresponds to finer subdiv level,
   // negative k corresponds to coarser, and
   // k == 0 means this Bbase.
   Bbase* subdiv_bbase(int k) const {
      if (k ==0) return (Bbase*)this;
      if (k > 0)
         return _child  ? _child-> subdiv_bbase(k-1) : 0;
      else
         return _parent ? _parent->subdiv_bbase(k+1) : 0;
   }

   // The level of this Bbase in the Bbase hierarchy:
   int    bbase_level() const   { return _bbase_level; }

   // Return child Bbase (if any) operating on the current
   // subdiv mesh:
   Bbase* cur_subdiv_bbase() const;

   // A Bbase can either propagate a copy of itself onto the
   // next-level subdivision mesh, or it can opt out and let
   // normal smooth subdivision rules take over the job of
   // shaping the mesh at finer levels. When "res level"
   // (informally, level of "resoluteness") is zero, the Bbase
   // opts out. When the res level is positive, the Bbase
   // propagates a child Bbase to the next level. The child
   // inherits a res level of one less than that of its parent.
   // Having a notion of a variable "res level" is a
   // generalization of the idea of "variable sharpness creases."

   int res_level()      const   { return _res_level; }

   // The "absolute" res level, expressed relative to the LMESH
   // hierarchy (e.g. level 0 means the control mesh):
   int abs_res_level()  const   { return res_level() + subdiv_level(); }

   // Set the "resolution level":
   virtual void set_res_level(int r);

   // Subclasses that expect to produce subdivision Bbases should
   // override this:
   virtual void produce_child() { }

   //******** DIAGNOSTIC ********

   virtual void draw_debug() {}

   // Show where the memes are and what state they're in:
   void show_memes(CCOLOR& hot_color, CCOLOR& cold_color, float point_size);

   // Enable / disable show_memes():
   static void toggle_show_memes()      { _show_memes  = !_show_memes; }

   //******** Bnode METHODS ********

   virtual Bnode_list inputs() const;

   virtual void recompute();

   // Identifier used in diagnostic output. Bbase types print their
   // class name and level in the subdivision hierarchy.
   virtual str_ptr identifier() const;

   // Callback used called once per frame for "active" Bnodes.
   // Returns true if it wants to remain active:
   // Bbase classes use this to implement "relaxation",
   // like laplacian smoothing:
   virtual bool tick();

   virtual void activate();     // get in the active list
   virtual void deactivate();   // get out of the active list

   //******** BMESHobs METHODS ********

   virtual void notify_change    (BMESH*, BMESH::change_t)  {}
   virtual void notify_xform     (BMESH*, CWtransf&, CMOD&) {}
   virtual void notify_merge     (BMESH*, BMESH*);
   virtual void notify_split     (BMESH*, CARRAY<BMESH*>&);
   virtual void notify_delete    (BMESH*);
   virtual void notify_sub_delete(BMESH*);

   virtual void notify_subdiv_gen(BMESH*);

   // The mesh wants to be updated, so update it:
   virtual void notify_update_request(BMESH* m);

   // For BMESHobs::name() return Bnode::name()
   virtual str_ptr name() const { return Bnode::name(); }
   
   //*****************************************************************
 protected: 
   LMESHptr             _mesh;          // mesh the Bbase operates on
   bool                 _selected;      // selection flag
   Bbase*               _parent;        // parent (coarser-level Bbase)
   Bbase*               _child;         // subdivision Bbase
   int                  _bbase_level;   // our level in Bbase hierarchy
   int                  _res_level;     // levels remaining to override subdiv
   VertMemeList         _vmemes;        // vert memes
   bool                 _is_shown;      // tells whether to draw it or not
   Action*              _action;        // action that created this Bbase

   //******** STATICS ********

   static bool          _show_memes;    // for debugging, viewing meme states
   static egg_timer     _fade_timer;    // Used to timeout selection status
   static Bbase*        _last;          // last one created
   static Bbase_list    _selection_list;// selected Bbases

   //******** PROTECTED METHODS ********

   // Record the new mesh, and also:
   //   For old mesh: get out of drawables and stop observing.
   //   For new mesh: get into drawables and start observing.
   virtual void set_mesh(CLMESHptr& mesh);

   void enter_drawables() {
      if (is_control() && ctrl_mesh() != NULL)
         ctrl_mesh()->drawables() += this;
   }
   void exit_drawables() {
      if (is_control() && ctrl_mesh() != NULL)
         ctrl_mesh()->drawables() -= this;
   }

   void delete_child() {
      delete _child;
      _child = 0;
   }
};

/*****************************************************************
 * _Bbase_list:
 *
 *      Convenience array for Bbase derived types.
 *****************************************************************/
template <class L, class T>
class _Bbase_list : public RIC_array<T> {
 public:
   using RIC_array<T>::num;
   using RIC_array<T>::empty;
   using RIC_array<T>::first;

   //******** MANAGERS ********
   _Bbase_list(int n=0)                  : RIC_array<T>(n)    {}
   _Bbase_list(const RIC_array<T>& list) : RIC_array<T>(list) {}

   // Construct the list with a single element:
   _Bbase_list(T* el) : RIC_array<T>(1) { add(el); }

   Bnode_list bnodes() const {
      Bnode_list ret;
      for (int i=0; i<num(); i++)
         ret += (*this)[i];
      return ret;
   }

   //******** CONVENIENCE ********

   // Returns true if the list is empty or they all belong to
   // the same mesh
   bool same_mesh() const {
      for (int i=1; i<num(); i++)
         if ((*this)[i]->mesh() != (*this)[0]->mesh())
            return false;
      return true;
   }

   // Returns mesh that all belong to, or none
   LMESH* mesh() const {
      return (same_mesh() && !empty()) ? first()->mesh() : 0;
   }

   // Set the "resolution level":
   void set_res_level(int r) const {
      for (int i=0; i<num(); i++)
         (*this)[i]->set_res_level(r);
   }

   // Find the minimum res level in the list
   int min_res_level() const {
      if (empty()) return 0;
      int ret = (*this)[0]->res_level();
      for (int i=1; i<num(); i++)
         ret = min(ret, (*this)[i]->res_level());
      return ret;
   }

   // Find the maximum res level in the list
   int max_res_level() const {
      if (empty()) return 0;
      int ret = (*this)[0]->res_level();
      for (int i=1; i<num(); i++)
         ret = max(ret, (*this)[i]->res_level());
      return ret;
   }

   // Variations dealing with "absolute" res level:
   int min_abs_res_level() const {
      if (empty()) return 0;
      int ret = (*this)[0]->abs_res_level();
      for (int i=1; i<num(); i++)
         ret = min(ret, (*this)[i]->abs_res_level());
      return ret;
   }
   int max_abs_res_level() const {
      if (empty()) return 0;
      int ret = (*this)[0]->abs_res_level();
      for (int i=1; i<num(); i++)
         ret = max(ret, (*this)[i]->abs_res_level());
      return ret;
   }

   void print_identifiers() const {
      if (empty()) {
         cerr << "empty";
      } else {
         for (int i=0; i<num(); i++) {
            cerr << (*this)[i]->identifier();
            if (i < num() - 1)
               cerr << ", ";
         }
      }
      cerr << endl;
   }

   L get_ctrl_list() const {
      L ret(num());
      for (int i=0; i<num(); i++) {
         assert((*this)[i]->control());
         ret += (*this)[i]->control();
      }
      return ret;
   }

   bool apply_xf(CWtransf& xf, CMOD& m) {
      bool ret = true;
      for (int i=0; i<num(); i++)
         ret = (*this)[i]->apply_xf(xf,m) && ret;
      return ret;
   }
   bool translate(CWvec& delt, CMOD& m) {
      return apply_xf(Wtransf::translation(delt), m);
   }

   //******** DRAWING ********

   // Draw them all in debug style:
   void draw_debug() const {
      for (int i=0; i<num(); i++)
         (*this)[i]->draw_debug();
   }

   //******** SELECTION ********

   void select() const {
      for (int i=0; i<num(); i++)
         (*this)[i]->set_selected();
   }
   void unselect() const {
      for (int i=0; i<num(); i++)
         (*this)[i]->unselect();
   }
   bool any_selected() const {
      for (int i=0; i<num(); i++)
         if ((*this)[i]->is_selected())
            return true;
      return false;
   }
   bool all_selected() const {
      for (int i=0; i<num(); i++)
         if (!(*this)[i]->is_selected())
            return false;
      return true;      // returns true if the list is empty
   }

   //******** SHOW/HIDE ********

   void show() const {
      for (int i=0; i<num(); i++)
         (*this)[i]->show();
   }
   void hide() const {
      for (int i=0; i<num(); i++)
         (*this)[i]->hide();
   }

   //******** ACTIVATION ********

   void activate() const {
      for (int i=0; i<num(); i++)
         (*this)[i]->activate();
   }
};


/*****************************************************************
 * Bbase_list
 *****************************************************************/
class Bbase_list : public _Bbase_list<Bbase_list,Bbase> {
 public:
   //******** MANAGERS ********
   Bbase_list(int n=0) :
      _Bbase_list<Bbase_list,Bbase>(n)    {}
   Bbase_list(const Bbase_list& list) :
      _Bbase_list<Bbase_list,Bbase>(list) {}
   Bbase_list(Bbase* c) :
      _Bbase_list<Bbase_list,Bbase>(c)    {}
};
typedef const Bbase_list CBbase_list;

/*****************************************************************
 * BbaseOwnerFilter:
 *
 *    Accepts a Bsimplex if it is owned by a particular Bbase.
 *****************************************************************/
class BbaseOwnerFilter : public SimplexFilter {
 public:
   BbaseOwnerFilter(Bbase* bb) : _bb(bb) {}

   virtual bool accept(CBsimplex* s) const {
      return _bb ? _bb->owns(s) : false;
   }

 protected:
   Bbase* _bb;
};

/*****************************************************************
 * BbaseBoundaryFilter:
 *
 *    Accepts a Bedge if it lies on the boundary of a given Bbase
 *****************************************************************/
class BbaseBoundaryFilter : public SimplexFilter {
 public:
   BbaseBoundaryFilter(Bbase* bb) : _bb(bb) {}

   virtual bool accept(CBsimplex* s) const {
      if (!is_edge(s))
         return false;
      Bedge* e = (Bedge*)s;
      return XOR(_bb->find_meme(e->f1())==0, 
                 _bb->find_meme(e->f2())==0);
   }

 protected:
   Bbase* _bb;
};

#endif // BBASE_H_IS_INCLUDED

/* end of file bbase.H */
