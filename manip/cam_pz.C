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
#include "disp/ray.H"
#include "geom/world.H"
#include "gtex/smooth_shade.H"  // used in CAMwidget_anchor
#include "mesh/lmesh.H"
#include "std/time.H"

#include "manip/cam_pz.H"

using namespace mlib;

static bool debug_fsa = Config::get_var_bool("DEBUG_CAM_FSA",false);

static double DOT_DIST = 0.012; // cheesy XY-space distance threshold

ARRAY<CamIcon*> CamIcon::_icons;
CamIcon*        CamIcon::_orig_icon;

CamIcon*
CamIcon::intersect_all(CXYpt &pt)
{
   for (int i=0; i<_icons.num(); i++)
      if (_icons[i]->intersect_icon(pt)) 
         return _icons[i];
   return 0;
}

/*****************************************************************
 * CAMwidget_anchor:
 *
 *      Class used for those blue "camera balls"
 *****************************************************************/
MAKE_PTR_SUBC(CAMwidget_anchor, GEOM);
class CAMwidget_anchor : public GEOM {
 public:
   CAMwidget_anchor() : GEOM() {
      // Start with an empty LMESH:
      LMESHptr mesh = new LMESH();

      // Ensure mesh knows its containing GEOM:
      mesh->set_geom(this);

      // Make a ball shape
      mesh->Icosahedron();
      mesh->refine();           // smooth it once

      // Regardless of current rendering mode,
      // always use Gouraud shading:
      mesh->set_render_style(SmoothShadeTexture::static_name());

      // Color the Patch blue:
      mesh->patch(0)->set_color(COLOR(0.1, 0.1, 0.9));

      // Store the mesh:
      _body = mesh;

      set_name("CAMwidget_anchor");
   }

   // We're not in the picking game:
   virtual int draw_vis_ref() { return 0; }

   virtual bool needs_blend() const { return false; }
};

/*****************************************************************
 *
 *  Cam_int : the camera interactor
 *
 *   The camera interactor is an FSA that allows the camera to
 * be translated, zoomed, and rotated.  The FSA has 4 states
 * corresponding to 
 *   a "virtual cylinder" rotation state - _cam_rot,
 *   a camera panning translation state  - _cam_pan,
 *   a camera zooming translation state  - _cam_zoom,  and
 *   a "choosing" state in which the interactor tries 
 *     to choose between zooming and panning. 
 *
 *  _entry -> _cam_rot
 *            _cam_choose -> _cam_pan
 *                        -> _cam_zoom
 *
 * The interactor is initiated when one of the _entry arcs  
 * occurs.  The primary entry event calls the down() function which 
 * determines if the camera should transtion to the rotation state 
 * or the choosing state.  The secondary entry event always 
 * transitions to the rotation state (this second event is often
 * connected to a dedicated camera rotation trackball).
 *
 *****************************************************************/
