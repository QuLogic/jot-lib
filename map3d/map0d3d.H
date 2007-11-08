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
#ifndef MAP0D3D_H_IS_INCLUDED
#define MAP0D3D_H_IS_INCLUDED

#include "coord_frame.H"
#include "map3d.H"

/*****************************************************************
 * Map0D3D:
 *
 *   Base class for constant functions that map to a point in R^3.
 *****************************************************************/
class Map0D3D : public Map, public CoordFrame {
 public:

   //******** MANAGERS ********

   Map0D3D(CWvec& n=Wvec::null(),
           CWvec& t=Wvec::null()) :
      _n(n), _t(t) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Map0D3D", Map0D3D*, Map, CBnode*);

   //******** VIRTUAL METHODS ********

   virtual Wpt  map() const     = 0;

   virtual bool set_pt(CWpt& p) = 0;

   //******** LOCAL COORD FRAME ********

   // Subclasses that can provide a "normal" direction at the point
   // can answer 'true' to the query is_oriented().  The base class
   // offers the option of specifying a constant normal direction.
   virtual bool is_oriented()   const { return !norm().is_null(); }

   // The local unit-length normal:
   virtual Wvec norm() const { return _n; }
   virtual void set_norm(CWvec& n) { _n = n.normalized(); invalidate(); }
   virtual void set_tan (CWvec& t) { _t = t.normalized(); invalidate(); }

   Wline   line()  const { return Wline(map(), norm()); }
   Wplane  plane() const { return Wplane(map(), norm()); }

   virtual Wvec tan() const {
      Wvec ret = _t.orthogonalized(norm()).normalized();
      return ret.is_null() ? norm().perpend() : ret;
   }
   virtual Wvec binorm() const { return cross(norm(), tan()); }
   Wtransf frame()       const { return Wtransf(map(), tan(), binorm()); }

   //******** MAP VIRTUAL METHODS ********

   virtual void transform(CWtransf&, CMOD&);

   //******** CoordFrame VIRTUAL METHODS ********

   virtual Wpt      o() { return map(); }
   virtual Wvec     t() { return tan(); }
   virtual Wvec     n() { return norm(); }

 protected:
   Wvec _n;     // normal at this point
   Wvec _t;     // tangent vector
};
typedef const Map0D3D CMap0D3D;

/*****************************************************************
 * WptMap:
 *
 *   Constant function that maps to a point in R^3.
 *****************************************************************/
class WptMap : public Map0D3D {
 public:

   //******** MANAGERS ********

   WptMap(CWpt& p, CWvec& n=Wvec::null(), CWvec& t=Wvec::null()) :
      Map0D3D(n,t),
      _p(p) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("WptMap", WptMap*, Map0D3D, CBnode*);

   //******** VIRTUAL METHODS ********

   virtual Wpt  map() const   { return _p; }
   
   virtual bool set_pt(CWpt& p);

   //******** MAP VIRTUAL METHODS ********

   virtual bool can_transform()         const    { return true; }
   virtual void transform(CWtransf& xf, CMOD& m);

   //******** Bnode VIRTUAL METHODS ********

   // No inputs. The buck stops here.
   virtual Bnode_list inputs()  const { return Bnode_list(); }

 protected:
   
   Wpt  _p;     // the point
};

/*****************************************************************
 * OffsetMap:
 *
 *   Returns a point relative to a given coord frame.
 *
 *   XXX - not tested yet.
 *
 *****************************************************************/
class OffsetMap : public Map0D3D {
 public:

   //******** MANAGERS ********

   OffsetMap(Map0D3D* m, CWpt& loc = Wpt::Origin()) :
      _map(m), _loc(loc) {
      hookup();
   }

   virtual ~OffsetMap() { unhook(); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("OffsetMap", OffsetMap*, Map0D3D, CBnode*);

   //******** VIRTUAL METHODS ********

   virtual Wpt  map() const   { return _map->xf() * _loc; }
   
   virtual bool set_pt(CWpt& p) {
      update();
      _loc = _map->xf().inverse() * p;
      outputs().invalidate();
      return true;
   }

   //******** MAP VIRTUAL METHODS ********

   virtual Wvec norm()  const { return _map->n(); }
   virtual Wvec tan()   const { return _map->t(); }

   // XXX - maybe fix this if needed...
   virtual bool can_transform()                 const   { return false; }
   virtual void transform(CWtransf&, CMOD&)             {}

   //******** Bnode VIRTUAL METHODS ********

   virtual Bnode_list inputs()  const { return Bnode_list(_map); }

 protected:

   Map0D3D*     _map;   // internal map0D3D that provides coord frame
   Wpt          _loc;   // local coords wrt frame
};

/*****************************************************************
 * CurvePtMap:
 *
 *   Constant function that maps to a point on a curve in R^3.
 *****************************************************************/
class CurvePtMap : public Map0D3D {
 public:

   //******** MANAGERS ********

   CurvePtMap(Map1D3D* curve, double t);
   virtual ~CurvePtMap();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("CurvePtMap", CurvePtMap*, Map0D3D, CBnode*);

   //******** ACCESSORS ********

   Map1D3D* curve()     const { return _curve; }
   double       t()     const { return _t; }

   //******** VIRTUAL METHODS ********

   virtual Wpt map() const;
   
   virtual bool set_pt(CWpt& p);

   //******** LOCAL COORD FRAME ********

   // The local unit-length normal:
   virtual Wvec norm()  const;
   virtual void set_norm(CWvec&) {
      // XXX - just need to pass the norm on to the curve...
      err_msg("CurvePtMap::set_norm: not implemented");
   }

   // direction along the curve:
   virtual Wvec tan() const;

   //******** Bnode VIRTUAL METHODS ********

   // The defining curve is the input:
   virtual Bnode_list inputs()  const;

 protected:
   
   Map1D3D*  _curve;    // Curve we cling to
   double    _t;        // Parameter along the curve
};

/*****************************************************************
 * SurfacePtMap:
 *
 *   Constant function that maps to a point on a surface in R^3.
 *****************************************************************/
#define CSurfacePtMap const SurfacePtMap
class SurfacePtMap : public Map0D3D {
 public:

   //******** MANAGERS ********

   SurfacePtMap(Map2D3D* surf, CUVpt& uv);
   virtual ~SurfacePtMap();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SurfacePtMap", SurfacePtMap*, Map0D3D, CBnode*);

   //******** ACCESSORS ********

   Map2D3D* surface()   const { return _surf; }
   CUVpt&   uv()        const { return _uv; }

   //******** VIRTUAL METHODS ********

   virtual Wpt map() const;

   virtual bool set_pt(CWpt& p);

   //******** LOCAL COORD FRAME ********

   // The local unit-length normal:
   virtual Wvec norm()  const;

   virtual void set_norm(CWvec&) {
      // XXX - there is no way to set the normal for this map
      err_msg("SurfacePtMap::set_norm: can't be done");
   }

   // direction of partial derivative du:
   virtual Wvec tan() const;

   //******** Bnode METHODS ********

   virtual Bnode_list inputs()  const;

 protected:
   Map2D3D*     _surf;  // "Surface" we cling to
   UVpt         _uv;    // our uv coords on the surface
};

#endif // MAP0D3D_H_IS_INCLUDED

// end of file map0d3d.H
