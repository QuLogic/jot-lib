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
#ifndef LMESH_H_HAS_BEEN_INCLUDED
#define LMESH_H_HAS_BEEN_INCLUDED

#include "lstrip.H"
#include "ledge_strip.H"
#include "lvert_strip.H"
#include "subdiv_calc.H"

#define CLMESHptr const LMESHptr
/*****************************************************************
 * LMESH:
 *
 *    Subdivision mesh based on rules defined in subdiv_calc.H
 *****************************************************************/
MAKE_PTR_SUBC(LMESH,BMESH);
class LMESH : public BMESH {

  public:
   //******** MANAGERS ********
   LMESH(int num_v=0, int num_e=0, int num_f=0);

   virtual ~LMESH();

   //******** RUN-TIME TYPE IDENTIFICATION ********
   DEFINE_RTTI_METHODS3("LMESH", LMESH*, BMESH, CDATA_ITEM*);

   // XXX - Should be protected, here for debugging:

   // Blow away subdivision mesh
   void delete_subdiv_mesh();

   // Put all vertices in the dirty list:
   void mark_all_dirty();

   // verts whose subdiv verts need recalculating:
   CBvert_list& dirty_verts() const { return _dirty_verts; }

   //******** CONVENIENT UPCASTS ********
   Lvert* lv(int k) const { return (Lvert*) _verts[k]; }
   Ledge* le(int k) const { return (Ledge*) _edges[k]; }
   Lface* lf(int k) const { return (Lface*) _faces[k]; }

   Lvert* dv(int k) const { return (Lvert*) _dirty_verts[k]; }

   //******** ACCESSORS ********

   // Control mesh:
   LMESH* control_mesh() const {
      return (LMESH*) (_parent_mesh ? _parent_mesh->control_mesh() : this);
   }

   bool   is_control_mesh()     const   { return (_parent_mesh == 0); }

   // Subdivision child of this mesh:
   LMESH* subdiv_mesh()         const   { return (LMESH*)_subdiv_mesh; }

   // Parent of this mesh
   LMESH* parent_mesh()         const	{ return (LMESH*)_parent_mesh; }

   // Subdivision child at level k relative to this one:
   // (k == 1 refers to immediate child)
   LMESH* subdiv_mesh(int k) const {
      if (k == 0)
         return (LMESH*)this;
      else if (k < 0)
         return parent_mesh(-k);
      else
         return _subdiv_mesh ? _subdiv_mesh->subdiv_mesh(k-1) : 0;
   }

   // Parent at level k relative to this mesh:
   // (k == 1 refers to immediate parent)
   LMESH* parent_mesh(int k) const {
      if (k == 0)
         return (LMESH*)this;
      else if (k < 0)
         return subdiv_mesh(-k);
      else
         return _parent_mesh ? _parent_mesh->parent_mesh(k-1) : 0;
   }

   // Generate the subdivision mesh,
   // unless a system error prevents it:
   bool allocate_subdiv_mesh();

   // The currently drawn mesh in the whole subdivision hierarchy:
   LMESH* cur_mesh() const { return control_mesh()->_cur_mesh; }

   bool is_cur_mesh() const { return cur_mesh() == this; }

   // Returns 4^k, where k is the current subdivision level.
   // (Scale factor relating num faces in ctrl mesh to cur subdiv mesh).
   int subdiv_face_scale() const { return (1 << 2*cur_level()); }

   // Returns 2^k, where k is the current subdivision level.
   // (Scale factor relating num edges in ctrl mesh to cur subdiv mesh).
   int subdiv_edge_scale() const { return (1 << cur_level()); }   

   //******** SUBDIVION CALCULATION RULES ********
   SubdivLocCalc*   loc_calc()   const { return _loc_calc; }
   SubdivColorCalc* color_calc() const { return _color_calc; }

   void set_subdiv_loc_calc(SubdivLocCalc* c);
   void set_subdiv_color_calc(SubdivColorCalc* c);

   Wpt   limit_loc   (CBvert* v) const { return _loc_calc->limit_val(v); }
   Wpt   subdiv_loc  (CBvert* v) const { return _loc_calc->subdiv_val(v); }
   Wpt   subdiv_loc  (CBedge* e) const { return _loc_calc->subdiv_val(e); }
   COLOR subdiv_color(CBvert* v) const { return _color_calc->subdiv_val(v); }
   COLOR subdiv_color(CBedge* e) const { return _color_calc->subdiv_val(e); }

   //******** SUBDIVISION ********
   void refine();       // switch to finer subdivision mesh
   void unrefine();     // switch to coarser subdivision mesh

   // Ensure subdiv meshes are updated to the given level,
   // and set the current level to the given one:
   // (returns true if anything happened):
   bool update_subdivision(int level);

