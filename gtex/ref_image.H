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
 * ref_image.H
 *****************************************************************/
#ifndef REF_IMAGE_H_IS_INCLUDED
#define REF_IMAGE_H_IS_INCLUDED

#include "std/support.H"
#include "glew/glew.H"

#include "disp/colors.H"         // Color namespace and utility functions
#include "disp/ref_img_client.H" 
#include "disp/rgba.H"           // RGBA conversions
#include "geom/image.H"          // Image
#include "geom/texturegl.H"      // tex mem ref img
#include "geom/winsys.H"         // bits per component
#include "mesh/base_ref_image.H"
#include "mesh/patch.H"
#include "mesh/simplex_filter.H"
#include "mlib/point2i.H"

#include "gtex/util.H"

/**********************************************************************
 * CoordSystem2d:
 *
 *   represents a rectangle. can access locations with PIXEL,
 *   NDC, and uint coordinates. the uint form encodes (x,y)
 *   integer coordinates in PIXELS into a single 32-bit integer.
 **********************************************************************/
class CoordSystem2d {
 public:
   //******** MANAGERS ********
   CoordSystem2d() :
      _width(0), _height(0), _max(0),
      _half_width(0), _half_height(0),
      _half_min_dim(0) {}

   virtual ~CoordSystem2d() {}
  
   //******** ACCESSORS ********
   uint  width()        const   { return _width; }
   uint height()        const   { return _height;}
   uint    max()        const   { return _max;}

   virtual bool resize(uint new_w, uint new_h, CNDCvec& v=VEXEL(0.5,0.5)) {
      _width        = new_w;
      _height       = new_h;
      _max          = _width * _height;
      _half_width   = _width/2.0;
      _half_height  = _height/2.0;
      _half_min_dim = min(_half_width,_half_height);
      _ndc_offset   = v;
      return true;
   }

   CNDCvec& ndc_offset()                { return _ndc_offset; }
  
   //******** RANGE FUNCTIONS ********
   bool uint_in_range(uint id) const { return (id<_max); }
   bool pix_in_range(Cpoint2i &pix) const {
      return (pix[0]>=0 && pix[1]>=0 &&
              (uint)pix[0]<_width && (uint)pix[1]<_height);
   }
  
   //******** CONVERSION FUNCTIONS ********

   // ******** PIX <--> UINT ********
   uint pix_to_uint(Cpoint2i& pix) const {
      return clamp(pix[1],0,(int)_height-1)*_width +
         clamp(pix[0],0,(int)_width-1);
   }
   Point2i uint_to_pix(uint id) const {
      return Point2i(id % _width, id / _width);
   }

   // ******** PIX <--> NDC ********
   NDCpt pix_to_ndc(Cpoint2i &pix) const {
      return NDCpt((pix[0]-_half_width)/_half_min_dim,
                   (pix[1]-_half_height)/_half_min_dim);
   }
   Point2i ndc_to_pix(CNDCpt &ndc) const {
      return Point2i(int(ndc[0]*_half_min_dim + _half_width),
                     int(ndc[1]*_half_min_dim + _half_height));
   }

   // ******** NDC <--> UINT ********
   NDCpt uint_to_ndc(uint id) const {
      return NDCpt((id%_width - _half_width)/_half_min_dim,
                   (id/_width - _half_height)/_half_min_dim) + _ndc_offset;
   }
   uint ndc_to_uint(CNDCpt& ndc) const {
      const int x = (int) (ndc[0]*_half_min_dim + _half_width);
      const int y = (int) (ndc[1]*_half_min_dim + _half_height);
      return clamp(y,0,(int)_height-1)*_width + clamp(x,0,(int)_width-1);
   }

 protected:
   //******** MEMBER DATA ********
   uint           _width;               // rectangle width
   uint           _height;              // rectangle height
   uint           _max;                 // width * height
   double         _half_width;          // width/2
   double         _half_height;         // height/2
   double         _half_min_dim;        // smaller of two half dims

   NDCvec         _ndc_offset;          // used in pix to ndc conversions
};

/**********************************************************************
 * Array2d:
 *
 *   A 2D array of values (base class for an image class),
 *   using coordinate info from CoordSystem2d.
 **********************************************************************/
