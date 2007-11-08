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
 * bcurve.H
 *****************************************************************/
#ifndef BCURVE_H_IS_INCLUDED
#define BCURVE_H_IS_INCLUDED

#include "geom/command.H"
#include "map3d/map1d3d.H"
#include "mesh/simplex_filter.H"
#include "mesh/vert_frame.H"
#include "tess/bpoint.H"

class WpathStroke;
class CurveMeme;        // defined below
/*****************************************************************
 * Bcurve:
 * 
 *      Implementation of a curve that can be embedded in a
 *      surface. The curve can be oversketched by the user; its
 *      endpoints (Bpoints) can also be manipulated directly by
 *      the user.
 *
 *      The actual curve geometry and connectivity is represented
 *      with mesh edges and vertices; subdivision provides
 *      smoothness. A curve may have a shadow that (partly)
 *      defines its shape. In that case the user can oversketch
 *      the curve to define its 3D shape using 2D input.
 *****************************************************************/

typedef const Bcurve CBcurve;
class Bcurve : public Bbase{
 public:

   //******** MANAGERS ********

   //! Create a curve from the given polyline with uniform sampling.
   //! If res level < 0 the curve will compute an appropriate level.
   Bcurve(CLMESHptr&    mesh,           //!< mesh to use
          CWpt_list&    pts,            //!< hi-res point trail
          CWvec&        n,              //!< the curve's "plane normal"
          int           num_edges,      //!< num edges at control level
          int           res_lev = -1,   //!< num subdiv levels to override
          Bpoint*       b1 = 0,         //!< optional 
          Bpoint*       b2 = 0);        //!< endpoints

   //! Similar to above, but with prescribed non-uniform sampling:
   Bcurve(CLMESHptr&      mesh,         //!< mesh to use
          CWpt_list&      pts,          //!< hi-res point trail
          CWvec&          n,            //!< the curve's "plane normal"
          CARRAY<double>& t,            //!< t-values at which to sample curve
          int             res_lev = -1, //!< num subdiv levels to override
          Bpoint*         b1 = 0,       //!< optional
          Bpoint*         b2 = 0);      //!< endpoints

   //! Same as above, except this one takes an explicit map object
   //! instead of a Wpt_list
   Bcurve(CLMESHptr&      mesh,
          Map1D3D*        map,
          CARRAY<double>& t,
          int             res_lev = -1,
          Bpoint*         b1 = 0,
          Bpoint*         b2 = 0);


   //! create bcurve that lives in specified surface:
   Bcurve(CUVpt_list&, 
          Map2D3D*, 
          CLMESHptr&, 
          CARRAY<double>&t,
          int res_level = -1,
          Bpoint*         b1 = 0,
          Bpoint*         b2 = 0);

   //! create bcurve that lives on a specified set of vertices
   Bcurve(CBvert_list& verts,
          UVpt_list uvpts,
          Map2D3D* surf,
          int res_lev);

   //! create bcurve that lives on a specified set of vertices on a skin
   //! the vert list must be closed
   Bcurve(CBvert_list& verts,
          Bsurface* skin,
          int res_lev);

   //! Create a child Bcurve of the given parent:
   Bcurve(Bcurve* parent);
   
   virtual ~Bcurve();     
   
   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Bcurve", Bcurve*, Bbase, CBnode*);

   //******** ACCESSORS ********

   //! The hi-res curve this approximates:
   Map1D3D*     map()           const   { return _map; }

   //! Returns the t-values for the curve memes
   //! (one for each vertex, in order, no duplicates):
   ARRAY<double> tvals()        const;

   //! Convenience: meme number i:
   CurveMeme* cm(int i) const;

   bool has_shadow()     const   { return _shadow_plane.is_valid(); }

   //! Meme list
   const EdgeMemeList& ememes() const { return _ememes; }
	
 protected:
   
   //! Get the edge strip at level k relative to the control curve:
   CEdgeStrip* strip(int k)     const;

 public:              
   //! Get the edge strip corresponding to *this* bcurve:
   CEdgeStrip* strip()          const   { return strip(_bbase_level); }

   //! Same as above but passed by copying the strip, for convenience:
   EdgeStrip get_strip() const {
      CEdgeStrip* ret = strip();
      return ret ? *ret : EdgeStrip();
   }