   // update a selected region down to desired level k.
   // faces must all belong to the same LMESH. corresponding
   // faces/edges/verts down to level k will be updated.
   static bool update_subdivision(CBface_list& faces, int k=1);

   // XXX - shouldn't have to update 1-ring faces...
   static bool update_subdivision(CBvert_list& verts, int k=1) {
      return update_subdivision(verts.one_ring_faces(), k);
   }

   // Ensure subdiv meshes are updated to CURRENT level:
   void update() {
      if (_cur_mesh != this)
         update_subdivision(cur_level());
   }

   // subdivide, replacing control mesh with the refined mesh:
   void subdivide_in_place();

   // Keeping track of vertices whose subdivision verts are out of
   // date (A.K.A. "dirty" vertices):
   void add_dirty_vert(Lvert* v) {
      if (v && v->is_clear(Lvert::DIRTY_VERT_LIST_BIT)) {
         v->set_bit(Lvert::DIRTY_VERT_LIST_BIT);
         _dirty_verts += v;
      }
   }
   void rem_dirty_vert(Lvert* v) {
      if (v && v->is_set(Lvert::DIRTY_VERT_LIST_BIT)) {
         v->clear_bit(Lvert::DIRTY_VERT_LIST_BIT);
         _dirty_verts -= v;
      }
   }

   static void fit(ARRAY<Lvert*>& verts, bool do_gauss_seidel=1);

   // Merging two meshes:
   //    Convenience method -- same as BMESH::merge() but
   //    returns an LMESHptr.
   static LMESHptr merge(LMESHptr m1, LMESHptr m2) {
      BMESHptr ret = BMESH::merge(m1, m2);
      return ret ? (LMESH*)&*ret : 0;
   }

   // Given a set of vertices from the same LMESH, return
   // the set of vertices of the parent LMESH that affect
   // the subdivision locations of the given vertices.
   static Bvert_list get_subdiv_inputs(CBvert_list& verts);

   //******** I/O - READ ********

   // Read an LMESH from a file and return it.  
   // Handles new .jot or .sm formats. 
   static LMESHptr read_jot_file  (char* filename);
   static LMESHptr read_jot_stream(istream& is);

   //******** BMESH methods ********

   // The level of the currently drawn mesh:
   virtual int cur_level() const { return cur_mesh()->_subdiv_level; }

   // The level of this mesh in the hierarchy
   // (0 == control mesh, next level is 1, etc.):
   virtual int subdiv_level() const { return _subdiv_level; }

   // Return the number of levels for which _subdiv_mesh is already
   // allocated (starting with the control mesh):
   int max_cur_level() const { return control_mesh()->_max_cur_level(); }

   // Vertices, edges and faces of the current subdiv mesh:
   virtual CBvert_list& cur_verts() const { return cur_mesh()->verts(); }
   virtual CBedge_list& cur_edges() const { return cur_mesh()->edges(); }
   virtual CBface_list& cur_faces() const { return cur_mesh()->faces(); }

   // creating elements
   virtual Bvert*    new_vert(CWpt& p = mlib::Wpt::Origin()) const;
   virtual Bedge*    new_edge(Bvert* u, Bvert* v) const;
   virtual Bface*    new_face(Bvert*,Bvert*,Bvert*,Bedge*,Bedge*,Bedge*) const;

   // XXX - in transition:
   virtual TriStrip*  new_tri_strip()  const { return new TriStrip; }

   virtual EdgeStrip* new_edge_strip() const { return new LedgeStrip; }
   virtual VertStrip* new_vert_strip() const { return new LvertStrip; }
   virtual Patch*     new_patch();

   // adding elements
   virtual Bvert* add_vertex(Bvert* v);
   virtual Bvert* add_vertex(CWpt& loc=mlib::Wpt::Origin());

   // removing elements
   virtual int remove_vertex(Bvert* v);
   virtual int remove_edge(Bedge* e);
   virtual int remove_face(Bface* f);

   // geometric ops:
   virtual double volume() const;
   virtual double area()   const;
   virtual void clear_creases();

   // freeing elements
   virtual void   delete_elements();

   // assigning
   virtual BMESH& operator =(CBMESH& m);
   virtual BMESH& operator =(CLMESH& m);
   virtual BMESH& operator =(BODY& b);

   // setting the geom
   virtual void set_geom(GEOM *geom);

   // I/O
   virtual int write_patches(ostream& os) const;

   // Edit level -
   //   Tells what level of subdivision is active for editing.
   //   The value is only meaningful in the control mesh.
   //
   virtual int edit_level() const {
      return control_mesh()->BMESH::edit_level();
   }

   virtual void inc_edit_level();
   virtual void dec_edit_level();
	
