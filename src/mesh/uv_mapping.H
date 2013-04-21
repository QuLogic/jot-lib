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
#ifndef UV_MAPPING_H_HAS_BEEN_INCLUDED
#define UV_MAPPING_H_HAS_BEEN_INCLUDED

////////////////////////////////////////////
// UVMapping
////////////////////////////////////////////
//
// -Walks a mesh (starting from a seed) and
//  bins the faces within a contiguous uv
//  region into a fast lookup uv->wpt structure
// -The number of bins is fixed, but the uv
//  scaling varies depending upon the uv
//  extent of the region
// -Wrapping is u or v (along constant lines
//  in u or v) is detected and checked for
//  consistency with the u,v extrema of region
//
////////////////////////////////////////////

#include "bface.H"


/*****************************************************************
 * UVMapping
 *****************************************************************/

#define CUVMapping const UVMapping
class UVMapping {

 protected:

   /******** MEMBERS VARIABLES ********/

   ARRAY<ARRAY<Bface*>*>        _mapping;
   Bface*                       _seed_face;
   int                          _face_cnt;
   int                          _bin_cnt;
   int                          _entry_cnt;
   int                          _use_cnt;
   double                       _du;
   double                       _dv;
   double                       _min_u;
   double                       _min_v;
   double                       _max_u;
   double                       _max_v;
   double                       _span_u;
   double                       _span_v;
   bool                         _wrap_u;
   bool                         _wrap_v;
   bool                         _wrap_bad;
   unsigned char *              _virgin_debug_image;
   unsigned char *              _marked_debug_image;

 public:

   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

   UVMapping(Bface *f);
   ~UVMapping();

 protected:

   /******** STATIC MEMBERS METHODS ********/

   //Handy for isecting two line segments
   static bool intersect(CUVpt &pt1a, mlib::CUVpt &pt1b, mlib::CUVpt &pt2a, mlib::CUVpt &pt2b);

 public:
   /******** MEMBERS METHODS ********/

   Bface*                find_face(CUVpt &uv, mlib::Wvec &bc);

   void                        draw_debug();
   void                        clear_debug_image();

   void                        debug_dot(double u,double val, unsigned char r,unsigned char g,unsigned char b);

   //When a FreeHatchingGroup sets its map
   //it registers, and when it dies is unregisters
   //If nobody cares about a map, it goes away...

   void                        register_usage()                { _use_cnt++; }
   void                        unregister_usage()        { --_use_cnt; assert(_use_cnt>=0); if (!_use_cnt) delete this;}

   //Accessors

   inline bool                wrap_u()                                        { return _wrap_u; }
   inline bool                wrap_v()                                        { return _wrap_v; }

   inline double        span_u()                                        { return _span_u; }
   inline double        span_v()                                        { return _span_v; }

   inline double        min_u()                                        { return _min_u; }
   inline double        min_v()                                        { return _min_v; }

   inline double        max_u()                                        { return _max_u; }
   inline double        max_v()                                        { return _max_v; }

   Bface*                        seed_face()                                { return _seed_face; }

   //Interpolating method
   void                                interpolate(CUVpt &uv1, double frac1,mlib::CUVpt &uv2, double frac2,mlib::UVpt &uv);
   void                                apply_wrap(UVpt &uv);

 protected:
   /******** MEMBERS TYPES ********/

   typedef void (UVMapping::*rec_fun_t)(Bface *); 

   /******** MEMBERS METHODS ********/

   void                        compute_mapping(Bface *f);
   void                        compute_limits(Bface *f);
   void                        compute_wrapping(Bface *f);
   void                        compute_debug_image();
   void                        recurse_wrapping(Bface *f);
   void                        recurse(Bface *f, rec_fun_t fun);
   void                        add_face(Bface *f);
   void                        add_limit(Bface *f);

};

#endif  // UV_MAPPING_H_HAS_BEEN_INCLUDED

/* end of file uv_mapping.H */
