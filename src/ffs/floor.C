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

/*!
 *  \file floor.C
 *  \brief Contains the definition of the FLOOR interactor.
 *
 *  \ingroup group_FFS
 *  \sa floor.H
 *
 */

#include "disp/colors.H"
#include "disp/cam_focus.H"
#include "ffs_util.H"
#include "geom/gl_view.H"
#include "geom/world.H"
#include "gtex/key_line.H" 
#include "gtex/sil_frame.H" 
#include "gtex/solid_color.H"
#include "std/config.H" 

#include "floor.H"

using namespace mlib;

FLOORptr FLOOR::_instance;
FLOOR*   FLOOR::null=0;         // for use in ray_geom()

inline KeyLineTexture*
get_keyline(Patch* p)
{
   return get_tex<KeyLineTexture>(p);
}

inline SilFrameTexture*
get_silframe(Patch* p)
{
   KeyLineTexture* key = get_keyline(p);
   return key ? key->sil_frame() : 0;
}

inline SolidColorTexture*
get_solidcolor(Patch* p)
{
   KeyLineTexture* key = get_keyline(p);
   return key ? key->solidcolor() : 0;
}

FLOOR::FLOOR(
   CEvent& down,        
   CEvent& move,
   CEvent& up) : DrawWidget()
{
   set_name("FLOOR");

   // build the mesh:
   BMESHptr m = new BMESH;
   set_body(m);
   m->Cube(Wpt(-1,-1,-1), Wpt(1,1,1));
   assert(m->npatches() == 1);
   Patch* p = m->patch(0);
   assert(p);
   m->compute_creases();
  
   m->set_render_style(KeyLineTexture::static_name());

   KeyLineTexture* key = get_keyline(p);
   assert(key);
   key->set_toon(0); // disable the toon part

   // dimensions of floor:
   _sx = _sz = 24;
   _sy=0.5;
   m->transform(Wtransf::scaling(Wvec(_sx/2, _sy/2, _sz/2)), MOD());
   _map = new PlaneMap(Wpt(-_sx/2, _sy/2, -_sz/2), Wvec::Z()*_sz, Wvec::X()*_sx);

   // set drawing parameters
   SilFrameTexture* sf = get_silframe(p);
   assert(sf);
   sf->set_crease_width(2.0f);
   sf->set_sil_width(2.0f);     // XXX - change this?

   // state transitions:
   _start               += Arc(down, DACb(&FLOOR::down_cb));
   _resize_uniform      += Arc(move, DACb(&FLOOR::resize_uniform_cb));
   _resize_nonuniform   += Arc(move, DACb(&FLOOR::resize_nonuniform_cb));
   _resize_uniform      += Arc(up,   DACb(&FLOOR::up_cb, (State*)-1));
   _resize_nonuniform   += Arc(up,   DACb(&FLOOR::up_cb, (State*)-1));

   if (_instance)
      err_msg("FLOOR::FLOOR: warning: instance exists");
   _instance = this;

   atexit(FLOOR::clean_on_exit);

   // Must set NO_NETWORK before calling WORLD::create() because of
   // observers that are called in WORLD::create()
   //
   // XXX - is this just ancient obsolete stuff no one understands?
   NETWORK.set(_instance, 0);
   NO_COLOR_MOD.set(_instance, 1);
   NO_XFORM_MOD.set(_instance, 1);
   WORLD::create(_instance, false);
   WORLD::undisplay(this, false);
}

bool
FLOOR::should_draw() const
{
   return !BMESH::show_secondary_faces();
}

bool
FLOOR::do_cam_focus(CVIEWptr& view, CRAYhit& r)
{
   if (Config::get_var_bool("DEBUG_CAM_FOCUS",false))
      cerr << "FLOOR::do_cam_focus" << endl;

   assert(r.geom() == this);

   CAMdataptr data(view->cam()->data());

   Wpt     o = data->from();                       // old camera location
   Wpt     c = r.surf();                           // new "center" point
   Wpt     a = c;                                  // new "at" point
   Wpt     f = c + r.norm() * o.dist(c);           // new "from" point
   Wtransf R = Wtransf::anchor_scale_rot(c, o, f); // rotation for old --> new
   Wvec    u = (R * (data->pup_v())).normalized(); // new "up" vector

   // value that is 0 when up vector is nearly parallel to Wvec::Y(),
   // 1 when the vectors are not almost parallel, with a smooth
   // transition between the two values:
   double d = smooth_step(M_PI/12, M_PI/6, line_angle(a - f,Wvec::Y()));

   // choose "up" to follow old up except when it aligns closely with Y:
   u = interp(u, Wvec::Y(), d);

   new CamFocus(view, f, a, f + u, c, data->width(), data->height());
   return true;
}

int
FLOOR::draw(CVIEWptr &v)
{
   if (!should_draw()) {
      return 0;
   }
   assert(mesh() != 0);

   // set the base color from the current background color, only lighter
   SolidColorTexture* sol = get_solidcolor(mesh()->patches().first());
   assert(sol);
   sol->set_color(interp(v->color(), COLOR::white, 0.5));

   return GEOM::draw(v);
}