   //! Return the strip for the child (or parent) Bcurve
   //! at the "current" subdivision level:
   CEdgeStrip* cur_strip() const;

   //! Same as above but passed by copying the strip, for convenience:
   EdgeStrip get_cur_strip() const {
      CEdgeStrip* ret = cur_strip();
      return ret ? *ret : EdgeStrip();
   }

   static bool
   splice_curve_on_closed(
      PIXEL_list   crv_a, //!< non-const because curve may need to be reversed
      PIXEL_list   crv_b, //!< non-const because curve may need to be shifted
      double thresh,
      PIXEL_list& new_crv);

   static bool
   splice_curves(
      PIXEL_list   crv_a, //!< non-const because curve may need to be reversed
      CPIXEL_list& crv_b, 
      double thresh,
      PIXEL_list& new_crv);

   //******** SUBDIVISION BCURVES ********

   //! Bcurve at top of hierarchy:
   Bcurve* ctrl_curve()         const   { return (Bcurve*) control(); }

   //! The subdiv parent and child curves:
   Bcurve* parent()     const   { return (Bcurve*) _parent; }
   Bcurve* child()      const   { return (Bcurve*) _child; }
   Bcurve* cur_curve()  const   { return (Bcurve*) cur_subdiv_bbase(); }

   //! Set the "resolution level":
   virtual void set_res_level(int r);

   //! apply update
   virtual bool apply_update();

   //******** CURVE ELEMENTS ********

   //! Edges of the curve:
   CBedge_list& edges() const {
      CEdgeStrip* s = strip();
      assert(s);
      return s->edges();
   }
   int          num_edges()             const   { return edges().num(); }
   Bedge*       edge(int i)             const   { return edges()[i]; }

   //! Vertices of this curve;
   Bvert_list   verts()                 const;

   //! Same as verts(), but for closed curves first vertex
   //! appears again at the end of the list.
   Bvert_list   full_verts()            const;

   //! Elements of the "current" subdiv curve:
   CBedge_list& cur_subdiv_edges()      const;
   Bvert_list   cur_subdiv_verts()      const;

   //******** CURVE POINTS ********

   //! Positions of control vertices:
   Wpt_list control_pts()       const   { return verts().pts(); }

   //! The Wpt_list of the curve at the current subdiv level:
   Wpt_list cur_subdiv_pts() const { return cur_subdiv_verts().pts(); }

   //! Current subdiv pts projected to PIXELs,
   //! returns true on success:
   bool cur_pixel_trail(PIXEL_list& pixels) const {
      return cur_subdiv_pts().project(pixels);
   }

   //******** LOCAL COORDINATE FRAMES ********

   //! Associated normal vector (e.g. for planar curves):
   Wvec normal() const;

   //! Returns vector from first point to second if not closed.
   Wvec vector() const;

   //! Local tangent, normal, and binormal (orthonormal vectors):
   Wvec t(CBvert* v) const;
   Wvec n(CBvert* v) const { return normal().orthogonalized(t(v)).normalized();}
   Wvec b(CBvert* v) const { return cross(n(v),t(v)); }

   Wvec t(CBedge* e) const;
   Wvec n(CBedge* e) const { return normal().orthogonalized(t(e)).normalized();}
   Wvec b(CBedge* e) const { return cross(n(e),t(e)); }

   //******** GEOMETRY ********

   bool is_closed() const;

   bool get_plane(Wplane& P, double len_scale=1e-3) const;
   bool is_planar(double len_scale=1e-3) const {
      Wplane P;
      return get_plane(P, len_scale);
   }
   Wplane plane() const {
      Wplane P;
      get_plane(P);
      return P;
   }      

   //! "center of mass" of curve:
   //! currently just average of control vertices
   Wpt center() const { return verts().center(); }

   //! Returns the winding number WRT the given point and direction.
   //! See mlib/point3.H for details
   double winding_number(CWpt& o, CWvec& t) const {
      // XXX - using object space points, ignoring mesh xform
      return full_verts().pts().winding_number(o, t);
   }

   //! Return true if the curve is straight.
   bool is_straight(double len_scale=1e-3) const {
      return control_pts().is_straight(len_scale);
   }

