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
#include "gtex/ref_image.H"
#include "gtex/aux_ref_image.H"
#include "gtex/buffer_ref_image.H"
#include "gtex/halo_ref_image.H" //blasphemy !! base class including a derived class

using namespace mlib;

static bool debug = Config::get_var_bool("DEBUG_REF_IMAGES",false);

/**********************************************************************
 * RefImage:
 **********************************************************************/
ARRAY<RefImage*> RefImage::_update_list;

RefImage::RefImage(CVIEWptr& v) :
   _view(v),
   _update_main_mem(false),
   _update_tex_mem(false)
{  
   _texture = new TEXTUREgl("", GL_TEXTURE_2D, TexUnit::REF_IMG + GL_TEXTURE0);
   assert(_texture);

    // let's try not doing power-of-2 expanding:
   _texture->set_expand_image(false);
   _texture->set_tex_fn(GL_REPLACE);
}

void 
RefImage::copy_to_ram()
{
   assert(_values);
   glReadPixels(0,0,_width,_height,GL_RGBA,GL_UNSIGNED_BYTE,_values); 
}

void 
RefImage::copy_to_tex()
{
   check_resize();
   assert(_texture && _texture->image().dims() == Point2i(VIEW::cur_size()));

   glPushAttrib(GL_ENABLE_BIT);

   // Specify texture
   _texture->apply_texture();   // GL_ENABLE_BIT

   glReadBuffer(GL_BACK);

   // Copies the frame buffer into a texture in gpu texture memory.
   glCopyTexImage2D(
      GL_TEXTURE_2D,  // The target to which the image data will be changed.
      0,              // Level of detail, 0 is base detail
      GL_RGBA,        // internal format
      0,              // x-coord of lower left corner of the window
      0,              // y-coord of lower left corner of the window
      _width,         // texture width
      _height,        // texture height
      0);             // border size, must be 0 or 1

   glPopAttrib();
}

void 
RefImage::draw_tex()
{
   // set opengl state:
   glPushAttrib(GL_ENABLE_BIT | GL_TRANSFORM_BIT);
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT

   // prevents depth testing AND writing:
   glDisable(GL_DEPTH_TEST);    // GL_ENABLE_BIT

   // so it doesn't bleed stuff from the front buffer
   // through the low alpha areas:
   glDisable(GL_BLEND);         // GL_ENABLE_BIT

   // load identity for model matrix
   glMatrixMode(GL_MODELVIEW);  // GL_TRANSFORM_BIT
   glPushMatrix();
   glLoadIdentity();

   // set up to draw in XY coords:
   glMatrixMode(GL_PROJECTION); // GL_TRANSFORM_BIT
   glPushMatrix();
   glLoadMatrixd(VIEW::peek()->xypt_proj().transpose().matrix());

   // Draw a quad with our texture on it
   assert(_texture);
   _texture->apply_texture();   // GL_ENABLE_BIT


   GLfloat t = 1;
   glColor4f(1.0, 1.0, 1.0, 1);       // GL_CURRENT_BIT
   glBegin(GL_QUADS);
   // draw vertices in CCW order starting at bottom left:
   GLenum unit = _texture->get_tex_unit();
   glMultiTexCoord2f(unit, 0, 0); glVertex2f(-t, -t);
   glMultiTexCoord2f(unit, 1, 0); glVertex2f( t, -t);
   glMultiTexCoord2f(unit, 1, 1); glVertex2f( t,  t);
   glMultiTexCoord2f(unit, 0, 1); glVertex2f(-t,  t);
   glEnd();

   // restore projection matrix
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();

   // restore modelview matrix
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   // restore state:
   glPopAttrib();
}

void 
RefImage::check_resize()
{
   int w, h;
   _view->get_size(w,h);
   resize(w,h);
}

void 
RefImage::draw_img() const 
{
   // before calling glRasterPos2i(),
   // first set up orthographic viewing
   // matrix for points in pixel coords:
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixd(VIEW::peek()->pix_proj().transpose().matrix());

   glRasterPos2i(0,0);

   glPushAttrib(GL_ENABLE_BIT);
   glDisable(GL_BLEND);

   glDrawPixels(_width,_height,GL_RGBA,GL_UNSIGNED_BYTE,_values);

   glPopAttrib();

   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
}

int
RefImage::read_file(char* file)
{
   // load up the ref image with an image from a file

   // the Image class already has file I/O capabilities, so we just
   // use on of those and copy the data into the RefImage format:
   Image img;

   // try to read the file as an image (ppm or png):
   if (!img.load_file(file)) {
      err_msg( "RefImage::read_file: can't read file %s", file);
      return 0;
   }

   // resize this
   resize(img.width(), img.height());

   // get ready for the switch statement below where we read the data
   // according to the internal format of the Image, based on the bits
   // per pixel (bpp):
   uchar* data = img.data();
   uint i=0;
   switch(img.bpp()) {
    case 4:
      // R,G,B,A unsigned char's
      for (i=0; i<max(); i++) {
         val(i) = build_rgba(data[0], data[1], data[2], data[3]);
         data += 4;
      }
      break;
    case 3:
      // R,G,B unsigned char's
      for (i=0; i<max(); i++) {
         val(i) = build_rgba(data[0], data[1], data[2]);
         data += 3;
      }
      break;
    case 2:
      // luminance/alpha unsigned char's
      for (i=0; i<max(); i++) {
         val(i) = build_rgba(data[0], data[0], data[0], data[1]);
         data += 2;
      }
      break;
    case 1:
      // luminance unsigned char's
      for (i=0; i<max(); i++) {
         val(i) = build_rgba(data[0], data[0], data[0]);
         data += 1;
      }
      break;
    default:
      // it's not in there
      err_msg( "RefImage::read_file: unknown bits per pixel (%d) in image",
               img.bpp());
      return 0;
   }

   // no more errors are possible!
   return 1;
}

