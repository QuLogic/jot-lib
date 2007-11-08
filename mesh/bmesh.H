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


/**********************************************************************/
/*! \file bmesh.H                                                     */
/**********************************************************************/
#ifndef BMESH_H_HAS_BEEN_INCLUDED
#define BMESH_H_HAS_BEEN_INCLUDED

#include "disp/view.H"
#include "dlhandler/dlhandler.H"
#include "geom/body.H"
#include "geom/geom.H"          // XXX - For _geom member; should remove
#include "std/thread_mutex.H"
#include "std/name_lookup.H"

#include "mesh/bmesh_curvature.H"
#include "mesh/edge_strip.H"
#include "mesh/patch.H"
#include "mesh/tri_strip.H"
#include "mesh/vert_strip.H"
#include "mesh/zcross_path.H"

class Patch;
/**********************************************************************/
/*!BMESH:
 *
 *      Triangle-based mesh class.
 *
 *      Primarily consists of a collection of vertices,
 *      edges, and faces (Bvert, Bedge, and Bface classes).
 *
 *      Subdivision surfaces are implemented in the subclass LMESH.
 */
/**********************************************************************/
MAKE_PTR_SUBC(BMESH,BODY);
typedef const BMESHptr CBMESHptr;
class BMESH : public BODY, public NameLookup<BMESH> {
 public:

   //******** ENUMS ********

   /// various types of meshes:
   enum mesh_type_t {   
      EMPTY_MESH        = 0,
      POINTS            = 1,
      POLYLINES         = 2,
      OPEN_SURFACE      = 4,
      CLOSED_SURFACE    = 8
   };

   /// various ways a mesh can change:
   enum change_t {
      TOPOLOGY_CHANGED = 0,
      PATCHES_CHANGED,
      TRIANGULATION_CHANGED,
      VERT_POSITIONS_CHANGED,
      VERT_COLORS_CHANGED,
      CREASES_CHANGED,
      RENDERING_CHANGED,
      NO_CHANGE
   };

   //******** MANAGERS ********

   BMESH(int num_v=0, int num_e=0, int num_f=0);
   BMESH(CBMESH& m);

   virtual ~BMESH();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("BMESH", BMESH*, BODY, CDATA_ITEM*);

   //******** ASSIGNMENT ********

   virtual BMESH& operator =(CBMESH& m);
   virtual BMESH& operator =(BODY& b);

   //******** ACCESSORS / CONVENIENCE ********

   // Vertices, edges and faces of this mesh:
   /// Vertices of this mesh
   CBvert_list&  verts()                   const { return _verts; }
   /// Edges in this mesh
   CBedge_list&  edges()                   const { return _edges; }
   /// Triangular faces in this mesh
   CBface_list&  faces()                   const { return _faces; }

   // Vertices, edges and faces of the current subdiv mesh (if any):
   /// Vertices ofthe current subdivision mesh, if any
   virtual CBvert_list& cur_verts()        const { return _verts; }
   /// Edges ofthe current subdivision mesh, if any
   virtual CBedge_list& cur_edges()        const { return _edges; }
   /// Faces ofthe current subdivision mesh, if any
   virtual CBface_list& cur_faces()        const { return _faces; }

   // turn on/off indexing in vert, edge, and face lists.
   // indexing allows O(1) index lookup and removal, but
   // adds data to simplex data lists:
   void begin_index(); ///< Turn on indexing in vert, edge and face lists
   void end_index(); ///< Turn off indexing in vert, edge and face lists

   // size of lists:
   /// Number of vertices
   int    nverts()      const   { return _verts.num(); }
   /// Number of edges
   int    nedges()      const   { return _edges.num(); }
   /// Number of faces
   int    nfaces()      const   { return _faces.num(); }
   /// Number of patches
   int    npatches()    const   { return _patches.num(); }
   /// Is the vertex list empty?
   bool   empty()       const   { return _verts.empty(); }

   // shorthand accessors of individual elements:
   /// Shorthand for accessing kth vertex
   Bvert* bv(int k)     const   { return _verts[k]; }
   /// Shorthand for accessing kth edge
   Bedge* be(int k)     const   { return _edges[k]; }
   /// Shorthand for accessing kth face
   Bface* bf(int k)     const   { return _faces[k]; }
   /// Shorthand for accessing kth patch
   Patch* patch(int k)  const   { return _patches[k]; }

   Bedge* lookup_edge (const Point2i &p);
   Bface* lookup_face (const Point3i &p);

   /// List of patches
   CPatch_list& patches()                       const { return _patches; }

   // All drawing takes place via the drawables list. 
   // E.g. patches are entered into the list.
   // XXX - modifiable -- anyone can add to or remove from the list:
   RefImageClient_list& drawables()   { return _drawables; }

   // GEOM accessor
   GEOM* geom() const   { return _geom; } 
   virtual void set_geom(GEOM *geom);

   /// Transformation from object to world coordinates for this mesh
   CWtransf&  xform()      const;
   /// Transformation from world to object coordinates for this mesh
   CWtransf&  inv_xform()  const;
  
   CWtransf& obj_to_ndc() const;

   /// Camera (eye) position in this object's coordinate system
   CWpt&     eye_local()  const;

   // optional flag for enabling/disabling drawing
   void enable_draw()           { _draw_enabled = true; }
   void disable_draw()          { _draw_enabled = false; }
   bool draw_enabled()  const   { return _draw_enabled && !empty(); }

   virtual void changed(change_t);
   virtual void changed() { changed(TOPOLOGY_CHANGED); }

