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
#include "disp/cam_focus.H"
#include "disp/ray.H"
#include "disp/view.H"
#include "geom/geom.H"
#include "geom/body.H"
#include "geom/gl_view.H"
#include "geom/gl_util.H"
#include "std/config.H"

#include "std/thread_mutex.H"

using namespace mlib;

BODY*       BODY::_factory = 0;
CXYpt_list  BODY::_dummy(0);

int         MOD::_TICK              = 0;
int         MOD::_START             = 0;
GEOM*       GEOM::null;
TAGlist*    GEOM::_geom_tags;

bool GEOM::_do_halo_view = Config::get_var_bool("DO_VIEW_HALOS",false);
bool GEOM::_do_halo_ref  = Config::get_var_bool("DO_REF_HALOS", false);


// Hash table fields
// MAKE_NET_HASHVAR is used so these values are networked
MAKE_NET_HASHVAR (XFORM_ON_BODY, int, 0);
MAKE_NET_HASHVAR (HIGHLIGHTED  , int, 0);
XformConstraint CONSTRAINT("CONSTRAINT", GEOM::TRANS_FREE, 1);

// See net/data_item.H for more information
static int gm=DECODER_ADD(GEOM);

STDdstream &
operator>>(STDdstream &ds, GEOMptr &p) 
{
   DATA_ITEM *d = DATA_ITEM::Decode(ds);
   if (d && GEOM::isa(d))
      p = (GEOM *)d;
   else {
      cerr << "operator >> Couldn't find GEOM in stream" << endl;
      p = 0;
   }
     
   return ds;
}

/* ---- geometric object interface ----- */
//
// These functions are place holders for intersect methods in GEOM subclasses
//
RAYnear &
GEOM::nearest(RAYnear &r, CWtransf &) const 
{
   return r;
}

void 
GEOM::set_body(CBODYptr& b)
{
   if (b == _body)
      return;
   if (_body)
      _body->set_geom(0);    // clear GEOM* in old body
   _body = b;
   if (_body)
      _body->set_geom(this); // set GEOM* in new body
}

CWtransf&
GEOM::inv_xform () const 
{
   if (_inv_xf_dirty) {
      GEOM *me = (GEOM*)this;
      me->_inv_xf=xform().inverse();
      me->_inv_xf_dirty = 0;
   }
   return _inv_xf;
}

bool
GEOM::find_cam_focus(CVIEWptr& view, mlib::CXYpt& x)
{
   static const bool debug = Config::get_var_bool("DEBUG_CAM_FOCUS",false);

   RAYhit r(view->intersect(x));
   GEOMptr geom = GEOM::upcast(r.geom());
   if (geom) {
      return geom->do_cam_focus(view, r);
   }
   // failed
   if (debug) {
      cerr << "GEOM::find_cam_focus: nothing to focus on" << endl;
   }
   return false;
}

bool
GEOM::do_cam_focus(CVIEWptr& view, CRAYhit& r)
{
   if (Config::get_var_bool("DEBUG_CAM_FOCUS",false))
      cerr << "GEOM::do_cam_focus (" << name() << ")"
           << endl;

   assert(r.geom() == this);

   CAMptr     cam (view->cam());
   CAMdataptr data(cam->data());

   // detect "silhouette" by doing ray test to the right and left
   // (in image space), to see if we hit background space on one
   // side but not the other:
   XYvec    D(VEXEL(25,0)); // left/right displacement
   RAYhit   sil_r(r.screen_point() + D);
   RAYhit   sil_l(r.screen_point() - D);
   intersect(sil_r);
   intersect(sil_l);

   // If angle between surface normal and world up vector
   // is over 12 degrees, and cheesy silhouette detection
   // returns true, do silhouette focus:
   if (rad2deg(r.norm().angle(Wvec::Y())) > 12.0 &&
       XOR(sil_r.success(), sil_l.success())) {
      Wvec sil_comp;
      if (sil_r.success())
         sil_comp = -data->right_v() * 6.0 * r.dist();
      else
         sil_comp =  data->right_v() * 6.0 * r.dist();

      Wvec v(sil_comp + r.norm() * 4.0 * r.dist());
      Wpt  newpos(r.surf() + r.dist() * v.normalized());
      newpos[1] = data->from()[1];
      newpos = r.surf() + (newpos - r.surf()).normalized() * r.dist();

      new CamFocus(view, newpos, r.surf(), newpos + Wvec::Y(), r.surf(),
                   data->width(), data->height());
   } else {
      // do normal focus:
      Wpt  center(r.surf());
      Wvec norm  (r.norm().normalized());
      if (norm * Wvec::Y() > 0.98)
         norm = -data->at_v();

      Wvec  off   (cross(Wvec::Y(),norm).normalized() * 3);
      Wvec  atv   (data->at_v() - Wvec::Y() * (data->at_v() * Wvec::Y()));
      Wvec  newvec(norm*6 + 4.0*Wvec::Y()); 
      if ((center + newvec + off - data->from()).length() > 
          (center + newvec - off - data->from()).length())
         off = newvec - off;
      if (data->persp())
         off = (newvec + off).normalized() * 
            (data->from()-Wline(data->from(),data->at()).project(center)).
            length();

      new CamFocus(view, center + off, center, center + off + Wvec::Y(), 
                   center, data->width(), data->height());
   }
   return true;
}