   //! Return true if the curve has faces along both sides of every edge:
   bool is_embedded() const { return edges().nfaces_is_equal(2); }

   //! Return true if all edges are border edges (1 adjacent face each):
   bool is_border()   const { return edges().nfaces_is_equal(1); }

   //! Return true if all edges are "polyline" edges (both f1 and f2 are null):
   bool is_polyline() const { return edges().nfaces_is_equal(0); }

   //! Return true if every edge has at least one adjacent face
   //! (counting f1, f2, and "multi" edges)
   bool is_in_surface() const;

   //! Return true if all edges are "secondary" edges:
   bool is_secondary() const { return edges().is_secondary(); }

   //! Check whether a condition is true for the curve, or
   //! any of its children:
   typedef bool (Bcurve::*crv_test_meth)() const;
   bool satisfies_any_level(crv_test_meth) const;

   //! Return true if the curve or any of its children are embedded
   bool is_embedded_any_level() const {
      return satisfies_any_level(&Bcurve::is_embedded);
   }
   //! Return true if the current subdiv edges are embedded
   bool is_embedded_cur_level() const {
      return cur_subdiv_edges().all_satisfy(InteriorEdgeFilter());
   }

   //! Return true if the curve or any of its children are border
   bool is_border_any_level() const {
      return satisfies_any_level(&Bcurve::is_border);
   }
   //! Return true if the current subdiv edges are border
   bool is_border_cur_level() const {
      return cur_subdiv_edges().all_satisfy(BorderEdgeFilter());
   }

   //! Return true if the curve or any of its children are in a surface
   bool is_in_surface_any_level() const {
      return satisfies_any_level(&Bcurve::is_in_surface);
   }
   //! curves surrounded by surfaces should not be drawn to the screen
   //! (but okay to draw to VisRefImage, for picking):
   bool should_draw() const;

   //******** RESAMPLING ********

   //! Uniform resampling:
   bool resample(int num_edges);
   
   //! Non-uniform resampling
   //! (resample at given t values):
   bool resample(CARRAY<double>& t) { return resample(t, NULL, NULL); }

   //! Checks preconditions:
   bool can_resample() const;

   //******** SHADOW CREATION ********
   void set_shadow_plane(CWplane& P) { _shadow_plane = P; }
   void remove_shadow()              { set_shadow_plane(Wplane()); }

   CWplane& shadow_plane() const { return _shadow_plane; }
   Wvec shadow_dir() const { return _shadow_plane.normal().normalized(); }

   //! sets this bcurve's map to specified one, and makes sure children
   //! get the same map.
   void set_map(Map1D3D* c);

   //! returns the list of worldpts that correspond to the ones created
   //! when the user drew this curve.
   Wpt_list get_wpts() const;
   //! gets the tvals corresponding to the wpts above;
   ARRAY<double> get_map_tvals() const;

   bool get_pixels(PIXEL_list& list) const;

   //******** ADJACENCY ********

   Bpoint* b1() const {
      EdgeStrip s = get_strip();
      return s.empty() ? 0 : Bpoint::lookup(s.first());
   }
   Bpoint* b2() const {
      EdgeStrip s = get_strip();
      return s.empty() ? 0 : Bpoint::lookup(s.last());
   }
   Bpoint* other_point(Bpoint* b) const {
      return (b == b1()) ? b2() : (b == b2()) ? b1() : 0;
   }

   bool contains(CBpoint* b) const { return b == b1() || b == b2(); }

   //! The curve is "isolated" if there are no adjacent faces and the
   //! set of control edges is "maximal" (no adjacent foreign edges):
   //
   // XXX -
   //   Doesn't address the possibility that at some 
   //   subdivision level > 0 the story might be different.
   bool is_isolated() const;

   Bsurface_list surfaces() const;

   //! Returns the surface this curve is constrained in,
   //! if it is constrained in one.
   Map2D3D* constraining_surface() const {
      if (SurfaceCurveMap::isa(_map)) {
         return ((SurfaceCurveMap*)_map)->surface();
      }
      return 0;
   }

   //! returns the skin this curve is constrained in,
   //! if it is constrained in one
   Bsurface* constraining_skin() const {
      return _skin;
   }

