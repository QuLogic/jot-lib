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
#ifndef PATCH_H_IS_INCLUDED
#define PATCH_H_IS_INCLUDED

/*!
 *  \file patch.H
 *  \brief Contains the definition of the Patch class and related classes.
 *
 *  \sa patch.C
 *
 */

#include "disp/ref_img_drawer.H"
#include "geom/appear.H"        // base class of Patch

#include "mesh/bface.H"
#include "mesh/edge_strip.H"
#include "mesh/gtexture.H"
#include "mesh/tex_coord_gen.H"
#include "mesh/zcross_path.H"

#include <vector>

class VisibilityTest {
 public:
   virtual ~VisibilityTest() {}

   // does visibility test 
   virtual double visibility(CWpt& p, double len) const { return 1.0; }
   bool is_visible(CWpt& p, double len) const { return visibility(p,len) > 0; }
};

class BMESH;

class PatchData;
typedef const PatchData CPatchData;

/*!
 *  \brief Base class for application-specific, custom data attached to the
 *   patch.
 *
 */
class PatchData {
   
 public:
   
   virtual ~PatchData() {}
      
   //! \name RTTI Methods
   //@{
      
   DEFINE_RTTI_METHODS_BASE("PatchData", CPatchData*);
      
   //@}

};

/*!
 *  \brief DynamicSample is used as a sampel point for Dynamic Hatching 
 *
 */

class DynamicSample;
typedef const DynamicSample CDynamicSample;
class DynamicSample {
 public:
   DynamicSample() : _face(0), _weight(0) {}
   DynamicSample(CBface* f, CWvec& bc, CPIXEL& pix, double weight) :
      _face(f),
      _bc(bc),
      _pix(pix),
      _weight(weight) {}
   DynamicSample(CDynamicSample& s, CPIXEL& pix, double weight) :
      _face(s._face),
      _bc(s._bc),
      _pix(pix),
      _weight(weight) {}
   virtual ~DynamicSample() {}

   // accessors:
   CBface* get_face()            const { return _face; }
   Wvec    get_bc()              const { return _bc;}
   double  get_weight()          const { return _weight; }
   PIXEL   get_pix()             const { return _pix; }

   // object space:
   Wpt     get_pos()             const { return _face->bc2pos(_bc); }
   Wvec    get_norm()            const { return _face->norm(); }

   // world space (given object-to-world transform xf):
   Wpt     get_pos(CWtransf& xf) const { return xf*get_pos(); }

 public:
   CBface*   _face;     // face containing the sample
   Wvec      _bc;       // barycentric coordinates wrt the face
   PIXEL     _pix;      // stored pixel location
   double    _weight;   // weight assigned to the sample

};

class Patch;
typedef const Patch CPatch;

/*!
 *  \brief A region of a BMESH that can be textured individually.
 *
 */
class Patch : public DATA_ITEM, public APPEAR, public RefImageClient {   
 public:

   //******** MANAGERS ********

   // no public constructor; use BMESH::new_patch()

   virtual ~Patch();

   //! \name Accessors
   //@{
      
   BMESH*       mesh ()                 const      { return _mesh; }
   void         set_mesh(BMESH* m)                 { _mesh = m; changed(); }
   Cstr_ptr&    name ()                 const      { return _name;}
   void         set_name(Cstr_ptr &str)            { _name = str;}
   Cstr_ptr&    texture_file ()         const      { return _texture_file;}
   void         set_texture_file(Cstr_ptr &str)    { _texture_file = str;}
   TexCoordGen* tex_coord_gen()         const      { return _tex_coord_gen; }
   void         set_tex_coord_gen(TexCoordGen* tg);
      
   CARRAY<uint>& pixels()               const      { return _pixels; }
   void          add_pixel(uint p)                 { _pixels += p; }
      
   int num_vert_strips() const { return _vert_strips.num(); }
   int num_edge_strips() const { return _edge_strips.num(); }
   int num_tri_strips()  const { return _tri_strips.num(); }
      
   virtual void          set_data(PatchData *d)     { _data = d; }
   virtual PatchData*    get_data()                 { return _data; }
      
