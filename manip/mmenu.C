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

#include "std/time.H"
#include "geom/text2d.H"
#include "geom/world.H"
#include "mmenu.H"

using mlib::Wpt;
using mlib::Wtransf;
using mlib::XYpt;
using mlib::CXYpt;
using mlib::XYvec;
using mlib::CXYvec;
using mlib::VEXEL;

int
MMENU::invoke(
   CEvent &e, 
   State *&s
   )
{
   if (_entry.consumes(e)) {
      s = _entry.event(e);
      return true;
   }
   else
      return false;
}

static const double _MENU_OFFSET = 0.02;

int
MMENU::down(
   CEvent  &e,
   State  *&
   )
{
   DEVice_buttons *btns = (DEVice_buttons *)e._d;
   DEVice_2d      *ptr  = btns->ptr2d();
   _view = e.view();

  _view->schedule(this);

  _t0   = the_time();
  _d    = ptr->cur();
  _sel = -1;
  for(int i=0;i<2;i++) _d[i] += _MENU_OFFSET;
  /*
  _disp = false;
  _sel  = NUM_DIRS;
  */

  return 0;
}
  
int
MMENU::move(
   CEvent  &e,
   State  *&
   )
{
   DEVice_2d  *ptr = (DEVice_2d *)e._d;
   CXYpt       p   = ptr->cur();
//  Find if cursor is touching any geoms
   _sel = -1;
   for(int i=0;i<_items.num();i++){
      GEOMptr gp(_items[i]._g);
      if(gp){
         CBBOX b = gp->bbox();
         if (TEXT2D::isa(gp)) {
            TEXT2Dptr  t2d  = (TEXT2D *)&*gp;

            t2d->set_is2d(1); // Make sure it is screen space text
       
            BBOX2D bbox = t2d->bbox2d(0, 0, 1);
            if ( bbox.contains(p) ){
               _sel = i;
               gp->set_color(_highlight_col);
            }

         }
      }
   }

   /*
   if (_disp && _sel != NUM_DIRS && _geoms[_sel])
      _geoms[_sel]->set_color(_nhighlight_col);
    
   _sel = NUM_DIRS;
   XYvec  v = (p - _d);
   if (v.length() > 0.05) {
      double a = atan2(v[0], v[1]) * (180.0 / M_PI);
      if (a < -45.0/2.0) 
         a += 360.0;
     
      for (int i=0; i<NUM_DIRS; i++)
         if (_geoms[i] && (a > i*45.0 - 45.0/2 && a < i*45.0 + 45.0/2))
            _sel = DIR(i);
    }
 
   if (_disp && _sel != NUM_DIRS && _geoms[_sel])
      _geoms[_sel]->set_color(_highlight_col);
   */
   return 0;
   
}

void MMENU::call(int sel){
   assert( (sel>=0) && (sel<_items.num()) );
   if( _items[sel]._f )
       _items[sel]._f->exec();
}

int
MMENU::up(
   CEvent  &e,
   State  *&
   )
{
//   DEVice_buttons *btns = (DEVice_buttons *)e._d;
   for(int i=0;i<_items.num();i++){
      if(GEOMptr gp = _items[i]._g)
         WORLD::undisplay(gp, false);
   }

   _view->unschedule(this);

   if(_sel>=0){
      call(_sel);
   }

   return 0;
}

int
MMENU::tick() {
   // bounding box of entire dialog box
   BBOX2D dbbox;

   mlib::XYpt curloc(_d);

   for(int i=0;i<_items.num();i++){
      GEOMptr gp(_items[i]._g);
      if(gp){
         WORLD::display(gp, false);

         if(i != _sel)
            gp->set_color(_nhighlight_col);

         // this is fairly absurd... why isn't bbox2d a virtual method?
         if (TEXT2D::isa(gp)) {
            TEXT2Dptr  t2d  = (TEXT2D *)&*gp;

            t2d->set_is2d(1); // Make sure it is screen space text
       
            BBOX2D bbox = t2d->bbox2d(0, 0, 1);
//            t2d->set_loc( bbox.min() );
            curloc[1] +=  bbox.min()[1] - bbox.max()[1];

            t2d->set_loc(curloc);
            dbbox += t2d->bbox2d(0,0,1);
         } else {
            Wpt curpt(curloc);
            Wtransf newloc(curpt);

            curloc[1] += -_MENU_OFFSET;
            gp->set_xform(newloc);
         }

         curloc[1] -= _MENU_OFFSET;

      }
   }

   double cur;
   for(int k=0;k<2;k++){
      if( (cur=dbbox.max()[k]) > 1.0 ){
         _d[k] -= (cur-1.0);
      }
      if( (cur=dbbox.min()[k]) < -1.0 ){
         _d[k] -= (cur+1.0);
      }
   }
   return 0;
}

#if 0
   const double threshold = 0.5;
   double dist = 75;
  
   if (!_disp && the_time() - _t0 > threshold) {
      for (int i=0; i<NUM_DIRS; i++) {
         if (_geoms[i]) {
            WORLD::display(_geoms[i], false);

            VEXEL vec(dist * sin(i * 2.0 * M_PI/ 8), 
                      dist * cos(i * 2.0 * M_PI/ 8));
            Wtransf newloc(Wpt(_d + vec));

            // Need to recenter it if it is text...
            if (TEXT2D::isa(_geoms[i])) {
               _geoms[i]->set_color(_nhighlight_col);
               TEXT2Dptr  t2d  = (TEXT2D *)&*_geoms[i];
               t2d->set_is2d(1); // Make sure it is screen space text
               t2d->set_loc(XYpt(newloc.origin()));

               BBOX2D bbox = t2d->bbox2d(0, 0, 1);
               XYvec shift;
               // Shift left for all but eastern (right) choices
                  shift = -XYvec(bbox.dim()[0]/2, 0);
//               }
               t2d->set_loc(bbox.min() + shift);
            } else {
               {
                  _geoms[i]->set_xform(newloc);
               }
            }
         }
      }
    
      _disp = true;
   }
  
   return 0;
}
#endif
