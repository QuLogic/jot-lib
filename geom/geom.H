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
#ifndef GEOM_H_IS_INCLUDED
#define GEOM_H_IS_INCLUDED

#include "disp/gel.H"
#include "disp/ref_img_drawer.H"
#include "mlib/points.H"
#include "net/data_item.H"
#include "net/stream.H"
#include "std/support.H"

#include "geom/body.H"
#include "geom/fsa.H"
#include "geom/appear.H"

// this defines a GEOMptr which inherits (sort of) from a
// GELptr.  Thus, GEOMptr's can be used wherever GELptr's are.
MAKE_PTR_SUBC(GEOM,GEL);

#ifdef WIN32
#undef min
#undef max
#endif

class Event;
typedef State_t<Event> State;
class RAYhit;
#define CRAYhit const RAYhit
class RAYnear;
#define CRAYnear const RAYnear
class CAM;
typedef REFptr<CAM> CAMptr;
#define CCAMptr const CAMptr
#define CEvent const Event
class VIEWptr;
#define CVIEWptr const VIEWptr
class GEOMptr;
#define CGEOMptr const GEOMptr

class GEOMlist;
typedef const GEOMlist CGEOMlist;

//----------------------------------------------
//
//  GEOM -
//     abstract class that defines a geometric
//  entity.   This class contains a data ptr to a
//  specific representation object.   The representation
//  object must minimally know how to draw itself to OGl 
//  or intersect a ray with itself when asked.
//
//  GEOM objects store color, transparency, and a transformation.
//
//  However, the default virtual method for drawing a GEOM
//  "draws" each attribute to OGl but does NOT draw the
//  BODY geometrys.   This is done in the MODELER subclass
//
//----------------------------------------------

class GEOM : public GEL, public APPEAR {
 public:  // this operator must come before any reference to GEOMptr
          // otherwise, the instantiation of GEOMptr, which uses the
          // << operator on GEOM * in an inline won't know about this
          // method.  This only is an issue with some compilers...
   friend  class GEOMlist;
   enum cons {
      NO_TRANS,
      TRANS_FREE,
      TRANS_LINE,
      TRANS_PLANE,
      SURFACE_SNAP,
      AXIS_ROTATE,
      SPHERE_ROTATE,
      SCREEN_TEXT,
      SCALE,
      SCALE_AXIS,
      LOOKUP,
      SCREEN_WIDGET
   };

 public: 
   //******** MANAGERS ********
   GEOM(CGEOMptr &o, Cstr_ptr &);  // copy constructor
   GEOM(Cstr_ptr&, CBODYptr& b = 0);
   GEOM();
   virtual ~GEOM();

   DEFINE_RTTI_METHODS3("GEOM", GEOM*, GEL, CDATA_ITEM *);

   virtual CTAGlist &tags() const;

   //******** DATA ACCESSORS ********
   Cstr_ptr &name()                     const { return _name; }
   virtual BODYptr   body()             const { return _body; }
   virtual void set_body(CBODYptr& b);

   // xform stored for this object
   // (possibly relative to some other GEOM):
   virtual mlib::CWtransf &xform     () const { return _xform; }
   virtual mlib::CWtransf &inv_xform () const;

   // transforms between object space and world space:
   virtual mlib::Wtransf obj_to_world() const { return xform(); }
   virtual mlib::Wtransf world_to_obj() const { return inv_xform(); }

   //******** GEOMETRY OPS ********
   virtual void write_xform  (mlib::CWtransf&, mlib::CWtransf&, CMOD&);
   virtual void mult_by      (mlib::CWtransf&);

   //******** ATTRIBUTES     ********
   virtual void set_color    (CCOLOR   &);
   virtual void unset_color  ()          ;
   virtual void set_texture  (CTEXTUREptr& t);
   virtual void unset_texture();
   virtual void set_xform    (mlib::CWtransf &x) { mult_by(x*inv_xform()); }
   void set_name     (Cstr_ptr &n) { _name       = n; }
   virtual void set_pickable (int f)       { PICKABLE.set(this,f);
      if(!f) set_transp(.8);
      else unset_transp(); }
   virtual bool needs_blend  () const { return has_transp() && transp() != 1;}