   int stencil_id() {
      if (!_stencil_id)
         _stencil_id = ++_next_stencil_id;
      return _stencil_id;
   }
      
   // List of Gtextures:
   const GTexture_list& gtextures() const { return _textures; }
      
   //@}

   //! \name Accessing Patch Geometry
   //@{
   
   //! \brief Lightweight: returns reference to existing face list:
   CBface_list& faces() const   { return _faces; }
   
   //! \brief Heavyweight: builds the lists and returns by copying
   //! (returns vertices and edges of this patch).
   Bvert_list   verts() const;
   //! \copydoc Patch::verts()
   Bedge_list   edges() const;
      
   //@}

   //! \name Silhouettes, Creases, Etc.
   //@{
   
   //! \brief Generates (if needed) and returns the requested silhouettes,
   //! creases, etc.  Operates on the CURRENT subdivision Patch.
   EdgeStrip&   cur_sils()
      { return cur_patch()->build_sils(); }
   //! \copydoc Patch::cur_sils()
   ZcrossPath&  cur_zx_sils()
      { return cur_patch()->build_zx_sils(); }

   EdgeStrip*   cur_creases()
      { return cur_patch()->build_creases(); }
   //! \copydoc Patch::cur_sils()
   EdgeStrip*   cur_borders()
      { return cur_patch()->build_borders(); }
   
      
 protected:
   
   //! \brief Generates (if needed) and returns the requested silhouettes,
   //! creases, etc. Operates on *this* Patch as opposed to the current
   //! subdivision Patch.
   EdgeStrip&   build_sils();
   //! \copydoc Patch::build_sils()
   ZcrossPath&  build_zx_sils();
   //! \copydoc Patch::build_sils()
   EdgeStrip*   build_creases();
   //! \copydoc Patch::build_sils()
   EdgeStrip*   build_borders();

 public:

 protected:
   
   //! \brief Simple accessor -- no checks for updating.
   //! \note May be phased out because it appears to be unused.
   EdgeStrip&   sils()          { return _sils; }
   //! \copydoc Patch::sils()
   ZcrossPath&  zx_sils()       { return _zx_sils; }
   //! \copydoc Patch::sils()
   EdgeStrip*   creases()       { return _creases; }
   //! \copydoc Patch::sils()
   EdgeStrip*   borders()       { return _borders; }
  
   //@}
   
 public:

   //! \name Subdivision
   //! Redefined by Lpatch (subdivision patches).
   //@{
      
   //! \brief Returns the corresponding patch at the "current"
   //! subdivision level.
   virtual Patch* cur_patch() { return this; }
      
   //! \brief Convenience: the mesh of the current subdivision patch.
   BMESH* cur_mesh();
      
   //! \brief The level of this Patch in the subdivision hierarchy
   //! (control mesh is 0, next level is 1, etc.).
   int subdiv_level() const;
      
   //! \brief The level of this Patch relative to its control Patch
   //! (filled in by Lpatch).
   virtual int rel_subdiv_level() { return 0; }
      
   //! \brief The mesh edit level, relative to this patch
   //! (filled in by Lpatch).
   virtual int rel_edit_level();

   //! \brief Returns mesh faces at current subdivision level.
   virtual CBface_list& cur_faces()     const   { return _faces; }
   //! \brief Returns mesh vertices at current subdivision level.
   virtual Bvert_list   cur_verts()     const   { return verts(); }
   //! \brief Returns mesh edges at current subdivision level.
   virtual Bedge_list   cur_edges()     const   { return edges(); }
      
   //! For BMESH patch: returns number of faces in patch.
   //! For LMESH patch: returns number of faces at current subdiv level.
   //! \remark Popular with Gtextures to report on how many faces they rendered.
   virtual int          num_faces()     const    { return _faces.num();}
      
   //! \brief Used by Lpatch to produce a child patch
   //! at the next subdivision level.
   virtual Patch* get_child() { return 0; }
      
   //! \brief Returns the highest-level parent Patch of this one.
   //!
   //! \note NB. the control patch may not belong to the control mesh,
   //! in the case that a patch has been added at a level > 0.
   virtual Patch* ctrl_patch() const { return (Patch*)this; }
      