   // clear flags in verts, edges, faces:
   void clear_flags();

   //******** RENDERING STYLE ********

   // The rendering style is only set on a mesh when you want to
   // override the current rendering style of the view. i.e., almost
   // never. Virtual methods over-rode in LMESH to deflect calls to
   // the control mesh.
   virtual void set_render_style(Cstr_ptr& s);
   virtual void push_render_style(Cstr_ptr& s);
   virtual void pop_render_style();
   virtual Cstr_ptr& render_style() const;

   void toggle_render_style(Cstr_ptr& s) {
      if (render_style() == s)  pop_render_style();
      else                      push_render_style(s);
   }

   /// Does a Patch with a current GTexture named name already exist in this mesh?
   int tex_already_exists(Cstr_ptr &name) const;
   /// Do the vertices have colors?
   bool has_vert_colors() const { return (nverts()>0 && bv(0)->has_color()); }

   //******** STRIPS ********

   int get_sil_strips       (); ///< extracts silhouette edge strips
   int build_sil_strips     (); ///< distributes them to patches

   /// extract silhouette edge strip and return a copy of it:
   EdgeStrip sil_strip();

   /// finds "strips" for zero-set definition of silhouettes (as in
   /// WYSIWYG NPR, Siggraph 2002):
   int get_zcross_strips    ();
   int build_zcross_strips  ();

   // deals with "strips" for isolated edges and vertices:
   int build_polyline_strips();
   int build_vert_strips    ();

   // LMESH over-rides this:
   virtual void clear_creases();
   void    compute_creases();

   /*! Given a filter that accepts a particular kind of edges
    * (e.g. crease edges), returns an edge strip consisting of
    *chains of edges of the given kind. */
   EdgeStrip get_edge_strip(CSimplexFilter& edge_filter) const {
      EdgeStrip ret;
      ret.build_with_tips(edges(), edge_filter);
      return ret;
   }
   /*! Like get_edge_strip(), but allocates the returned EdgeStrip.
    * So for an LMESH this returns an LedgeStrip.
    * (The caller is responsible to delete the strip).
    */
   EdgeStrip* new_edge_strip(CSimplexFilter& edge_filter) {
      EdgeStrip* ret = new_edge_strip();
      assert(ret);
      *ret = get_edge_strip(edge_filter);
      return ret;
   }

   /*! Specialzed for crease and border strips:
    *  these strips are cached, so it's a lightweight
    *  operation after the first time
    */
   EdgeStrip* get_crease_strip() const {
      if (!_creases) {
         ((BMESH*)this)->_creases = new_edge_strip();
         _creases->build_with_tips(edges(),CreaseEdgeFilter());
      }
      return _creases;
   }
   EdgeStrip* get_border_strip() const {
      if (!_borders) {
         ((BMESH*)this)->_borders = new_edge_strip();
         _borders->build_with_tips(edges(),BorderEdgeFilter());
      }
      return _borders;
   }

   /*! Given a filter that accepts a particular kind of edges
    *(e.g. crease edges), returns an edge list consisting of
    * edges of the given kind.
    *
    * For crease and border edges, see get_creases() and
    * get_borders(), below.
    */
   Bedge_list get_edges(CSimplexFilter& edge_filter) const {
      return edges().filter(edge_filter);
   }

   // Note: the following use cached edge strips, so they run in
   // O(1) time (after the first time). Crease edges can also be
   // retrieved via get_edges(CreaseEdgeFilter()), which is O(n)
   // every time (where n is the number of edges).
   CBedge_list& get_creases() const { return get_crease_strip()->edges(); }
   CBedge_list& get_borders() const { return get_border_strip()->edges(); }

   //******** FACTORY METHODS ********
   /// Convert the given world-space point to a vertex
   virtual Bvert*  new_vert(CWpt& p=mlib::Wpt::Origin()) const {
      return new Bvert(p);
   }

   /// NB: caller should first check u,v doesn't have an edge already
   virtual Bedge*  new_edge(Bvert* u, Bvert* v)   const {
      return new Bedge(u,v);
   }

   /// NB: caller should first check requested face doesn't exist already
   virtual Bface*  new_face(Bvert* u,
                            Bvert* v,
                            Bvert* w,
                            Bedge* e,
                            Bedge* f,
                            Bedge* g) const {
      return new Bface(u,v,w,e,f,g);
   }

   virtual TriStrip*    new_tri_strip()   const { return new TriStrip; }
   virtual EdgeStrip*   new_edge_strip()  const { return new EdgeStrip;}
   virtual VertStrip*   new_vert_strip()  const { return new VertStrip;}
   virtual Patch*       new_patch();

   /// Remove the Patch from the patch list; returns 1 on success
   bool unlist(Patch* p);

   void                 make_patch_if_needed();

   //******** ADDING MESH ELEMENTS ********

   /// Include the given vertex in this mesh and set the vertex's mesh to this one.
   virtual Bvert* add_vertex(Bvert* v);
   /// Include the given world-space point as a vertex in this mesh and set the vertex's mesh to this one.
   virtual Bvert* add_vertex(CWpt& loc=mlib::Wpt::Origin());
   /// Add the given edge to this mesh's edge-list, setting the edge's mesh to this one.
   virtual Bedge* add_edge(Bedge* e);
   /// Add the edge between the two given vertices to this mesh's list, checking error conditions and setting the edge's mesh to this one.
   virtual Bedge* add_edge(Bvert* u, Bvert* v);
   /// Add edge between the ith and jth vertices, checking for error conditions, and set the edge's mesh to this one.
   virtual Bedge* add_edge(int i, int j);
   /// Add the given face to the face-list in this mesh, and set the face's mesh to this one. Set the face to use the given patch (if any).
   virtual Bface* add_face(Bface* f, Patch* p=0);
   /*! Add the triangular face defined by the given vertices and patch (if any),
    * creating the necessary edges and checking for error conditions. */
   virtual Bface* add_face(Bvert* u, Bvert* v, Bvert* w, Patch* p=0);
   /*! Add the triangular face defined by the ith, jth and kth vertices in this mesh's vertex-list,
    * creating the necessary edges and checking for error conditions. */
   virtual Bface* add_face(int i, int j, int k, Patch* p=0);

