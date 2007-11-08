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
#include "halo_ref_image.H"


// The halo reference image, winter 2007 by KS

static bool debug = Config::get_var_bool("DEBUG_REF_IMAGES",false);
HASH HaloRefImage::_hash(16);

HaloRefImage::HaloRefImage(CVIEWptr& v) : RefImage(v)
{
   
   _texture->set_tex_unit(TexUnit::REF_HALO + GL_TEXTURE0);
   
   _blur_filter = new HaloBlurShader();

   _pass_color = Color::black;

   _scratch_tex = new TEXTUREgl("", GL_TEXTURE_2D, TexUnit::REF_IMG + GL_TEXTURE0);
   assert(_scratch_tex);

    // let's try not doing power-of-2 expanding:
   _scratch_tex->set_expand_image(false);
   _scratch_tex->set_tex_fn(GL_REPLACE);
   _scratch_tex->set_tex_unit(GL_TEXTURE1);
   _scratch_tex->set_mipmap(false);
   _scratch_tex->set_min_filter(GL_LINEAR);
   _scratch_tex->set_max_filter(GL_LINEAR);
   _scratch_tex->set_wrap_r(GL_CLAMP_TO_EDGE);
   _scratch_tex->set_wrap_s(GL_CLAMP_TO_EDGE);
   _scratch_tex->set_wrap_t(GL_CLAMP_TO_EDGE);
   _blur_filter->set_input_tex(_scratch_tex);


}

HaloRefImage* 
HaloRefImage::lookup(CVIEWptr& v)
{
   if (!v) {
      err_msg( "HaloRefImage::lookup: error:  view is nil");
      return 0;
   }
   if (!v->impl()) {
      err_msg( "HaloRefImage::lookup: error:  view->impl() is nil");
      return 0;
   }

   // hash on the view implementation rather than the view itself
   long key = (long)v->impl();
   HaloRefImage* ret = (HaloRefImage*) _hash.find(key);
   if (!ret && (ret = new HaloRefImage(v)))
      _hash.add(key, (void *)ret);
   return ret;
}

void 
HaloRefImage::schedule_update(bool main_mem, bool tex_mem, CVIEWptr& v)
{
  HaloRefImage* c = lookup(v);
   if (!c) {
      cerr << "HaloRefImage::schedule_update: can't get image "  << endl;
      return;
   }
   c->_update_main_mem = main_mem;
   c->_update_tex_mem  =  tex_mem;

   static bool debug = Config::get_var_bool("DEBUG_HALO_UPDATE",false);
   if (debug) {
      cerr << "HaloRefImage::schedule_update: "
           << (main_mem ? "1" : "0")
           << ( tex_mem ? "1" : "0")
           << endl;
   }
}

bool
HaloRefImage::resize(uint w, uint h, CNDCvec& v)
{
   // returns true if size changes
   bool ret = RefImage::resize(w,h,v);

   // handle scratch texture now too
   assert(_scratch_tex);
   const Image& img = _scratch_tex->image();
   if (Point2i(w,h) != img.dims()) {
      _scratch_tex->image().resize(w,h);
   }
   return ret;
}