   //@}

   //! \name Convenience - Mesh Accessors
   //@{

   // defined in bmesh.H:
   CWtransf& xform()      const;
   CWtransf& inv_xform()  const;
   CWtransf& obj_to_ndc() const;
   CWpt&     eye_local()  const;
   double    pix_size()   const;
      
   //@}

   //! \name Building
   //@{
      
   void add(Bface* f);
   void remove(Bface* f);

   void add(CBface_list& faces) {
      for (int i=0; i<faces.num(); i++)
         add(faces[i]);
   }

   void add(VertStrip*);
   void remove(VertStrip*);
   
   void add(EdgeStrip*);
   void remove(EdgeStrip*);
   
   void build_tri_strips();
   virtual double tris_per_strip() const {
      return _tri_strips.empty() ? 0.0 :
         double(_faces.num())/_tri_strips.num();
   }
      
   //@}

   //! \name Versioning/Caching
   //! used by textures to tell if they are up-to-date
   //! (e.g., display lists may be out of date).
   //@{
      
   virtual void changed() { _stamp++; }
      
   virtual uint stamp();
      
   //@}

   //! \name Notification
   //@{
   
   virtual void triangulation_changed();
   
   void creases_changed();
   void borders_changed();
  
   //@}

   //! \name RefImageClient Methods
   //@{
      
   virtual int draw(CVIEWptr&);
      
   //@}

   //! \name Drawing
   //@{
   
   // XXX - in transition.
   //       (above comment is probably from 1998 or something...)
   virtual int  draw_tri_strips(StripCB *);
   virtual int  draw_sil_strips(StripCB *);

   // draw the n-ring faces, optionally excluding the faces of this patch:
   virtual int draw_n_ring_triangles(
      int n,                    // the "n" in "n-ring"
      StripCB* cb,
      bool exclude_interior
      );

   int          draw_crease_strips(StripCB *);
   int          draw_border_strips(StripCB *);
   int          draw_edge_strips(StripCB *);
   int          draw_vert_strips(StripCB *);
      
   //@}
   
   //! \name Procedural Textures
   //@{
      
   void      set_texture(GTexture* gtex);
   void      set_texture(Cstr_ptr& style);
   void      next_texture();
   
   //! \brief Returns an existing texture of the given type, or nil.
   GTexture* find_tex(Cstr_ptr& tex_name) const;
   
   //! \brief Same as find_tex() but makes a new GTexture if needed.
   GTexture* get_tex(Cstr_ptr& tex_name);
   
   //! \brief Returns an index for a texture.
   int get_tex_index(Cstr_ptr& tex_name) {
      return _textures.get_index(get_tex(tex_name));
   }
   
   //! \brief Return "current" GTexture, based on the mesh's render style
   //! if it has one, or based on the view's render style.
   virtual GTexture* cur_tex(CVIEWptr& v);
   
   //! \brief Texture to use according to the "current texture index".
   GTexture* cur_tex() const {
      return _textures.valid_index(_cur_tex_i) ? _textures[_cur_tex_i] : 0;
   }
      
   //@}
   
   //! \name I/O Functions
   //@{
      
   virtual int write_stream     (ostream &os);
   virtual int read_stream      (istream &is, str_list &leftover);
   virtual int read_texture     (istream &is, str_list &leftover);
   virtual int read_color       (istream &is, str_list &leftover);
   virtual int read_texture_map (istream &is, str_list &leftover);
   virtual int read_patchname   (istream &is, str_list &leftover);
      
   //@}

   //! \name RefImageClient Methods
   //@{
      
   virtual void request_ref_imgs();
   virtual int  draw_vis_ref();
   virtual int  draw_id_ref();
   virtual int  draw_id_ref_pre1();
   virtual int  draw_id_ref_pre2();
   virtual int  draw_id_ref_pre3();
   virtual int  draw_id_ref_pre4();
   virtual int  draw_halo_ref();
   virtual int  draw_color_ref(int i);
   virtual int  draw_final(CVIEWptr &v);
      
   //@}
      
   //! \name RTTI Methods
   //@{
      
   DEFINE_RTTI_METHODS3("Patch", Patch*, DATA_ITEM, Patch *);
      