Cam_int::Cam_int(
   CEvent    &down_ev,
   CEvent    &move_ev, 
   CEvent    &up_ev,

   CEvent    &down2_ev, 
   CEvent    &up2_ev, 

   CEvent    &rot_down_ev,
   CEvent    &trans_down_ev,
   CEvent    &zoom_down_ev,

   CEvent    &rot_up_ev,
   CEvent    &trans_up_ev,
   CEvent    &zoom_up_ev
   )
{
   _cam_rot    += Arc(move_ev, Cb(&Cam_int::rot));
   _cam_rot    += Arc(up_ev,   Cb(&Cam_int::up,    (State *)-1));
   _cam_rot    += Arc(up2_ev,  Cb(&Cam_int::up,    (State *)-1));
   _cam_rot.set_name("Cam_int::cam_rot");

   _cam_pan    += Arc(move_ev, Cb(&Cam_int::pan));
   _cam_pan    += Arc(up_ev,   Cb(&Cam_int::up,    (State *)-1));
   _cam_pan    += Arc(down2_ev,Cb(&Cam_int::noop,  &_cam_rot));
   _cam_pan.set_name("Cam_int::cam_pan");

   _cam_zoom   += Arc(move_ev, Cb(&Cam_int::zoom));
   _cam_zoom   += Arc(up_ev,   Cb(&Cam_int::up,    (State *)-1));
   _cam_zoom   += Arc(down2_ev,Cb(&Cam_int::noop,  &_cam_rot));
   _cam_zoom.set_name("Cam_int::cam_zoom");

   _cam_choose += Arc(down2_ev,Cb(&Cam_int::noop,  &_cam_rot));
   _cam_choose += Arc(move_ev, Cb(&Cam_int::choose));
   _cam_choose += Arc(up_ev,   Cb(&Cam_int::up,    (State *)-1));
   _cam_choose.set_name("Cam_int::cam_choose");

   _cam_drag   += Arc(move_ev, Cb(&Cam_int::drag,  &_cam_drag));
   _cam_drag   += Arc(up_ev,   Cb(&Cam_int::dragup,(State *)-1));
   _cam_drag.set_name("Cam_int::cam_drag");
   
   _move_view  += Arc(move_ev, Cb(&Cam_int::move,  &_move_view));
   _move_view  += Arc(up_ev,   Cb(&Cam_int::moveup, (State *)-1));
   _move_view.set_name("Cam_int::move_view");

   _icon_click += Arc(move_ev, Cb(&Cam_int::iconmove,  &_icon_click));
   _icon_click += Arc(up_ev,   Cb(&Cam_int::iconup, (State *)-1));
   _icon_click.set_name("Cam_int::icon_click");

   _entry      += Arc(down_ev, Cb(&Cam_int::down,  &_cam_choose));
   _entry      += Arc(down2_ev,   Cb(&Cam_int::down2));
   _entry.set_name("Cam_int::entry");

   _but_trans  += Arc(move_ev      , Cb(&Cam_int::pan2,  &_but_trans));
   _but_trans  += Arc(trans_up_ev  , Cb(&Cam_int::noop,    (State *)-1));
   _but_trans.set_name("Cam_int::but_trans");

   _but_rot    += Arc(move_ev      , Cb(&Cam_int::rot2,  &_but_rot));
   _but_rot    += Arc(rot_up_ev    , Cb(&Cam_int::noop,    (State *)-1));
   _but_rot.set_name("Cam_int::but_rot");

   _but_zoom   += Arc(move_ev      , Cb(&Cam_int::zoom2, &_but_zoom));
   _but_zoom   += Arc(zoom_up_ev   , Cb(&Cam_int::noop,    (State *)-1));
   _but_zoom.set_name("Cam_int::but_zoom");

   _entry2     += Arc(trans_down_ev, Cb(&Cam_int::predown,&_but_trans));
   _entry2     += Arc(rot_down_ev,   Cb(&Cam_int::predown,&_but_rot));
   _entry2     += Arc(zoom_down_ev,  Cb(&Cam_int::predown,&_but_zoom));
   _entry2.set_name("Cam_int::entry2");
}

Cam_int::CAMwidget::CAMwidget():_a_displayed(0)
{
   _anchor = new CAMwidget_anchor;
   _anchor->set_pickable(0); // Don't allow picking of anchor
   NETWORK.set(_anchor, 0);// Don't network the anchor
   CONSTRAINT.set(_anchor, GEOM::SCREEN_WIDGET);
   WORLD::create   (_anchor, false); 
   WORLD::undisplay(_anchor, false);
}

void
Cam_int::CAMwidget::undisplay_anchor()
{
   WORLD::undisplay(_anchor, false);
   _a_displayed=0; 
}

void   
Cam_int::CAMwidget::display_anchor(
   CWpt &p
   )
{
   WORLD::display(_anchor, false);  
   _a_displayed = 1;

   Wvec    delt(p-Wpt(XYpt(p)+XYvec(VEXEL(5,5)), p));
   Wtransf ff = Wtransf(p) * Wtransf::scaling(delt.length()); 
   _anchor->set_xform(Wtransf(p) * Wtransf::scaling(1.5*delt.length())); 
}