	// specify edit level for current mesh,
	// only operate on control mesh ... john
	virtual bool set_edit_level(int level);

   virtual void changed(change_t);
   virtual void changed() { changed(TOPOLOGY_CHANGED); }

   //******** DIAGNOSTIC ********

   // Return the approximate memory used for this mesh.
   virtual int  size()  const;
   virtual void print() const;

  //******** PATCH BLENDING WEIGHTS ********
   virtual void update_patch_blend_weights();
   virtual int  patch_blend_smooth_passes() const;

   //******** RENDERING STYLE ********

   // The rendering style is only set on a mesh when you want to
   // override the current rendering style of the view. i.e., almost
   // never. Virtual methods over-rode in LMESH to deflect calls to
   // the control mesh.
   virtual void set_render_style(Cstr_ptr& s) {
      control_mesh()->BMESH::set_render_style(s);
   }
   virtual void push_render_style(Cstr_ptr& s) {
      control_mesh()->BMESH::push_render_style(s);
   }
   virtual void pop_render_style() {
      control_mesh()->BMESH::pop_render_style();
   }
   virtual Cstr_ptr& render_style() const {
      return control_mesh()->BMESH::render_style();
   }

   //**************** BODY methods ****************
   // missing: copy, vertices
   virtual int    draw(CVIEWptr &v);
   virtual int    intersect(RAYhit&, CWtransf&, mlib::Wpt&,
                            Wvec&, double&, double&,mlib::XYpt&) const;
   virtual void   transform(CWtransf &xform, CMOD& m);
   virtual BODY  *subtract (BODYptr &subtractor)   { return 0; }
   virtual BODY  *combine (BODYptr &combiner)      { return 0; }
   virtual BODY  *intersect (BODYptr &intersector) { return 0; }
   virtual CBBOX &get_bb();

   //**************** DATA_ITEM methods ****************
   
   virtual CTAGlist &tags() const;

   virtual DATA_ITEM   *dup()  const { return new LMESH(0,0,0);}

   //******** NAME LOOKUP ********

   static LMESH* get_lmesh(const string& exact_name);

 //*******************************************************
 // PROTECTED
 //*******************************************************
 protected:
   LMESH*       _parent_mesh;  // subdivision parent
   LMESH*       _cur_mesh;     // current subdivision mesh
   LMESHptr     _subdiv_mesh;  // subdivision child
   int          _subdiv_level; // level of this mesh (control mesh = level 0)
   Bvert_list   _dirty_verts;  // verts whose subdiv verts need recalculating

   // engines of subdivision calculation:
   SubdivLocCalc*       _loc_calc;
   SubdivColorCalc*     _color_calc;

   //******** INTERNAL METHODS ********

   void set_parent(LMESH* parent);

   void set_cur_mesh(LMESH* cur);

   // Return the number of levels finer than this mesh
   // for which _subdiv_mesh is already allocated:
   int _max_cur_level() const {
      return _subdiv_mesh ? _subdiv_mesh->_max_cur_level() + 1 : 0;
   }

   virtual void send_update_notification();

   // Merge the elements of the given mesh into this one:
   virtual void _merge(BMESH*);

   //******** I/O ********
   TAGlist*             _lmesh_tags;
};

/*****************************************************************
 * inlines
 *****************************************************************/

// Convenient "upcast" (though GEL actually may contain an LMESH)
inline LMESHptr 
gel_to_lmesh(CGELptr &gel) 
{
   BMESHptr mesh = gel_to_bmesh(gel);
   return mesh ? LMESH::upcast(&*mesh) : 0;
}

inline BMESH*
get_ctrl_mesh(CBMESH* m)
{
   return (LMESH::isa(m)) ? ((LMESH*)m)->control_mesh() : (BMESH*)m;
}

inline BMESH*
get_cur_mesh(CBMESH* m)
{
   return (LMESH::isa(m)) ? ((LMESH*)m)->control_mesh()->cur_mesh() :
      (BMESH*)m;
}

// Return the LMESH that owns the simplex, if it exists:
inline LMESH*
get_lmesh(CBsimplex* s)
{
   return s ? LMESH::upcast(s->mesh()) : 0;
}

inline int
subdiv_level(CBsimplex* s)
{
   if (!s) return -1;
   LMESH* m = LMESH::upcast(s->mesh());
   return m ? m->subdiv_level() : -1;
}

inline Bface*
get_ctrl_face(CBface* f)
{
   if (!f)
      return 0;
   return LMESH::isa(f->mesh()) ? ((Lface*)f)->control_face() : (Bface*)f;
}

inline Bface*
get_parent_face(CBface* f)
{
   return (f && LMESH::isa(f->mesh())) ? ((Lface*)f)->parent() : 0;
}

