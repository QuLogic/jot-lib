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
#include "map2d3d.H"

using namespace mlib;

/*****************************************************************
 * Map:
 *****************************************************************/
void 
Map::transform(CWtransf&, CMOD& m)  
{
   // Each subclass of Map that implements transform() should
   // test whether _mod.current() is false; if so it should call
   // Map::transform() and also apply the transform.
   // Map::transform() prints some optional debug stuff and
   // updates the MOD sequence number to prevent the same
   // tranform from being applied twice.

   static bool debug = Config::get_var_bool("DEBUG_BNODE_XFORM");

   err_adv(debug, "%s::transform: mod %d, incoming mod: %d",
           **class_name(), _mod.val(), m.val());

   if (!can_transform())
      cerr << class_name() << "::transform: not implemented" << endl;

   // Update the MOD sequence number:
   _mod = m;

   // Notify dependents...
   invalidate();
}

/*****************************************************************
 * Map0D3D:
 *****************************************************************/
void 
Map0D3D::transform(CWtransf& xf, CMOD& m)  
{
   if (!_mod.current()) {
      set_norm(xf * _n);
      set_norm(xf * _t);
      Map::transform(xf, m);
   }
}

/*****************************************************************
 * WptMap:
 *****************************************************************/
bool
WptMap::set_pt(CWpt& p)
{
   // Do nothing of it's the same
   if (p == _p)
      return true;

   // Record the new point
   _p = p;

   invalidate(); // tell dependents

   return true;
}

void 
WptMap::transform(CWtransf& xf, CMOD& m) 
{
   if (!_mod.current()) {
      _p = xf * _p;
      Map0D3D::transform(xf, m);
   }
}

/*****************************************************************
 * CurvePtMap:
 *****************************************************************/
CurvePtMap::CurvePtMap(Map1D3D* curve, double t) :
   _curve(curve),
   _t(t)
{
   assert(_curve);
   hookup();
}

CurvePtMap::~CurvePtMap()
{
  unhook();
}

Wpt
CurvePtMap::map() const
{
   return _curve->map(_t);
}

bool 
CurvePtMap::set_pt(CWpt& p)
{
   if ( _curve->invert(p, _t, _t) ) {
      invalidate();
      return true;
   }
   err_msg("CurvePtMap::set_pt: failed to solve for new parameter t");
   return false;
}

Bnode_list
CurvePtMap::inputs() const
{
   // The defining curve is the input:

   return Bnode_list(_curve);
}

Wvec 
CurvePtMap::norm() const
{
   return _curve->norm(_t);
}

Wvec 
CurvePtMap::tan() const
{
   return _curve->tan(_t);
}

/*****************************************************************
 * SurfacePtMap:
 *****************************************************************/
SurfacePtMap::SurfacePtMap(Map2D3D* surf, CUVpt& uv) :
   _surf(surf),
   _uv(uv)
{
   assert(_surf);
   hookup();
}

SurfacePtMap::~SurfacePtMap()
{
   unhook();
}

Wpt
SurfacePtMap::map() const
{
   return _surf->map(_uv);
}

bool 
SurfacePtMap::set_pt(CWpt& p)
{
   if ( _surf->invert(p, _uv, _uv) ) {
      invalidate();
      return true;
   } 
   err_msg("SurfacePtMap::set_pt: failed to solve for new uv position");
   return false;
}

Bnode_list
SurfacePtMap::inputs() const
{
   // The defining surface is the input:

   return Bnode_list(_surf);
}

Wvec 
SurfacePtMap::norm() const
{
   return _surf->norm(_uv);
}

Wvec 
SurfacePtMap::tan() const
{
   return _surf->du(_uv).normalized();
}

/* end of file map0d3d.C */
