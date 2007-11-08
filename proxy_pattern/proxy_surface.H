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
/*****************************************************************
 * proxy_surface.H
 *****************************************************************/
#ifndef PROXYSURFACE_H_IS_INCLUDED
#define PROXYSURFACE_H_IS_INCLUDED

#include "mesh/patch.H"
#include "proxy_stroke.H"
#include "stroke/base_stroke.H"

#include <vector>
#include <map>

class ProxySurface {
 public:  
   static UVpt baseUVpt(Bface* f);
   bool baseUVpt(Bface* f, UVpt& uv);
   //******** MANAGERS ********
   ProxySurface(Patch* p=0);
   virtual ~ProxySurface();
   void     update_proxy_surface();
   CBMESHptr& proxy_mesh() const { return _proxy_mesh; }
   
   /// given a uv point will give a world point relative to the proxy mesh...
   Wpt      getWptfromUV(CUVpt& uv);
   UVpt     getUVfromNDC(CNDCpt& ndc);
   /// give neighbor face given a dicaction, 0-3 with 0 being down, going clockwise
   Bface*   neighbor_face(int dir, Bface* face);
   double   scale()          { return _scale; }
 
   PIXEL    get_o()          { return _o; }          
   PIXEL    get_u_o()        { return _u_o; }        
   PIXEL    get_v_o()        { return _v_o; }  
  
   void        set_patch(Patch* p);
   Patch*      patch() { return _patch; }

 protected:
   typedef map<int, CBvert*, less<int> > Col_Map;  // one column of the grid
   typedef map<int, Col_Map, less<int> > Grid_Map; // the whole grid of vertices
   Grid_Map               _grid_map;    // Look up table to do UV -> Bvert

   BMESHptr               _proxy_mesh;        // actual proxy mesh that lives in "2d" 
   Patch*                 _patch;       // mesh that holds the proxy surface

   // Vars to return globaly
   double                 _scale;       // keeps track of current scale factor

   // These are used when growing new faces, chaced values of the first quad, so that 
   // given a new UV point can figure out new location of the points
   PIXEL                  _o;
   PIXEL                  _u_o;        
   PIXEL                  _v_o;

   //******** UTILITIES ********
   void create_proxy_surface();
   void apply_xform    ();

   void grow_quad_from_edge(BMESH* m, EdgeStrip* boundery, int i);
   int  grow_proxy_surface();
   void trim_proxy_surface();
   void lod_update();

   bool is_inside_bounding_box(CBedge* e);
   
   //if vert does not exist creates one
   Bvert* vertFromUV(CUVpt& uv);

   void    remove_vert_grid(CUVpt& uv);
   void    put_vert_grid(CUVpt& uv, CBvert* bvert);

   //returns ether a vert of a null
   Bvert*  get_vert_grid(CUVpt& uv); 
   void    debug_grid();
};

/**********************************************************************
 * ProxyData:
 * Puts lebels on the faces so that when trimming we know which faces
 * we can delete...
 **********************************************************************/
class ProxyData : public SimplexData {
 public:
   ProxyData(Bsimplex* s, ProxySurface* p, bool marked) :
      SimplexData(uint(p), s),
      _marked(marked) {}

   //******** RUN TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("ProxyData", ProxyData*, SimplexData, CSimplexData*);

   //******** LOOKUP ********

   // return the data stored on a simplex:
   static ProxyData* lookup(CBsimplex* s, ProxySurface* p) {
      return (s && p) ? upcast(s->find_data(uint(p))) : 0;
   }

   // return the data stored on a simplex, and create it if needed:
   static ProxyData* get_data(Bface* v, ProxySurface* p, bool m) {
      assert(v && p);
      ProxyData* ret = lookup(v,p);
      if (!ret) {
         ret = new ProxyData(v, p, m);
      }
      return ret;
   }

   // return the pixel value stored on a vertex:
   static bool get_mark(Bface* v, ProxySurface* p) {
      ProxyData* pd = get_data(v,p, false);
      assert(pd);
      return pd->get_marked();
   }

   // set the pixel value for a vertex:
   static void set_mark(Bface* v, ProxySurface* p, bool m) {
      ProxyData* pd = get_data(v,p, m);
      assert(pd);
      pd->set_marked(m);
   }

   void     set_marked(bool m)                   { _marked = m;  }
   bool     get_marked()                 const   { return _marked; }

 protected:
   bool _marked;
};

/**********************************************************************
 * PixelsData:
 * Strores pixel locations of the verts...
 **********************************************************************/
class PixelsData : public SimplexData {
 public:
   PixelsData(Bsimplex* s, ProxySurface* p, PIXEL px) :
      SimplexData(uint(p), s),
      _pixel(px) {}

   //******** RUN TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("PixelsData", PixelsData*, SimplexData, CSimplexData*);

   //******** LOOKUP ********

   // return the data stored on a simplex:
   static PixelsData* lookup(CBsimplex* s, ProxySurface* p) {
      return (s && p) ? upcast(s->find_data(uint(p))) : 0;
   }

   // return the data stored on a simplex, and create it if needed:
   static PixelsData* get_data(Bvert* v, ProxySurface* p) {
      assert(v && p);
      PixelsData* ret = lookup(v,p);
      if (!ret) {
         ret = new PixelsData(v, p, v->pix());
      }
      return ret;
   }

   // return the pixel value stored on a vertex:
   static PIXEL get_pix(Bvert* v, ProxySurface* p) {
      PixelsData* pd = get_data(v,p);
      assert(pd);
      return pd->get_pixel();
   }

   // set the pixel value for a vertex:
   static void set_pix(Bvert* v, ProxySurface* p, CPIXEL& pix) {
      PixelsData* pd = get_data(v,p);
      assert(pd);
      pd->set_pixel(pix);
   }

   void     set_pixel(CPIXEL& pix)              { _pixel = pix;  }
   PIXEL    get_pixel()                 const   { return _pixel; }

 protected:
   PIXEL _pixel;
};

#endif // PROXYSURFACE_H_IS_INCLUDED

// end of file proxy_surface.H
