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
/**********************************************************************
 * tex_body.H:
 **********************************************************************/
#ifndef TEX_BODY_H_IS_INCLUDED
#define TEX_BODY_H_IS_INCLUDED

#include "geom/geom.H"
#include "mesh/lmesh.H"

class Action;
class Script;

/**********************************************************************
 * HaloBase:
 *
 *   pure virtual base class for the halo effect singleton
 *   that has the ability to draw a halo around an object
 *   based on its bounding box
 **********************************************************************/
class HaloBase {
 public:
   virtual void draw_halo(CVIEWptr &v, CBBOX& box, double scale) = 0;
   virtual ~HaloBase() { }

   static HaloBase* get_instance() { return _instance; }

 protected:
   static HaloBase* _instance;
};

/**********************************************************************
 * TEXBODY:
 *
 *    A kind of GEOM that has 1 or more BMESHes, which are
 *    treated as alternative represenatations for a shape.
 *    Usually just one is drawn. Meshes are drawn when
 *    BMESH::draw_enabled() returns true.
 *
 **********************************************************************/
MAKE_PTR_SUBC(TEXBODY,GEOM);
typedef const TEXBODY CTEXBODY;
typedef const TEXBODYptr CTEXBODYptr;
class TEXBODY : public GEOM, public XFORMobs {
 public:

   //******** MANAGERS ********

   TEXBODY();
   TEXBODY(CGEOMptr&, Cstr_ptr&);
   TEXBODY(CBMESHptr& m, Cstr_ptr& name=NULL_STR);

   virtual ~TEXBODY();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("TEXBODY", TEXBODY*, GEOM, CDATA_ITEM*);

   //******** ACCESSORS ********

   BMESH_list&    meshes()                { return _meshes; }
   CBMESH_list&   meshes()       const    { return _meshes; }
   int            num_meshes()   const    { return _meshes.num(); }
   BMESHptr       mesh(int i)    const    { return _meshes[i]; }
   BMESHptr       last()         const    { return _meshes.last(); }

   bool           empty()        const    { return _meshes.empty(); }
   bool           contains(CBMESHptr& m)   const    { return _meshes.contains(m); }

   void show_skel_curves() { _skel_curves_visible = true; }
   void hide_skel_curves() { _skel_curves_visible = false; }

   // This method exists for convenience and backwards compatibility.
   // Returns the last draw-enabled mesh (if any), or a null pointer.
   CBMESHptr&     cur_rep()      const;

   bool add(BMESHptr m);        // add the mesh to the list of representations
   bool push(BMESHptr m);       // add the mesh as the first mesh in the list 
   void remove(CBMESHptr& m);   // remove the mesh from the representations

   // Return a mesh suitable to use for building a mesh by
   // inflation from given mesh.
   BMESHptr get_inflate_mesh(BMESHptr mesh);

   // Enable/disble writing xform directly to mesh vertices:
   void           set_apply_xf(bool b)    { _apply_xf = b; }
   bool           apply_xf()     const    { return _apply_xf; }
   // See comments below
   Cstr_ptr&      mesh_file()    const    { return _mesh_file; }         

   //******** CONVENIENCE ********

   // Returns meshes consisting just of points
   BMESH_list point_meshes() const;

   // Returns meshes consisting of curves
   // (maybe with points) and no surfaces
   BMESH_list curve_meshes() const;

   // Returns meshes consisting of panels
   // (maybe with points and curves)
   BMESH_list panel_meshes() const;

   // Returns meshes consisting of surfaces
   // (maybe with points and curves)
   BMESH_list surface_meshes() const;

   // Returns the "skeleton" meshes, i.e. those made up
   // just of points and curves or panels:
   BMESH_list skel_meshes() const { return point_meshes() + curve_meshes() + panel_meshes(); }

   bool has_surfaces()  const;
   bool has_skels()     const;

   // Is it a skeleton mesh of this TEXBODY?
   bool is_skel(BMESH* m) const { return skel_meshes().contains(m); }

   // Is it a skeleton enclosed in a surface?
   bool is_inner_skel(BMESH* m) const;

   void add_post_drawer(RefImageClient* p) { _post_drawers.add_uniquely(p); }
   void rem_post_drawer(RefImageClient* p) { _post_drawers -= p; }

   //******** ACTIONS ********

   // returns the script (if any) belonging to this TEXBODY:
   Script* script() const { return _script; }

   // allocates a script if needed, then returns it:
   Script* get_script();

   // is there a non-empty script?
   bool has_script() const;

   // add an action to the script:
   void add_action(Action* a);

   //******** STATICS ********

   // Wrap given mesh in a TEXBODY and throw it in the world:
   static void display  (CBMESHptr& mesh, MULTI_CMDptr cmd=0);
   static void undisplay(CBMESHptr& mesh, MULTI_CMDptr cmd=0);

   // TEXBODY::null is used in ray_geom() to get a TEXBODY
   // from a RAYhit after doing intersection, e.g.:
   //   TEXBODYptr tex = ray_geom(ray,TEXBODY::null);
   static TEXBODY *null; 

   // Get all currently displayed meshes:
   static BMESH_list all_drawn_meshes();

   static TEXBODY* bmesh_to_texbody(CBMESHptr& mesh) {
      return upcast(bmesh_to_gel(mesh));
   }

   static TEXBODY* cur_mesh_tex_body() {
      return bmesh_to_texbody(BMESH::focus());
   }
   static TEXBODY* cur_tex_body() {
      return get_focus() ? get_focus() : cur_mesh_tex_body();
   }
   static TEXBODY* get_focus()                  { return _focus; }
   static void     set_focus(TEXBODY* t)        { _focus = t; }

   //******** TEXBODYs specialized for FFS ********

