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
 * gesture_stroke_drawer.C
 *
 **********************************************************************/

#include "geom/gl_view.H"
#include "std/config.H"

#include "gesture_stroke_drawer.H"

using mlib::PIXEL;
using mlib::PIXEL_list;

inline double
pressure_to_grey(double p)
{
   // map stylus pressure to greyscale non-linearly.

   // pressure is in range [0,1]

   // treat all pressures as being at least this:
   const double MIN_PRESSURE = 0.3; 

   // greyscale color at min pressure
   const double GREY0 = 0.6; 

   // this function increases in darkness from GREY0 at MIN_PRESSURE
   // to black at pressure 1. the square root makes darkness increase
   // rapidly for light pressures, then taper off to black as pressure
   // nears 1:
   return GREY0*(1 - sqrt(max(p, MIN_PRESSURE) - MIN_PRESSURE));
}


GestureStrokeDrawer::GestureStrokeDrawer()
{
   _base_stroke_proto = 0;
}

GestureStrokeDrawer::~GestureStrokeDrawer()
{
   if(_base_stroke_proto) 
      delete _base_stroke_proto;
}


int
GestureStrokeDrawer::draw(const GESTURE* gest, CVIEWptr& v)
{

   if (Config::get_var_bool("DEBUG_GEST_STROKE_DRAW",false))
      err_msg("GestureStrokeDrawer::draw");

   if (!_base_stroke_proto) {
      cerr << "GestureStrokeDrawer::draw(), WARNING: " <<
         "Null _base_stroke_proto, aborting"  << endl;
      return 0;
   }

   const PIXEL_list&    pts   = gest->pts();
   const ARRAY<double>& press = gest->pressures();

   assert(pts.num() == press.num());


   const StrokeVertexArray& verts 
      = _base_stroke_proto->get_verts();

   _base_stroke_proto->clear();

   PIXEL prev_pix;

   static bool debug = Config::get_var_bool("DEBUG_GEST_DRAWER",false);
                                                   

   for (int k=0; k< pts.num(); k++) {

      if (k>0 && prev_pix == pts[k]) {
         if (debug)
            cerr << "gesture drawer, dropping duplicate "
                 << "gesture pixel" << endl;
         continue;
      }

      _base_stroke_proto->add(pts[k]);

      if ( _base_stroke_proto->get_press_vary_width() ) {
	verts[verts.num()-1]._width = (float)press[k];

	 // XXX Hack to hide antialiased lines when the width is small
	 if ((float)press[k]<=0.1f) {
	   verts[verts.num()-1]._alpha = 10.0f*(float)press[k];
	 } else {
	   verts[verts.num()-1]._alpha = 1.0f;
	 }
      }

      
      if ( _base_stroke_proto->get_press_vary_alpha() ) {
         verts[verts.num()-1]._alpha = (float)press[k];
      }

      prev_pix = pts[k];
   }

   if (verts.num() < 2) 
      return 0;


   _base_stroke_proto->draw_start();
   _base_stroke_proto->draw(v);
   _base_stroke_proto->draw_end();

   return 0;
}


/* end of file gesture_stroke_drawer.C */
