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
#include "disp/ray.H"

using namespace mlib;

RAYhit::RAYhit(
   CXYpt     &p
   ) : _s(false),
       _is_surf(0),
       _d(-1),
       _d_2d(-1),
       _g(0),
       _visibility(1),
       _appear(0),
       _tolerance(12),
       _from_camera(1)
{
   _p = Wpt (p);
   _v = Wvec(p);
}

int
RAYhit::test(
   double    d, 
   int       is_surface,
   double    d_2d
   )
{
   // keep the ray intersection if :
   // 1) nothing has been intersected yet *and* we're either a
   //      a) surface 
   //      b) 2d object that's within the picking tolerance
   // 2) we're a 2D object that is :
   //      a) within the picking tolerance
   //      b) closer than any other 2D object
   // 3) we're a 3D object and :
   //      a) no 2D objects have been picked yet
   //      b) we're closer than any other surface 
   if ((_s == false && (is_surface || d_2d < _tolerance)) || 
       (!is_surface && (d_2d < _tolerance) && (_is_surf || d_2d < _d_2d)) ||
       ( is_surface && d < _d && _is_surf))
      return 1;
   return 0;
}

void    
RAYhit::check(
   double    d, 
   int       is_surface,
   double    d_2d,
   CGELptr  &g, 
   CWvec    &n,
   CWpt     &nearpt,
   CWpt     &surfl,
   APPEAR   *app,
   CXYpt    &tex_coord
   )
{
   if (test(d, is_surface, d_2d)) {
      _s       = true; 
      _d       = d;
      _d_2d    = is_surface ? _d_2d : d_2d;
      _is_surf = is_surface;
      _n       = n; 
      _nearpt  = nearpt;
      _g       = g;
      _surfl   = surfl;
      _uv      = tex_coord;
      _appear  = app;
   } 
}

RAYhit    
RAYhit::invert(
   CWpt &pt
   ) const
{
   Wvec v((point() - pt).normalized());
   return RAYhit(pt + v*1e-5, v);
}



XYpt
RAYhit::screen_point() const
{
   return XYpt(_p + _v);
}


void
RAYhit::clear(void)
{
   _d      = 0;
   _d_2d   = -1;
   _g      = 0;
   _s      = false;
   _appear = 0;
}

RAYnear::RAYnear(
   CXYpt     &p
   ) : _s(false), _d_for_geom(-1)
{
   _p = Wpt (p);
   _v = Wvec(p);
}

void    
RAYnear::check(
   double    d,
   CGELptr &g
   )
{
   if (_s == false ) { 
      _s = true; 
      _d = d; 
      if (_d_for_geom == -1 || d < _d_for_geom)
         _g = g; 
   } else if (d <= _d) {
      _d = d;
      if (_d_for_geom == -1 || d < _d_for_geom - 1e-7)
         _g = g; 
   }
}

void    
RAYnear::check_geom(
   double    d,
   CGELptr &g
   )
{
   if ((_d_for_geom == -1 && d <= _d+1e-7) ||
       (_d_for_geom != -1 && d <= _d_for_geom)) {
      _d_for_geom = d;
      _g     = g; 
   }
}


void
RAYnear::clear(void)
{
   _d_for_geom = -1;
   _g = 0;
   _s = false;
}
