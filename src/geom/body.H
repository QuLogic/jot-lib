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
#ifndef BODY_JOT_H
#define BODY_JOT_H

#include "disp/bbox.H"
#include "disp/ref_img_client.H"
#include "mlib/point3i.H"
#include "mlib/points.H"
#include "net/data_item.H"
#include "std/support.H"
#include "mod.H"

#define CMVEC2 mlib::CXYvec
#define MVEC2   mlib::XYvec

class VIEWptr;
#define CVIEWptr const VIEWptr

class TEXTURE;

class GEOM;

#define CFACElist const FACElist
class FACElist : public ARRAY<mlib::Point3i> {
   public:
      FACElist(int num=16) : ARRAY<mlib::Point3i>(num) {}
};

#define CBODYptr const BODYptr
MAKE_PTR_BASEC(BODY);
class BODY : public REFcounter,
             public DATA_ITEM,
             public RefImageClient {
   private:
      static mlib::CXYpt_list    _dummy;
   protected:
      int           _tris;
      BBOX          _bb;
      mlib::EDGElist      _edges;

     virtual BODYptr create_cube()     {return 0;}
     virtual BODYptr create_cone()     {return 0;}
     virtual BODYptr create_cylinder() {return 0;}
     virtual BODYptr create_sphere()   {return 0;}
     
     virtual BODYptr create_torus     (double        /*radius*/) {return 0;}
     virtual BODYptr create_trunc_cone(double        /*radius */) {return 0;}
     virtual BODYptr create_revolve   (mlib::CWpt_list &/*profile*/,
                                       BODYptr       &/*ob*/)
        {return 0;}
     virtual BODYptr create_extrusion(mlib::CWpt_list &/*profile*/,
                                      BODYptr       &/*ob*/)
        {return 0;}
     virtual BODYptr create_extrusion(mlib::CWpt_list &/*profile*/,
                                      mlib::CWpt_list &/*path*/,
                                      BODYptr       &/*ob*/)
        {return 0;}
     virtual BODYptr create_trunc_pyr(CMVEC2        &/*taper*/,
                                      CMVEC2        &/*shear*/)
        {return 0;}
     static  BODY   *_factory;
     static  str_ptr _base_name;
     static  TAGlist _body_tags;
   public:
	     BODY() : _tris(0) {}
	     virtual ~BODY() {}

     virtual CTAGlist&tags()      const   { return _body_tags; }

     virtual BODYptr  copy(int y=1) const = 0;
     virtual double   nearest(mlib::CWtransf&,mlib::CWpt&,double /*nearest*/)
      {return HUGE;}
     virtual int      intersect(RAYhit &r, mlib::CWtransf &xf,
                                mlib::Wpt &near, mlib::Wvec &n,
                                double &d, double &d2d, 
                                mlib::XYpt &uvc) const = 0;

   int intersect(RAYhit &r, mlib::CWtransf &xf) const {
      mlib::Wpt p;
      mlib::Wvec n;
      double d=0, d2d=0;
      mlib::XYpt xy;
      return intersect(r, xf, p, n, d, d2d, xy);
   }
   int intersect(RAYhit &r) const {
      mlib::Wtransf xf;
      return intersect(r, xf);
   }
     // transform body by xform
     virtual void     transform(mlib::CWtransf &xform, CMOD&) = 0;
     virtual BODY    *subtract (BODYptr &subtractor) = 0;   // CSG subtract
     virtual BODY    *intersect(BODYptr &intersector)= 0;  // CSG intersect
     virtual BODY    *combine  (BODYptr &unionor    )= 0;  // CSG union

   virtual mlib::CEDGElist     &body_edges() = 0;
     // Not necessarily the vertices of the triangulation of the BODY
     virtual mlib::CWpt_list     &vertices() = 0;
     virtual void           set_vertices(mlib::CWpt_list &) = 0;
     // Should *not* be used for rendering, but just for translation
     // Subclass should redefine this method to return a list of vertices and
     // vertex indices that describes this BODY
     virtual void triangulate(mlib::Wpt_list &verts, FACElist &faces)  {
        verts.clear(); faces.clear();
     }
     virtual void changed() = 0;

     virtual void set_geom(GEOM*) {}

     //!METHS: Texture stuff
     virtual int         uv_able      () const {return false;}
     virtual int         can_set_uv   () const {return false;}
     virtual void        set_texcoords(mlib::CXYpt_list &) {}
     virtual mlib::CXYpt_list &get_texcoords() { return _dummy;}

     //!METHS: Static factory methods
     static BODYptr cube()     {return _factory->create_cube();}
     static BODYptr cone()     {return _factory->create_cone();}
     static BODYptr cylinder() {return _factory->create_cylinder();}
     static BODYptr sphere()   {return _factory->create_sphere();}
     
     static BODYptr torus     (double         radius)
            {return _factory->create_torus(radius);}
     static BODYptr trunc_cone(double         radius)
            {return _factory->create_trunc_cone(radius);}
     static BODYptr revolve   (mlib::CWpt_list &profile, BODYptr &ob)
            {return _factory->create_revolve(profile, ob);}
     static BODYptr extrusion (mlib::CWpt_list &profile, BODYptr &ob)
            {return _factory->create_extrusion(profile, ob);}
     static BODYptr extrusion (mlib::CWpt_list &profile, mlib::CWpt_list &path,
                               BODYptr &ob)
            {return _factory->create_extrusion(profile, path, ob);}
     static BODYptr trunc_pyr (CMVEC2        &taper,    CMVEC2  &shear)
            {return _factory->create_trunc_pyr(taper, shear);}
     virtual BODYptr body_copy(int y = 1) {return copy(y);}
     static int set_factory(BODY *b) {_factory = b; return _factory != 0;}

     virtual CBBOX &get_bb() = 0;
     bool bb_valid() const { return _bb.valid();}
                


     static str_ptr base_name() {return _base_name;}

     virtual DATA_ITEM  *dup()                 const = 0;

     DEFINE_RTTI_METHODS2("BODY", DATA_ITEM, CDATA_ITEM*);
};
#endif