/*****************************************************************
 * FLOOR_XFORM_CMD
 *****************************************************************/
MAKE_PTR_SUBC(FLOOR_XFORM_CMD,COMMAND);

//! \brief Transform the floor.
class FLOOR_XFORM_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   FLOOR_XFORM_CMD(FLOOR* f, CWtransf& xf, bool done=false) :
      COMMAND(done), _floor(f), _xf(xf) { assert(f); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("FLOOR_XFORM_CMD", FLOOR_XFORM_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   // Transform it if needed:
   virtual bool doit() {
      if (is_done())
         return true;
      _floor->transform(_xf, 0);
      return COMMAND::doit();   // update state in COMMAND
   }

   // Undo the transform if needed:
   virtual bool undoit() {
      if (!is_done())
         return true;
      _floor->transform(_xf.inverse(), 0);
      return COMMAND::undoit(); // update state in COMMAND
   }

 protected:
   FLOORptr _floor;
   Wtransf  _xf;
};

void
FLOOR::_transform(CWtransf &xf, MULTI_CMDptr cmd)
{
   // XXX - should update _sizex, _sizey, _sizez
   //       (if they are being used)

   MOD::tick();
   MOD id;
   mesh()->transform(xf, id);
   _map->transform(xf, id);
   if (cmd)
      cmd->add(new FLOOR_XFORM_CMD(this, xf, true));
}

inline double
dist(CBvert* v, CWpt& c, CWvec& n)
{
   assert(v);
   return (v->loc() - c)*n;
}

void
FLOOR::realign(BMESH* m, MULTI_CMDptr cmd)
{
   bool debug = Config::get_var_bool("DEBUG_FLOOR_REALIGN",false);
   if (!(m && m->nverts() > 0)) {
      err_adv(debug, "FLOOR::_realign: bad mesh: %s",
              m ? "empty" : "null");
      return;
   }

   Wpt  c = _instance->_center();
   Wvec n = _instance->_normal();

   // Find vertex most interior to the floor:
   double min_d = dist(m->bv(0), c, n), d = 0;
   for (int i=1; i<m->nverts(); i++)
      if ((d = dist(m->bv(i), c, n)) < min_d)
         min_d = d;

   // Mesh is "above" floor:
   if (min_d >= 0)
      return;

   if( cmd == NULL) {
      cmd  = new MULTI_CMD;
      _instance->_transform(Wtransf::translation(n*min_d), cmd);
      WORLD::add_command(cmd);
   }
   else {
      _instance->_transform(Wtransf::translation(n*min_d), cmd);
   }
}

void
FLOOR::write_xform(CWtransf &xf, CWtransf &delta, CMOD &mod)
{
   err_msg("FLOOR::write_xform: shouldn't be called");
}

#ifdef JOT_NEEDS_FULL_CLASS_TYPE
FLOOR::intersect_t
#else
intersect_t
#endif
FLOOR::intersect(
   CPIXEL& /* p */,     // ray position (pixels)
   Wpt&    /* hit */,   // hit point
   Wvec&   /* norm */   // "normal" at hit point
   ) const
{
   // XXX - not implemented
   return INTERSECT_NONE;
}

RAYhit&
FLOOR::intersect(RAYhit &r, CWtransf&, int) const
{
   Wvec  normal;
   Wpt   nearpt;
   XYpt  texc;
   double d, d2d;
   mesh()->intersect(r, xform(), nearpt, normal, d, d2d, texc);

   return r;
}

int
FLOOR::interactive(CEvent &e, State *&s, RAYhit*) const
{
   if (intersect(PIXEL(DEVice_2d::last->cur())) != INTERSECT_NONE &&
       _start.consumes(e)) {
      s = _start.event(e);
      return true;
   }
   return 0;
}
 
BBOX
FLOOR::bbox(int) const
{
   // GEOM::bbox() is const
   // BODY::get_bb() is not
   // rather than fix all of jot, just cast away the const:
   return ((FLOOR*)this)->mesh()->get_bb();
}


int
FLOOR::down_cb(CEvent &, State *&)
{
   _clock.set();
//     _last_pt = DEVice_2d::last->cur();

//     Wvec norm;
//     switch (_down_intersect = intersect(_last_pt, _last_wpt, norm)) {
//      case   INTERSECT_SIDE:              s = &_resize_nonuniform;
//      brcase INTERSECT_CORNER:            s = &_resize_uniform;
//      brdefault:
//         ;
//     }

   return 0;
}

int
FLOOR::resize_uniform_cb(CEvent &, State *&)
{
   return 0;
}

int
FLOOR::resize_nonuniform_cb(CEvent &, State *&)
{
   return 0;
}

int
FLOOR::up_cb(CEvent &, State *&)
{
   return 0;
}

void
FLOOR::show()
{
   assert(_instance);
   WORLD::display(_instance, false);
}

void
FLOOR::hide()
{
   assert(_instance);
   WORLD::undisplay(_instance, false);
}

void
FLOOR::toggle()
{
   assert(_instance);
   WORLD::toggle_display(_instance, false);
}

/* end of file floor.C */