   /* ---  print and formatting functions --- */
   virtual ostream &print     (ostream  &)  const;
   virtual void     get_name  (TAGformat &d)       { *d >> _name; }

   // XXX - Loading from file can occur as an update
   // to a GEOM and this must be done in a way
   // that distributes the notifications...!
   virtual void     get_xf    (TAGformat &d) {
      mlib::Wtransf x; *d >> x; mult_by(x*inv_xform());
   }

   virtual void     put_name  (TAGformat &d) const { d.id() << _name; }
   virtual void     put_xf    (TAGformat &d) const { d.id() << _xform;}

   //******** ADDITIONAL GEOM INTERFACE ********
   virtual int      lookup_constraint(CEvent &, State *&, cons &) { return 0; }
   virtual int      interactive      (CEvent &, State *&, RAYhit * =0) const
      { return 0; }
   virtual BBOX     bbox             (int i=0)      const;

   //******** STATICS ********

   static  GEOM*        null;

   //******** HALOING ********

   // static interface:
   static bool do_halo_view()           { return _do_halo_view; }
   static bool do_halo_ref ()           { return _do_halo_ref; }
   static void set_do_halo_view(bool b) { _do_halo_view = b; }
   static void set_do_halo_ref (bool b) { _do_halo_ref  = b; }
   static bool toggle_do_halo_view() {
      return (_do_halo_view = !_do_halo_view);
   }
   static bool toggle_do_halo_ref () {
      return (_do_halo_ref  = !_do_halo_ref);
   }

   // virtual method for drawing a halo around the object:
   virtual int draw_halo(CVIEWptr& = VIEW::peek()) const { return 0; }

   // GEL virtual method:
   virtual bool is_3D()       const { return bbox().valid(); }
   virtual bool can_do_halo() const { return _do_halo; }
   virtual void set_can_do_halo(bool can) { _do_halo = can; }

   //******** CAMERA INTERACTION ********

   // Called by camera interactor when user does double tap
   // (with camera button). This method finds the GEOM (if any)
   // tapped on by the user, and calls a virtual method of that
   // GEOM to animate the camera:
   static  bool find_cam_focus(CVIEWptr& view, mlib::CXYpt& x);
 protected:
   
   bool _do_halo;

   // Called in find_cam_focus() for the tapped-on GEOM to
   // animate the camera. The method is virtual so special
   // objects may animate the camera in special ways; the base
   // class offers a simple default camera animation intended
   // to give a better view of the tapped-on point:
   virtual bool   do_cam_focus(CVIEWptr& view, CRAYhit& r);
 public:

   //******** GEL VIRTUAL METHODS ********
   virtual RAYhit&
   intersect(RAYhit  &r,mlib::CWtransf&m=mlib::Identity,int uv=0)const;
   virtual RAYnear&
   nearest(RAYnear &r,mlib::CWtransf&m=mlib::Identity)         const;
   virtual bool cull(const VIEW *v)     const;

   //******** RefImageClient VIRTUAL METHODS ********
   virtual void request_ref_imgs() { if (ric()) ric()->request_ref_imgs(); }

   virtual int draw(CVIEWptr &v)    { return draw_img(RegularDrawer(v)); }
   virtual int draw_vis_ref()       { return draw_img(VisRefDrawer(),false); }
   virtual int draw_id_ref()        { return draw_img(IDRefDrawer() ,false); }
   virtual int draw_id_ref_pre1()   { return draw_img(IDPre1Drawer(),false); }
   virtual int draw_id_ref_pre2()   { return draw_img(IDPre2Drawer(),false); }
   virtual int draw_id_ref_pre3()   { return draw_img(IDPre3Drawer(),false); }
   virtual int draw_id_ref_pre4()   { return draw_img(IDPre4Drawer(),false); }
   virtual int draw_halo_ref()      { return draw_img(HaloRefDrawer(),false);}
   virtual int draw_color_ref(int i){ return draw_img(ColorRefDrawer(i));}
   virtual int draw_final(CVIEWptr &v);

 protected:
   // utility method used in various draw calls above:
   virtual int draw_img(const RefImgDrawer& r, bool enable_shading=true);

   // used in draw_img():
   virtual RefImageClient* ric() { return body(); }
 public:

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual GEOMptr dup(Cstr_ptr &n) const { return new GEOM((GEOM*)this,n);}
   virtual DATA_ITEM* dup() const { return new GEOM(_name); }