   /// Add the given list of world-space points, as vertices, to this mesh, set their mesh to this one. Return this list as a vertex-list.
   Bvert_list add_verts(CWpt_list& pts);

   Bface* add_face(Bvert* u, Bvert* v, Bvert* w,
                   CUVpt& a, mlib::CUVpt& b, mlib::CUVpt& c, Patch* p=0);
   Bface* add_face(int    i, int    j, int    k,
                   CUVpt& a, mlib::CUVpt& b, mlib::CUVpt& c, Patch* p=0);

   /// Creates the quad from the given vertices and patch, if possible, and returns the quad rep, or nil.
   Bface* add_quad(Bvert* u, Bvert* v, Bvert* w, Bvert* x, Patch* p=0);
   /// Creates the quad if possible, and returns the quad rep, or nil:
   Bface* add_quad(Bvert* u, Bvert* v, Bvert* w, Bvert* x,
                   CUVpt& a, mlib::CUVpt& b, mlib::CUVpt& c, mlib::CUVpt& d, Patch* p=0);
   /// Creates the quad from the ith, jth, kth and lth vertices in this mesh's vertex list, if possible, and returns the quad rep, or nil.
   Bface* add_quad(int    i, int    j, int    k, int    l, Patch* p=0);
   /// Creates the quad if possible, and returns the quad rep, or nil:
   Bface* add_quad(int    i, int    j, int    k, int    l,
                   CUVpt& a, mlib::CUVpt& b, mlib::CUVpt& c, mlib::CUVpt& d, Patch* p=0);

   //******** DELETING ELEMENTS ********

   /// Simply releases memory allocated to the vertex by calling delete
   virtual void delete_vert(Bvert* v) const   { delete v; }
   /// Simply releases memory allocated to the edge by calling delete
   virtual void delete_edge(Bedge* e) const   { delete e; }
   /// Simply releases memory allocated to the face by calling delete
   virtual void delete_face(Bface* f) const   { delete f; }

   virtual void delete_elements();
   virtual void delete_patches();

   //******** REMOVING MESH ELEMENTS ********

   /// Removes the given face from this mesh's face-list, and releases memory allocated to it.
   virtual int    remove_face(Bface* f);
   /// Removes the given edge from this mesh's edge-list, and releases memory allocated to it.
   virtual int    remove_edge(Bedge* e);
   /// Removes the given vertex from this mesh's vertex-list, and releases memory allocated to it.
   virtual int    remove_vertex(Bvert* v);

   /// Remove each face in the list given and release memory allocated to it.
   virtual int    remove_faces(CBface_list& faces);
   /// Remove each edge in the list given and release memory allocated to it.
   virtual int    remove_edges(CBedge_list& edges);
   /// Remove each vertex in the list given and release memory allocated to it.
   virtual int    remove_verts(CBvert_list& verts);

   //******** PRIMITIVES ********
   /// Create a cube with the two given points as the body diagonal, and apply the patch if given (default if not).
   virtual void   Cube       (CWpt& =mlib::Wpt(0,0,0),mlib::CWpt& =mlib::Wpt(1,1,1), Patch* p=0);

   /// Create an octahedron with the given points, and apply the patch if given (default if not).
   virtual void   Octahedron (CWpt& =mlib::Wpt( 0, -M_SQRT2, 0),
                              CWpt& =mlib::Wpt( 1,        0, 1),
                              CWpt& =mlib::Wpt( 1,        0,-1),
                              CWpt& =mlib::Wpt(-1,        0,-1),
                              CWpt& =mlib::Wpt(-1,        0, 1),
                              CWpt& =mlib::Wpt( 0,  M_SQRT2, 0), Patch* p=0);

   /// Create a canonical icosahedron and apply the patch if given (default if not)
   virtual void   Icosahedron(Patch* p=0);
   //sphere with correct spherical texture coordinates
   virtual void   Sphere(Patch* p=0);
   virtual void   UV_BOX(Patch* &p);

   //******** MEASURES ********
   /// Sum up the volume using the divergence theorem.
   virtual double volume() const;
   /// Sum of areas of all faces.
   virtual double area  () const;

   /// Approximate pixel extent based on bounding box
   double pix_size() const { 
      if (_pix_size_stamp != VIEW::stamp())
         ((BMESH*)this)->compute_pix_size();
      return _pix_size;
   }
   void compute_pix_size();

   //******** MISC UTIL METHODS ********
   void           recenter();
   double         avg_len () const;
   double         z_span  (double& zmin, double& zmax) const;
   double         z_span  ()                           const;