void 
Cam_int::CAMwidget::drag_anchor(
   CXYpt &p2d
   ) 
{
   Wpt     apos(_anchor->xform().origin());
   Wpt     p   (Wplane(apos,Wvec(XYpt())).intersect(p2d));
   double  rad = (p-apos).length();
   Wtransf ff  = Wtransf(apos) * Wtransf::scaling(rad);
   _anchor->set_xform(Wtransf(apos) * Wtransf::scaling(rad));
}

int
Cam_int::predown(
   CEvent &e, 
   State *&
   )
{
   // de-activate current CamFocus, if any:
   CamFocus::cancel_cur();
   
   DEVice_buttons *btns = (DEVice_buttons *)e._d;
   DEVice_2d      *ptr  = btns->ptr2d();

   _view = e.view();

   _do_reset = 0;
   _dtime    = the_time();
   _dist     = 0;
   _scale_pt = ptr->cur();
   _start_pix= ptr->cur();

   CAMdataptr  data(_view->cam()->data());
   RAYhit      r   (_view->intersect(ptr->cur()));
   if (r.success()) {
      _down_pt = r.surf();
   } else {
      _down_pt = Wplane(data->center(),data->at_v()).intersect(ptr->cur());
   }
   _down_pt_2d = _down_pt;

   //Notify all the CAMobs's of the start of mouse down
  
   data->start_manip();
   return 0;
}

int
Cam_int::down(
   CEvent &e, 
   State *&s
   )
{
   if (debug_fsa)
      cerr << "Cam_int::down"
           << endl;

   DEVice_buttons *btns = (DEVice_buttons *)e._d;
   DEVice_2d      *ptr  = btns->ptr2d();
   VIEWptr         view(e.view());
   bool            is_dot = _camwidg.anchor_displayed();
   int             x, y; view->get_size(x,y);
   PIXEL           curpix(ptr->cur());
   XYpt            curpt (curpix[0]/x*2-1, curpix[1]/y*2-1);

   predown(e,s);

   // Did we click on a camera icon?
   CamIcon *icon = CamIcon::intersect_all(ptr->cur()); 
   if (icon) {

      // XXX - fix

      _icon = icon;
      switch (icon->test_down(e, s)) {
       case CamIcon::RESIZE :
         _resizing = true;
         // drops through to next case:
       case CamIcon::FOCUS  :
         s = &_icon_click;
         break;
       case CamIcon::MOVE   :
         s = &_move_view;
         return move(e, s);
      }
      return 0;
   } else if ((fabs(curpt[0]) > .85 || fabs(curpt[1]) > .9) || is_dot) {
      if (is_dot)
         view->cam()->data()->set_center(_camwidg.anchor_wpt());
      _do_reset = is_dot;

      if (!is_dot){
         /* alexni- is there a better way to handle this?  we want to
          * save the camera if we're going to enter a mode that causes
          * change to the camera
          */
         _view->save_cam();
      }

      s = &_cam_rot;
      return 0;
   }

   return 0;
}

//left-click:test for any buttons used in this camera mode
int
Cam_int::down2(
   CEvent &e, 
   State *&s
   )
{
   if (debug_fsa)
      cerr << "Cam_int::down2"
           << endl;

   DEVice_buttons *btns = (DEVice_buttons *)e._d;
   DEVice_2d      *ptr  = btns->ptr2d();

   _view = e.view();

   CAMdataptr  data(_view->cam()->data());
   RAYhit      r   (_view->intersect(ptr->cur()));
   if (r.success()) {
      if(r.geom()->name() == "eye_button")
         {
            s = (State *)-1;
            BaseJOTapp::cam_switch(e,s);
         }
   }
   return 0;
}


int 
Cam_int::choose(
   CEvent &e,
   State *&s
   )
{
   if (debug_fsa)
      cerr << "Cam_int::choose"
           << endl;

   DEVice_2d *ptr =(DEVice_2d *)e._d;
   PIXEL      te   (ptr->cur());
   XYvec      delta(ptr->delta());
   double     tdelt(the_time() - _dtime);
   
   _dist += sqrt(delta * delta);

   VEXEL sdelt(te - _start_pix);

   int xa=0,ya=1;
   if (Config::get_var_bool("FLIP_CAM_MANIP",false))
      swap(xa,ya);
     
   if (fabs(sdelt[ya])/sdelt.length() > 0.9 && tdelt > 0.05) {
      s = &_cam_zoom;
      ptr->set_old(_start_pix);
   } else if (tdelt < 0.1 && _dist < 0.03)
      return 0;
   else {
      if (fabs(sdelt[xa])/sdelt.length() > 0.6 )
         s = &_cam_pan;
      else s = &_cam_zoom;
      ptr->set_old(_start_pix);
   }
   /* if we got this far, we actually have a valid choice, so save the
    * camera */
   _view->save_cam();

   return 0;
}