template <class T>
class Array2d : public CoordSystem2d {
 public:
   //******** MANAGERS ********
   Array2d() : CoordSystem2d(), _values(0) {}
   Array2d(const Array2d<T>& arr) : _values(0) {
      resize(arr._width, arr._height);
      for (uint i=0; i<_max; i++)
         _values[i] = arr._values[i];
   }

   //******** SETTING UP ********
   virtual ~Array2d() { delete[] _values; }

   void clear(int clear_val=0) {
      memset(_values, clear_val, _max*sizeof(T));
   }

   virtual bool resize(uint new_w, uint new_h, CNDCvec& v=VEXEL(0.5,0.5)) {
      if (new_w == _width && new_h == _height)
         return false;
      CoordSystem2d::resize(new_w, new_h, v);
      allocate();
      return true;
   }

   //******** ACCESSORS ********
   T& val(uint id)        const   { return _values[id];}
   T& val(CNDCpt& ndc)    const   { return val(ndc_to_uint(ndc));  }
   T& val(Cpoint2i& pix)  const   { return val(pix_to_uint(pix));  }
   T& val(int x, int y)   const   { return val(Point2i(x,y)); } 

 protected: 
   T*   _values;        // 1D array representing 2D grid of values
  
   void allocate() {
      delete [] _values;
      _values = 0;
      if (_max>0) {
         _values = new T[_max];
         assert(_values);
      }
   }
};

/**********************************************************************
 * RefImage: 
 **********************************************************************/
class RefImage : public Array2d<GLuint> {
 public:
   //******** MANAGERS ********

   RefImage(CVIEWptr& v);

   //******** STATICS ********

   // runs a poll on all scene objects to find out what ref
   // images are needed, then updates just the needed ones:
   static void update_all(VIEWptr v);

   // when VIEW is resized, call this to resize ref images accordingly.  
   // XXX - ref images should observe changes in window size themselves
   static void view_resize(VIEWptr v);

   //******** ACCESSORS ********
   CVIEWptr& view() const { return _view; }

   // the texture (if using texture memory):
   TEXTUREglptr get_texture() const { return _texture; }

   //******** VIRTUAL METHODS ********

   // update the image
   virtual void update() = 0;

   // for debugging: string ID for this class:
   virtual str_ptr class_id() const = 0;

   //******** IMAGE COPYING ********
   // return an Image with same dimensions
   // as this, but formatted for RGB:
   int copy_rgb(Image& img) const;

   // copying frame buffer <---> main memory
   void copy_to_ram();    // copy frame buffer pixels to main memory
   void draw_img() const; // draw image in main memory to frame buffer

   // copying frame buffer <---> texture memory:
   void copy_to_tex();    // copy frame buffer pixels to texture memory
   void draw_tex();       // draw image in texture memory to frame buffer

   //******** SETTING PIXELS ********
   // set every pixel to the given rgba color:
   void fill(uchar r, uchar g, uchar b, uchar a=255U) {
      fill(build_rgba(r,g,b,a));
   }
   void fill(uint fill_color);

   // set the given pixel to the given color:
   void set(Cpoint2i& pix, uint rgba_color) { val(pix) = rgba_color; }
   void set(Cpoint2i& pix, uchar r, uchar g, uchar b, uchar a=255U) {
      set(pix, build_rgba(r,g,b,a));
   }
   void set(int x, int y, uint rgba_color) {
      set(Cpoint2i(x,y), rgba_color);
   }
   void set(int x, int y, uchar r, uchar g, uchar b, uchar a=255U) {
      set(Cpoint2i(x,y), build_rgba(r, g, b, a));
   }

   // set the color at location x,y to the given color/alpha rgba value:
   void set(int x, int y, CCOLOR& c, double alpha = 1) {
      set(x, y, Color::color_to_rgba(c, alpha));
   }
   // blend the given color into the image using opacity alpha:
   void blend(int x, int y, CCOLOR& c, double alpha) {
      set(x, y, interp(color(x, y), c, alpha), 1);
   }

   //******** CONVENIENT COLOR ACCESSORS ********

   // return the color at image location x,y (ignoring alpha):
   COLOR color(int x, int y) const {
      return Color::rgba_to_color(val(x,y));
   }
   // return the color (ignoring alpha) at given NDC location:
   COLOR color(CNDCpt& ndc) const {
      return Color::rgba_to_color(val(ndc));
   }

