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
/***************************************************************************
proxy_stroke.H

    ProxyStroke   
***************************************************************************/
#ifndef _PROXY_STROKE_H_IS_INCLUDED_
#define _PROXY_STROKE_H_IS_INCLUDED_

#include "std/config.H"
#include "geom/gl_view.H"
#include "stroke/base_stroke.H"
#include "mesh/patch.H"

// *****************************************************************
// * ProxyStroke
// *****************************************************************
class ProxySurface;
class HatchingTexture;
class Bface;

#define CProxyStroke ProxyStroke const
class ProxyStroke:  public BaseStroke {
 
 public:   
   ProxyStroke();
   virtual ~ProxyStroke() {}   
      
   // ******** BaseStroke METHODS ******** //   
   virtual BaseStroke*  copy() const;   
   virtual void         copy(CProxyStroke& v); 
   virtual void         copy(CBaseStroke& v) { BaseStroke::copy(v); }
   virtual bool         check_vert_visibility(CBaseStrokeVertex &v);   
  
   void add(mlib::CNDCZpt& pt, double width, double alpha, bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      v._width = (float)width;
      v._alpha = (float)alpha;
      set_dirty();
   }
   virtual int draw(CVIEWptr& v);
   void set_patch(Patch* p) { _p = p; }
   virtual void draw_start();
   virtual void draw_end();

   // ******** DATA_ITEM METHODS ******** //
   DEFINE_RTTI_METHODS2("ProxyStroke", BaseStroke, CDATA_ITEM *);
   virtual DATA_ITEM*   dup() const             { return copy(); }
   static bool taggle_vis;
 private:
   Patch* _p;

	
};
#define CProxyUVStroke ProxyUVStroke const
class ProxyUVStroke {
public:   
   ProxyUVStroke(CUVpt_list& pts, ProxySurface* ps, BaseStroke* proto);
   virtual ~ProxyUVStroke() {}
   void draw_start();
   void draw(CVIEWptr& v);
   void draw_end();
   void	stroke_setup(Bface* face, bool selected);  
   void  set_ps(ProxySurface* ps)     { _ps = ps; }    
private:
   UVpt_list      _uv_pts;
   //NDCpt_list     _pts;
   ProxyStroke*   _stroke;
   BaseStroke*    _proto;
   ProxySurface*  _ps;   

};

#endif