void 
RefImage::fill(uint fill_color)
{
   for (uint i=0; i<_max; i++) {
      val(i) = fill_color;
   }
}

int
RefImage::write_file(char* file)
{
   return Image::write_png(
      _width,
      _height,
      4,        // 4 bytes: GL_RGBA
      (GLubyte*)_values,
      str_ptr(file)
      );
}

int
RefImage::copy_rgb(Image& img) const
{
   // return an Image with same dimensions
   // as this, but formatted for RGB:

   // resize image
   if (!img.resize(_width,_height,3)) {
      err_msg( "RefImage::copy_rgb_image: can't resize image");
      return 0;
   }

   // copy data
   uchar* d = img.data();
   for (uint i=0; i<_max; i++) {
      uint v = val(i);
      *d++ = rgba_to_r(v);
      *d++ = rgba_to_g(v);
      *d++ = rgba_to_b(v);
   }

   return 1;
}

bool
RefImage::find_val_in_box(uint v, Cpoint2i& center, uint rad) const
{
   // Return true if the given value v is found in an (n x n) square
   // region around the given center location, where n = 2*rad + 1.
   // E.g., if rad == 1 the search is within a 3 x 3 region.

   // Get bounds of search region, careful at image boundary:
   uint min_x = (uint) ::max(center[0] - rad, uint(0));
   uint max_x = (uint) ::min(center[0] + rad, uint(_width-1));
   uint min_y = (uint) ::max(center[1] - rad, uint(0));
   uint max_y = (uint) ::min(center[1] + rad, uint(_height-1));

   for (uint y=min_y; y<=max_y; y++) {
      uint d = y * _width;
      for (uint x=min_x; x<=max_x; x++)
         if (val(d + x) == v)
            return true;
   }
   return false;
}

bool
RefImage::find_val_in_box(
   uint v,
   uint mask,
   Cpoint2i& center,
   uint rad,
   int nbr
   ) const
{
   // Return true if the given value v is found in an (n x n) square
   // region around the given center location, where n = 2*rad + 1.
   // E.g., if rad == 1 the search is within a 3 x 3 region.

   // XXX in this call we're looking only at a certain section 
   // of the bits, cause we're encoding more than just ID into
   // the color here    - philipd

   // Get bounds of search region, careful at image boundary:
   uint min_x = (uint) ::max(center[0] - rad, uint(0));
   uint max_x = (uint) ::min(center[0] + rad, uint(_width-1));
   uint min_y = (uint) ::max(center[1] - rad, uint(0));
   uint max_y = (uint) ::min(center[1] + rad, uint(_height-1));

   for (uint y=min_y; y<=max_y; y++) {
      uint d = y * _width;
      for (uint x=min_x; x<=max_x; x++)
         if ( (val(d + x) & mask) == (v & mask)) { 
            //we've found the right id ( upper bits match )
            //but should we do something more, like check
            //that parameter is in range?
            int found_l   = (int)(val(d+x)& 0x000000ff);
            int target_l  = (int)(v       & 0x000000ff);
            if ( abs( found_l - target_l ) <= nbr ) {  
               return true;
            }
         }
   }
   return false;
}

void 
RefImage::update_all(VIEWptr v)
{
   // Perhaps it's bad form to have a base class method
   // that is aware of various derived classes...
   // anyway this function replaces functionality
   // (coordinating updates to ref images) that used
   // to be in NPRview.

   err_adv(0 && debug, "RefImage::update_all: frame number %d", VIEW::stamp());

   // Ask each GEL to request the ref images it needs:
   v->drawn().request_ref_imgs();

   // Now update the ref images that need updates...

   // First update the ID image:
   assert(IDRefImage::lookup(v));
   IDRefImage::lookup(v)->update();

   //I hope this is the right way to do it
   // XXX - this sucks, it updates the halo image ALL THE TIME
   //       when no one wants it, ruining the frame rate, 
   //       killing the non-proxy demos, and pissing me off
   assert(HaloRefImage::lookup(v));
   HaloRefImage::lookup(v)->update();

   // Now update each color reference image:
   ColorRefImage::update_images(v);
}