   // return the given component as a uint in range [0,255]:
   uint red(uint id)            const      { return rgba_to_r(val(id));  }
   uint red(CNDCpt& ndc)        const      { return rgba_to_r(val(ndc)); }
   uint red(Cpoint2i& pix)      const      { return rgba_to_r(val(pix)); }
   uint red(int x, int y)       const      { return rgba_to_r(val(x,y)); }

   uint green(uint id)          const      { return rgba_to_g(val(id));  }
   uint green(CNDCpt& ndc)      const      { return rgba_to_g(val(ndc)); }
   uint green(Cpoint2i& pix)    const      { return rgba_to_g(val(pix)); }
   uint green(int x, int y)     const      { return rgba_to_g(val(x,y)); }

   uint blue(uint id)           const      { return rgba_to_b(val(id));  }
   uint blue(CNDCpt& ndc)       const      { return rgba_to_b(val(ndc)); }
   uint blue(Cpoint2i& pix)     const      { return rgba_to_b(val(pix)); }
   uint blue(int x, int y)      const      { return rgba_to_b(val(x,y)); }

   uint alpha(uint id)          const      { return rgba_to_a(val(id));  }
   uint alpha(CNDCpt& ndc)      const      { return rgba_to_a(val(ndc)); }
   uint alpha(Cpoint2i& pix)    const      { return rgba_to_a(val(pix)); }
   uint alpha(int x, int y)     const      { return rgba_to_a(val(x,y)); }

   // return the luminance (ignoring alpha) as a uint in range
   // [0,255], where 0 is black and 255 is white:
   uint grey(uint id)       const       { return rgba_to_grey(val(id));  }
   uint grey(CNDCpt& ndc)   const       { return rgba_to_grey(val(ndc)); }
   uint grey(Cpoint2i& pix) const       { return rgba_to_grey(val(pix)); }
   uint grey(int x, int y)  const       { return rgba_to_grey(val(x,y)); }

   // return the luminance (ignoring alpha) as a double in range
   // [0,1], where 0 is black and 1 is white:
   double grey_d(uint id)       const   { return rgba_to_grey_d(val(id));  }
   double grey_d(CNDCpt& ndc)   const   { return rgba_to_grey_d(val(ndc)); }
   double grey_d(Cpoint2i& pix) const   { return rgba_to_grey_d(val(pix)); }
   double grey_d(int x, int y)  const   { return rgba_to_grey_d(val(x,y)); }

   //******** FILE I/O ********
   int read_file (char* file);
   int write_file(char* file);

   //******** SEARCH FUNCTIONS ********

   // Return true if the given value v is found in an (n x n) square
   // region around the given center location, where n = 2*rad + 1.
   // E.g., if rad == 1 the search is within a 3 x 3 region:
   bool find_val_in_box(uint v, Cpoint2i& center, uint rad=1) const;
   bool find_val_in_box(uint v, CNDCpt&   center, uint rad=1) const {
      return find_val_in_box(v, ndc_to_pix(center), rad);
   }

   bool find_val_in_box(
      uint v, uint mask, Cpoint2i& center, uint rad=1, int nbr=256
      ) const;
   bool find_val_in_box(
      uint v, uint mask, CNDCpt&   center, uint rad=1, int nbr=256
      ) const {
      return find_val_in_box(v, mask, ndc_to_pix(center), rad, nbr);
   }

  //******** CoordSystem2d VIRTUAL METHODS ********

   // resize image and texture too:
   virtual bool resize(uint new_w, uint new_h, CNDCvec& v=VEXEL(0.5,0.5));


 protected:
   VIEWptr       _view;            // associated VIEW
   bool          _update_main_mem; // need image in main memory?
   bool          _update_tex_mem;  // need image in texture memory?
   TEXTUREglptr  _texture;         // texture (if using texture memory)
   
   // list of images that need to be updated
   // for the upcoming frame:
   static ARRAY<RefImage*> _update_list;

   //******** UTILITIES ********
  
 

   // tell if the image needs updating:
   bool need_update() const { return _update_main_mem || _update_tex_mem; }
   void check_resize();
   void schedule();
};
typedef const RefImage CRefImage;

/**********************************************************************
 * ColorRefImage:
 **********************************************************************/
