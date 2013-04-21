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
#ifndef RAY_H_IS_INCLUDED
#define RAY_H_IS_INCLUDED

#include "std/support.H"
#include "mlib/points.H"
#include "disp/gel.H"

using namespace mlib;

class APPEAR;
#define CRAYhit const RAYhit
class RAYhit {
  protected:
   int       _s;
   Wpt       _p;
   Wvec      _v;
   int       _is_surf;
   double    _d;
   double    _d_2d;
   Wpt       _nearpt;
   GELptr    _g;
   Wvec      _n;
   Wpt       _surfl;
   int       _uv_valid;
   int       _visibility;
   XYpt      _uv;
   APPEAR   *_appear;
   int       _tolerance;
   int       _from_camera;

  public:
   RAYhit(CWpt  &p, CWvec &v):_s(false), _p(p), _v(v.normalized()),
                              _is_surf(0),
                              _d(-1),_d_2d(-1), _uv_valid(0), _visibility(1),
                              _appear(0), _tolerance(12),_from_camera(0) {}
   RAYhit(CWpt  &a, CWpt  &b):_s(false), _p(a), _v((b-a).normalized()),
                              _is_surf(0),
                              _d(-1),_d_2d(-1), _uv_valid(0), _visibility(1),
                              _appear(0), _tolerance(12),_from_camera(0) {}
   RAYhit(CXYpt &p);
   virtual ~RAYhit() {}

   RAYhit    copy        () const         { return RAYhit(point(),vec()); }
   int       success     () const         { return _s; }
   Wpt       point       () const         { return _p; }
   Wvec      vec         () const         { return _v; }
   Wline     line        () const         { return Wline(_p, _v); }
   XYpt      screen_point() const;
   int       from_camera () const         { return _from_camera; }
   RAYhit    invert      (CWpt &) const;

   // These make sense iff _s is true
   double    dist        () const         { return _d; }
   Wplane    plane       () const         { return Wplane(surf(), norm()); }
   Wpt       surf        () const         { return _is_surf ? _p + _d*_v : _nearpt; } 
   Wpt       surfl       () const         { return _surfl; } 
   Wvec      norm        () const         { return _n; }
   CGELptr  &geom        () const         { return _g; }
   CXYpt    &uv          () const         { return _uv;}
   APPEAR   *appear      () const         { return _appear; }
   int       get_vis     () const         { return _visibility; }
   int       get_tol     () const         { return _tolerance; }

   void      set_vis     (int  vis)       { _visibility = vis; }
   void      set_tol     (int  pixels)    { _tolerance = pixels; }
   void      set_norm    (CWvec    &n)    { _n = n; }
   void      set_geom    (CGELptr  &g)    { _g = g;}
   void      set_uv      (CXYpt    &uv)   { _uv = uv;}

   static  STAT_STR_RET static_name()      { RET_STAT_STR("RAYhit"); }
   virtual STAT_STR_RET class_name () const{ return static_name(); }
   virtual int          is_of_type(Cstr_ptr &t)const { return IS(t); }
   static  int          isa       (CRAYhit &r)       { return ISA((&r)); }

   void      clear       (void);
   int       test        (double, int, double);
   virtual void check    (double, int, double, CGELptr &, CWvec &,
                          CWpt &, CWpt &, APPEAR *a, CXYpt &);
};

#define CRAYnear const RAYnear
class RAYnear {
  protected:
   int     _s;
   Wpt     _p;
   Wvec    _v;
   double  _d;
   double  _d_for_geom;
   GELptr  _g;

  public:
   RAYnear(CWpt &p, CWvec &v):_s(false), _p(p), _v(v.normalized()),
                                     _d_for_geom(-1) {}
   RAYnear(CWpt &a, CWpt &b):_s(false), _p(a), _v((b-a).normalized()),
                                     _d_for_geom(-1) {}
   RAYnear(CXYpt    &p);

   int     success     () const                  { return _s; }
   Wpt     point       () const                  { return _p; }
   Wvec    vec         () const                  { return _v; }
   double  dist        () const                  { return _d; }
   CGELptr&geom        () const                  { return _g; }

   void    set_geom    (CGELptr &g)              { _g = g; }

   void    clear       (void);
   void    check       (double d, CGELptr &g);
   void    check_geom  (double d, CGELptr &g);
};

inline int operator == (CRAYhit  &r, int b)  { return r.success() == b; }
inline int operator == (CRAYnear &r, int b)  { return r.success() == b; }

template <class T>
inline T* 
ray_geom(CRAYhit& ray, T* init) 
{
   return T::upcast(ray.geom());
}

template <class T>
inline T* 
ray_geom(CRAYnear& ray, T* init) 
{
   return T::upcast(ray.geom());
}

/*****************************************************************
 * GEL_list:
 *****************************************************************/
template <class T>
inline RAYhit& 
GEL_list<T>::intersect(RAYhit& r, CWtransf& m) const 
{
   for (int i=0; i<num(); i++)
      (*this)[i]->intersect(r, m);
   return r;
}

#endif // RAY_H_IS_INCLUDED