bool
RefImage::resize(uint w, uint h, CNDCvec& v)
{
   // returns true if any change happened

   bool ret = Array2d<GLuint>::resize(w,h,v);

   // handle texture now too
   assert(_texture);
   const Image& img = _texture->image();
   if (Point2i(w,h) != img.dims()) {
      if (debug) {
         cerr << class_id() << "::resize: resizing from "
              << img.width() << "x" << img.height() << " to "
              << w << "x" << h << endl;
      }
      _texture->image().resize(w,h);
   }
   return ret;
}



void 
RefImage::view_resize(VIEWptr v)
{
   // old code in NPRview used to keep ref images a bit smaller
   // than the actual rendering window. instead we'll try a
   // simpler policy: keep the image the same size as the view.

   err_adv(debug, "RefImage::view_resize");

   int w,h;
   v->get_size(w,h);

   // XXX - ref images should observe changes in window size
   assert(IDRefImage::lookup(v));
   IDRefImage::lookup(v)->resize(w,h);
   
   HaloRefImage::lookup(v)->resize(w,h);
   
   ColorRefImage::resize_all(v,w,h);

   assert(VisRefImage::lookup(v));
   VisRefImage::lookup(v)->resize(w,h);
   assert(BufferRefImage::lookup(v));
   BufferRefImage::lookup(v)->force_dirty();
   AuxRefImage::lookup(v); // XXX - what does this do?
}

/**********************************************************************
 * ColorRefImage:
 **********************************************************************/
HASH ColorRefImage::_hash(16);

ColorRefImage::ColorRefImage(CVIEWptr& v, int i) :
   RefImage(v),
   _index(i) 
{
}

ColorRefImage::ColorRefImage_list*
ColorRefImage::get_list(CVIEWptr& v)
{
   if (!v) {
      err_msg( "ColorRefImage::get_list: error -- view is nil");
      return 0;
   }
   if (!v->impl()) {
      err_msg( "ColorRefImage::get_list: error -- view->impl() is nil");
      return 0;
   }

   // hash on the view implementation rather than the view itself
   long key = (long)v->impl();
   ColorRefImage_list* ret = (ColorRefImage_list*) _hash.find(key);
   if (!ret && (ret = new ColorRefImage_list()))
      _hash.add(key, (void*)ret);
   return ret;
}

ColorRefImage* 
ColorRefImage::lookup(int i, CVIEWptr& v)
{
   // return image number i associated with view v

   // to support additional images, increase MAX_IMAGES:
   const int MAX_IMAGES=8;
   if (i<0 || i>=MAX_IMAGES) {
      err_msg("ColorRefImage::lookup: error: image %d not available", i);
      return 0;
   }

   // get the list of color ref images for this view:
   ColorRefImage_list* list = get_list(v);
   if (!list) {
      err_msg("ColorRefImage::lookup: error: can't get list of images");
      return 0;
   }

   // create images if needed up to number i:
   while (i >= list->num())
      list->add(new ColorRefImage(v,list->num()));

   // now image i exists, so return it:
   assert(list->valid_index(i));
   return (*list)[i];
}

// schedule an update for color ref image number i,
// specifying whether to save image in main memory
// or texture memory:
void 
ColorRefImage::schedule_update(int i, bool main_mem, bool tex_mem, CVIEWptr& v)
{
   if (!(main_mem || tex_mem)) {
      cerr << "ColorRefImage::schedule_update: image "
           << i << ": no storage requested, skipping"
           << endl;
      return;
   }
   ColorRefImage* c = lookup(i,v);
   if (!c) {
      cerr << "ColorRefImage::schedule_update: can't get image " << i << endl;
      return;
   }
   c->_update_main_mem = main_mem;
   c->_update_tex_mem  =  tex_mem;
}

void 
ColorRefImage::update_images(VIEWptr v)
{
   ColorRefImage_list* l = get_list(v);
   assert(l);
   for (int i=0; i<l->num(); i++)
      (*l)[i]->update();
}

void 
ColorRefImage::resize_all(VIEWptr v, int w, int h, CNDCvec& offset)
{
   ColorRefImage_list* l = get_list(v);
   assert(l);
   for (int i=0; i<l->num(); i++)
      (*l)[i]->resize(w,h,offset);
}

// used in:
//   ColorRefImage::update()
//   IDRefImage::update()
inline void
set_default_gl_state()
{
   glLineWidth(1.0);
   glDepthMask(GL_TRUE);
   glDepthFunc(GL_LEQUAL);
   glEnable(GL_DEPTH_TEST);
   glDisable(GL_NORMALIZE);
   glDisable(GL_BLEND);
   glEnable(GL_CULL_FACE); // XXX - bad?
}