   //******** TOPOLOGY ********
   int type() const {
      if (!_type_valid)
         ((BMESH*)this)->check_type();
      return _type;
   }
   bool is_points()             const   { return (type() & POINTS)         != 0; }
   bool is_polylines()          const   { return (type() & POLYLINES)      != 0; }
   bool is_open_surface()       const   { return (type() & OPEN_SURFACE)   != 0; }
   bool is_closed_surface()     const   { return (type() & CLOSED_SURFACE) != 0; }
   bool is_surface() const {
      return is_open_surface() || is_closed_surface();
   }

   bool fix_orientation();

   /// Reverse orientation of all the faces
   void reverse() {
      for (int k=0; k<nfaces(); k++) 
         bf(k)->reverse();
   }

   void get_enclosed_verts(CXYpt_list& boundary,
                           Bface* startface,
                           ARRAY<Bvert*>& ret);

   void remove_duplicate_vertices(bool keep_vert=1);

   /// Returns separate Bface_lists, one for each connected component
   /// of the mesh:
   ARRAY<Bface_list> get_components() const;

   /// split mesh into connected components:
   virtual ARRAY<BMESH*> split_components();

   void split_patches();

   /// wipe out a connected component containing the start vertex:
   virtual void           kill_component  (Bvert* start_vert);

   /*! Merging two meshes:
    *    Merge the two meshes together if it's legal to do
    *    so. The result is that one mesh ends up an empty husk,
    *    with its elements and patches sucked into the other
    *    mesh. The smaller mesh is the one that gets sucked
    *    dry; the other one is returned.  If the meshes are
    *    different types the operation fails and a null pointer
    *    is returned. (In that case the meshes are not changed).
    */
   static BMESHptr merge(BMESHptr m1, BMESHptr m2);

   //******** RETRIANGULATION ********
   // edge operations
   int     try_swap_edge(Bedge* edge, bool favor_deg_6 = 0);
   int     try_collapse_edge(Bedge* e, Bvert* v); // checks legality first
   void    merge_vertex(Bvert* v, Bvert* u, bool keep_vert=0); // collapses
   Bvert*  split_edge(Bedge* edge, CWpt &new_pt); // there is no try, only do
   Bvert*  split_edge(Bedge* edge) {
      Wpt mid;
      return split_edge(edge, edge->mid_pt(mid));
   }

   // face operations:
   Bvert*  split_face(Bface* face, CWpt &pt);
   int     split_faces(CXYpt_list&, ARRAY<Bvert*>&, Bface* =0); 
   void    split_tris(Bface* start_face, Wplane plane, ARRAY<Bvert*>& nv);

   //******** SUBDIVISION/EDIT LEVEL ********

   /// The level of this mesh in the hierarchy (0 == control mesh, next level is 1, etc.):
   virtual int subdiv_level() const { return 0; }

   /// Level of "current" mesh in the hierarchy:
   virtual int cur_level()    const { return 0; }

   /// Level of "active" mesh in the hierarchy:
   virtual int edit_level()   const { return _edit_level; }

   /// Same as above, but relative to the level of this mesh:
   int rel_cur_level()  const { return cur_level()  - subdiv_level(); }
   int rel_edit_level() const { return edit_level() - subdiv_level(); }

   virtual void inc_edit_level() { _edit_level++; }
   virtual void dec_edit_level() { _edit_level = max(_edit_level - 1, 0); }

   //******** PICKING ********

   /*! Brute force intersection:
    *   Given ray in world space, return intersected face (if any)
    *   and intersection point in world space:
    */
   Bface* pick_face(CWline& world_ray, mlib::Wpt& world_hit) const;

   //******** DIAGNOSTIC ********

   // Return the approximate memory used for this mesh.
   virtual int  size()  const;
   virtual void print() const;

   static void toggle_freeze_sils() { _freeze_sils = !_freeze_sils; }
   static void toggle_random_sils() { _random_sils = !_random_sils; }

   //******** FOCUS ********
   /*! Focus:
    *    Typically the mesh that the user has been lately
    *    interacting with in some way.
    */
   static void     set_focus(BMESHptr m, Patch* p);
   static void     set_focus(BMESHptr m);
   static BMESHptr focus()               { return _focus; }
   static bool     is_focus(BMESHptr m)  { return _focus == m;}

   //******** I/O HELPER ********

   int set_crease(int i, int j)         const;
   int set_weak_edge(int i, int j)      const;
   int set_patch_boundary(int i, int j) const;

   bool valid_vert_indices(int i, int j) const {
      return (_verts.valid_index(i) && _verts.valid_index(j));
   }
   bool valid_vert_indices(int i, int j, int k) const {
      return valid_vert_indices(i,j) && _verts.valid_index(k);
   }
   bool valid_vert_indices(int i, int j, int k, int l) const {
      return valid_vert_indices(i,j,k) && _verts.valid_index(l);
   }

   //******** I/O - READ ********

   // Read a mesh from a stream and return it.  Handles new
   // .jot or .sm formats, as well as old .sm format. If ret
   // is null, a new mesh will be generated (BMESH or LMESH,
   // depending on what type is specified in the stream). If
   // ret is a BMESH and the file specifies LMESH, an LMESH is
   // generated, read from the stream, and then the mesh data
   // is copied into ret.
   static BMESHptr read_jot_stream(istream& is, BMESHptr ret=0);

   // Opens the file and calls BMESH::read_jot_stream(), above.
   static BMESHptr read_jot_file(char* filename, BMESHptr ret=0);

   // Read a mesh file into *this* mesh:
   bool read_file(char* filename);

   // XXX - the following are all deprecated in favor of the
   //       new I/O using tags:
   virtual int  read_update_file  (char* filename);
   virtual int  read_update_stream(istream& is);