   //! starting with this curve, return a list of curves that
   //! forms a closed loop:
   bool         extend_boundaries(Bcurve_list& result);
   Bcurve_list  extend_boundaries();

   void replace_endpt(Map0D3D* old_pt, Map0D3D* new_pt) {
     if (map())
       map()->replace_endpt(old_pt, new_pt);
   }

   //******** ADDING ELEMENTS ********

   // Vertices:
   virtual VertMeme* add_vert_meme(VertMeme* v);

   // Edges:
   virtual EdgeMeme* add_edge_meme(EdgeMeme* e);
   virtual EdgeMeme* add_edge_meme(Ledge*);
   EdgeMeme* add_edge(Bvert* u, Bvert* v) {
      return add_edge_meme((Ledge*)_mesh->add_edge(u,v));
   }

   //******** SIMPLEX NOTIFICATION ********
   
   void notify_edge_deleted(EdgeMeme* ed);

   //******** LOOKUP/INTERSECTION ********

   //! Returns the Bcurve that owns the boss meme on the simplex:
   static Bcurve* find_owner(CBsimplex* s) {
      return upcast(Bbase::find_owner(s));
   }

   //! Find the Bcurve owner of this simplex, or if there is none,
   //! find the Bcurve owner of its subdivision parent (if any),
   //! continuing up the subdivision hierarchy until we find a
   //! Bcurve owner or hit the top trying.
   static Bcurve* find_controller(CBsimplex* s) {
     return upcast(Bbase::find_controller(s));
   }

   //! Returns the Bcurve (if any) that owns all the edges (if any):
   static Bcurve* lookup(CBedge_list& edges) {
      Bcurve* ret = edges.empty() ? 0 : find_controller(edges.first());
      for (int i=1; i<edges.num(); i++)
         if (find_controller(edges[i]) != ret)
            return 0;
      return ret;
   }

   //! Returns the set of Bcurves owning any of the given edges:
   static Bcurve_list get_curves(CBedge_list& edges);

   //! convenience: if a single curve owns the edges, return it.
   //! otherwise return null.
   static Bcurve* get_curve(CBedge_list& edges);

   //! Given screen-space point p and search radius (in pixels),
   //! return the Bcurve closest to p within the search radius (if
   //! any). When successful, also fills in the Wpt that was hit,
   //! and the hit edge (if requested).
   //
   // Note: the edge may not belong directly to the curve if it's
   // from a subdivision level not controlled directly by a Bcurve.
   //
   static Bcurve* hit_curve(CNDCpt& p, double rad, Wpt& hit, Bedge** e=0);
   static Bcurve* hit_curve(CNDCpt& p, double rad=6) {
      Wpt hit;
      return hit_curve(p, rad, hit);
   }

   //! Same as above, but returns the control-level Bcurve:
   static Bcurve* hit_ctrl_curve(CNDCpt& p, double rad, Wpt& hit, Bedge** e=0){
      Bcurve* ret = hit_curve(p, rad, hit, e);
      return ret ? ret->ctrl_curve() : 0;
   }
   static Bcurve* hit_ctrl_curve(CNDCpt& p, double rad=6) {
      Wpt hit;
      return hit_ctrl_curve(p, rad, hit);
   }
   
   //******** COMPUTATION ********

   //! Returns length of curve's edges at given subdivision level
   //! (relative to this curve)
   double length(int level) const {
      return get_strip().edges(level).total_length();
   }

   //! Length of curve in its hi-res form:
   double length() const { return _map ? _map->length() : length(0); }

   //! Returns average edge length at given subdivision level:
   //! (relative to this curve)
   double avg_edge_length(int level=0) const {
      return get_strip().edges(level).avg_len();
   }

   //******** Oversketch Method *******

   virtual bool oversketch(CPIXEL_list& sketch_pixels);
   void    rebuild_seg(Wpt_list& newpts, int base_start, int base_end,
                       ARRAY<Lvert*>& affected, ARRAY<int>& indices);

   //! cycles to the next reshape mode 
   void    cycle_reshape_mode();

   //******** Visibility methods
   void HideShadow(void) { _shadow_visible=false;}
   void ShowShadow(void) { _shadow_visible=true;}
   
   //******** Bbase METHODS ********

   //! Delete the edge meme:
   virtual void rem_edge_meme(EdgeMeme* e);

