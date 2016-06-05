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
#ifndef MAP_3D_H_IS_INCUDED
#define MAP_3D_H_IS_INCUDED

#include "mlib/points.H"
#include "bnode.H"

using namespace mlib;

class Map0D3D;  // Map describing a point in R^3
class Map1D3D;  // Map describing a curve in R^3
class Map2D3D;  // Map describing a surface in R^3
/*****************************************************************
 * Map:
 *
 *      Base class for mapping functions from R^k to R^n.
 *****************************************************************/
class Map : public Bnode {
 public:

   //******** MANAGERS ********

   Map() {}
   virtual ~Map() {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Map", Map*, Bnode, CBnode*);

   // Some maps can be transformed by multiplying by a 4x4 matrix:
   virtual bool can_transform() const { return false; }

   // Each subclass of Map that implements transform() should test
   // whether _mod.current() is false; if so it should call
   // Map::transform() and also apply the transform.  Map::transform()
   // prints some optional debug stuff and updates the MOD sequence
   // number to prevent the same transform from being applied twice.
   virtual void transform(mlib::CWtransf&, CMOD&);

 protected:

   //******** Bnode VIRTUAL METHODS ********

   // Classes that cache info should over-ride this, to recompute
   // cached info after their inputs are updated. Since many Map
   // subclasses don't cache anything, this is here for
   // convenience:
   virtual void recompute() {}
};

/*****************************************************************
 * WrapCoord:
 *
 *  Simple class for doing calculations with coordinates that
 *  wrap around; e.g. .9 + .2 = .1
 *
 *  Initial implementation is hard-coded for period 1.
 *****************************************************************/
class WrapCoord {
 public:

   // Normalize the given parameter to an equivalent value
   // in the range [0,1):
   static double N(double t) { return t - floor(t); }

   // Normalize the given delta to an equivalent delta in
   // the range [-0.5, 0.5):
   static double NC(double d) { return N(d + 0.5) - 0.5; }

   // add one value to another, possible wrapping around
   double add(double t1, double t2) const { return N(t1 + t2); }

   // Return the weighted average between the 2 values.
   // The weight is the fraction of the way from t1 to t2:
   double interp(double t1, double t2, double w) const {
      return add(t1, NC(t2 - t1)*w);
   }
};

#endif // MAP_3D_H_IS_INCUDED

// end of file map_3d.H