class ColorRefImage : public RefImage {
 public:
   //******** MANAGERS ********

   // no public constructor -- use lookup:

   // look up color ref image number i
   // associated with given view:
   static ColorRefImage* lookup(int i, CVIEWptr& v = VIEW::peek());

   // schedule an update for color ref image number i,
   // specifying whether to save image in main memory
   // or texture memory:
   static void schedule_update(
      int i,
      bool main_mem,
      bool tex_mem,
      CVIEWptr& v = VIEW::peek()
      );

   // called in RefImage::update_all():
   static void update_images(VIEWptr v);
   static void resize_all(VIEWptr v, int w, int h, CNDCvec& =VEXEL(0.5,0.5));

   //******** ACCESSORS ********

   // lookup the ColorRefImage and return a pointer to its internal texture:
   static TEXTUREglptr lookup_texture(int i, CVIEWptr& v = VIEW::peek()) {
      ColorRefImage* c = lookup(i, v);
      return c ? c->get_texture() : 0;
   }
   
   // lookup the texture unit used by the current ColorRefImage;
   // "raw" means it returns 0 for GL_TEXTURE0, etc.
   static GLenum lookup_raw_tex_unit(int i, CVIEWptr& v = VIEW::peek()) {
      TEXTUREglptr tex = lookup_texture(i, v);
      return tex ? tex->get_raw_unit() : TexUnit::REF_IMG;
   }

   // "cooked" version: returns GL_TEXTURE0 for GL_TEXTURE0, etc.
   static GLenum lookup_tex_unit(int i, CVIEWptr& v = VIEW::peek()) {
      return lookup_raw_tex_unit(i, v) + GL_TEXTURE0;
   }

   static void activate_tex_unit(int i, CVIEWptr& v = VIEW::peek()) {
      TEXTUREglptr tex = lookup_texture(i, v);
      assert(tex);
      tex->apply_texture();     // GL_ENABLE
   }
 

   // base class RefImage also supports copying:
   //   frame buffer <---> main memory

   //******** RefImage VIRTUAL METHODS ********

   // update the image
   virtual void update();

   // for debugging: string ID for this class:
   virtual str_ptr class_id() const {
      return str_ptr("ColorRefImage") + str_ptr(_index);
   }

 protected:
   //******** MEMBER DATA ********
   int           _index;                // index in main list
   static  HASH  _hash;

   //******** MANAGERS ********

   ColorRefImage(CVIEWptr& v, int i);

   //******** UTILITIES ********

   // get list of color ref images for given view:
   typedef ARRAY<ColorRefImage*> ColorRefImage_list;
   static ColorRefImage_list* get_list(CVIEWptr& v);


   // draw objects, handling opaque and transparent ones:
   void draw_objects(CGELlist&) const;
};

/**********************************************************************
 * IDRefImage:
 *
 *      Also known as the "ID reference image."
 *
 *      Implements an "item buffer," where each pixel encodes an
 *      object ID. Specifically, "objects" in this case are mesh
 *      faces, edges, or vertices (usually just faces). Given a pixel
 *      location, the item buffer can be used to perform O(1) picking
 *      (not counting the cost of preparing the item buffer). This
 *      technique has been described in various papers; e.g.:
 *
 *        Rodney J. Recker, David W. George, Donald P. Greenberg.
 *        Acceleration Techniques for Progressive Refinement
 *        Radiosity, 1990 Symposium on Interactive 3D Graphics, 24
 *        (2), pp. 59-66 (March 1990). Edited by Rich Riesenfeld and
 *        Carlo Séquin. ISBN 0-89791-351-5.
 *
 *      This is also described in Lee Markosian's Ph.D. thesis,
 *      "Art-based Modeling and Rendering for Computer Graphics."
 *      Brown University, May 2000.
 **********************************************************************/
class IDRefImage : public RefImage {
 public:
   //******** MANAGERS ********
   // no public constructor:
   static IDRefImage* lookup(CVIEWptr& v = VIEW::peek());

   // request image to be updated in next frame.
   static void schedule_update(
      CVIEWptr& v = VIEW::peek(),
      bool pixels_to_patches=false,
      bool main_mem = true,
      bool tex_mem = false
      );

   static TEXTUREglptr lookup_texture(CVIEWptr& v = VIEW::peek()) {
      IDRefImage* c = lookup(v);
      return c ? c->get_texture() : 0;
   }