 protected:
   str_ptr              _name;
   BBOX                 _bbox;
   mlib::Wtransf        _xform;
   BODYptr              _body;
   bool                 _inv_xf_dirty;
   mlib::Wtransf        _inv_xf;
   int                  _bbox_id;

   //******** STATICS ********
   static  TAGlist*     _geom_tags;

   // flags that enable halos globally
   static bool     _do_halo_view; // draw halos in main view
   static bool     _do_halo_ref;  // draw halos in color ref image

   //******** PROTECTED METH0DS ********

   // polygon offset:
   virtual float po_factor() const { return 1.0; }
   virtual float po_units()  const { return 1.0; }
};

// for convenience, when your gel is a
// derived type that contains a body:
inline BODYptr 
gel_to_body(CGELptr &gel)
{
   GEOM* geom = GEOM::upcast(gel);
   return geom ? geom->body() : 0;
}

STDdstream  &operator>>(STDdstream &d, GEOMptr &p);


// ---------- define observers for GEOM operations

#include "geom/geom_obs.H"

// ---------- define hash variables for GEOM objects

class XformConstraint : public hashenum<GEOM::cons> {
 public :
   XformConstraint(Cstr_ptr &name, GEOM::cons cons, int net):
      hashenum<GEOM::cons>(name, cons, net) { }
   virtual str_ptr enum_to_str(GEOM::cons e) const { 
      switch (e) {
       case GEOM::TRANS_FREE    : return "trans_free";
       case GEOM::TRANS_LINE    : return "trans_line";
       case GEOM::TRANS_PLANE   : return "trans_plane";
       case GEOM::SURFACE_SNAP  : return "surface_snap";
       case GEOM::AXIS_ROTATE   : return "axis_rotate";
       case GEOM::SPHERE_ROTATE : return "sphere_rotate";
       case GEOM::SCREEN_TEXT   : return "screen_text";
       case GEOM::SCALE         : return "scale";
       case GEOM::SCALE_AXIS    : return "scale_axis";
       case GEOM::LOOKUP        : return "lookup";
       case GEOM::SCREEN_WIDGET : return "screen_widget";
       default                  : return "none";
      }
   }
   virtual GEOM::cons str_to_enum(Cstr_ptr &s) const { 
      if (s == "trans_free")    return GEOM::TRANS_FREE   ;
      else if (s == "trans_line")    return GEOM::TRANS_LINE   ;
      else if (s == "trans_plane")   return GEOM::TRANS_PLANE  ;
      else if (s == "surface_snap")  return GEOM::SURFACE_SNAP ;
      else if (s == "axis_rotate")   return GEOM::AXIS_ROTATE  ;
      else if (s == "sphere_rotate") return GEOM::SPHERE_ROTATE;
      else if (s == "screen_text")   return GEOM::SCREEN_TEXT  ;
      else if (s == "scale")         return GEOM::SCALE        ;
      else if (s == "scale_axis")    return GEOM::SCALE_AXIS   ;
      else if (s == "lookup")        return GEOM::LOOKUP       ;
      else if (s == "screen_widget") return GEOM::SCREEN_WIDGET;
      return GEOM::NO_TRANS;
   }
};

extern XformConstraint  CONSTRAINT;
extern hashvar<int>     XFORM_ON_BODY, HIGHLIGHTED, NO_CULL;

/*****************************************************************
 * GEOMlist:
 *
 *    Instantiation of GEL_list, for GEOMs.
 *****************************************************************/
class GEOMlist : public GEL_list<GEOMptr> {
 public :
   //******** MANAGERS ********
   GEOMlist(const GEL_list<GEOMptr>& l) : GEL_list<GEOMptr>(l) {}
   GEOMlist(int n=16)                   : GEL_list<GEOMptr>(n) {}
   GEOMlist(CGEOMptr& g) { add(g); }
   GEOMlist(GEOM*     g) { add(g); }

   void mult_by(CWtransf& xf) const {
      for (int i=0; i<num(); i++)
         (*this)[i]->mult_by(xf);
   }
};
typedef const GEOMlist CGEOMlist;

#endif // GEOM_H_IS_INCLUDED

// geom.H