   //! Wipe out edges and non-Bpoint vertices
   virtual void delete_elements();

   virtual void produce_child();

   virtual CCOLOR& selection_color() const;
   virtual CCOLOR& regular_color()   const;

   virtual void draw_debug();
   virtual void draw_normals(double len, // world length of each norm 
                             CCOLOR& col,
                             bool draw_binormals=false);

   virtual void draw_points (int size=2, 
                             CCOLOR& col=COLOR::green);

   virtual bool apply_xf(CWtransf& xf, CMOD& mod);

   //******** SELECTION ********

   static Bcurve_list   selected_curves();
   static Bcurve*       selected_curve();

   virtual void set_selected();
   virtual void unselect();

   //******** BMESHobs METHODS ********

   virtual void notify_xform(BMESH*, CWtransf&, CMOD&);

   //******** Bnode METHODS ********

   virtual Bnode_list inputs()  const;
   
   //******** RefImageClient METHODS

   //! draw to the screen:
   virtual int draw(CVIEWptr&);
   virtual int draw_final(CVIEWptr&);    
   virtual int draw_id_ref_pre1();
   virtual int draw_id_ref_pre2(); 
   virtual int draw_id_ref_pre3(); 
   virtual int draw_id_ref_pre4(); 

   //******** RefImageClient METHODS ********

   //! draw to the visibility reference image (for picking)    
   virtual void request_ref_imgs();
   virtual int draw_vis_ref();

   void get_style();  //this is just a test function to see if things work

 protected:

   //******** INTERNAL DATA ******** 

   Map1D3D*     _map;           //!< hi-res curve shape
   LedgeStrip   _strip;         //!< for rendering the curve
   Wplane       _shadow_plane;  //!< optional plane for shadow
   EdgeMemeList _ememes;        //!< edge memes

   Bsurface*    _skin;

   // for rendering with a stroke instead of GL_LINE_STRIP:
   WpathStroke* _stroke_3d;     //!< for rendering the curve
   bool         _got_style;     //!< to make style be applied only once

   // XXX - to be replaced by memes:
   // used by curves that have a shadow:
   ARRAY<double> _heights;      //!< heights of control points above shadow
   double        _max_height;   //!< used to see if they're all zero

   //! Drawing info
   bool          _shadow_visible;

   //! Temporary variable to handle closed curve
   //! movement. shd be zero most of the time:
   double        _move_by; 

   enum reshape_mode_t {
      RESHAPE_NONE = 0,
      RESHAPE_MESH_NORMALS,
      RESHAPE_MESH_BINORMALS,
      RESHAPE_MESH_RIGHT_FACE_TAN,
      RESHAPE_MESH_LEFT_FACE_TAN,
      NUM_RESHAPE_MODES
   };

   reshape_mode_t  _cur_reshape_mode;

   //******** PROTECTED METHODS ********   

   void set_upper(Bcurve*);

   Ledge* le(int k) const {
      CEdgeStrip* s = strip();
      return s ? (Ledge*)s->edge(k) : 0;
   }
   Lvert* lv(int k) const {
      CEdgeStrip* s = strip();
      return s ? (Lvert*)s->vert(k) : 0;
   }

   //******** BUILDING ********

   //! Resampling at prescribed t-values
   bool resample(CARRAY<double>& t, const Bpoint* bpt1, const Bpoint* bpt2);

   //! Internal resampling that does actual work.
   //! Assumes this bcurve can be resampled.
   void _resample(CARRAY<double>& t, const Bpoint* bpt1, const Bpoint* bpt2);

   //! Builds the edge strip, generating interior vertices
   //! and edges:
   void build_strip();

   //******** Bnode PROTECTED METHODS ********
   virtual void recompute();

   //******** UTILITIES ********

   bool reshape_on_plane(const PIXEL_list &new_curve);
   bool reshape_on_surface(const PIXEL_list &new_curve);
   bool reshape_on_skin(const PIXEL_list &new_curve);
   bool reshape_along_normals(const PIXEL_list &new_curve);

   //! Returns a set of 3D lines given by the set of the map's world
   //! points and their associated normals.  If get_binormals is true,
   //! then the binormals (instead of the normals) are used for the line
   //! directions.
   bool get_map_normal_lines(ARRAY<Wline>& out_vecs, 
                             double len=1.0, // desired world length
                             bool get_binormals=false);