int
Cam_int::drag(
   CEvent &e,
   State *&s
   )
{
   if (debug_fsa)
      cerr << "Cam_int::drag"
           << endl;

   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   PIXEL       curpt(ptr->cur());

   // Switch to different fsa?
   if ((curpt - _start_pix).length() > 20 &&
       (curpt - _start_pix).normalized() * VEXEL(1,1).normalized() > 0.5) {
      make_view(ptr->cur());
      s = &_move_view;
      return move(e, s);
   }
   if ((curpt - _start_pix).length() > 20 &&
       (curpt - _start_pix).normalized() * VEXEL(-1,-1).normalized() > 0.5)
      _camwidg.drag_anchor(_camwidg.anchor_pos());
   else _camwidg.drag_anchor(ptr->cur());

   return 0;
}

int
Cam_int::zoom2(
   CEvent &e,
   State *&
   )
{
   if (debug_fsa)
      cerr << "Cam_int::zoom2"
           << endl;

   DEVice_2d  *ptr =  (DEVice_2d *)e._d;
   CAMptr      cam    (e.view()->cam());
   PIXEL       curpt  (ptr->cur());
   XYpt        startpt(_start_pix);
   int w,h;    e.view()->get_size(w,h);

   double zoom_factor =  1 + Sign(ptr->delta()[0]) * 
      (PIXEL(ptr->cur())-PIXEL(ptr->old())).length()/(w/4);

   cam->set_zoom(cam->zoom() * zoom_factor);
   cam->set_min (cam->min() + NDCvec(XYpt(_start_pix) - startpt));

   ptr->set_cur(curpt);
   cam->data()->changed();
   
   return 0;
}

int
Cam_int::rot2(
   CEvent &e,
   State *&
   )
{
   if (debug_fsa)
      cerr << "Cam_int::rot2"
           << endl;

   CAMptr  cam(e.view()->cam());

   cam->set_zoom(1);
   cam->set_min(NDCpt(XYpt(-1,-1)));
   cam->data()->changed();

   return 0;
}

int
Cam_int::pan2(
   CEvent &e,
   State *&
   )
{
   if (debug_fsa)
      cerr << "Cam_int::pan2"
           << endl;

   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   CAMptr      cam  (e.view()->cam());
   PIXEL       curpt(ptr->cur());

   cam->set_min(cam->min() + NDCvec(ptr->delta()));
   cam->data()->changed();
   ptr->set_cur(curpt);
   return 0;
}

int
Cam_int::pan(
   CEvent &e,
   State *&
   )
{
   if (debug_fsa)
      cerr << "Cam_int::pan"
           << endl;

   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   CAMptr      cam  (e.view()->cam());
   CAMdataptr  data (cam->data());
   Wvec        delta(Wpt(ptr->cur(),_down_pt) - Wpt(ptr->old(),_down_pt));

   data->translate(-delta);
   data->set_at(Wline(data->from(), data->at_v()).project(_down_pt));
   data->set_center(data->at());
   return 0;
}