void
ColorRefImage::update()
{
   // render the scene and read pixels to main memory
   // and/or texture memory

   if (!need_update())
      return;

   check_resize();

   err_adv(0 && debug, "ColorRefImage::update: image %d, frame %d",
           _index, VIEW::stamp());

   glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);

   glDrawBuffer(GL_BACK);
   glReadBuffer(GL_BACK);

   // XXX - remove this:
   assert(_view && _view == VIEW::peek());

   // set up lights
   _view->setup_lights();

   // set default state
   // XXX - check
   set_default_gl_state();

   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);
   glShadeModel(GL_SMOOTH);
  
   // set viewport to ref image size:
   glViewport(0,0,_width,_height);

   glClearColor(1,1,1,1);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   draw_objects(_view->drawn());

   // copy image to main memory or texture memory
   // (or both) as requested:
   if (_update_main_mem)
      copy_to_ram();
   if (_update_tex_mem){
      copy_to_tex();
   }

   // clear update flags until next update request is made:
   _update_main_mem = _update_tex_mem = false;

   glPopAttrib();

   // restore viewport to window size:
   int w, h;
   _view->get_size(w,h);
   glViewport(0,0,w,h);
}

void
ColorRefImage::draw_objects(CGELlist& drawn) const
{
   assert(_view);

   // sort into two groups 
   GELlist blended, opaque;
   for (int i=0; i < drawn.num(); i++ ) {
      if (!drawn[i]->is_3D()) {
         ; // skip it
      } else if (drawn[i]->can_do_halo() && GEOM::do_halo_ref()) {
         blended += drawn[i];
      } else {
         opaque  += drawn[i];
      }
   }
   blended.sort(GL_VIEW::depth_compare);

   // setup projection matrix
   glMatrixMode(GL_PROJECTION); // GL_TRANSFORM_BIT
   glPushMatrix();
   glLoadMatrixd(_view->cam()->projection_xform().transpose().matrix());

   // setup modelview matrix
   glMatrixMode(GL_MODELVIEW);  // GL_TRANSFORM_BIT
   glPushMatrix();
   Wtransf mat = _view->cam()->xform().transpose();
   glLoadMatrixd(mat.matrix());
  
   // draw opaque objects first:
   opaque.draw_color_ref(_index);

   // now semi-transparent objects:
   glEnable(GL_BLEND);                                // GL_ENABLE_BIT
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_COLOR_BUFFER_BIT
   blended.draw_color_ref(_index);

   // restore projection matrix
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();

   // restore modelview matrix
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
}


/**********************************************************************
 * IDRefImage:
 **********************************************************************/
HASH IDRefImage::_hash(16);
uint IDRefImage::_red_bits   = 0;
uint IDRefImage::_green_bits = 0;
uint IDRefImage::_blue_bits  = 0;
uint IDRefImage::_alpha_bits = 0;

bool IDRefImage::_nonstandard_bits = false;

IDRefImage* IDRefImage::_instance = 0;

IDRefImage* 
IDRefImage::lookup(CVIEWptr& v)
{
   if (!v) {
      err_msg( "IDRefImage::lookup: error:  view is nil");
      return 0;
   }
   if (!v->impl()) {
      err_msg( "IDRefImage::lookup: error:  view->impl() is nil");
      return 0;
   }

   // hash on the view implementation rather than the view itself
   long key = (long)v->impl();
   IDRefImage* ret = (IDRefImage*) _hash.find(key);
   if (!ret && (ret = new IDRefImage(v)))
      _hash.add(key, (void *)ret);
   return ret;
}

void
IDRefImage::schedule_update(CVIEWptr& v, bool pixels_to_patches, bool main_mem, bool tex_mem)
{
   IDRefImage* id = lookup(v);
   if (!id) {
      cerr << "IDRefImage::schedule_update: can't get ID image" << endl;
      return;
   }
   id->_need_update = true;
   id->_pixels_to_patches = pixels_to_patches;

   IDRefImage* c = lookup(v);
   if (!c) {
      cerr << "IDRefImage::schedule_update: can't get image " << endl;
      return;
   }

   c->_update_main_mem = main_mem;
   c->_update_tex_mem  =  tex_mem;
}

IDRefImage::IDRefImage(CVIEWptr& v) :
   RefImage(v),
   _need_update(false),
   _pixels_to_patches(false)
{
   _texture = new TEXTUREgl("", GL_TEXTURE_2D, TexUnit::PERLIN + GL_TEXTURE0);
   assert(_texture);

    // let's try not doing power-of-2 expanding:
   _texture->set_expand_image(false);
   _texture->set_tex_fn(GL_REPLACE);

   setup_bits(v);
}