   //! Returns a set of 3D lines given by the mesh vertex locations
   //! along the curve and their associated normals.  If get_binormals
   //! is true, then the vertex binormals (instead of the normals) are
   //! used for the line directions.
   bool get_reshape_constraint_lines(ARRAY<Wline>& ret_lines,
                                     reshape_mode_t mode,
                                     double len=1.0 // desired world length
                                     );
};

//! This function attempts to resample the curves so they
//! have the same number of edges. If possible they will
//! both be resampled with 'preferred_num_edges'. Returns
//! true if at the end they have the same number of edges:
bool ensure_same_sampling(Bcurve* a, Bcurve* b, int preferred_num_edges=1);

/*****************************************************************
 * Bcurve_list
 *****************************************************************/
class Bcurve_list : public _Bbase_list<Bcurve_list,Bcurve> {
 public:

   //******** MANAGERS ********

   Bcurve_list(int n=0) :
      _Bbase_list<Bcurve_list,Bcurve>(n)    {}
   Bcurve_list(const Bcurve_list& list) :
      _Bbase_list<Bcurve_list,Bcurve>(list) {}
   Bcurve_list(Bcurve* c) :
      _Bbase_list<Bcurve_list,Bcurve>(c)    {}

   Bcurve_list(CBbase_list& bbases) :
      _Bbase_list<Bcurve_list,Bcurve>(bbases.num()) {
      for (int i=0; i<bbases.num(); i++) {
         Bcurve* c = Bcurve::upcast(bbases[i]);
         if (c)
            *this += c;
      }
   }
   
   //******** CONVENIENCE ********

   Bcurve_list children() const {
      Bcurve_list ret;
      for (int i=0; i<_num; i++) {
         Bcurve* c = _array[i]->child();
         if (c)
            ret += c;
      }
      return ret;
   }

   //******** TOPOLOGY ********

   //! Are the curves connected in the order of the list?
   bool is_connected() const;

   //! Are curves are connected together, one after the other?
   bool forms_chain()           const;

   //! Do curves form a chain, with last connected to first?
   bool forms_closed_chain()    const;

   //! If the curves form a chain, return the bpoints in
   //! order. (If chain is closed, last point == first).
   Bpoint_list get_chain() const;

   //******** GEOMETRY ********

   //! Are all the curves planar and lie in the same plane?
   bool is_planar() const;

   Wplane get_plane() const; 

   //! Are they each straight
   bool is_each_straight() const;

   //! Returns total length of curves
   double total_length()        const;

   //! Average length of curves:
   double avg_length() const { return _num ? total_length()/_num : 0.0; }

   //! Length of shortest curve:
   double min_length()          const;

   //! Length of longest curve:
   double max_length()          const;

   //! Returns average edge length of curves at given subdivision
   //! level (relative to each curve)
   double avg_edge_len(int level=0) const;

   //! Returns average of each curve's "center"
   Wpt center() const;

   Bedge_list all_edges() const;

   //******** BUILDING ********

   //! Can they all be resampled?
   bool can_resample() const;

   //! Given a connected sequence of Bcurves forming a closed path,
   //! extract the control vertices, arranged to run in a consistent
   //! order around the path (agreeing with the first Bcurve).
   bool extract_boundary(Bvert_list& ret) const;

   //! Similar to above, but the control vertices of each Bcurve are
   //! returned in a separate list (1 for each Bcurve):
   bool extract_boundary(ARRAY<Bvert_list>& ret) const;

   //! Convenience form of above
   Bvert_list boundary_verts() const {
      Bvert_list ret;
      if (!extract_boundary(ret))
         ret.clear();
      return ret;
   }

   //! the returned boundary is ordered so that running forward through
   //! the list corresponds to winding CCW around the interior:
   bool extract_boundary_ccw(Bvert_list& ret)           const;
   bool extract_boundary_ccw(ARRAY<Bvert_list>& ret)    const;

   void replace_endpt(Map0D3D* old_pt,
                      Map0D3D* new_pt) {
     for ( int i=0; i<_num ; i++ ) {
       _array[i]->replace_endpt(old_pt, new_pt);
     }
   }