void 
HaloRefImage::copy_to_scratch()
{
   check_resize();
   assert(_scratch_tex &&
          _scratch_tex->image().dims() == Point2i(VIEW::cur_size()));

   glPushAttrib(GL_ENABLE_BIT);

   // Specify texture
   _scratch_tex->apply_texture();   // GL_ENABLE_BIT

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
HaloRefImage::draw_scratch()
{
   // set opengl state:
   glPushAttrib(GL_ENABLE_BIT | GL_TRANSFORM_BIT);
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT

   // prevents depth testing AND writing:
   glDisable(GL_DEPTH_TEST);    // GL_ENABLE_BIT

   // so it doesn't bleed stuff from the front buffer
   // through the low alpha areas:

   // load identity for model matrix
   glMatrixMode(GL_MODELVIEW);  // GL_TRANSFORM_BIT
   glPushMatrix();
   glLoadIdentity();

   // set up to draw in XY coords:
   glMatrixMode(GL_PROJECTION); // GL_TRANSFORM_BIT
   glPushMatrix();
   glLoadMatrixd(VIEW::peek()->xypt_proj().transpose().matrix());

   // Draw a quad with our texture on it
   assert(_scratch_tex);
   _scratch_tex->apply_texture();   // GL_ENABLE_BIT

   GLfloat t = 1;
   glColor4f(1, 1, 1, 1);       // GL_CURRENT_BIT
   glBegin(GL_QUADS);
   // draw vertices in CCW order starting at bottom left:
   GLenum unit = _scratch_tex->get_tex_unit();
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
HaloRefImage::copy_to_tex_aux()
{
   check_resize();
   assert(_texture && _texture->image().dims() == Point2i(VIEW::cur_size()));

   glPushAttrib(GL_ENABLE_BIT);

   // Specify texture
   _texture->apply_texture();   // GL_ENABLE_BIT

   //copy contents of the aux0 buffer to output texture
   glReadBuffer(GL_AUX0);

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
HaloRefImage::draw_objects(CGELlist& drawn)
{
   assert(_view);

   GELlist objects;
   for (int i=0; i < drawn.num(); i++ ) {
      if (drawn[i]->is_3D()) 
         if (drawn[i]->name()!=str_ptr("Skybox")) // XXX - hack!!
            objects+=drawn[i];
   }

   objects.sort(GL_VIEW::depth_compare);

   // setup projection matrix
   glMatrixMode(GL_PROJECTION); // GL_TRANSFORM_BIT
   glPushMatrix();
   glLoadMatrixd(_view->cam()->projection_xform().transpose().matrix());

   // setup modelview matrix
   glMatrixMode(GL_MODELVIEW);  // GL_TRANSFORM_BIT
   glPushMatrix();
   Wtransf mat = _view->cam()->xform().transpose();
   glLoadMatrixd(mat.matrix());
         
   glEnable(GL_BLEND);                                // GL_ENABLE_BIT
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_COLOR_BUFFER_BIT

   //clear the aux0 buffer 
   glDrawBuffer(GL_AUX0);
   glClearColor(1.0,1.0,1.0,0.0);     
   glClear(GL_COLOR_BUFFER_BIT);
      

   if (get_kernel_size()>0) { //no need to do this if kernel size is zero
      glReadBuffer(GL_BACK);

      for(int i=0; i<objects.num(); i++) {  
         
         glDrawBuffer(GL_BACK);
           
         //clear back buffer
         glClearColor(1.0,1.0,1.0,0.0);
         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
         if (objects[i]->can_do_halo()) {
            _pass_color = Color::black;
            objects[i]->draw_halo_ref();


            //perform the blur in GLSL
              
            copy_to_scratch();
            _blur_filter->draw(_view);

            //render the white version on top

            //clear Z-buffer
            glClear(GL_DEPTH_BUFFER_BIT);
         }
         glDisable(GL_BLEND);
          
         _pass_color = Color::white;  //white
         objects[i]->draw_halo_ref();

         //accumulate    
         copy_to_scratch();       
         glEnable(GL_BLEND);
           
         glDrawBuffer(GL_AUX0);
         draw_scratch(); //composite into AUX0 buffer
      }  
   }

   //at this point the combined image exists in GL_AUX0 buffer
   //it is copied to texture from there instead of being copied to back buffer
  
   // restore projection matrix
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();

   // restore modelview matrix
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
}

void
HaloRefImage::update()
{
  // render the scene and read pixels to main memory
   // and/or texture memory

   if (!need_update())
      return;

   static bool debug = Config::get_var_bool("DEBUG_HALO_UPDATE",false);
   err_adv(debug, "HaloRefImage::update: updating...");

    check_resize();

   glPushAttrib(
      GL_LINE_BIT               |
      GL_DEPTH_BUFFER_BIT       |
      GL_ENABLE_BIT             |
      GL_LIGHTING_BIT           |
      GL_VIEWPORT               | // XXX - not needed
      GL_COLOR_BUFFER_BIT       | // XXX - not needed
      GL_TEXTURE_BIT              // XXX - not needed
      );

   // XXX - remove this:
   assert(_view && _view == VIEW::peek());

   // set up lights
   _view->setup_lights();

   // set default state
   // XXX - check
   glLineWidth(1.0);            // GL_LINE_BIT
   glDepthMask(GL_TRUE);        // GL_DEPTH_BUFFER_BIT
   glDepthFunc(GL_LESS);        // GL_DEPTH_BUFFER_BIT
   glEnable(GL_DEPTH_TEST);     // GL_DEPTH_BUFFER_BIT
   glDisable(GL_NORMALIZE);     // GL_ENABLE_BIT
   glDisable(GL_BLEND);         // GL_ENABLE_BIT
   glEnable(GL_CULL_FACE);      // GL_ENABLE_BIT

   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL); // GL_ENABLE_BIT
   glShadeModel(GL_SMOOTH);     // GL_LIGHTING_BIT
  
   // set viewport to ref image size:
   glViewport(0,0,_width,_height); // GL_VIEWPORT_BIT
   draw_objects(_view->drawn());

   // copy image to main memory or texture memory
   // (or both) as requested:
   if (_update_main_mem) {
      glReadBuffer(GL_AUX0); //because the image is constructed in aux0
      copy_to_ram();
      glReadBuffer(GL_BACK);
   }
   if (_update_tex_mem)
      copy_to_tex_aux(); //copies from aux0

   // clear update flags until next update request is made:
   _update_main_mem = _update_tex_mem = false;

   glPopAttrib();

   // restore viewport to window size:
   int w, h;
   _view->get_size(w,h);
   glViewport(0,0,w,h);
}

void 
HaloRefImage::set_kernel_size(int filter_size) 
{
   _blur_filter->set_kernel_size(filter_size);
}

int 
HaloRefImage::get_kernel_size()
{
   return _blur_filter->get_kernel_size();
}

void
HaloRefImage::draw_output()
{
   glReadBuffer(GL_AUX0);
   glDrawBuffer(GL_BACK);
   glDisable(GL_BLEND);
   glCopyPixels(0,0,800,600,GL_COLOR);
   glEnable(GL_BLEND);
   glReadBuffer(GL_BACK);
}

// end of halo_ref_image.C