   // lookup the texture unit used by the current IDRefImage;
   // "raw" means it returns 0 for GL_TEXTURE0, etc.
   static GLenum lookup_raw_tex_unit( CVIEWptr& v = VIEW::peek()) {
      TEXTUREglptr tex = lookup_texture(v);
      return tex ? tex->get_raw_unit() : TexUnit::REF_IMG;
   }

   // "cooked" version: returns GL_TEXTURE0 for GL_TEXTURE0, etc.
   static GLenum lookup_tex_unit( CVIEWptr& v = VIEW::peek()) {
      return lookup_raw_tex_unit( v) + GL_TEXTURE0;
   }

   static void activate_tex_unit( CVIEWptr& v = VIEW::peek()) {
      TEXTUREglptr tex = lookup_texture( v);
      assert(tex);
      tex->apply_texture();     // GL_ENABLE
   }

   //******** RefImage VIRTUAL METHODS ********

   // update the image
   virtual void update();

   // for debugging: string ID for this class:
   virtual str_ptr class_id() const { return str_ptr("IDRefImage"); }

   //******** RGBA <---> KEY CONVERSIONS ********
   // Convert Bsimplex key value to 32-bit RGBA that can be given to
   // OpenGL, then read back from the frame buffer to reliably yield
   // the original key value:

   // At compilation time you can define REF_IMG_32_BIT if you know
   // you will run your application on a machine (and with a visual)
   // that supports 32-bit RGBA. In that case these are no-ops and we
   // can save a function call if the following are inlined. To define
   // REF_IMG_32_BIT add a line saying REF_IMG_32_BIT=yes in your
   // Makefile.local. 

   static uint key_to_rgba(uint key)    { 
      return key; 
#ifdef REF_IMG_32_BIT
      return key; 
#else
      return key_to_rgba2(key);      
#endif
   }
   static uint rgba_to_key(uint rgba)   { 
      return rgba;
#ifdef REF_IMG_32_BIT
      return rgba;
#else      
      return rgba_to_key2(rgba);
#endif
   }
   static uint key_to_rgba2(uint key);      
   static uint rgba_to_key2(uint rgba);


   //******** LOOKUP -- CONVENIENCE METHODS ********
   Bsimplex* simplex(uint id)       const {
      return Bsimplex::lookup(rgba_to_key(val(id)));
   }
   Bsimplex* simplex(CNDCpt& ndc)   const { return simplex(ndc_to_uint(ndc)); }
   Bsimplex* simplex(Cpoint2i& pix) const { return simplex(pix_to_uint(pix)); }

   Bvert* vert(uint id) const {
      Bsimplex* sim = simplex(id);
      return is_vert(sim) ? (Bvert*)sim : 0;
   }
   Bedge* edge(uint id) const {
      Bsimplex* sim = simplex(id);
      return is_edge(sim) ? (Bedge*)sim : 0;
   }
   Bface* face(uint id) const {
      Bsimplex* sim = simplex(id);
      return is_face(sim) ? (Bface*)sim : 0;
   }
   Bvert* vert(CNDCpt& ndc)         const { return vert(ndc_to_uint(ndc)); }
   Bedge* edge(CNDCpt& ndc)         const { return edge(ndc_to_uint(ndc)); }
   Bface* face(CNDCpt& ndc)         const { return face(ndc_to_uint(ndc)); }
   Patch* patch(CNDCpt& ndc)        const { return patch(simplex(ndc)); }
   Patch* face_patch(CNDCpt& ndc)   const { return face_patch(simplex(ndc));}

   Bvert* vert(Cpoint2i& pix)       const { return vert(pix_to_uint(pix)); }
   Bedge* edge(Cpoint2i& pix)       const { return edge(pix_to_uint(pix)); }
   Bface* face(Cpoint2i& pix)       const { return face(pix_to_uint(pix)); }

   // ret = approximate world-space point found by ray test,
   // returns false if the ray test failed:
   bool approx_wpt(CNDCpt& ndc, Wpt& ret) const;

   //******** INTERSECTION ********

   // Do exact intersection. If a vert or an edge is hit exactly,
   // return it as the simplex.
   Bsimplex* intersect_sim(CNDCpt& ndc, Wpt& obj_pt) const {
      Bsimplex* sim = simplex(ndc);
      if (sim && sim->get_face())
         return sim->get_face()->find_intersect_sim(ndc, obj_pt);
      else return 0;
   }

