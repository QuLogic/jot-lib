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
 * base_vis_ref_image.H
 *****************************************************************/
#ifndef _JOT_MESH_BASE_VIS_REF_IMAGE_H
#define _JOT_MESH_BASE_VIS_REF_IMAGE_H

#include "disp/view.H"

using namespace mlib;


class Bsimplex;
class Bface;
class BaseVisRefImage;
/*****************************************************************
 * BaseVisRefImageFactory:
 *
 *     class that can produce a usable BaseVisRefImage as needed.
 *     (i.e., it produces a derived type rather than an instance
 *     of the pure virtual base class BaseVisRefImage).
 *****************************************************************/
class BaseVisRefImageFactory {
 public:
   // nothing to construct
   virtual ~BaseVisRefImageFactory() {}

   // raison d'etre:
   virtual BaseVisRefImage* produce(CVIEWptr&) = 0;
};

/**********************************************************************
 * BaseVisRefImage
 *
 *    An abstract base class that describes the functionality needed from
 *    a VisRefImage
 **********************************************************************/
class BaseVisRefImage {
 public:
   virtual ~BaseVisRefImage() {}

   // No public constructor. Instead, use the following to lookup the
   // BaseVisRefImage associated w/ a view.  (If the view could know about
   // this type, we would probably just store the it on the view.)
   static BaseVisRefImage* lookup(CVIEWptr& v);

   virtual void      vis_update() = 0;
   virtual Bsimplex* vis_simplex(CNDCpt& ndc) const  = 0;
   virtual Bface*    vis_intersect(CNDCpt& ndc, mlib::Wpt& obj_pt) const = 0;

   virtual void debug(CNDCpt& p) const {}

 protected:

   // factory for producing usable instance of pure virtual base class
   // BaseVisRefImage:
   static  BaseVisRefImageFactory* _factory;

   // associates a BaseVisRefImage with a view:
   static HASH _hash;
};

#endif // _JOT_MESH_BASE_VIS_REF_IMAGE_H

// end of file base_ref_image.H