   //@}
      
   //! \name DATA_ITEM Methods
   //@{
      
   virtual CTAGlist    &tags()  const;
   virtual DATA_ITEM  *dup  ()  const;
      
   //@}
      
   //! \name Appear Methods
   //@{
      
   virtual CCOLOR&   color         ()                const;
   virtual bool      has_color     ()                const;
   virtual void      set_color     (CCOLOR& c);
   virtual void      unset_color   ();
      
   virtual double    transp        ()                const;
   virtual bool      has_transp    ()                const;
   virtual void      set_transp(double t);
   virtual void      unset_transp();
      
   //! \brief Set normal texture map.
   virtual void      set_texture   (CTEXTUREptr& t);
   //! \brief Unset normal texture map.
   virtual void      unset_texture ();
      
   //! \brief convenience
   void apply_texture() {
      if (has_texture()) {
         _texture->apply_texture(&_tex_xform);
      }
   }
      
   //! \brief Function to use for initializing a fade from one GTexture
   //! to another.
   static init_fade_func_t _init_fade;
   
   //! \name Dynamic Samples
   //@{

   int      get_sps_height()             const{ return _sps_height; }
   double   get_sps_min_dist()           const{ return _sps_min_dist; }
   double   get_sps_regularity()         const{ return _sps_regularity; }
   double   get_sps_spacing()           const { return _sample_spacing; }
   void     set_sps_height(int v)        { _sps_height = v; }
   void     set_sps_min_dist(double v)   { _sps_min_dist = v; }
   void     set_sps_regularity(double v) { _sps_regularity = v; }

   //! \brief center of the samples from last frame
   PIXEL    get_old_sample_center()  { return _old_center; }
   //! \brief center of the samples
   PIXEL    get_sample_center()  { return _new_center; }
   //! \brief Non-accumulated z
   VEXEL    get_z()              { return _z; }

   PIXEL    get_patch_center()  { return _patch_center; }


   bool     get_do_dynamic_stuff() const     { return _do_dynamic_stuff; }
   void     do_dynamic_stuff(bool d)         { _do_dynamic_stuff = d; }
   void     set_do_lod(bool d)               { _do_lod = d; }
   bool     get_do_lod()                     { return _do_lod; }
   void     set_do_rotate(bool d)            { _do_rotation = d; }
   bool     get_do_rotate()                  { return _do_rotation; }
   void     set_direction_stroke(Wpt_list l) { _direction_stroke = l; }
   CWpt_list& get_direction_stroke()   const { return _direction_stroke; }
   VEXEL    get_direction_vec()const;

   void     set_use_direction_vec(bool v)    {_use_direction_vec = v;}
   bool     get_use_direction_vec()    const { return _use_direction_vec;}

   void     set_use_weighted_ls(bool v)    {_use_weighted_ls = v;}
   bool     get_use_weighted_ls()    const { return _use_weighted_ls;}

   void     set_use_visibility_test(bool v)    {_use_visibility_test = v;}
   bool     get_use_visibility_test()    const { return _use_visibility_test;}

   PIXEL    sample_origin()     const   { return _o; }
   VEXEL    sample_u_vec()      const   { return (_u_o - _o); }
   VEXEL    sample_v_vec()      const   { return (_v_o - _o); }
   double   sample_scale()      const   { return _scale; }
   const vector<DynamicSample>&  get_samples() const{ return _old_samples; }

   double lod_t() const { return _lod_t; }
   double target_scale() const { return _target_scale; } // XXX - can this be removed?
   double timed_lod_hi() const { return _timed_lod_hi; }
   double timed_lod_lo() const { return _timed_lod_lo; }
   VEXEL  lod_u() const { return _lod_u; }
   VEXEL  lod_v() const { return _lod_v; }

   void  set_use_timed_lod_transitions(bool use) { _use_timed_lod_transitions = use; }
   bool  get_use_timed_lod_transitions() { return _use_timed_lod_transitions; }
   void  update_dynamic_samples(const VisibilityTest& vis=VisibilityTest());  
   void  reset_dynamic_samples();
   VEXEL get_z_dynamic_samples(const vector<DynamicSample>& old_samples);