   // Find the simplex that is intersected exactly (if any), but
   // return a face (either the sim itself or one that contains it):
   Bface* intersect(CNDCpt& ndc, Wpt& obj_pt) const {
      Bsimplex* sim = intersect_sim(ndc, obj_pt);
      return sim ? sim->get_face() : 0;
   }

   // Exact intersection -- return a face:
   Bface* intersect(CNDCpt& ndc) const {
      static Wpt foo;
      return intersect(ndc, foo);
   }

   // Search in a region defined by a screen point and radius.
   // Return true if some pixel corresponds to a simplex that
   // matches the search criterion. Also return (in 'hit' parameter)
   // the closest matching pixel of the reference image. This is
   // similar to find_near_simplex (below), but can also be used
   // when the search criterion accepts a null simplex:
   bool search(
      CNDCpt&           center,         // search center
      double            screen_pix_rad, // search radius in screen pixels
      CSimplexFilter&   filt,           // filter giving search criterion
      Point2i&          hit             // return value - hit point
      );

   // Similar to search() above, but returns the matching simplex
   // itself, if any. Returns 0 on failure.
   Bsimplex* find_near_simplex(
      CNDCpt& center,
      double screen_pix_rad = 1.0,
      CSimplexFilter& filt = SimplexFilter());

   // return true if the given pixel location records the ID of an
   // edge, and the edge is in the given patch, and it's a silhouette
   // edge.
   bool is_patch_sil_edge(Cpoint2i& pix, const Patch* patch) const {
      static Bedge* temp = 0;
      if (pix_in_range(pix) && (temp = edge(pix)) 
          && (temp->patch() == patch) && (temp->is_sil())) 
         return true;
      else return false;
   }
   bool is_patch_sil_edge(CNDCpt& pix, const Patch* patch) const {
      return is_patch_sil_edge(ndc_to_pix(pix), patch);
   }

   Bedge* find_neighbor(CNDCpt& p, Bedge* current, int radius = 1) const;
   Bedge_list find_all_neighbors(CNDCpt& p, Patch* patch, int radius=1) const;
   Bedge_list find_all_neighbors(Cpoint2i& p, Patch* patch, int radius = 1) const;
   bool is_simplex_near(CNDCpt& p, const Bsimplex* simp, int radius = 1) const;
   bool is_patch_sil_edge_near(CNDCpt& ndc, const Patch* patch, int radius = 1) const;

   bool is_face_visible( CNDCpt& ndc, const Bface* bf ) const {
      Bface* f = face(ndc);
      if (!f) return false;
      if (f==bf) return true;
      static Wpt obj_pt;
      Bsimplex* sim = f->find_intersect_sim(ndc, obj_pt);
      return sim ? sim->on_face(bf) : false;
   }
   bool near_pix(Cpoint2i& pix, Point2i& ret, Patch* patch) {
      Point2i p;
      if (pix_in_range(pix) && face_patch(simplex(pix)) == patch)
         ret = pix;
      else if (pix_in_range(p = pix + Vec2i(-1, 0)) &&
               face_patch(simplex(p)) == patch)
         ret = p;
      else if (pix_in_range(p = pix + Vec2i( 1, 0)) &&
               face_patch(simplex(p)) == patch)
         ret = p;
      else if (pix_in_range(p = pix + Vec2i(-1,-1)) &&
               face_patch(simplex(p)) == patch)
         ret = p;
      else if (pix_in_range(p = pix + Vec2i(-1, 0)) &&
               face_patch(simplex(p)) == patch)
         ret = p;
      else if (pix_in_range(p = pix + Vec2i(-1, 1)) &&
               face_patch(simplex(p)) == patch)
         ret = p;
      else if (pix_in_range(p = pix + Vec2i( 1,-1)) &&
               face_patch(simplex(p)) == patch)
         ret = p;
      else if (pix_in_range(p = pix + Vec2i( 1, 0)) &&
               face_patch(simplex(p)) == patch)
         ret = p;
      else if (pix_in_range(p = pix + Vec2i( 1, 1)) &&
               face_patch(simplex(p)) == patch)
         ret = p;
      else
         return 0;
      return 1;
   }

