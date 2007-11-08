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
/*!
 *  \file mode_name.C
 *  \brief Contains the implementation of the ModeName class.
 *
 *  \sa mode_name.H
 *
 */

#include "std/support.H"
#include "net/rtti.H"
#include "mlib/points.H"

using namespace mlib;

#include "geom/world.H"
#include "geom/text2d.H"

#include "mode_name.H"

/*!
 *  \brief A specialization of the TEXT2D class that is used by the ModeName
 *  class.
 *
 *  \sa ModeName
 *
 */
class MODE_TEXT : public TEXT2D {
   
   public:
   
      MODE_TEXT()
         : TEXT2D("mode", "", XYpt(-0.95,0.93)) { }

      //******** RUN-TIME TYPE ID ********
      
      DEFINE_RTTI_METHODS3("MODE_TEXT", MODE_TEXT*, GEOM, CDATA_ITEM*);

      int interactive(CEvent &e, State *&s, RAYhit *r) const
      {
         return 0;
      }
   
      RAYhit &intersect(RAYhit &r, CWtransf&, int) const
      {
         //Wvec  normal;
         //Wpt   nearpt;
         //XYpt  texc;
         //double d, d2d;
         //_mesh->intersect(r, xform(), nearpt, normal, d, d2d, texc);
      
         // Check 2d intersection
         XYpt p = r.screen_point();
         BBOX2D bbox = bbox2d(0, 0, 1);
         if( bbox.contains(p) ){
            Wpt      nearpt;
            Wvec     n;
            XYpt     uvc;
            Wpt obj_pt;
            // don't know what d value (first param) to use here
            r.check(1.0, 0, 0.0, (GEOM*)this, n, nearpt, obj_pt, NULL, uvc); 
            //r.check(1.0, *this); 
         }
      
         return r;
      }

};

TEXT2Dptr       ModeName::_mode_name;
LIST<str_ptr>   ModeName::_names;

void
ModeName::init()
{
   if (_mode_name)
      return;
   _mode_name = new MODE_TEXT();
   NETWORK     .set(_mode_name, 0);
   NO_COLOR_MOD.set(_mode_name, 1);
   NO_XFORM_MOD.set(_mode_name, 1);
   NO_DISP_MOD .set(_mode_name, 1);
   NO_COPY     .set(_mode_name, 1);

   WORLD::create(_mode_name, false);
}

void
ModeName::push_name(Cstr_ptr& n)
{
   _names += n;
   set_name(n);
}

void
ModeName::pop_name()
{
   if (_names.empty()) {
      cerr << "ModeName::pop_name: error: empty stack" << endl;
      return;
   }
   _names.pop();
   set_name(_names.empty() ? NULL_STR : _names.last());
}