/* ---- GEOM constructors ----- */

GEOM::GEOM() :
   GEL(),
   _inv_xf_dirty(1),
   _bbox_id(-1) 
{
   _do_halo = false;
}

GEOM::GEOM(Cstr_ptr &n, CBODYptr& b) :
   GEL(),
   _name(n),
   _body(b),
   _inv_xf_dirty(1),
   _bbox_id(-1)
{
 _do_halo = false;
   if (_body)
      _body->set_geom(this);
}

GEOM::GEOM(
   CGEOMptr &o,
   Cstr_ptr &name
   ) :
   GEL(o),
   APPEAR(o),
   _name(name),
   _xform(o->_xform),
   _inv_xf_dirty(1),
   _bbox_id(-1)
{
   _do_halo = false;
   if (o && o->body()) {
      _body = (BODY*)o->body()->dup();
      assert(_body != 0);
      _body->set_geom(this);
      *_body = *o->body();
   }
}

GEOM::~GEOM()
{
}

CTAGlist &
GEOM::tags() const
{
   if (!_geom_tags) {
      _geom_tags = new TAGlist;
      *_geom_tags += GEL::tags();
      *_geom_tags += new TAG_meth<GEOM>("name",  &GEOM::put_name,
                                        &GEOM::get_name, 0);
      *_geom_tags += new TAG_meth<GEOM>("xform", &GEOM::put_xf,
                                        &GEOM::get_xf, 0);
      *_geom_tags += new TAG_meth<GEOM>("color", &GEOM::put_color,
                                        &GEOM::get_color);
      *_geom_tags += new TAG_meth<GEOM>("texture",&GEOM::put_texture,
                                        &GEOM::get_texture,1);
      *_geom_tags += new TAG_meth<GEOM>("transp",&GEOM::put_transp,
                                        &GEOM::get_transp);
   }
   return *_geom_tags;
}

/* ---- GEOM geometry/display ----- */

int 
GEOM::draw_img(const RefImgDrawer& r, bool enable_shading)
{
   if (!ric())
      return 0;

   // save opengl state
   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);

   // set opengl state
   if (enable_shading) {
      // use glEnable(GL_NORMALIZE) only when needed
      if (!xform().is_orthonormal())
         glEnable (GL_NORMALIZE);       // GL_ENABLE_BIT
      GL_COL(color(), transp());        // GL_CURRENT_BIT
   }

   // set modelview matrix:
   glMatrixMode(GL_MODELVIEW);          // GL_TRANSFORM_BIT
   glPushMatrix();
   glMultMatrixd(xform().transpose().matrix());

   // draw mesh
   GL_VIEW::init_polygon_offset(        // GL_ENABLE_BIT
      po_factor(),po_units()
      );
   int ret = r.draw(ric());
   GL_VIEW::end_polygon_offset();       // GL_ENABLE_BIT

   // restore modelview matrix
   // being cautious (mode could change in draw):
   glMatrixMode(GL_MODELVIEW);          // GL_TRANSFORM_BIT
   glPopMatrix();

   // restore opengl state
   glPopAttrib();

   return ret;
}

int
GEOM::draw_final(CVIEWptr &v)
{
   if (!ric())
      return 0;

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glMultMatrixd(xform().transpose().matrix());

   int ret = ric()->draw_final(v);

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   return ret;
}

RAYhit&
GEOM::intersect(RAYhit& ray, CWtransf&, int) const
{
   if (_body) {
      _body->intersect(ray, xform());
   }
   return ray;
}