   // global access to the one true ID ref image
   static void set_instance(IDRefImage * id_ref) { _instance = id_ref;      }
   static void set_instance(CVIEWptr & v)        { set_instance(lookup(v)); }
   static IDRefImage* instance()                 { return _instance;        }

   static void setup_bits(CVIEWptr&);

 protected:
   //******** MEMBER DATA ********
   bool _need_update;       // tells if update is needed
   bool _pixels_to_patches; // should pixels be distributed to patches?

   // used to print out framebuffer info:
   static uint _red_bits;
   static uint _green_bits;
   static uint _blue_bits;
   static uint _alpha_bits;
   static bool _nonstandard_bits;

   // associates an IDRefImage with a VIEW:
   static HASH _hash; 

   // global instance 
   // XXX - bogus, should remove:
   static IDRefImage* _instance;

   //******** PROTECTED METHODS ********
   IDRefImage(CVIEWptr& v);

   Patch* patch(Bsimplex* sim) const {
      return (is_face(sim) ? ((Bface*)sim)->patch() :
              is_edge(sim) ? ((Bedge*)sim)->patch() :
              0);
   }

   Patch* face_patch(Bsimplex* sim) const {
      return (is_face(sim) ? ((Bface*)sim)->patch() : 0); 
   }
   //******** UTILITIES ********

   virtual void draw_objects(GELlist&) const;
};

class VisRefImageFactory;
/**********************************************************************
 * VisRefImage:
 *
 *   Visibility reference image
 **********************************************************************/