   void     create_dynamic_samples(const VisibilityTest& vis=VisibilityTest());  

   //******** FOCUS ********
   /*! Focus:
    *    Typically the Patch that the user has been lately
    *    interacting with in some way.
    */
   static void     set_focus(Patch* p, bool use_control=true);
   static Patch*   focus()             { return _focus; }
   static bool     is_focus(Patch* p)  { return _focus == p;}

  
   //@}

   //******** PROTECTED ********
 protected:
   
   BMESH*            _mesh;             //!< owner
   
   Bface_list        _faces;            //!< unordered faces
   ARRAY<TriStrip*>  _tri_strips;       //!< faces organized into tri strips
   
   EdgeStrip         _sils;             //!< silhouette edges (as strip)
   ZcrossPath        _zx_sils;          //!< zero_crossing sils
   EdgeStrip*        _creases;          //!< crease edges (maybe derived type)
   EdgeStrip*        _borders;          //!< border edges (maybe derived type)
   ARRAY<EdgeStrip*> _edge_strips;      //!< edges to be rendered as line strips
   ARRAY<VertStrip*> _vert_strips;      //!< vertices to be rendered as points
   EdgeStrip*        _boundaries;      

   GTexture_list     _textures;         //!< list of textures
   int               _cur_tex_i;        //!< index of current texture
   int               _non_tex_i;
   
   GTexture*         _prev_tex;         //!< last GTexture used
   
   ARRAY<uint>       _pixels;           //!< pixels on patch
   uint              _stamp;            //!< version number
   uint              _mesh_version;     //!< last recorded mesh version
   bool              _tri_strips_dirty; //!< used to rebuild _tri_strips
   
   str_ptr           _name;             //!< name
   str_ptr           _texture_file;     //!< name of texture map image file
   
   TexCoordGen*      _tex_coord_gen;    //!< for generating texture coordinates
   PatchData*        _data;             //!< optional custom data
   
   int               _stencil_id;
   static int        _next_stencil_id;
   
   //! \name Dynamic Samples Variables
   //@{
   int              _sps_height;
   double           _sps_min_dist;
   double           _sps_regularity;
   double           _sample_spacing;

   vector<DynamicSample>  _old_samples;  //!< old sample positions (in pixel space)  
   
   PIXEL             _old_center;   //!< sample's center in the last frame
   PIXEL             _new_center;   //!< sample's center in the current frame
   VEXEL             _z;            //!< non-accumulated z
   double            _scale;        //!< total accumulated scale...
   PIXEL             _patch_center; //!< XXX testing...
   
   //! These are used when growing new faces, chaced values of the first quad, so that 
   //! given a new UV point can figure out new location of the points
   PIXEL             _o;
   PIXEL             _u_o;        
   PIXEL             _v_o;
   uint              _dynamic_stamp;
   bool              _do_dynamic_stuff;
   bool              _do_lod;
   bool              _do_rotation;
   double            _lod_t; // interpolation parameter between 2 LOD scales
   VEXEL             _lod_u; // u vector to use when doing LOD
   VEXEL             _lod_v; // v vector to use when doing LOD

   // Timed lod transitions stuff
   double            _target_scale;
   double            _down_lim;
   double            _up_lim;
   double            _transition_duration;
   double            _transition_start_time;
   double            _timed_lod_hi;
   double            _timed_lod_lo;
   bool              _use_timed_lod_transitions;
   bool              _transition_has_started;
   bool              _saw_periodic_sync;

   // the stroke stuck on the surface used to determine direction:
   Wpt_list          _direction_stroke;     

   // should the derection actually be used?
   bool              _use_direction_vec;    

   // use weighted least squares?
   bool              _use_weighted_ls;

   // do visibility test?
   bool              _use_visibility_test;

   // samples become invalid if the patch is redefined
   // (triangles added/removed):
   bool              _d2d_samples_valid;

   static Patch*     _focus;
   //@}

   //******** MANAGERS ********
   Patch(BMESH* = 0);
   friend class BMESH;

   //! \name Serialization Methods
   //@{
      