   virtual int  read_stream     (istream& is);  // deprecated: old .sm format
   virtual int  read_blocks     (istream& is);
   virtual int  read_header     (istream& is);
   virtual int  read_vertices   (istream& is);
   virtual int  read_faces      (istream& is);
   virtual int  read_creases    (istream& is);
   virtual int  read_polylines  (istream& is);
   virtual int  read_weak_edges (istream& is, str_list &leftover);
   virtual int  read_colors     (istream& is, str_list &leftover);
   virtual int  read_texcoords2 (istream& is, str_list &leftover);
   virtual int  read_patch      (istream& is, str_list &leftover);
   virtual int  read_include    (istream& is, str_list &leftover);

   //******** I/O - WRITE ********
   virtual int  write_file      (char *filename);
   virtual int  write_stream    (ostream& os);
   //virtual int  write_header    (ostream& os) const;
   virtual int  write_vertices  (ostream& os) const;
   virtual int  write_faces     (ostream& os) const;
   virtual int  write_creases   (ostream& os) const;
   virtual int  write_weak_edges(ostream& os) const;
   virtual int  write_polylines (ostream& os) const;
   virtual int  write_colors    (ostream& os) const;
   virtual int  write_texcoords2(ostream& os) const;
   virtual int  write_patches   (ostream& os) const;

   //******** NameLookup METHODS ********

   virtual str_ptr class_id() { return class_name(); }

   //******** RefImageClient METHODS: ********
   virtual void request_ref_imgs();

 protected:
   // utility used in draw methods below:
   // if draw enabled:
   //   1. send update notification
   //   2. call appropriate draw method
   int draw_img(const RefImgDrawer& r);
 public:
   virtual int draw(CVIEWptr& v)       { return draw_img( RegularDrawer(v)); }
   virtual int draw_color_ref(int i)   { return draw_img(ColorRefDrawer(i)); }
   virtual int draw_vis_ref()          { return draw_img(  VisRefDrawer( )); }
   virtual int draw_id_ref()           { return draw_img(   IDRefDrawer( )); }
   virtual int draw_id_ref_pre1()      { return draw_img(  IDPre1Drawer( )); }
   virtual int draw_id_ref_pre2()      { return draw_img(  IDPre2Drawer( )); }
   virtual int draw_id_ref_pre3()      { return draw_img(  IDPre3Drawer( )); }
   virtual int draw_id_ref_pre4()      { return draw_img(  IDPre4Drawer( )); }
   virtual int draw_halo_ref()         { return draw_img( HaloRefDrawer( )); }
   virtual int draw_final(CVIEWptr &v) { return draw_img(   FinalDrawer(v)); }

   //******** BODY VIRTUAL METHODS ********
   virtual BODYptr copy(int make_new=1) const {
      if (make_new)
         return BODYptr(new BMESH(*this));
      else
         return BODYptr((BODY*)this);
   }
   virtual int  intersect(RAYhit&, CWtransf&, mlib::Wpt&, mlib::Wvec&,
                          double& d, double&, XYpt&) const;
   // apply xform to each vertex
   virtual void transform(CWtransf &xform, CMOD& m = MOD());

   // CSG methods:
   virtual BODY* subtract (BODYptr&) { return 0; } // CSG subtract
   virtual BODY* combine  (BODYptr&) { return 0; } // CSG union
   virtual BODY* intersect(BODYptr&) { return 0; } // CSG intersect

   virtual void         set_vertices(CWpt_list &) { }
   virtual CWpt_list&   vertices();
   virtual CEDGElist&   body_edges() {return BODY::_edges;}
   virtual CBBOX&       get_bb();
   virtual void         triangulate(Wpt_list &verts, FACElist &faces);

   //******** DATA_ITEM METHODS ********
   virtual CTAGlist &tags           ()             const;

   virtual void   put_vertices      (TAGformat &)  const;
   virtual void   put_faces         (TAGformat &)  const;
   virtual void   put_uvfaces       (TAGformat &)  const;
   virtual void   put_creases       (TAGformat &)  const;
   virtual void   put_polylines     (TAGformat &)  const;
   virtual void   put_weak_edges    (TAGformat &)  const;
   virtual void   put_sec_faces     (TAGformat &)  const; 
   virtual void   put_colors        (TAGformat &)  const;
   virtual void   put_texcoords2    (TAGformat &)  const;
   virtual void   put_render_style  (TAGformat &)  const;
   virtual void   put_patches       (TAGformat &)  const;
                                             
   virtual void   get_vertices      (TAGformat &);
   virtual void   get_faces         (TAGformat &);
   virtual void   get_uvfaces       (TAGformat &);
   virtual void   get_creases       (TAGformat &);
   virtual void   get_polylines     (TAGformat &);
   virtual void   get_weak_edges    (TAGformat &);
   virtual void   get_sec_faces     (TAGformat &); 
   virtual void   get_colors        (TAGformat &);
   virtual void   get_texcoords2    (TAGformat &);
   virtual void   get_render_style  (TAGformat &);
   virtual void   get_patches       (TAGformat &);

   virtual DATA_ITEM   *dup()  const { return new BMESH(0,0,0); }
   virtual STDdstream  &format(STDdstream &d) const;
   virtual STDdstream  &decode(STDdstream &d);

   //******** NPR SOFT SHADOWS ********

   // these return references for use in I/O TAG_val methods:
   bool& occluder() { return _is_occluder; }
   bool& reciever() { return _is_reciever; }
   void set_occluder(bool oc) { _is_occluder = oc; } 
   void set_reciever(bool re) { _is_reciever = re; }
   
