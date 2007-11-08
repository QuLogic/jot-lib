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
#ifndef HALO_IMAGE_H_IS_INCLUDED
#define HALO_IMAGE_H_IS_INCLUDED

#include "ref_image.H"
#include "halo_blur_shader.H"
#include "util.H"

/*****************************************************************
 * HaloRefImage:
 *
 *    Reference image for drawing halos.
 *****************************************************************/
class HaloRefImage :  public RefImage {
 public:

   //******** MANAGERS ********

  static HaloRefImage* lookup(CVIEWptr& v = VIEW::peek());

  static void schedule_update(bool main_mem, bool tex_mem,
                              CVIEWptr& v = VIEW::peek());

   //******** RefImage VIRTUAL METHODS ********

   // for debugging: string ID for this class:
   virtual str_ptr class_id() const { return str_ptr("HaloRefImage"); }


   //******** OTHER ********

 // lookup the ColorRefImage and return a pointer to its internal texture:
   static TEXTUREglptr lookup_texture( CVIEWptr& v = VIEW::peek()) {
      RefImage* c = lookup(v);
      return c ? c->get_texture() : 0;
   }
   
   // lookup the texture unit used by the current ColorRefImage;
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

   virtual void update();
   virtual bool resize(uint new_w, uint new_h, CNDCvec& v=VEXEL(0.5,0.5));

   CCOLOR get_pass_color() { return _pass_color; };

   void set_kernel_size(int filter_size);
   int get_kernel_size();

   void draw_output();

 protected:
   //******** MANAGERS ********
   HaloRefImage(CVIEWptr& v);
   
   void draw_objects(CGELlist&);
   void copy_to_scratch();
   void copy_to_tex_aux();
   void draw_scratch();

   // associates an HaloRefImage with a VIEW:
   static HASH _hash; 

   COLOR _pass_color;
   TEXTUREglptr _scratch_tex; //objects are rendered here in order for the GLSL blur to work
   HaloBlurShader* _blur_filter;

};

#endif // HALO_IMAGE_H_IS_INCLUDED

//end of HaloRefImage.H