void
IDRefImage::setup_bits(CVIEWptr& view)
{
   if (!view)
      return;

   // Only do it the first time
   if (_red_bits || _green_bits || _blue_bits) {
      // Already done, quit early. But check if it's the same answer.
      if (view->win()->red_bits()   != _red_bits   ||
          view->win()->green_bits() != _green_bits ||
          view->win()->blue_bits()  != _blue_bits  ||
          view->win()->alpha_bits() != _alpha_bits)
         err_msg( "IDRefImage::setup_bits: Error: mismatch of bits");
      return;
   }
   _red_bits    = view->win()->red_bits();
   _green_bits  = view->win()->green_bits();
   _blue_bits   = view->win()->blue_bits();
   _alpha_bits  = view->win()->alpha_bits();

   // XXX - alpha is not considered at this time
   _nonstandard_bits = (
      (_red_bits   != 8) ||
      (_green_bits != 8) ||
      (_blue_bits  != 8)
      );

   uint r,g,b,a,s,d; 

   r = view->win()->accum_red_bits();
   g = view->win()->accum_green_bits();
   b = view->win()->accum_blue_bits();
   a = view->win()->accum_alpha_bits();
   s = view->win()->stencil_bits();
   d = view->win()->depth_bits();

   err_msg( "");

   err_msg( "GL Vendor:       %s", glGetString(GL_VENDOR));
   err_msg( "GL Renderer:     %s", glGetString(GL_RENDERER));
   err_msg( "GL Version:      %s", glGetString(GL_VERSION));

   err_msg( "GL Color RGBA:   %u,%u,%u,%u",
            _red_bits, _green_bits, _blue_bits, _alpha_bits);
   err_msg( "GL Accum RGBA:   %u,%u,%u,%u", r,g,b,a);
   err_msg( "GL Stencil Bits: %u", s);
   err_msg( "GL Depth Bits:   %u", d);

   GLint num_units=0;
   glGetIntegerv(GL_MAX_TEXTURE_UNITS, &num_units);
   err_msg( "Texture units:   %d", num_units);

   err_msg( "");   
}