   static TAGlist      *_patch_tags;
   COLOR   &color_    () { return _color;}
   str_ptr &name_     () { return _name;}
   int     &cur_tex_i_() { return _cur_tex_i;}

   // D2D I/O
   int&     sps_height()       { return _sps_height;   }
   double&  sps_min_dist()     { return _sps_min_dist; }
   double&  sps_regularity()   { return _sps_regularity; }
   bool&    do_dynamic_stuff() { return _do_dynamic_stuff; }
   bool&    do_lod()           { return _do_lod; }
   bool&    do_rotation()      { return _do_rotation; }
   bool&    use_timed_lod_transitions()  { return _use_timed_lod_transitions; }
   bool&    use_direction_vec() { return _use_direction_vec; } 
   Wpt_list& direction_stroke() { return _direction_stroke; }
   bool&    use_weighted_ls() { return _use_weighted_ls; } 
   bool&    use_visibility_test() { return _use_visibility_test; } 

   virtual void get_faces(TAGformat &d);
   virtual void put_faces(TAGformat &d) const;
   virtual void get_color  (TAGformat &d)       { COLOR c; *d >> c; set_color(c); }
   virtual void put_color  (TAGformat &d) const {
      if (has_color())
         d.id() << color();
   }
   virtual void get_texture (TAGformat &d);
   virtual void put_textures(TAGformat &d) const;
   virtual void recompute();
   void start_timed_lod_transition();
      
   //@}
   
   //! \brief Fill the given edge strip with edges of a given type that
   //! belong to this patch.
   EdgeStrip* build_edge_strip(EdgeStrip* strip, CSimplexFilter& filter);
};

/*****************************************************************
 * inline convenience methods
 *****************************************************************/

//! \brief Returns the Patch of a Bface.
//! \relates Patch
//! \relatesalso Bface
inline Patch* 
get_patch(CBface* f) 
{
   return f ? f->patch() : 0; 
}

//! \brief Returns the Patch of a Bsimplex.
//! \note If you know it's actually a Bface, get_patch(CBface*) is faster.
//! \relates Patch
//! \relatesalso Bsimplex
inline Patch*
get_patch(CBsimplex* s)
{
   // For an edge, prefer the patch of the front-facing Bface.
   // For a face return its own patch.
   // For a vertex just return any adjacent patch.

   return is_edge(s) ? ((CBedge*)s)->patch() : get_patch(get_bface(s));
}

//! \brief Returns the control patch of a Patch
//! \relates Patch
inline Patch*
get_ctrl_patch(Patch* p)
{
   return p ? p->ctrl_patch() : 0;
}

//! \brief Returns the control patch of an APPEAR
//! \relates Patch
//! \relatesalso APPEAR
inline Patch*
get_ctrl_patch(APPEAR* a)
{
   return get_ctrl_patch(dynamic_cast<Patch*>(a));
}

//! \brief Similar to get_patch(CBface*), but returns the control Patch.
//! \relates Patch
//! \relatesalso Bface
inline Patch*
get_ctrl_patch(CBface* f)
{
   return get_ctrl_patch(get_patch(f));
}

//! \brief Similar to get_patch(CBsimplex*), but returns the control Patch.
//! \relates Patch
//! \relatesalso Bsimplex
inline Patch*
get_ctrl_patch(CBsimplex* s)
{
   return get_ctrl_patch(get_patch(s));
}

/*****************************************************************
 * PatchEdgeFilter:
 *****************************************************************/
class PatchEdgeFilter : public SimplexFilter {
 public:
   PatchEdgeFilter(Patch* p) : _patch(p) {}

   virtual bool accept(CBsimplex* s) const {
      if (!is_edge(s))
         return false;
      Bedge* e = (Bedge*)s;
      return (
         !_patch                      ||
         get_patch(e->f1()) == _patch ||
         get_patch(e->f2()) == _patch
         );
   }

 protected:
   Patch*  _patch;
};

/*****************************************************************
 * PatchFaceFilter:
 *****************************************************************/
class PatchFaceFilter : public SimplexFilter {
 public:
   PatchFaceFilter(Patch* p) : _patch(p) {}