int
Cam_int::zoom(
   CEvent &e,
   State *&
   )
{
   if (debug_fsa)
      cerr << "Cam_int::zoom"
           << endl;

   DEVice_2d *ptr=(DEVice_2d *)e._d;
   CAMptr     cam  (e.view()->cam());
   CAMdataptr data (cam->data());
   XYvec      delta(ptr->delta());
   double     ratio;

   if (data->persp()) {
      Wvec    movec(_down_pt - data->from());

      data->set_center(_down_pt);
      data->translate(movec.normalized() * (movec.length() * delta[1] * -4));
   
      ratio = cam->height() / cam->width() * 
         (movec * data->at_v()) * data->width() / data->focal();

   } else {
      Wpt     spt  (XYpt(ptr->cur()[0],_scale_pt[1]));
      Wvec    svec (spt - Wline(data->from(), data->at_v()).project(spt));
      double  sfact(1 + delta[1]);
      data->translate( svec * (1.0 - sfact));
      data->set_width (data->width() * sfact);
      data->set_height(data->height()* sfact);
      ratio = data->height();
   }

   data->translate(-delta[0]/2 * data->right_v() * ratio);
   data->set_at(Wline(data->from(), data->at_v()).project(data->center()));
   data->set_center(data->at());


   return 0;
}

int
Cam_int::rot(
   CEvent &e, 
   State *&
   ) 
{
   if (debug_fsa)
      cerr << "Cam_int::rot"
           << endl;

   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
   DEVice_2d  *ptr=(DEVice_2d *)e._d;

   cam->set_zoom(1);
   cam->set_min(NDCpt(XYpt(-1,-1)));
   cam->data()->changed();

   XYpt        cpt   = data->center();
   double      radsq = sqr(1+fabs(cpt[0])); // squared rad of virtual cylinder
   XYpt        tp    = ptr->old(); 
   XYpt        te    = ptr->cur();

   Wvec   op  (tp[0], 0, 0);             // get start and end X coordinates
   Wvec   oe  (te[0], 0, 0);             //    of cursor motion
   double opsq = op * op, oesq = oe * oe;
   double lop  = opsq > radsq ? 0 : sqrt(radsq - opsq);
   double loe  = oesq > radsq ? 0 : sqrt(radsq - oesq);
   Wvec   nop  = Wvec(op[0], 0, lop).normalized();
   Wvec   noe  = Wvec(oe[0], 0, loe).normalized();
   double dot  = nop * noe;

   if (fabs(dot) > 0.0001) {
      data->rotate(Wline(data->center(), Wvec::Y()),
                   -2*Acos(dot) * Sign(te[0]-tp[0]));

      Wvec   dvec  = data->from() - data->center();
      double rdist = te[1]-tp[1];

      CAMdata   dd = CAMdata(*data);

      Wline raxe(data->center(),data->right_v());
      data->rotate(raxe, rdist);
      data->set_up(data->from() + Wvec::Y());
      if (data->right_v() * dd.right_v() < 0)
         *data = dd;
   }

   return 0;
}

inline void
print_gel(GEL* gel)
{
   if (gel)
      cerr << ": " << gel->class_name();
   cerr << endl;
}

int
Cam_int::focus(
   CEvent &e,
   State *&
   ) 
{
   if (debug_fsa)
      cerr << "Cam_int::focus"
           << endl;

   GEOM::find_cam_focus(e.view(), DEVice_2d::last->cur());
   return 0;
}

int
Cam_int::move(
   CEvent &e, 
   State *&
   )
{
   if (debug_fsa)
      cerr << "Cam_int::move"
           << endl;

   if (_icon) _icon->set_icon_loc(((DEVice_2d *)e._d)->cur());
   return 0;
}

int
Cam_int::moveup(
   CEvent &, 
   State *&
   )
{
   if (debug_fsa)
      cerr << "Cam_int::moveup"
           << endl;

   for (int i = 0; i < _up_obs.num(); i++) 
      _up_obs[i]->reset(_do_reset);  // reset everyone watching us
   _geom = 0;
   _icon = 0;
   return 0;
}

int
Cam_int::iconmove(
   CEvent &e, 
   State *&s
   )
{
   if (debug_fsa)
      cerr << "Cam_int::iconmove"
           << endl;

   if (_resizing) return _icon->icon_move(e, s);
   return 0;
}