void
IDRefImage::update()
{
   // only update if there was a request:
   if (!_need_update)
      return;
   _need_update = false; // future updates depend on future requests

   err_adv(0 && debug, "IDRefImage::update: frame number %d", VIEW::stamp());

   glDrawBuffer(GL_BACK);
   glReadBuffer(GL_BACK);

   glPushAttrib(GL_ENABLE_BIT);

   // disable lights
   glDisable(GL_LIGHTING);
   glDisable(GL_LIGHT0);        // XXX - overkill?
   glDisable(GL_LIGHT1);        // XXX - overkill?

   // set default state
   // XXX - check
   set_default_gl_state();

   glDisable(GL_COLOR_MATERIAL);
   glShadeModel(GL_FLAT);

   // change viewport to ref image size
   glViewport(0,0,_width,_height);

   // clear to black (ID 0):
   glClearColor(0,0,0,0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // draw all objects (see below)
   draw_objects(_view->drawn());

   // read to main memory
//   copy_to_ram();
   if (_update_main_mem)
     copy_to_ram();
   if (_update_tex_mem){
      copy_to_tex();
   }

   // clear update flags until next update request is made:
   //_update_main_mem = _update_tex_mem = false;

   glPopAttrib();

   // distribute pixels to patches if requested:
   if (_pixels_to_patches) {
      err_adv(debug, "IDRefImage::update: copying pixels to patches");
      // future updates depend on future requests:
      _pixels_to_patches = false;
      for (unsigned int p=0; p < this->max(); p++) {
         Patch* patch = ::get_ctrl_patch(simplex(p));
         if (patch)
            patch->add_pixel(p);
      }
   }

   // restore viewport to window size
   int w, h;
   _view->get_size(w,h);
   glViewport(0,0,w,h);
}

void
IDRefImage::draw_objects(GELlist& drawn) const
{
   // Setup projection matrix:
   glMatrixMode(GL_PROJECTION);
   glLoadMatrixd(_view->cam()->projection_xform().transpose().matrix());

   // Setup modelview matrix with world to eye transform:
   // NOTE: we assume that in the drawing loops below, the
   //       projection and modelview matrices are not disturbed!
   Wtransf mat = _view->cam()->xform().transpose();
   glMatrixMode(GL_MODELVIEW);
   glLoadMatrixd(mat.matrix());

   // Triangles for see thru objects
   drawn.draw_id_ref_pre1();

   // Clear Z buffer
   glClear(GL_DEPTH_BUFFER_BIT);

   // Hidden lines with z testing
   drawn.draw_id_ref_pre2();

   // Clear Z buffer - and only write z's!
   glClear(GL_DEPTH_BUFFER_BIT);
   glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

   // Triangles for see thru objects (just write the z's)
   drawn.draw_id_ref_pre3();

   // Restore writing to color buffer
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

   // Visible lines with z testing
   drawn.draw_id_ref_pre4();

   // Non-see thru objects to their business...
   drawn.draw_id_ref();
}

bool
IDRefImage::search(
   CNDCpt&         c,              // center of search region
   double          screen_pix_rad, // search radius in screen pixels
   CSimplexFilter& filt,           // tells what kind of simplex to look for
   Point2i&        hit             // closest ref image pixel matching search
   )
{
   // Search around the given location within the given
   // radius (specified in units of screen pixels, not
   // pixels of this image), to find the closest Bsimplex
   // matching the criteria given by the filter function.

   // for safety disallow negative values for the radius.
   screen_pix_rad = ::max(0.0, screen_pix_rad);

   // convert radius in screen pixels to ndc units:
   double ndc_rad = VIEW::pix_to_ndc_scale() * screen_pix_rad;

   // now to pixel units in this image:
   double R = _half_min_dim * ndc_rad;
   // get the box containing the search region but not going
   // outside the boundaries of this image:
   Point2i center = ndc_to_pix(c);
   int      min_x = ::max(center[0] - int(ceil(R)), 0);
   int      max_x = ::min(center[0] + int(ceil(R)), int(_width-1));
   int      min_y = ::max(center[1] - int(ceil(R)), 0);
   int      max_y = ::min(center[1] + int(ceil(R)), int(_height-1));

   bool           ret = 0;
   double    min_dist = 0, d;
   for (int y = min_y; y <= max_y; y++) {
      for (int x = min_x; x <= max_x; x++) {
         Point2i cur(x,y);
         if ((d = cur.dist(center)) < R && // if we're in the radius and
             filt.accept(simplex(cur))  && // the simplex is acceptable
             (!ret || d < min_dist)) {     // and no winner or new winner
            ret      = true;
            hit      = cur;
            min_dist = d;
         }
      }
   }
   return ret;
}

Bsimplex* 
IDRefImage::find_near_simplex(
   CNDCpt&         c,              // location - center of search region
   double          screen_pix_rad, // search radius in screen pixels
   CSimplexFilter& filt            // tells what kind of simplex to look for
   )
{
   Point2i hit;
   if (search(c, screen_pix_rad, filt, hit))
      return simplex(hit);
   return 0;
}

bool 
IDRefImage::approx_wpt(CNDCpt& ndc, Wpt& ret) const
{
   // ret = approximate world-space point found by ray test,
   // returns false if the ray test failed:

   Bface* f = face(ndc);
   if (!f)
      return false;

   Wline ray = f->mesh()->inv_xform()*Wline(ndc); // object space view ray
   double depth;
   if (f->ray_intersect(ray, ret, depth)) {
      ret = f->mesh()->xform()*ret; // map to world space
      return true;
   }
   // need the nearest point on f to the ray...
   // Bface::view_intersect() seems to not work
   // XXX - hack for now under deadline pressure:
   //       hey it did say "approx" in the function name!!!
   // XXX - or maybe Bface::view_intersect() was working... w/e, moving on
   ret = f->mesh()->xform()*f->centroid();
   return true;
}

Bedge* 
IDRefImage::find_neighbor(CNDCpt& p, Bedge* current, int radius) const 
{
   // check neighbors, spiralling out from middle
   Point2i center = ndc_to_pix(p);
   Bedge* temp = 0;
   Point2i check;
   for (int rad = 0; rad <= radius; rad++) {
      for (int i = -rad; i <= rad; i++) {
         for (int j = -rad; j <= rad; j++) {
            if ((abs(i)==rad) || (abs(j)==rad)) {
               check = Point2i(center[0]+i, center[1]+j);
               if (pix_in_range(check)  &&
                   (temp = edge(check)) &&
                   (temp != current)
                   && (temp->patch() == current->patch()) && (temp->is_sil()))
                  return temp;
            }
         }
      }
   }
   
   return 0; // no neighbor found
}

Bedge_list
IDRefImage::find_all_neighbors(CNDCpt& p, Patch* patch, int radius) const 
{
   Point2i center = ndc_to_pix(p);

   return find_all_neighbors(center, patch, radius);
}

Bedge_list
IDRefImage::find_all_neighbors(
   Cpoint2i&    center,
   Patch*       patch,
   int          radius
   ) const 
{
   static Bedge_list neighbors(radius*radius);
   neighbors.clear();
   Bedge* temp = 0;
   Point2i check;
   for (int i = -radius; i <= radius; i++) {
      for (int j = -radius; j <= radius; j++) {
         check = center + Vec2i(i,j);
         if (!pix_in_range(check))
            continue;
         temp = edge(check);
         if (!temp)
            continue;
         if ((temp->patch() == patch) && (temp->is_sil()))
            neighbors.add_uniquely(temp);
      }
   }
   
   return neighbors; 
}

bool 
IDRefImage::is_simplex_near(CNDCpt& p, const Bsimplex* simp, int radius) const
{
   // check neighbors, spiralling (?) out from middle
   Point2i center = ndc_to_pix(p);
   Bsimplex* temp = 0;
   Point2i check;
   for (int rad = 0; rad <= radius; rad++) {
      for (int i = -rad; i <= rad; i++) {
         for (int j = -rad; j <= rad; j++) {
            if ((abs(i)==rad) || (abs(j)==rad)) {
               check = Point2i(center[0]+i, center[1]+j);
               if (pix_in_range(check) &&
                   (temp = simplex(check)) &&
                   (temp == simp))
                  return true;
            }
         }
      }
   }
   
   return false;
}

bool
IDRefImage::is_patch_sil_edge_near(
   CNDCpt& ndc,
   const Patch* patch,
   int radius
   ) const
{
   // check neighbors, spiralling (?) out from middle
   Point2i center = ndc_to_pix(ndc);
   Point2i check; 
   for (int rad = 0; rad <= radius; rad++) {
      for (int i = -rad; i <= rad; i++) {
         for (int j = -rad; j <= rad; j++) {
            if ((abs(i)==rad) || (abs(j)==rad)) {
               check = Point2i(center[0]+i, center[1]+j);
               if (is_patch_sil_edge(check, patch)) return true;
            }
         }
      }
   }
   return false;
}

/**********************************************************************
 * VisRefImage:
 **********************************************************************/

// factory class for producing VisRefImages
class VisRefImageFactory : public BaseVisRefImageFactory {
 public:
   virtual BaseVisRefImage* produce(CVIEWptr& v) { return new VisRefImage(v); }
};

VisRefImage::VisRefImage(CVIEWptr& v) :
   IDRefImage(v),
   _countup(0)
{
   reset();
   observe();
}

void
VisRefImage::init()
{
   if (!BaseVisRefImage::_factory)
      BaseVisRefImage::_factory = new VisRefImageFactory;
}

bool
VisRefImage::need_update() 
{
   // Resizes the ref image to match the aspect ratio of the
   // view, keeping the minimum side <= 256 pixels.
   int w, h;
   _view->get_size(w,h);
   int min_side = min(w,h);
   const double D = 256; // shorter side will be at most D pixels
   double s = 1.0;
   if (min_side > D) {
      s = D/min_side;
      w = int(round(w*s));
      h = int(round(h*s));
   }
   return (resize(w,h,VEXEL(s/2,s/2)) || _need_update);
}

void 
VisRefImage::debug(CNDCpt& p) const
{
   cerr << "VisRefImage::debug: ndc: " << p;
   uint id = ndc_to_uint(p);
   cerr << ", uint: " << id;
   GLuint v = val(id);
   cerr << ", val: " << v;
   uint key = rgba_to_key(v);
   cerr << ", key: " << key;
   Bsimplex* s = Bsimplex::lookup(key);
   cerr << ", simplex: " << s;
   cerr << endl;
}

void
VisRefImage::draw_objects(GELlist& drawn) const
{
   static bool debug = Config::get_var_bool("DEBUG_VIS_REF",false);
   err_adv(debug, "VisRefImage::draw_objects");

   // setup projection matrix
   glMatrixMode(GL_PROJECTION);
   glLoadMatrixd(_view->cam()->projection_xform().transpose().matrix());

   // Setup modelview matrix with world to eye transform:
   // NOTE: we assume that in the drawing loop below, the
   //       projection and modelview matrices are not disturbed!
   Wtransf mat = _view->cam()->xform().transpose();
   glMatrixMode(GL_MODELVIEW);
   glLoadMatrixd(mat.matrix());

   drawn.draw_vis_ref();
}

void 
VisRefImage::observe() 
{
   if (_view) {
      // BMESHobs:
      subscribe_all_mesh_notifications();

      // CAMobs:
      _view->cam()->data()->add_cb(this);

      // DISPobs:
      disp_obs();

      // EXISTobs:
      exist_obs();

      // XFORMobs:
      xform_obs();

      // FRAMEobs:
      WORLD::get_world()->schedule(this);
   }
}

void 
VisRefImage::unobserve() 
{
   if (_view) {
      // BMESHobs:
      unsubscribe_all_mesh_notifications();

      // CAMobs:
      _view->cam()->data()->rem_cb(this);

      // DISPobs:
      unobs_display();

      // EXISTobs:
      unobs_exist();

      // XFORMobs:
      unobs_xform();

      // FRAMEobs:
      WORLD::get_world()->unschedule(this);
   }
}

int
VisRefImage::tick()
{
   // XXX - remove this:
   assert(!_pixels_to_patches);

   if (_countup++ > 4 && _need_update) {
      err_adv(0 && ::debug, "VisRefImage::tick: updating...");
      update();
   }
   return 1;
}

uint
IDRefImage::key_to_rgba2(uint key) 
{
   // Convert a Bsimplex "key" value to an RGBA value. Assumes the
   // frame buffer does not store alpha. Thus the key should not store
   // any info in the highest-order byte. On non-intel platforms, we
   // shift the key value left by 8 bits, resulting in an RGBA value
   // with no info in the lowest order byte. This value can be written
   // to the frame buffer and later read back (as long as 8 bits is
   // allocated per color component as is usual). After shifting the
   // result 8 bits to the right, we end up with the original key
   // value. This is done in rgba_to_key(), below.

   // XXX -
   //   Handles other than 8 bits per component
   //   only on Intel platforms (pending fixes).

#if defined(WIN32) || defined(i386)

   // Intel platform: colors strored as ABGR, no shift needed
   if (_nonstandard_bits) {
      // bits per component is other than 8.
      //
      // divide up the info in 'key' into 3 groups with the same
      // number of bits as allotted to red, green, and blue color
      // components. arrange the 3 groups into 3 bytes of a 4-byte
      // unsigned int, which can be fed to OpenGL using
      // glColor4ubv().

      // E.g., if bits for red, green and blue components are 5, 6, 5,
      // respectively, then we want to rearrange the bits in 'key' as
      // follows:
      //
      // ******** ******** bbbbbggg gggrrrrr          key
      //
      //                  |
      //                  |
      //                  V
      //
      // ******** bbbbb*** gggggg** rrrrr***         rgba
      //    A        B        G        R

      // masks to select the 3 chunks of data within 'key':
      static uint mask_r = ((1<<_red_bits)   - 1);
      static uint mask_g = ((1<<_green_bits) - 1) << (_red_bits);
      static uint mask_b = ((1<<_blue_bits)  - 1) << (_red_bits + _green_bits);

      // shifts needed to place the blue and green chunks at the
      // proper position in the unsigned int:
      static int shift_r =  8 - _red_bits;
      static int shift_g = 16 - _red_bits - _green_bits;
      static int shift_b = 24 - _red_bits - _green_bits - _blue_bits;

      return (((key & mask_r) << shift_r) |
              ((key & mask_g) << shift_g) |
              ((key & mask_b) << shift_b));
   }

   // (Intel)
   // No fancy bit-fussing needed
   return key;

#else
   // non-Intel platform:
   // Colors stored as RGBA, shift data past A on the assumption
   // that the framebuffer doesn't actually store alpha:

   // XXX - should handle bits per component other than 8

   return ((key << 8) | 0xff);

#endif
}

uint 
IDRefImage::rgba_to_key2(uint rgba) 
{
   // companion method for key_to_rgba():  see comment above

#if defined(WIN32) || defined(i386)
   if (_nonstandard_bits) {
      // fewer than 8 bits per component.
      //
      // select out valid bits and repack them into the 'key'
      // format. (i.e. the valid bits put back together to form an
      // integer value).

      // ******** bbbbb*** gggggg** rrrrr***         rgba
      //    A        B        G        R
      //
      //                  |
      //                  |
      //                  V
      //
      // ******** ******** bbbbbggg gggrrrrr          key


      // masks for selecting the valid bits where they lay within the
      // rgba unsigned int:
      static uint mask_r = ((1<<_red_bits)  -1) << ( 8 - _red_bits);
      static uint mask_g = ((1<<_green_bits)-1) << (16 - _green_bits);
      static uint mask_b = ((1<<_blue_bits) -1) << (24 - _blue_bits);

      // shifts needed to get the bits into position in the 'key'
      // unsigned int:
      static int shift_r =  8 - _red_bits;
      static int shift_g = 16 - _red_bits - _green_bits;
      static int shift_b = 24 - _red_bits - _green_bits - _blue_bits;
       
      return (((rgba & mask_r) >> shift_r) |
              ((rgba & mask_g) >> shift_g) |
              ((rgba & mask_b) >> shift_b));
                
   }

   // Intel:
   //   Colors stored as ABGR, no shift needed.
   //   But If alpha is not stored in framebuffer, 
   //   have to  mask away those garbage bits:
   if (_alpha_bits == 8)
      return rgba;
   else
      return (rgba & 0xffffff);

#else

   // Non-Intel:

   // XXX - should handle nonstandard bits here
   // colors stored as RGBA. undo the shift in key_to_rgba() above:
   return (rgba >> 8);
#endif
}

Bface* 
VisRefImage::get_face(CNDCpt& cur, double screen_rad) 
{
   // Return the nearest Bface within the given screen region,
   // with a preference for front-facing Bfaces:
   
   Bface* ret = (Bface*)get_simplex(cur, screen_rad, FrontFacingBfaceFilter());
   return ret ? ret : (Bface*)get_simplex(cur, screen_rad, BfaceFilter());
}

Bface_list
VisRefImage::get_faces(const PIXEL_list& pix, double screen_rad)
{
   // Call get_face() on each screen point in sequence:

   Bface_list ret;
   for (int i=0; i<pix.num(); i++) {
      Bface* f = get_face(pix[i], screen_rad);
      if (f)
         ret.add_uniquely(f);
   }
   return ret;
}


Bface*          // Bface* at given subdivision level for a given screen point
VisRefImage::get_face_bc (
   Wvec& bc,    // returned: barycentric coords of near point on face
   CNDCpt& ndc, // given screen point
   double rad   // search radius 
   ) 
{
   // 1. Search the VisRefImage to find the nearest Bface
   //    within a radius of the given screen location.
   //    Get the face at currently drawn subdivision level.
   Bface* f = get_face(ndc, rad);

   // 2. Convert the screen location to a barycentric
   //    coordinate on the Bface.
   if (f)
      f->near_pt(ndc, bc);

   return f;
}

Bface*          // Bface* at given subdivision level for a given screen point
VisRefImage::get_sub_face(
   int level,   // given subdivision level
   Wvec& bc,    // returned: barycentric coords of near point on face
   CNDCpt& ndc, // given screen point
   double rad   // search radius 
   ) 
{
   Bface* f = get_face_bc(bc, ndc, rad);

   if (level == 0)
      return f;
   else if (f && LMESH::isa(f->mesh()))
      return ((Lface*)f)->bc_to_level(level, bc);
   else
      return 0;
}

Bface*          // Bface* at current mesh edit level for a given screen point
VisRefImage::get_edit_face(
   Wvec& bc,    // returned: barycentric coords of near point on face
   CNDCpt& ndc, // given screen point
   double rad   // search radius 
   ) 
{
   Bface* f = get_face_bc(bc, ndc, rad);

   if (f && LMESH::isa(f->mesh()))
      return ((Lface*)f)->bc_to_edit_level(bc);
   else
      return 0;
}

// end of file ref_image.C