   virtual bool accept(CBsimplex* s) const {
      if (!is_face(s))
         return false;
      return ((Bface*)s)->patch() == _patch;
   }

 protected:
   Patch*  _patch;
};

/*****************************************************************/
/*!
 * \brief List of Patches w/ convenience methods (defined in patch.H)
 */
/*****************************************************************/
class Patch_list : public RIC_array<Patch> {
 public:

   //! \name MANAGERS
   //@{

   Patch_list(int n=0)                : RIC_array<Patch>(n)    {}
   Patch_list(const Patch_list& list) : RIC_array<Patch>(list) {}

   // creates a list of patches, 1 per face
   // result has same number of patches as there are faces.
   // (i.e., may have duplicates):
   Patch_list(CBface_list& faces)     : RIC_array<Patch>(faces.num()) {
      for (int k=0; k<faces.num(); k++) {
         assert(faces[k] && faces[k]->patch());
         add(faces[k]->patch());
      }
   }

   // returns a list of patches taken from  the given faces,
   // with no duplicates:
   static Patch_list get_unique_patches(CBface_list& faces) {
      Patch_list ret;
      for (int k=0; k<faces.num(); k++) {
         assert(faces[k]);
         if (faces[k]->patch())
            ret.add_uniquely(faces[k]->patch());
      }
      return ret;
   }

   //@}

   //! \name Convenience methods
   //@{

   double min_alpha()                   const;

   void delete_all();

   void creases_changed()               const;
   void triangulation_changed()         const;

   void write_stream(ostream& os)       const;

   //@}
};
typedef const Patch_list CPatch_list;

/*****************************************************************
 * Patch_list
 *
 *   Defined in bmesh.H, inlined convenience methods defined here.
 *****************************************************************/
inline void
Patch_list::creases_changed() const
{
   for (int k=0; k<_num; k++)
      _array[k]->creases_changed();
}

inline void
Patch_list::triangulation_changed() const
{
   for (int k=0; k<_num; k++)
      _array[k]->triangulation_changed();
}

inline void
Patch_list::write_stream(ostream& os) const
{
   for (int k=0; k<_num; k++)
      _array[k]->write_stream(os);
}

inline void
Patch_list::delete_all()
{
   while (!empty())
      delete pop();
}

inline double 
Patch_list::min_alpha() const
{
   double ret=1;
   for (int k=0; k<_num; k++)
      ret = min(ret, _array[k]->transp());
   return ret;
}

inline BMESH*
GTexture::mesh() const 
{
   return _patch ? _patch->mesh() : 0; 
}

// Given a Patch, return a particular type of texture (class G)
// to render the Patch. Create the texture if needed:
template <class G>
inline G*
get_tex(Patch* p)
{
   return p ? G::upcast(p->get_tex(G::static_name())) : 0;
}


// Stores Blend values for shaders to use if multiple patches need to
// be blended
class PatchBlend : public SimplexData {
 public:
   //******** MANAGERS ********
   // using key, record data on v
   PatchBlend(uint key, Bvert* v, double blend) :
      SimplexData(key, v),
      _blend(blend) {}

   //******** RUN TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("PatchBlend", PatchBlend*, SimplexData, CSimplexData*);

   //******** ACCESSORS ********
   Bvert* vertex()            const { return (Bvert*)simplex(); }
   double blend()             const { return _blend; }
   void   set_blend(double v)       { _blend = v; }

   //******** LOOKUPS ********
   // get the PatchBlend on v
   static PatchBlend* lookup(uint key, Bvert* v) {
      return v ? upcast(v->find_data(key)) : 0;
   }   
   static void lookup_create(uint key, Bvert* v, double b) {
      PatchBlend* ret = lookup(key,v);
      if (!ret) {
         ret = new PatchBlend(key,v, b);
      }
   }

   // If one already exists overwrites the values to given one if
   // already exists
   static void lookup_create_overwrite(uint key, Bvert* v, double b) {
      PatchBlend* ret = lookup(key,v);
      if (!ret) {
         ret = new PatchBlend(key,v, b);
      }
      ret->_blend = b;      
   }  
 protected:
   double _blend;
};

#endif // PATCH_H_IS_INCLUDED

// end of file patch.H