MAKE_PTR_SUBC(VisRefImage, FRAMEobs);
class VisRefImage : public    IDRefImage,
                    protected BMESHobs,
                    protected CAMobs,
                    protected DISPobs,
                    protected EXISTobs,
                    public    FRAMEobs,
                    protected XFORMobs,
                    public    BaseVisRefImage {
 public:
   //******** MANAGERS ********
   virtual ~VisRefImage() { unobserve(); }

   //******** UPDATING ********
   bool need_update();
   void force_dirty() { _need_update = 1; }

   //******** RefImage VIRTUAL METHODS ********

   // for debugging: string ID for this class:
   virtual str_ptr class_id() const { return str_ptr("VisRefImage"); }

   //******** STATICS ********
   static void init();
   static VisRefImage* lookup(CVIEWptr& view = VIEW::peek()) {
      if (!view)
         return 0;
      if (!BaseVisRefImage::_factory)
         init();
      return (VisRefImage*)BaseVisRefImage::lookup(view);
   }

   //******** PICKING METHODS ********
   static NDCpt get_cursor() {
      if (!DEVice_2d::last) {
         err_msg( "VisRefImage::get_cursor: error: Device_2d::last is nil");
         return NDCpt();
      }
      return NDCpt(DEVice_2d::last->cur());
   }

   static Bsimplex* get_simplex(CNDCpt& cur = get_cursor(),
                                double screen_rad=1,
                                CSimplexFilter& filt = SimplexFilter()) {
      VisRefImage* vis = lookup(VIEW::peek());
      if (vis) {
         vis->update();
         return vis->find_near_simplex(cur, screen_rad, filt);
      }
      return (Bsimplex*)0;
   }

   // Return the nearest Bface within the given screen region,
   // with a preference for front-facing Bfaces:
   static Bface* get_face(CNDCpt& cur = get_cursor(), double screen_rad=1);

   // Call get_face() on each screen point in sequence:
   static Bface_list get_faces(const PIXEL_list& pix, double screen_rad=1);

   // Convenience:
   static Bface* Intersect(CNDCpt& ndc, Wpt& obj_pt) {
      VisRefImage* vis = lookup(VIEW::peek());
      if (vis) {
         vis->update();
         return vis->intersect(ndc, obj_pt);
      }
      return (Bface*)0;
   }
   static Bface* Intersect(CNDCpt& ndc) {
      Wpt foo;
      return Intersect(ndc, foo);
   }

   // 1. Search the ID image to find the nearest Bface
   //    within a radius of the given NDC screen location.
   // 2. Convert the screen location to a barycentric
   //    coordinate on the Bface.
   // 3. Return the Bface and barycentric coordinate.
   //    (Returns NULL on failure.)
   static Bface* get_face_bc(
      Wvec& bc, CNDCpt& ndc = get_cursor(), double rad=1
      );
   
   // Same as above, but also convert the Bface / barycentric
   // coordinate to an equivalent pair at the given mesh subdivision
   // level.
   static Bface* get_sub_face(
      int level, Wvec& bc, CNDCpt& ndc = get_cursor(), double rad=1
      );

   // Same as above, but also convert the Bface / barycentric
   // coordinate to an equivalent pair at the mesh edit level.
   static Bface* get_edit_face(
      Wvec& bc, CNDCpt& ndc = get_cursor(), double rad=1
      );

   static Bface* get_edit_face(CNDCpt& ndc = get_cursor(), double rad=1) {
      Wvec bc; return get_edit_face(bc, ndc, rad);
   }

   static Bface* get_ctrl_face(CNDCpt& cur = get_cursor(),
                               double screen_rad=1) {
      return ::get_ctrl_face(get_face(cur, screen_rad));
   }

   static Bedge* get_edge(CNDCpt& cur = get_cursor(),
                          double screen_rad=1) {
      return (Bedge*)get_simplex(cur, screen_rad, BedgeFilter());
   }

   static Bvert* get_vert(CNDCpt& cur = get_cursor(),
                          double screen_rad=1) {
      return (Bvert*)get_simplex(cur, screen_rad, BvertFilter());
   }

   static Patch* get_patch(CNDCpt& cur = get_cursor(),
                           double screen_rad=1) {
      // see mesh/patch.H:
      return ::get_patch(get_simplex(cur, screen_rad));
   }

   static Patch* get_ctrl_patch(CNDCpt& cur = get_cursor(),
                                double screen_rad=1) {
      // see mesh/patch.H:
      return ::get_ctrl_patch(get_simplex(cur, screen_rad));
   }

   static BMESH* get_mesh(CNDCpt& cur = get_cursor(),
                          double screen_rad=1) {
      // see mesh/bsimplex.H:
      return ::get_mesh(get_simplex(cur, screen_rad));
   }

   static BMESH* get_ctrl_mesh(CNDCpt& cur = get_cursor(),
                               double screen_rad=1) {
      // see mesh/lmesh.H
      return ::get_ctrl_mesh(get_mesh(cur, screen_rad));
   }

   //******** IDRefImage METHODS ********

   virtual bool resize(uint new_w, uint new_h, CNDCvec& v=VEXEL(0.5,0.5)) {
      if (!IDRefImage::resize(new_w, new_h, v))
         return false;
      reset();
      return true;
   }

   //******** BaseVisRefImage METHODS ********

   virtual void      vis_update() { _update_main_mem = true; update(); }
//   virtual void      vis_update() { update(); }
   virtual Bsimplex* vis_simplex(CNDCpt& ndc) const { return simplex(ndc); }
   virtual Bface*    vis_intersect(CNDCpt& ndc, Wpt& obj_pt) const {
      return intersect(ndc, obj_pt);
   }
   virtual void debug(CNDCpt& p) const;

   //******** OBSERVER METHODS ********

   void observe  ();
   void unobserve();

   // FRAMEobs:
   virtual int  tick();

   //*******************************************************
   // PROTECTED
   //*******************************************************
 protected:
   friend class VisRefImageFactory;

   //******** DATA ********
   int          _countup;

   //******** MANAGERS ********
   VisRefImage(CVIEWptr& v);

   //******** UPDATING ********
   void reset() { _need_update = 1; _countup = 0; }

   //******** IDRefImage METHODS ********
   virtual void draw_objects(GELlist&) const;

   //******** OBSERVER METHODS ********:
   // BMESHobs:
   virtual void notify_change(BMESH*, BMESH::change_t)  { reset(); }
   virtual void notify_xform (BMESH*, CWtransf&, CMOD&) { reset(); }

   // CAMobs:
   virtual void notify(CCAMdataptr&)    { reset(); }

   // DISPobs:
   virtual void notify(CGELptr&, int)   { reset(); }

   // EXISTobs:
   virtual void notify_exist(CGELptr&, int) { reset(); }

   // XFORMobs
   virtual void notify_xform(CGEOMptr&, STATE) { reset(); }
};

#endif // REF_IMAGE_H_IS_INCLUDED

// end of file ref_image.H