MAKE_NET_HASHVAR(NO_CULL, int, 0);

bool
GEOM::cull(
   const VIEW *
   ) const
{
   // say no if body is missing or culling is not allowed
   CBODYptr &bod = body();
   if (!bod || NO_CULL.get(((GEOM *)this)))
      return 0;

   // let the bounding box decide
   return bbox().is_off_screen();
}

// FIXME - Should be per-object, not global
static ThreadMutex bbox_mutex;

//
// Only compute world space bounding box if the object has been transformed
// or the object space bounding box is invalid
//
BBOX
GEOM::bbox(int) const
{
   if (!body()) {
      ((GEOM *) this)->_bbox.reset();
   } else {
      CriticalSection cs(&bbox_mutex);
      if (!body()->bb_valid()) {                     // Obj space bb valid?
         BBOX bb = body()->get_bb();
         double s = 1.0; // scale factor; was 1.1
         Wtransf xf = Wtransf::scaling(xform() * bb.center(), s) * xform();
         ((GEOM*)this)->_bbox = xf * bb;
      }
   }
   return _bbox;
}


/* ---- GEOM printing/networking ----- */

// Print out a GEOM for debugging purposes
ostream &
GEOM::print(ostream &os)   const
{ 
   os << class_name() << "::" << name() << "\n\t";
   return os;
}

//
// When a GEOM's transformation is changed, that change has to be
// propagated to GEOM's that depend on that transformation
//
void
GEOM::write_xform(
   CWtransf &x, 
   CWtransf &y, 
   CMOD     &m
   ) 
{
   _xform = x;
   _inv_xf_dirty = true;
   REFlock lock(this);
   XFORMobs::notify_xform_every_obs(this);
}

void       
GEOM::mult_by(
   CWtransf &x
   )
{
   write_xform(x * _xform, Wtransf(), MOD());
}

void 
GEOM::set_color(
   CCOLOR  &c
   )
{ 
   REFlock lock(this);
   APPEAR::set_color(c); 
   COLORobs::notify_color_obs(this); 
}

void 
GEOM::unset_color()
{ 
   REFlock lock(this);
   APPEAR::unset_color(); 
   COLORobs::notify_color_obs(this); 
}

void
GEOM::set_texture(CTEXTUREptr& t)
{  
   REFlock lock(this);
   APPEAR::set_texture(t);
   TEXTUREobs::notify_texture_obs(this);
}

void
GEOM::unset_texture()
{
   REFlock lock(this);
   APPEAR::unset_texture();
   TEXTUREobs::notify_texture_obs(this);
}


/* ---- GEOMobs routines ----- */

GEOMobs_list GEOMobs::_all_geom(32);

/* ---- TRANSPobs routines ----- */

TRANSPobs_list TRANSPobs::_all_transp(32);

/* ---- COLORobs routines ----- */

COLORobs_list   *COLORobs::_all_col = 0;

/* ---- TEXTUREobs routines ----- */

TEXTUREobs_list *TEXTUREobs::_all_texture = 0;

/* ---- XFORMobs routines ----- */

XFORMobs_list XFORMobs::_every_xf(32);
XFORMobs_list XFORMobs::_all_xf(32);
HASH          XFORMobs::_hash_xf(32);

/* -------------------------------------------------------------
 *
 * XFORMobs
 *
 *   This class provides callbacks when an object's transformation 
 * changes.  The following method is a static helper method that
 * provides an easy way to make the XFORMobs callbacks for all XFORMobs
 * observing a particular GEOM
 *
 * ------------------------------------------------------------- */
void
XFORMobs::notify_xform_obs(
   CGEOMptr        &g, 
   XFORMobs::STATE  st
   )
{
   // Notify observers who want to know about all xform
   int i;
   for (i = 0; i < _all_xf.num(); i++)
      _all_xf[i]->notify_xform(g, st);

   // Notify all xform observers
   XFORMobs_list &list = xform_obs_list(g);
   for (i = 0; i <list.num(); i++) {
      XFORMobs *obs = list[i];
      obs->notify_xform(g, st);
   }
}

void
XFORMobs::notify_xform_every_obs(
   CGEOMptr        &g
   )
{
   // Notify observers who want to know about all xform
   int i;
   for (i = 0; i < _every_xf.num(); i++)
      _every_xf[i]->notify_xform(g, EVERY);
}

// end of file geom.C