   double& shadow_scale() { return _shadow_scale; }
   void set_shadow_scale(double scale) { _shadow_scale = scale; }

   double& shadow_softness() { return _shadow_softness; }
   void set_shadow_softness(double soft) { _shadow_softness = soft; }

   double& shadow_offset() { return _shadow_offset; }
   void set_shadow_offset(double ofset) { _shadow_offset = ofset; }

   //******* CURVATURE STUFF ********
   
   BMESHcurvature_data *curvature() const {
      if (!_curv_data)
         _curv_data = new BMESHcurvature_data(this);
      return _curv_data;
   }
        
   //******** OBSOLETE STUFF ********

   uint   version()     const   { return _version; }

   // XXX - static function in bmesh.C, unused elsewhere
   static void grow_oriented_face_lists(Bface*, ARRAY<Bface*>&,
                                        ARRAY<Bface*>&);

   // XXX - not used by anyone, brute force approach
   Bedge* nearest_edge(CWpt&);
   Bvert* nearest_vert(CWpt&);

   // XXX - remove or make set & accessor methods?
   static bool _freeze_sils;
   static bool _random_sils;

   //*******************************************************
   // PROTECTED
   //*******************************************************
 protected:
 
   //******** MESH ELEMENTS ********
   Bvert_list   _verts;    ///< list of vertices
   Bedge_list   _edges;    ///< list of edges
   Bface_list   _faces;    ///< list of faces

   //******** PATCHES ********
   Patch_list   _patches;  ///< list of patches
   uint         _version;  ///< increment to invalidate display lists
   ThreadMutex  _patch_mutex;

   //******** STRIPS ********
   ZcrossPath   _zx_sils;               ///< zero-cross sil-strip class
   EdgeStrip    _sils;                  ///< silhouette edge strips
   EdgeStrip*   _creases;               ///< crease edge strips
   EdgeStrip*   _borders;               ///< border edge strips

   EdgeStrip*   _polylines;             ///< pointer to allow subclasses
   VertStrip*   _lone_verts;            ///< pointer to allow subclasses

   // used for updating silhouette strips just when needed:

   uint        _sil_stamp;
   uint        _zx_stamp;

   //******** RENDERING STUFF ********
   RefImageClient_list  _drawables;     ///< list of drawable things
   LIST<str_ptr>        _render_style;  ///< name of render style to use (if any)
   bool                 _draw_enabled;  ///< flag to enable/disable drawing
   double               _pix_size;      ///< approx pix size
   uint                 _pix_size_stamp;///< for caching per-frame

   //******** MESH TYPE ********
   int          _type;          ///< type of surface from enum above
   bool         _type_valid;    ///< true if _type has been set

   //******** BODY STUFF ********
   Wpt_list     _vert_locs;       // XXX - rename
   BODYptr      _body;            // XXX - TODO: understand what this is for

   //******** XFORM STUFF ********
   GEOM*        _geom;            // XXX - shouldn't need to know about GEOM
   
   Wtransf      _pm;              ///< object space to NDCZ space transform
   // _pm = Projection matrix * Model matrix:
   uint         _pm_stamp;        ///< frame number last updated. \todo XXX - fix
   Wpt          _eye_local;       ///< camera location in local coordinates
   uint         _eye_local_stamp; ///< frame number last updated. \todo XXX - fix

   Wtransf  _obj_to_world;
   Wtransf  _world_to_obj;

   //******** CURVATURE STUFF ********
   mutable BMESHcurvature_data *_curv_data;

   //******** I/O ********
   /// Full set of tags
   static TAGlist*      _bmesh_tags;
   /// Partial set of tags used for loading frames of animation (e.g. just the vertices change position)
   static TAGlist*      _bmesh_update_tags;

   //******** CACHED MEASURES ********
   double      _avg_edge_len;
   bool        _avg_edge_len_valid;

   //******** EDITING ********
   int          _edit_level;

   //******** PATCH BLENDING WEIGHTS ********
   bool  _patch_blend_weights_valid;
   int   _patch_blend_smooth_passes;
   float _blend_remap_value;
   bool  _do_patch_blend;

   //******** NPR SOFT SHADOWS ********
   bool   _is_occluder;
   bool   _is_reciever;
   double _shadow_scale;
   double _shadow_softness;
   double _shadow_offset;

 public:
   
   virtual void  update_patch_blend_weights();
   virtual int   patch_blend_smooth_passes() const {
      return _patch_blend_smooth_passes;
   }
   virtual void   set_patch_blend_smooth_passes(int i)  {
      _patch_blend_smooth_passes = i;
      _patch_blend_weights_valid = false;
   }
   virtual float get_blend_remap_value() const { return _blend_remap_value; }
   virtual void  set_blend_remap_value(float i)  { _blend_remap_value =  i; }
   virtual bool  get_do_patch_blend() const { return _do_patch_blend; }
   virtual void  set_do_patch_blend(bool i)  { _do_patch_blend =  i; }

 protected:

   static BMESHptr _focus;

   //******** SHOW SECONDARY FACES ********
   static bool _show_secondary_faces;   // initially false
 public:
   static bool show_secondary_faces()        { return _show_secondary_faces; }
   static void toggle_show_secondary_faces() {
      _show_secondary_faces = !_show_secondary_faces;
   }

 protected:

   //******** PROTECTED METHODS ********
   int check_type();    ///< recalculate the mesh type

   // Clean out any empty patches:
   void clean_patches();
   bool remove_patch(int k);

