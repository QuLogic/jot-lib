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
#include "std/support.H"
#include "disp/ray.H"
#include "geom/fsa.H"
#include "geom/geom.H"
#include "geom/world.H"
#include "manip/manip.H"

using mlib::Wpt;
using mlib::CWpt;
using mlib::Wvec;
using mlib::CWvec;
using mlib::Wline;
using mlib::Wtransf;
using mlib::XYpt;
using mlib::CXYpt;

/* -------------------------------------------------------------
 *
 * Simple_int : base 3-state FSA
 *
 *   This base class for an interactor is an FSA that calls 
 * virtual functions for down(), motion(), and up() events.
 *
 * ------------------------------------------------------------- */
Simple_int::Simple_int(
   CEvent &down_ev, // Definition of the Down event
   CEvent &move_ev, //                   Move
   CEvent &up_ev    //                   Up
   ) 
{
   static Cstr_ptr default_entry_name("Simple_int Entry");
   static Cstr_ptr default_move_name ("Simple_int Move");
   _entry     .set_name(default_entry_name);
   _manip_move.set_name(default_move_name);

   _manip_move += Arc(move_ev, Cb(&Simple_int::move));
   _manip_move += Arc(up_ev,   Cb(&Simple_int::up,   (State *)-1));
   _entry      += Arc(down_ev, Cb(&Simple_int::down, &_manip_move));
}

void
Simple_int::add_events(
   CEvent &down_ev, // Definition of the Down event
   CEvent &move_ev, //                   Move
   CEvent &up_ev    //                   Up
   )
{
   _manip_move += Arc(move_ev, Cb(&Simple_int::move));
   _manip_move += Arc(up_ev,   Cb(&Simple_int::up,    (State *)-1));
   _entry      += Arc(down_ev, Cb(&Simple_int::down, &_manip_move));
}

/* -------------------------------------------------------------
 *
 * Key_int : base keyboard interactor
 *
 *   Provides callbacks when any registered keyboard key is
 * pressed or released.   This interactor adds an arc to the
 * start state for each key.  When the key down event occurs, 
 * this interactor transitions to the _key_down state.  When 
 * the key up event occurs, the interactor goes back to the 
 * start state.
 *
 * ------------------------------------------------------------- */
void
Key_int::add_event(
   char  *k,
   State *start     // where to go when this FSA finishes
   )
{
   for (char *c = k; *c; c++) {
      Event  down_ev(NULL, Evd(*c, KEYD));
      Event  up_ev  (NULL, Evd(*c, KEYU));
      _key_down += Arc(up_ev,   Cb(&Key_int::up,    start));
      _entry    += Arc(down_ev, Cb(&Key_int::down, &_key_down));
   }
}

void
Key_int::add_event(
   char   k,
   State *start    // where to go when this FSA finishes
   )
{
   Event  down_ev(NULL, Evd(k, KEYD));
   Event  up_ev  (NULL, Evd(k, KEYU));
   _key_down += Arc(up_ev,   Cb(&Key_int::up,    start));
   _entry    += Arc(down_ev, Cb(&Key_int::down, &_key_down));
}




FilmTrans::FilmTrans(
   CEvent    &d,
   CEvent    &m,
   CEvent    &u
   ) : Simple_int(d,m,u), _call_xform_obs(true)
{
   _entry     .set_name("FilmTrans Entry");
   _manip_move.set_name("FilmTrans Move");
}

int
FilmTrans::down(
   CEvent &e,
   State *&s
   )
{
   DEVice_buttons *btns=(DEVice_buttons *)e._d;
   DEVice_2d      *ptr=btns->ptr2d();
   RAYhit          r(ptr->cur());
   GEOMptr         geom;

   _view = e.view();
   if (_view->intersect(r,VIEW::H_TEXT).success() && GEOM::isa(r.geom())) {
      geom = ray_geom(r, GEOM::null);
      if (geom->interactive(e, s, &r)) {
         _obj      = 0;
         _no_xform = 1;
         return 1; // Interactive - return....
      }
   }
   // interactive() could have changed r so that now !success(),
   //  so we have to check again
   if (!r.success() || (geom && NO_XFORM_MOD.get(geom))) {
      _no_xform = 1;
      _obj      = geom;
   } else {
      if (CONSTRAINT.get(geom) == GEOM::SCREEN_WIDGET)
           _down_pt = geom->xform();
      else _down_pt = r.surf();

      _down_norm = r.norm();
      _obj       = geom;

      if (_call_xform_obs)
         XFORMobs::notify_xform_obs(_obj, XFORMobs::START);
      _no_xform = 0;
   }

   return 0;
}

int
FilmTrans::move(
   CEvent &e,
   State *&
   )
{
   DEVice_2d* ptr = dynamic_cast<DEVice_2d*>(e._d);
   assert(ptr);
   if (_obj) {
      Wpt cur(Wpt(ptr->cur(),_down_pt));
      Wpt old(Wpt(ptr->old(),_down_pt));
      _obj->mult_by(Wtransf::translation(cur-old));
      if (_call_xform_obs)
         XFORMobs::notify_xform_obs(_obj, XFORMobs::MIDDLE);
   }

   return 0;
}


int
FilmTrans::up(
   CEvent &,
   State *&
   )
{
   // tell observers the ordeal is over
   if (_obj && _call_xform_obs)
      XFORMobs::notify_xform_obs(_obj, XFORMobs::END);

   // forget this obj
   _obj = (GEOMptr) 0;

   return 0;
}


void
scale_along_normal(
   CGEOMptr &obj,
   CWpt     &scale_cent,
   CWpt     &down_pt,
   CWvec    &down_norm,
   CXYpt    &cur
   )
{
   Wtransf xf  (obj->xform());
   Wpt     dpt (xf * down_pt);
   Wpt     spt (xf * scale_cent);
   Wvec    upv ((xf * down_norm).normalized());

   if (upv * (dpt - spt) < 0)
      upv = -upv;

   if ((cur - XYpt(dpt)).length() < 0.01)
      return;

   Wpt npt(Wline(dpt,upv).intersect(cur));
   Wpt ctr(xf * scale_cent);
   double  sval(((npt-dpt) * upv) / (dpt-ctr).length());

   if ((npt-ctr) * (dpt-ctr) < 0)
      return;

   Wvec   dnorm (fabs(down_norm[0]),fabs(down_norm[1]),
                  fabs(down_norm[2]));

   ((GEOMptr)obj)->set_xform(xf *
                  Wtransf::scaling(scale_cent, 
                                    (dnorm*sval+Wvec(1,1,1))));
}