   //******** DRAWING ********

   //! Draw them all in debug style:
   void draw_debug() const;

   //******** GRAPH SEARCHES ********

   static Bcurve_list reachable_curves(Bpoint* p);

 protected:

   //******** GRAPH SEARCHES ********
   static void grow_connected(Bpoint* p, Bcurve_list& ret);

};

typedef const Bcurve_list CBcurve_list;
/*****************************************************************
 * Bcurve inline methods
 *****************************************************************/
inline Bcurve_list
Bcurve::get_curves(CBedge_list& edges)
{
   Bcurve_list ret;
   Bcurve* bc = 0;
   for (int i=0; i<edges.num(); i++)
      if ((bc = find_controller(edges[i])))
         ret.add_uniquely(bc);
   return ret;
}

inline Bcurve* 
Bcurve::get_curve(CBedge_list& edges) 
{
   Bcurve_list curves = get_curves(edges);
   return (curves.num() == 1) ? curves.first() : 0;
}

/*****************************************************************
 * CurveMeme:
 *
 *   Vert meme class that uses a Map1D3D to tessellate a Bcurve.
 *   Each CurveMeme has a t-value that it maps to a 3D point
 *   using the Bcurve's Map1D3D.
 *
 *****************************************************************/
class CurveMeme : public VertMeme {
 public:
   //******** MANAGERS ********
   CurveMeme(Bcurve* b, Lvert* v, double t, bool force_boss) :
      VertMeme(b, v, force_boss),
      _t(t), _dt(0), _count(0) {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("CurveMeme", CurveMeme*, VertMeme, CSimplexData*);

   //******** ACCESSORS ********

   Bcurve*  curve()     const   { return (Bcurve*)bbase(); }
   Map1D3D* map()       const   { return curve()->map(); }
   double   t()         const   { return _t; }
   void     reset_count() { _count = 0; }

   Wvec     tan()       const   { return map()->deriv(_t); }

   void set_t(double t);

   //******** VertMeme VIRTUAL METHODS ********

   //! Get a current _t value from the meme(s) of the parent mesh:
   virtual void copy_attribs_v(VertMeme*);
   virtual void copy_attribs_e(VertMeme*, VertMeme*);

   //! Target location for being more "relaxed":
   virtual Wpt  smooth_target() const;

   //! Compute where to move to a more "relaxed" position:
   virtual bool compute_delt();

   //! Move to the previously computed relaxed position:
   virtual bool apply_delt();

   //! Compute 3D vertex location using the curve map
   virtual CWpt& compute_update();

   //! notify simplex changed
   virtual void notify_simplex_changed();

   //! apply update
   virtual bool apply_update(double thresh = 0.01);

   // The following protected methods can assume error
   // checking was done in the VertMeme base class. See
   // comments for these methods in VertMeme class.

   //! Generate a child vert meme for this vert, passing on
   //! the t-val:
   virtual VertMeme* _gen_child(Lvert* subvert) const;

   //! Generate a child vert meme for an edge shared w/ the
   //! given vert meme, assigning t-value between the two:
   virtual VertMeme* _gen_child(Lvert* subvert, VertMeme* vm) const;

   virtual VertMeme* _gen_child(Lvert*, VertMeme*, VertMeme*, VertMeme*) const
      { assert(0); return 0; }

	protected:
   double       _t;     //!< parameter along curve
   double       _dt;    //!< displacement computed in compute_delt()
   int          _count; //!< count to avoid infinite relaxation
};

inline CurveMeme*
Bcurve::cm(int i) const {
   return (CurveMeme*)_vmemes[i];
}

inline bool
is_connected(Bcurve* a, Bcurve* b)
{
   if (!(a && b))
      return false;

   if ((a->b1() && a->b1()->adjacent_curves().contains(b)) ||
       (a->b2() && a->b2()->adjacent_curves().contains(b)))
      return true;

   return false;
}

/*******************************************************
 * BcurveFilter:
 *
 *      Used in Bcurve::hit_curve() below
 *******************************************************/
class BcurveFilter : public SimplexFilter {
 public:
   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const {
      return Bcurve::find_controller(s) != NULL;
   }
};

#endif // BCURVE_H_IS_INCLUDED

/* end of file bcurve.H */
