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
#ifndef SKY_BOX_H_IS_INCLUDED
#define SKY_BOX_H_IS_INCLUDED

/******************************************************
 * Sky BOX class (under construction)
 * supports hashing and texturing 
 * translates with camera movements so camera is always 
 * in the center <- infinite distance illusion
 *******************************************************/

//***************
// Karol Szerszen
//***************

#include "geom/geom.H"

MAKE_PTR_SUBC(SKY_BOX,GEOM);
typedef const SKY_BOXptr CSKY_BOXptr;
class SKY_BOX : public GEOM {
 public :

   //*************MANAGERS**************

   SKY_BOX();
   virtual ~SKY_BOX();

   //*************RUNTIME ID************

   DEFINE_RTTI_METHODS3("SKY_BOX", SKY_BOX*, GEOM, CDATA_ITEM*);

   //******** UTILITIES ********

   void update_position (); // centers the sky box on the camera

   //******** STATICS ********

   static bool is_active() {
      return _sky_instance && DRAWN.contains(_sky_instance);
   }

   static void clean_on_exit() {
      if (Config::get_var_bool("DEBUG_CLEAN_ON_EXIT",false))
         cerr << "SKY_BOX::clean_on_exit()\n";
      _sky_instance = 0;
   }
   static SKY_BOX* null;

   static SKY_BOXptr lookup() {
      return _sky_instance ? _sky_instance : SKY_BOXptr(new SKY_BOX());
   }
 
   static void show();
   static void hide();
   static void toggle();

   //******** GEL VIRTUAL METHODS ********

   virtual int     draw     (CVIEWptr &v);

   // GEL virtual method:
   // skybox can never cast a halo, so no point doing all thw work 
   // in the halo_ref_image
   virtual bool can_do_halo() const { return false; }

   //******** RefImageClient METHODS ********
   
   // GEOM utility method used in RefImageClient draw calls:
   virtual int draw_img(const RefImgDrawer& r, bool enable_shading=false);

   // XXX - probably should not need this:
   virtual int draw_color_ref(int i);

   virtual bool needs_blend() const {return false; }
   
   //******** GEL METHODS ********

   virtual DATA_ITEM* dup() const { return 0; } // there's only one

   virtual bool do_viewall() const { return false; }

   //*************INTERNAL STUFF*********
   Patch* get_patch();

 protected:
   
   static SKY_BOXptr    _sky_instance;

   int  DrawGradient (CVIEWptr &v);

   void test_perlin(CVIEWptr &v);


};

#endif // SKY_BOX_H_IS_INCLUDED

// end of file sky_box.H
