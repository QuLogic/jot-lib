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
 * npr_view.H:
 **********************************************************************/
#ifndef NPRVIEW_HEADER
#define NPRVIEW_HEADER

#include "geom/gl_view.H"
#include "gtex/ref_image.H"
#include "gtex/paper_effect.H"
#include "gtex/halo_ref_image.H"

MAKE_PTR_SUBC(NPRview, GL_VIEW);
class NPRview : public GL_VIEW {
 public:
   //******** MANAGERS ********
   NPRview() : _distribute_pixels_stamp(0) {}
   virtual       ~NPRview() {}

   //******** HELPFUL STUFF ********

   // On demand: iterate over the pixels of the ID image,
   // distributing pixels to each patch. Only does work the first
   // time it is called for a frame:
   void distribute_pixels_to_patches();

   //******** VIEWimpl METHODS ********
   virtual void   set_view(CVIEWptr &);
   virtual void   set_rendering(Cstr_ptr &s);
   virtual void   set_size(int w, int h, int x, int y);
   virtual void   draw_setup();

   virtual int    draw_background();
   virtual bool   antialias_check();

   virtual int    draw_frame  (CAMdata::eye = CAMdata::MIDDLE);
   virtual void   swap_buffers();

   IDRefImage*     id_ref()         { return     IDRefImage::lookup(_view); }
   HaloRefImage*   halo_ref()       { return     HaloRefImage::lookup(_view); }
   ColorRefImage*  color_ref(int i) { return  ColorRefImage::lookup(i, _view); }

   // For debugging -- to display a given reference image:
   enum ref_img_enum_t {
      SHOW_NONE = 0,
      SHOW_VIS_ID,
      SHOW_ID_REF,
      SHOW_COLOR_REF0,
      SHOW_COLOR_REF1,
      SHOW_TEX_MEM0,
      SHOW_TEX_MEM1,
      SHOW_BUFFER,
      SHOW_AUX,
      SHOW_HALO,
      NUM_REF_IMGS   // Should always be last in this list
   };

   static int _draw_flag;
   static void next_ref_img();

   // hacky way to get a screen grab:
   static int _capture_flag;

 protected:
   uint _distribute_pixels_stamp;

   //******** INTERNAL METHODS ********

   void draw_canvas();
};

#endif // NPRVIEW_HEADER

/* end of file npr_view.H */