int
Cam_int::iconup(
   CEvent &e, 
   State *&s
   )
{
   if (debug_fsa)
      cerr << "Cam_int::iconup"
           << endl;

   if (_resizing) {
      _resizing = false;
      const int retval = _icon->resize_up(e, s);
      _icon = 0;
      return retval;
   }
   DEVice_buttons *btns=(DEVice_buttons *)e._d;
   DEVice_2d      *ptr=btns->ptr2d();
   PIXEL           curpt(ptr->cur());

   // Delete
   if (the_time() - _dtime < 0.25 && _icon &&
       (curpt - _start_pix).length() > 20 &&
       (curpt - _start_pix).normalized() * VEXEL(1,1).normalized() > 0.5) {
      _icon->remove_icon();
      _icon = 0;
      return 0;
   }

   if (_icon->intersect_icon(ptr->cur())) {
      new CamFocus(e.view(), _icon->cam());
   }
   return 0;
}

int
Cam_int::dragup(
   CEvent &e, 
   State *&s
   )
{
   if (debug_fsa)
      cerr << "Cam_int::dragup"
           << endl;

   DEVice_buttons *btns=(DEVice_buttons *)e._d;
   DEVice_2d      *ptr=btns->ptr2d();
   VIEWptr         view(e.view());
   CAMptr          cam (view->cam());
   PIXEL           curpt(ptr->cur());

   double elapsed = the_time() - _dtime;
   // Are we close to the cam globe?
   if (ptr->cur().dist(_camwidg.anchor_pos()) < DOT_DIST && elapsed < 0.25)
      return up(e,s);
   
   reset(1);

   for (int i = 0; i < _up_obs.num(); i++) 
      _up_obs[i]->reset(_do_reset);  // reset everyone watching us

   if ((curpt - _start_pix).length() > 20 &&
       (curpt - _start_pix).normalized() * VEXEL(-1,0).normalized() > 0.5)
      return 0;
   
   RAYhit ray(view->intersect(_camwidg.anchor_pos()));
   if (!ray.success())
      return 0;

   CAMdataptr data(cam->data());
   double dist = _camwidg.anchor_size() * 2*data->focal()/data->height();
   Wvec dirvec((data->from() - ray.surf()).normalized());

   Wpt from = ray.surf() + dist * dirvec;
   new CamFocus(view, 
                from,                   // from
                ray.surf(),             // at
                from + Wvec::Y(),       // up
                ray.surf(),             // center
                data->width(), data->height());

   return 0;
}

int
Cam_int::up(
   CEvent &e, 
   State *&s
   )
{
   if (debug_fsa)
      cerr << "Cam_int::up"
           << endl;

   DEVice_buttons *btns=(DEVice_buttons *)e._d;
   DEVice_2d      *ptr=btns->ptr2d();
   VIEWptr         view(e.view());
   CAMptr          cam (view->cam());

   for (int i = 0; i < _up_obs.num(); i++) 
      _up_obs[i]->reset(_do_reset);  // reset everyone watching us

   if (_camwidg.anchor_displayed()) {
      if (ptr->cur().dist(_camwidg.anchor_pos()) < DOT_DIST)
         focus(e,s);
      reset(1);
   } else if (ptr->cur().dist(_down_pt_2d) < DOT_DIST) {
      RAYhit ray(view->intersect(ptr->cur()));
      if (ray.success()) {
         // Create the anchor (blue ball) on the surface:
         _camwidg.display_anchor(ray.surf());
         // If clicked on a mesh, make it the "focus":
         BMESHptr m = gel_to_bmesh(ray.geom());
         if (m)
            BMESH::set_focus(m, dynamic_cast<Patch*>(ray.appear()));
      } else {
         Wplane hitp(cam->data()->center(), cam->data()->at_v());
         _camwidg.display_anchor(hitp.intersect(ray.screen_point()));
      }
   }
   cam->data()->end_manip();
   return 0;
}

//
// reset() - UP observer method
//   Cam_int is an observer of 'UP' events on other objects.
// In particular, Cam_int observes the 'UP' events of ObjManip
// so that the Cam_int can clear its widget after an object
// is dragged.
//
void
Cam_int::reset(int rst)
{
   if (rst)
      _camwidg.undisplay_anchor();
}

// Make a view (camera icon)
void
Cam_int::make_view(
   CXYpt  &curpt
   )
{
   // Create new cam icon
   _icon = CamIcon::create(curpt, _view->cam());
   _geom = 0;
   
   reset(1);
}

// end of file cam_pz.C