   /// internal version of merge(), after error checking:
   virtual void _merge(BMESH* mesh);

   /*! In BMESH, just calls BMESHobs::broadcast_update_request(this),
    * but in LMESH also updates subdivision meshes: */
   virtual void send_update_notification();

   // obsolete?
   void grow_mesh_equivalence_class(
      Bvert*, 
      ARRAY<Bface*>&, ARRAY<Bedge*>&, ARRAY<Bvert*>&
      );
};

/*****************************************************************
 * inlines
 *****************************************************************/

inline BMESHptr
gel_to_bmesh(CGELptr &gel) 
{
   return BMESH::upcast(gel_to_body(gel));
}

inline GEL*
bmesh_to_gel(BMESH* mesh)
{
   return mesh ? mesh->geom() : 0;
}

inline GEOM*
bmesh_to_geom(BMESH* mesh)
{
   return mesh ? mesh->geom() : 0;
}

/*! Convenience method for returning the GEOM that owns a
 * given BMESH, and upcasting it to a given type, if valid.
 */
template <class T>
inline T* mesh_geom(BMESH* mesh, T* init) 
{
   GEOM* geom = bmesh_to_geom(mesh);
   return T::isa(geom) ? (T*)geom : init; 
}

inline void
set_edit_level(BMESHptr m, int level)
{
   if (m) {
      while (level > m->edit_level()) // increase edit level as needed
         m->inc_edit_level();
      while (level < m->edit_level()) // decrease edit level as needed
         m->dec_edit_level();
   }
}

/************************************************************
 * BMESHobs_list:
 *
 *      Convenience class: ARRAY of BMESH observers
 *      (class BMESHobs) with methods for forwarding
 *      notifications to all observers in the list.
 *      See BMESHobs class, below.
 ************************************************************/
class BMESHobs;
class BMESHobs_list : public ARRAY<BMESHobs*> {
 public:

   //******** MANAGERS ********

   BMESHobs_list(int k=0) : ARRAY<BMESHobs*>(k) { set_unique(); }

   //******** CONVENIENCE METHODS ********

   // Forward notifications to all observers in the list:
   void notify_change        (BMESH*, BMESH::change_t)  const;
   void notify_xform         (BMESH*, CWtransf&, CMOD&) const;
   void notify_merge         (BMESH*, BMESH*)           const;
   void notify_split         (BMESH*, CARRAY<BMESH*>&)  const;
   void notify_subdiv_gen    (BMESH* parent)            const;
   void notify_delete        (BMESH*)                   const;
   void notify_sub_delete    (BMESH*)                   const;
   void notify_update_request(BMESH*)                   const;

   //******** UTILITIES ********

   void print_names() const;
};
typedef const BMESHobs_list CBMESHobs_list;

/************************************************************
 * BMESHobs:
 *
 *      A "BMESH observer" object that gets notifications of
 *      BMESH events.
 ************************************************************/
class BMESHobs {
 public:

   virtual ~BMESHobs() {}

   //******** SIGN-UP METHODS ********

   // Used by observers to subscribe or unsubscribe 
   // to mesh notifications for a given BMESH:
   void subscribe_mesh_notifications(BMESH* m) {
      bmesh_obs_list(m).add_uniquely(this);
   }
   void unsubscribe_mesh_notifications(BMESH* m) {
      bmesh_obs_list(m).rem(this);
   }

   // Used by observers to subscribe or unsubscribe 
   // to mesh notifications for all BMESHes:
   void subscribe_all_mesh_notifications() {
      _all_observers.add_uniquely(this);
   }
   void unsubscribe_all_mesh_notifications() {
      _all_observers.rem(this);
   }

   //******** NOTIFICATION CALLBACKS ********

   // Virtual methods filled in by BMESHobs subclasses, 
   // to respond to various BMESH events:
   virtual void notify_change    (BMESH*, BMESH::change_t)  {}
   virtual void notify_xform     (BMESH*, CWtransf&, CMOD&) {}
   virtual void notify_merge     (BMESH*, BMESH*)           {}
   virtual void notify_split     (BMESH*, CARRAY<BMESH*>&)  {}
   virtual void notify_subdiv_gen(BMESH*)                   {}
   virtual void notify_delete    (BMESH*)                   {}
   virtual void notify_sub_delete(BMESH*)                   {}

   // This is for observers who want to update the mesh before it
   // does something important like try to draw itself:
   virtual void notify_update_request(BMESH*)               {}

   //******** STATICS ********

   // These are called by a BMESH to notify observers:
   static  void broadcast_change        (BMESH*, BMESH::change_t);
   static  void broadcast_xform         (BMESH*, CWtransf& xf, CMOD&);
   static  void broadcast_merge         (BMESH* joined, BMESH* removed);
   static  void broadcast_split         (BMESH*, CARRAY<BMESH*>&);
   static  void broadcast_subdiv_gen    (BMESH*);
   static  void broadcast_delete        (BMESH*);
   static  void broadcast_sub_delete    (BMESH*);
   static  void broadcast_update_request(BMESH*);

   //******** UTILITIES ********

   // For debugging, e.g.:
   virtual str_ptr name() const { return "bmesh_obs"; }

   // Returns the observer list for a particular mesh:
   static const BMESHobs_list& observers(BMESH* m)  {
      return bmesh_obs_list(m);
   }

   static void print_names(BMESH* m) { observers(m).print_names(); }

 protected:
   // Hash table that maps an observer list to a particular mesh:
   static HASH  _hash;

   // List of observers that want to get notified if ANY mesh
   // changes:
   static BMESHobs_list _all_observers;