   // In FFS, TEXBODYs contain 2 meshes:
   enum mesh_t {
      SKELETON_MESH = 0, // holds skeleton points, curves, panels, etc.
      SURFACE_MESH,      // holds inflated mesh defined from skeleton
      NUM_MESH_T
   };

   // Return current TEXBODY (create if needed).
   // Normally it's the one containing BMESH::focus().
   // It is set up to contain 2 meshes: one for skeleton
   // components, the other for everything else.
   static TEXBODYptr get_ffs_tex_body(MULTI_CMDptr cmd);

   // Return a particular mesh from current ffs TEXBODY:
   static LMESHptr   get_ffs_mesh(mesh_t, MULTI_CMDptr cmd); 

   // Returns the skeleton mesh of the current ffs TEXBODY:
   static LMESHptr get_skel_mesh(MULTI_CMDptr cmd) {
      return get_ffs_mesh(SKELETON_MESH, cmd);
   }
   // Returns the non-skeleton mesh of the current ffs TEXBODY:
   static LMESHptr get_surf_mesh(MULTI_CMDptr cmd) {
      return get_ffs_mesh( SURFACE_MESH, cmd);
   }

   // access FFS meshes from *this* TEXBODY:
   LMESHptr get_skel_mesh() const {
      return LMESH::upcast(
         _meshes.valid_index(SKELETON_MESH) ? _meshes[SKELETON_MESH] : 0
         );
   }
   LMESHptr get_surf_mesh() const {
      return LMESH::upcast(
         _meshes.valid_index(SURFACE_MESH) ? _meshes[SURFACE_MESH] : 0
         );
   }

 protected:
   // Create a mesh for FFS (does not add it to current FFS TEXBODY):
   static LMESHptr create_mesh(const string& base_name);
 public:

   //******** RefImageClient METHODS: ********

   virtual int  draw_vis_ref();
   virtual int  draw_color_ref(int i);
 protected:
   // used in GEOM::draw_img():
   virtual RefImageClient* ric() { return &_meshes; }
 public:

   //******** GEOM METHODS ********

   virtual RAYhit&   intersect(RAYhit&, CWtransf& =mlib::Identity, int=0)const;
   virtual RAYnear&  nearest(RAYnear &r, CWtransf& =mlib::Identity)      const;
   virtual bool      inside(CXYpt_list&)                                 const;
   virtual BBOX      bbox(int recurse=0)                                 const;
   virtual GEOMptr   dup(Cstr_ptr &n)                                    const;
   virtual int       draw(CVIEWptr &v);
   virtual BODYptr   body()  const   { return cur_rep(); }

   virtual bool needs_blend() const;

   virtual int draw_halo(CVIEWptr& v = VIEW::peek()) const;

   //******** DATA_ITEM METHODS********

   virtual DATA_ITEM*   dup()            const;
   virtual CTAGlist&    tags()           const;

   virtual void put_script(TAGformat &d)       const;
   virtual void get_script(TAGformat &d);

   virtual void put_mesh_data(TAGformat &d)     const;
   virtual void get_mesh_data(TAGformat &d);

   virtual void put_mesh_data_file(TAGformat &d)const;
   virtual void get_mesh_data_file(TAGformat &d);

   virtual void put_mesh_data_update(TAGformat &d)     const;
   virtual void get_mesh_data_update(TAGformat &d);

   virtual void put_mesh_data_update_file(TAGformat &d)const;
   virtual void get_mesh_data_update_file(TAGformat &d);
        
   // XXX - Deprecated...

   virtual void put_mesh(TAGformat &d)     const;
   virtual void get_mesh(TAGformat &d);

   virtual void put_mesh_file(TAGformat &d)     const;
   virtual void get_mesh_file(TAGformat &d);

   virtual void put_mesh_update(TAGformat &d)     const;
   virtual void get_mesh_update(TAGformat &d);

   virtual void put_mesh_update_file(TAGformat &d)     const;
   virtual void get_mesh_update_file(TAGformat &d);

   //******** XFORMobs METHODS ********

   virtual void notify_xform(CGEOMptr &, STATE state);
        
   //*******************************************************
   // PROTECTED
   //*******************************************************
 protected:
   BMESH_list           _meshes;        // the representations
   bool                 _apply_xf;      // apply xforms to mesh directly?
   RefImageClient_list  _post_drawers;  // things that draw after the rest

   // if true, show skel curves over surfaces:
   bool         _skel_curves_visible;

   // If not NULL_STR, it's the (.sm) filename a mesh was
   // loaded from (in a .jot scene) subsequent saving of the
   // scene (s key) or mesh (w key) will use this filename
   //
   // XXX -
   //   This policy needs work, and in particular a glui widget
   //   so you can confirm overwrites and invoke renaming...
   str_ptr      _mesh_file;       

   // If not NULL_STR, it's the (.sm) filename that was used last
   // to update this texboy's mesh.  It's used when resaving
   // frames of an animation, when we step though each frame, 1st
   // loading it and then resaving to a new destination.
   //
   // XXX - This policy needs work:
   str_ptr      _mesh_update_file;

   Script*      _script;

   // Serialization:
   static TAGlist* _texbody_tags;
   static TAGlist* _action_tags;

   // currently selected TEXBODY:
   static TEXBODYptr _focus;

   CTAGlist& action_tags() const;

   //******** INTERNAL METHODS ********
   void init_xform_obs();    // enable observation of GEOM::xform
   void end_xform_obs();     // disable observation of GEOM::xform
};

inline GEOM*
bmesh_to_geom(CBMESHptr& mesh)
{
   return GEOM::upcast(bmesh_to_gel(mesh));
}

#endif // TEX_BODY_H_IS_INCLUDED

// end of file tex_body.H