inline Bsimplex*
get_parent_simplex(CBedge* e)
{
   return (e && LMESH::isa(e->mesh())) ? ((Ledge*)e)->parent() : 0;
}

inline Bsimplex*
get_parent_simplex(CBvert* v)
{
   return (v && LMESH::isa(v->mesh())) ? ((Lvert*)v)->parent() : 0;
}

inline Bsimplex*
get_parent_simplex(CBsimplex* s)
{
   return (!(s && LMESH::isa(s->mesh())) ? 0 :
           is_vert(s) ? get_parent_simplex((Bvert*)s) :
           is_edge(s) ? get_parent_simplex((Bedge*)s) :
           is_face(s) ? (Bsimplex*)get_parent_face((Bface*)s) :
           0);
}

inline void
claim_edit_level(LMESHptr m)
{
   // make the mesh m the "current" one in its
   // subdivision hierarchy, and set the edit level
   // to the level of m.
   if (m) {
      m->control_mesh()->update_subdivision(m->subdiv_level());
      set_edit_level(m, m->subdiv_level());
   }
}

inline void
update_subdivision(CBMESHptr& mesh, int level)
{
   if (mesh && LMESH::isa(&*mesh)) {
      LMESH* ctrl = ((LMESH*)&*mesh)->control_mesh();
      if (ctrl)
         ctrl->update_subdivision(level);
   }
}

inline LMESH*
get_lmesh(CBface_list& faces)
{
   return LMESH::upcast(faces.mesh());
}

inline LMESH*
get_subdiv_mesh(BMESH* m, int level)
{
   // XXX -- could handle negative levels but not doing it now
   if ( level < 0 || !LMESH::isa(m) )
      return 0;

   LMESH* lm = (LMESH*)m;

     for ( int i=0; lm && i<level; i++ ) {
       lm = lm->subdiv_mesh();
     }

   return lm;
}

// Return faces at given level relative to these faces.
// Assumes these faces are from the same LMESH.
inline Bface_list 
get_subdiv_faces(CBface_list& faces, int level)
{
   Bface_list ret;
   LMESH* m = LMESH::upcast(faces.mesh());
   if (!(level >= 0 && m && m->subdiv_mesh(level)))
      return ret;
   for (int i=0; i<faces.num(); i++)
      ((Lface*)faces[i])->append_subdiv_faces(level, ret);
   return ret;
}

// Return parent faces at given level up from the given ones.
// Assumes these faces are from the same LMESH.
inline Bface_list
get_parent_faces(CBface_list& faces, int level=1)
{
   Bface_list ret;
   LMESH* m = LMESH::upcast(faces.mesh());
   if (!(level >= 0 && m && m->parent_mesh(level)))
      return ret;

   // first clear the flag of each parental face
   Lface* p = 0;
   int i;
   for (i = 0; i<faces.num(); i++) {
      if ((p = ((Lface*)faces[i])->parent()))
         p->clear_flag();
   }

   // now store each parent face that hasn't yet been reached:
   for (i = 0; i<faces.num(); i++) {
      if ((p = ((Lface*)faces[i])->parent()) && !p->flag()) {
         p->set_flag(1);
         ret += p;
      }
   }
   return ret;
}

inline Bface_list
remap_level(CBface_list& faces, int k)
{
   // Given a set of faces from the same LMESH,
   // remap to corresponding faces at the given level k,
   // relative to the original mesh.

   if (k == 0)
      return faces;
   else if (k < 0)
      return get_parent_faces(faces, -k);
   else
      return get_subdiv_faces(faces, k);
}

inline Bface_list
get_top_level(CBface_list& faces)
{
   Bface_list ret = faces;
   
   for (Bface_list p = get_parent_faces(faces);
        !p.empty();
        p = get_parent_faces(p))
      ret = p;
   return ret;
}

inline bool
has_secondary_any_level(CBface_list& faces)
{
   for (Bface_list sub = faces; !sub.empty(); sub = get_subdiv_faces(sub,1))
      if (sub.has_any_secondary())
         return true;
   return false;
}

template <class L, class S>
inline Bvert_list
child_verts(const L& list)
{
   // L can be Bvert_list or Bedge_list.
   // S must then be Lvert or Ledge, respectively.
   // 
   // Return the child vertices of the given list.
   // list elements must all belong to the same LMESH.
   // Child vertices must already exist.
   // We aren't creating them, just returning a list of them.

   Bvert_list ret(list.num());

   if (list.empty())
      return ret;

   assert(LMESH::isa(list.mesh()));

   for (int i=0; i<list.num(); i++) {
      Lvert* v = ((S*)list[i])->subdiv_vertex();
      assert(v);
      ret += v;
   }
   return ret;
}

#endif  // LMESH_H_HAS_BEEN_INCLUDED

// end of file lmesh.H