   // Returns the observer list for a particular mesh:
   static BMESHobs_list& bmesh_obs_list(BMESH* m)  {
      BMESHobs_list *list = (BMESHobs_list*) _hash.find((long)m);
      if (!list)
         _hash.add((long)m, (void*)(list = new BMESHobs_list));
      return *list;
   }
};
typedef const BMESHobs CBMESHobs;


/************************************************************
 * BMESHray:
 *
 *    A RAYhit subclass that stores the Bsimplex that was
 *    hit when a BMESH was intersected.
 ************************************************************/
#define CBMESHray const BMESHray
class BMESHray : public RAYhit {
 public :
   //******** MANAGERS ********
   BMESHray(CWpt  &p, mlib::CWvec &v):RAYhit(p,v), _simplex(0) { }
   BMESHray(CWpt  &a, mlib::CWpt  &b):RAYhit(a,b), _simplex(0) { }
   BMESHray(CXYpt &p)          :RAYhit(p),   _simplex(0) { }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("BMESHray", RAYhit, CRAYhit *);
   static  int isa (CRAYhit &r)       { return ISA((&r)); }

   //******** ACCESSORS ********
   Bsimplex* simplex()          const   { return _simplex; }
   void      set_simplex(Bsimplex *s)   { _simplex = s; }

   BMESH* mesh() const { return _simplex ? _simplex->mesh() : (BMESH*)0; }

   //******** CONVENIENT UPCASTS ********
   Bvert* vert() const { return bvert(_simplex); }
   Bedge* edge() const { return bedge(_simplex); }
   Bface* face() const { return bface(_simplex); }

   //******** STATIC METHODS ********
   static Bsimplex* simplex(CRAYhit &r) {
      return BMESHray::isa(r) ? ((BMESHray &)r).simplex():0;
   }
   static  void  set_simplex (CRAYhit &r, Bsimplex *f) { 
      if (BMESHray::isa(r)) ((BMESHray &)r).set_simplex(f);
   }

   static Bvert* vert(CRAYhit& r) { return bvert(simplex(r)); }
   static Bedge* edge(CRAYhit& r) { return bedge(simplex(r)); }
   static Bface* face(CRAYhit& r) { return bface(simplex(r)); }

   //******** RAYhit METHODS ********
   virtual void check(double, int, double, CGELptr &, CWvec &, mlib::CWpt &,
                      CWpt &, APPEAR *a, mlib::CXYpt &);

   //*******************************************************
   // PROTECTED
   //*******************************************************
 protected:
   Bsimplex*     _simplex; // the mesh simplex that was hit
};

/*****************************************************************
 * BMESH_list:
 *
 *      A LIST of BMESHes with convenience methods for passing
 *      calls to all the items in the list.
 *****************************************************************/
class BMESH_list : public RIC_list<BMESHptr> {
 public:

   //******** MANAGERS ********

   BMESH_list(int n=0)                     : RIC_list<BMESHptr>(n) {}
   BMESH_list(const RIC_list<BMESHptr>& l) : RIC_list<BMESHptr>(l) {}
   BMESH_list(const GELlist& gels) : RIC_list<BMESHptr>(gels.num()) {
      for (int i=0; i<gels.num(); i++) {
         BMESHptr m = gel_to_bmesh(gels[i]);
         if (m) {
            add(m);
         }
      }
   }
   //******** CONVENIENCE ********

   // Notify all meshes of a change
   void changed(BMESH::change_t c = BMESH::TOPOLOGY_CHANGED) {
      for (int i=0; i<_num; i++)
         _array[i]->changed(c);
   }

   // Returns true if any of the meshes are draw enabled:
   bool draw_enabled() const {
      bool ret = false;
      for (int i=0; i<_num; i++)
         ret |= _array[i]->draw_enabled();
      return ret;
   }

   // Get the aggregate BBOX of all the meshes:
   BBOX bbox() const {
      BBOX ret;
      for (int i=0; i<_num; i++)
         if (_array[i]->draw_enabled())
            ret += _array[i]->get_bb();
      return ret;
   }

   // XXX - why is the compiler making me do this??
   BMESH_list operator+(const BMESH_list& m) const {
      BMESH_list ret(num() + m.num());
      ret = *this;
      for (int i=0; i<m.num(); i++)
         ret += m[i];
      return ret;
   }

   // minimum alpha (transparency in OpenGL):
   double min_alpha() const {
      double ret=1;
      for (int k=0; k<_num; k++)
         ret = min(ret, _array[k]->patches().min_alpha());
      return ret;
   }

   // apply xform to each mesh
   void transform(CWtransf &xf, CMOD& m = MOD()) {
      for (int i=0; i<_num; i++)
         _array[i]->transform(xf, m);
   }

   // print the mesh names to an output stream:
   void print_names(ostream& out) const {
      for (int i=0; i<num(); i++)
         out << (*this)[i]->get_name() << " ";
      out << endl;
   }
};
typedef const BMESH_list CBMESH_list;

inline CWtransf& Patch::xform()      const   { return _mesh->xform(); }
inline CWtransf& Patch::inv_xform()  const   { return _mesh->inv_xform(); }
inline CWtransf& Patch::obj_to_ndc() const   { return _mesh->obj_to_ndc(); }
inline CWpt&     Patch::eye_local()  const   { return _mesh->eye_local(); }
inline double    Patch::pix_size()   const   { return _mesh->pix_size(); }

#endif  // BMESH_H_HAS_BEEN_INCLUDED

// end of file bmesh.H
