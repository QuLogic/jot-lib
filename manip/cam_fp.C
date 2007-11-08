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
#include "manip/cam_fp.H"

using namespace mlib;
 
static double DOT_DIST = 0.012; // cheesy XY-space distance threshold

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
//      mesh->patch(0)->set_transp(0.9);

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
 *  Cam_int_fp : the camera interactor
 *
 * The interactor is initiated when one of the _entry arcs  
 * occurs.  The primary entry event calls the down() function which 
 * rotates the camera. 
 *
 *****************************************************************/
Cam_int_fp::Cam_int_fp(
   CEvent    &down_ev,
   CEvent    &move_ev, 
   CEvent    &up_ev,   // up_ev,

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
        
   //create key events
   Event  for_up(NULL, Evd('8', KEYU));
   Event  for_down(NULL, Evd('8', KEYD));
   Event  back_up(NULL, Evd('2', KEYU));
   Event  back_down(NULL, Evd('2', KEYD));
   Event  left_up(NULL, Evd('4', KEYU));
   Event  left_down(NULL, Evd('4', KEYD));
   Event  right_up(NULL, Evd('6', KEYU));
   Event  right_down(NULL, Evd('6', KEYD));
   Event  breathe(NULL, Evd('5', KEYD));
   Event  orbit(NULL, Evd('9', KEYD));
   Event  toggle(NULL, Evd('Y', KEYD));

   //events possible while moving foward
  // _cam_forward += Arc(for_up,   Cb(&Cam_int_fp::moveup,    (State *)-1));
  // _cam_forward += Arc(for_down, Cb(&Cam_int_fp::forward));
   //_cam_forward += Arc(down_ev,   Cb(&Cam_int_fp::down,  &_cam_rot));

   //events possible while moving backward
  // _cam_back    += Arc(back_up,   Cb(&Cam_int_fp::moveup,    (State *)-1));
  // _cam_back    += Arc(back_down, Cb(&Cam_int_fp::back));
  // _cam_back    += Arc(down_ev,   Cb(&Cam_int_fp::down,  &_cam_rot));

   //events possible while moving left
  // _cam_left    += Arc(left_up,   Cb(&Cam_int_fp::moveup,    (State *)-1));
  // _cam_left    += Arc(left_down, Cb(&Cam_int_fp::left));
   //_cam_left    += Arc(down_ev,   Cb(&Cam_int_fp::down,  &_cam_rot));

   //events possible while moving right
  // _cam_right   += Arc(right_up,   Cb(&Cam_int_fp::moveup,    (State *)-1));
   //_cam_right   += Arc(right_down, Cb(&Cam_int_fp::right));
   //_cam_right   += Arc(down_ev,   Cb(&Cam_int_fp::down,  &_cam_rot));

   //events possible while rotating the camera
   _cam_rot    += Arc(move_ev, Cb(&Cam_int_fp::rot));
   _cam_rot    += Arc(up_ev,   Cb(&Cam_int_fp::up,    (State *)-1));
   _cam_rot    += Arc(up2_ev,  Cb(&Cam_int_fp::up,    (State *)-1));

   _cam_rot    += Arc(for_up,   Cb(&Cam_int_fp::moveup));
   _cam_rot    += Arc(for_down, Cb(&Cam_int_fp::forward));

   _cam_rot    += Arc(back_up,   Cb(&Cam_int_fp::moveup));
   _cam_rot    += Arc(back_down, Cb(&Cam_int_fp::back));

   _cam_rot    += Arc(left_up,   Cb(&Cam_int_fp::moveup));
   _cam_rot    += Arc(left_down, Cb(&Cam_int_fp::left));

   _cam_rot    += Arc(right_up,   Cb(&Cam_int_fp::moveup));
   _cam_rot    += Arc(right_down, Cb(&Cam_int_fp::right));

   //evens possible while the camera is orbiting
   _orbit      += Arc(orbit,     Cb(&Cam_int_fp::stop_orbit, (State *)-1));
   _orbit      += Arc(down_ev,   Cb(&Cam_int_fp::down,  &_cam_choose));//&_orb_rot));
   _orbit      += Arc(down2_ev,   Cb(&Cam_int_fp::down2));
   _orbit          += Arc(toggle,   Cb(&Cam_int_fp::toggle_buttons));

   _cam_choose += Arc(move_ev, Cb(&Cam_int_fp::choose));
   _cam_choose += Arc(up_ev,   Cb(&Cam_int_fp::up,    &_orbit));

   _orb_rot    += Arc(move_ev, Cb(&Cam_int_fp::orbit_rot));
   _orb_rot    += Arc(up_ev,   Cb(&Cam_int_fp::orbit_rot_up,    &_orbit));
   _orb_zoom   += Arc(move_ev, Cb(&Cam_int_fp::orbit_zoom));
   _orb_zoom   += Arc(up_ev,   Cb(&Cam_int_fp::up,   &_orbit));


   //events possible while in cruise control
   _cruise      += Arc(down_ev,   Cb(&Cam_int_fp::cruise_down));
   _cruise      += Arc(down2_ev,   Cb(&Cam_int_fp::down2));
   _cruise      += Arc(toggle,   Cb(&Cam_int_fp::toggle_buttons));

   _cruise		+= Arc(for_up,   Cb(&Cam_int_fp::forward));
   _cruise		+= Arc(for_down, Cb(&Cam_int_fp::back));

   _cruise_rot    += Arc(move_ev, Cb(&Cam_int_fp::rot));
   _cruise_rot    += Arc(up_ev,   Cb(&Cam_int_fp::up,    &_cruise));

   _cruise_zoom    += Arc(move_ev, Cb(&Cam_int_fp::cruise_zoom));
   _cruise_zoom    += Arc(up_ev,   Cb(&Cam_int_fp::cruise_zoom_up,  &_cruise));

   //events possible while in grow mode
   _grow           += Arc(down_ev,   Cb(&Cam_int_fp::grow, &_grow_down));
   _grow           += Arc(down2_ev,   Cb(&Cam_int_fp::down2));
   _grow           += Arc(toggle,   Cb(&Cam_int_fp::toggle_buttons));
   _grow_down      += Arc(move_ev, Cb(&Cam_int_fp::grow));
   _grow_down      += Arc(up_ev,   Cb(&Cam_int_fp::up, &_grow));

   //events possible while in the initial state
   _entry      += Arc(down_ev,   Cb(&Cam_int_fp::down,  &_cam_rot));
   _entry      += Arc(down2_ev,   Cb(&Cam_int_fp::down2));
   _entry      += Arc(for_down,  Cb(&Cam_int_fp::forward,  &_cam_forward));
   _entry      += Arc(back_down, Cb(&Cam_int_fp::back,  &_cam_back));
   _entry      += Arc(left_down, Cb(&Cam_int_fp::left,  &_cam_left));
   _entry      += Arc(right_down,Cb(&Cam_int_fp::right,  &_cam_right));
   _entry      += Arc(breathe,   Cb(&Cam_int_fp::breathe));
   _entry      += Arc(orbit,     Cb(&Cam_int_fp::orbit, &_orbit));

   _breathing = false;
   _size = 1;
   _speed = .1;
   _gravity =  new Gravity(); 
   _collision = new Collide(.005,6,20,.35);
}

Cam_int_fp::CAMwidget::CAMwidget():_a_displayed(0)
{
   _anchor = new CAMwidget_anchor;
   _anchor->set_pickable(0); // Don't allow picking of anchor
   NETWORK.set(_anchor, 0);// Don't network the anchor
   CONSTRAINT.set(_anchor, GEOM::SCREEN_WIDGET);
   WORLD::create   (_anchor, false); 
   WORLD::undisplay(_anchor, false);
}

void
Cam_int_fp::CAMwidget::undisplay_anchor()
{
   WORLD::undisplay(_anchor, false);
   _a_displayed=0; 
}

void   
Cam_int_fp::CAMwidget::display_anchor(
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
Cam_int_fp::CAMwidget::drag_anchor(
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
Cam_int_fp::predown(
   CEvent &e, 
   State *&
   )
{
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
      double dist = r.dist();
      _down_pt = r.point() + dist * r.vec();
   } else {
      _down_pt = Wplane(data->center(),data->at_v()).intersect(ptr->cur());
   }
   _down_pt_2d = _down_pt;

   //Notify all the CAMobs's of the start of mouse down
  
   //if we're in cruise mode give it the scale and down point
   if(CamCruise::cur()) 
      {
         CamCruise::cur()->set_scale_pt(_scale_pt);
         CamCruise::cur()->set_down_pt(_down_pt);
      }


   data->start_manip();
   return 0;
}

////////////////////////////////////
//Right Click
////////////////////////////////////

int
Cam_int_fp::down(
   CEvent &e, 
   State *&s
   )
{
   DEVice_buttons *btns = (DEVice_buttons *)e._d;
   DEVice_2d      *ptr  = btns->ptr2d();
   VIEWptr         view(e.view());
   int             is_dot = _camwidg.anchor_displayed();
   int             x, y; view->get_size(x,y);
   PIXEL           curpix(ptr->cur());
   XYpt            curpt (curpix[0]/x*2-1, curpix[1]/y*2-1);

   //set timer...
   _clock.set();

   predown(e,s);
   if(CamBreathe::cur()) CamBreathe::cur()->pause();


   // Did we click on a camera icon?
   CamIcon *icon = CamIcon::intersect_all(ptr->cur()); 
   if (icon) {
      // XXX - fix
      _icon = icon;
      switch (icon->test_down(e, s)) {
       case CamIcon::RESIZE : _resizing = true;
       case CamIcon::FOCUS  : s = &_icon_click;
         brcase CamIcon::MOVE   : {
            s = &_move_view;
            return move(e, s);
         }
      }
      return 0;
   } else
      {
         if (is_dot)
            view->cam()->data()->set_center(_camwidg.anchor_wpt());
         _do_reset = is_dot;

         if(!is_dot){

            VIEWptr         view(e.view());
            view->save_cam();
         }
         return 0;
      }

   return 0;
}

//////////////////////////////
//Left-Click:tests for intersection
//with camera buttons
/////////////////////////////

int
Cam_int_fp::down2(
   CEvent &e, 
   State *&s
   )
{
   DEVice_buttons *btns = (DEVice_buttons *)e._d;
   DEVice_2d      *ptr  = btns->ptr2d();

   _view = e.view();

   CAMdataptr  data(_view->cam()->data());
   RAYhit      r   (_view->intersect(ptr->cur()));
   if (r.success()) {
      //check if intersects with buttons
      //////////////////
      //Orbit Button
      //////////////////
      if(r.geom()->name() == "orbit")
         {
            //if(CamBreathe::cur()) CamBreathe::cur()->stop(); 
            BaseJOTapp::deactivate_button();
            //_button = r.geom();
            //GELptr sad = r.geom();
            //r.geom()->toggle_active();
            //_button = r.geom();
            if(s == &_orbit)
               { 
                  s = (State *)-1;
                  return stop_orbit(e,s);
               } 
            else
               {
                  BaseJOTapp::activate_button("orbit");
                  stop_actions(e,s);
                  s = &_orbit;
                  return orbit(e,s);
               } 
         }
      //////////////////
      //Breathing Button
      //////////////////
      else if(r.geom()->name() == "breathe")
         {
            BaseJOTapp::toggle_button("breathe");
            return breathe(e,s); 
         }
      //////////////////
      //Cruise Button
      //////////////////
      else if(r.geom()->name() == "cruise")
         {
            if(CamBreathe::cur()) CamBreathe::cur()->stop(); 
            BaseJOTapp::deactivate_button();
            if(s == &_cruise)
               { 
                  s = (State *)-1;
                  return stop_cruise(e,s);
               } 
            else
               { 
                  BaseJOTapp::activate_button("cruise");
                  stop_actions(e,s);
                  s = &_cruise;
                  //BaseCollide::update_scene();
                  return cruise(e,s); 
               }
         }
      //////////////////
      //Grow Button
      //////////////////
      else if(r.geom()->name() == "grow")
         {
            BaseJOTapp::deactivate_button();
            if(s == &_grow)
               s = (State *)-1;
            else
               { 
                  BaseJOTapp::activate_button("grow");
                  stop_actions(e,s);
                  s = &_grow; 
               }
         }
      //////////////////
      //Gravity Button
      //////////////////
      else if(r.geom()->name() == "gravity")
         {
			 cout << "BUTTON IS HIT" << endl;
            //BaseJOTapp::deactivate_button();
            BaseJOTapp::update_button("gravity");
			_gravity->toggle_type();
         }
      //////////////////
      //CamSwitch Button
      //////////////////
      else if(r.geom()->name() == "eye_button")
         {
            stop_actions(e,s);

            s = (State *)-1;
            BaseJOTapp::cam_switch(e,s);
         }
   }

   return 0;
}
///////////////////////////
//Stop Actions:stops all
//possible scheduled actions
///////////////////////////
int
Cam_int_fp::stop_actions(
   CEvent &e, 
   State *&s
   )
{
   if(CamOrbit::cur()) CamOrbit::cur()->stop(); 
   //if(CamBreathe::cur()) CamBreathe::cur()->stop();
   if(CamCruise::cur()) CamCruise::cur()->stop(); 
   return 0;
}

////////////////////////////////////////////////////////////////////////////
//while in the orbit state, when mouse down chooses to zoom or change orbit 
//speed and direction
///////////////////////////////////////////////////////////////////////////
int 
Cam_int_fp::choose(
   CEvent &e,
   State *&s
   )
{
   _clock.set();

   DEVice_2d *ptr =(DEVice_2d *)e._d;
   PIXEL      te   (ptr->cur());
   XYvec      delta(ptr->delta());
   double     tdelt(the_time() - _dtime);

   _dist += sqrt(delta * delta);

   VEXEL sdelt(te - _start_pix);

   int xa=0,ya=1;
   if (Config::get_var_bool("FLIP_CAM_MANIP",false,true))
      swap(xa,ya);
     
   if (fabs(sdelt[ya])/sdelt.length() > 0.9 && tdelt > 0.05) {
      s = &_orb_zoom;
      ptr->set_old(_start_pix);
   } else if (tdelt < 0.1 && _dist < 0.03)
      return 0;
   else {
      if (fabs(sdelt[xa])/sdelt.length() > 0.6 )
         s = &_orb_rot;
      else s = &_orb_zoom;
      ptr->set_old(_start_pix);
   }


   /* if we got this far, we actually have a valid choice, so save the camera */  
   
   VIEWptr         view(e.view());
   view->save_cam();

   return 0;
}


int
Cam_int_fp::stop_orbit(CEvent &e, State *&s) 
{
   CamOrbit::cur()->stop(); 

   return 0;
}


//Orbit function, schedules the camera to orbit around a point
int
Cam_int_fp::orbit(CEvent &e, State *&s) 
{

   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
//   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   VIEWptr         view(e.view());
     


   //get the object to orbit
   RAYhit r(view->intersect(data->center()));  
   //if it hits nothing or the sky box, can't orbit so return
   if (!r.success() || (r.geom()->name() == "Skybox Geom")) 
      {
         BaseJOTapp::toggle_button("orbit");
         s = (State *)-1;
         return 0;
      }

   GEOMptr geom(ray_geom(r,GEOM::null));

   //start orbit
   CamOrbit::cur() = new CamOrbit(cam, geom->bbox().center());

   view->save_cam();
   view->schedule(&*CamOrbit::cur());

   return 0;
}



//Breathe function, schedules a breathing animation for the camera
int
Cam_int_fp::breathe(CEvent &e, State *&) 
{
   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
//   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   VIEWptr         view(e.view());
      
//if(CamBreathe::cur()) CamBreathe::cur()->stop();

   if(CamBreathe::cur())
      {
         CamBreathe::cur()->stop();
         CamBreathe::cur() = NULL;
      }
   else
      {
         CamBreathe::cur() = new CamBreathe(cam);
         view->save_cam();
         view->schedule(&*CamBreathe::cur());
         _breathing = true;
      }

   return 0;
}


int
Cam_int_fp::cruise_down(CEvent &e, State *&s) 
{
   DEVice_buttons *btns = (DEVice_buttons *)e._d;
   DEVice_2d      *ptr  = btns->ptr2d();
   VIEWptr         view(e.view());
   int             x, y; view->get_size(x,y);
   PIXEL           curpix(ptr->cur());
   XYpt            curpt (curpix[0]/x*2-1, curpix[1]/y*2-1);

   //robcm
   if(_land_clock.elapsed_time()<.5)
      {
         RAYhit      r   (_view->intersect(ptr->cur()));
         if (r.success() && CamCruise::cur())
            {
			   cout << "Loading Model Collision" << endl; 
               GEOMptr g = GEOM::upcast(r.geom());
               CamCruise::cur()->travel(r.point());
			   _speed = .00001 * g->bbox().dim().length();
               CamCruise::cur()->speed(.00001 * g->bbox().dim().length());
               //_gravity->set_grav(g, Wvec(1,0,0), 0);
			   _collision->set_land(g);
			   cout << "Loading Finished" << endl;
               //_gravity->set_globe(g);
               //cout << "VOL: " << g->bbox().volume() << endl;
               //cout << "DIM: " << g->bbox().dim().length() << endl;
            }
         return 0;
      }

   predown(e,s);
   _clock.set();
   _land_clock.set();
   
   //if the user clicks at the edge of the screen rotate
   if (fabs(curpt[0]) > .6 )//|| fabs(curpt[1]) > .6)
      {
         s = &_cruise_rot;
         return 0;
      }

   //else zoom
   s = &_cruise_zoom;
   return 0;
}


//in orbit state zooms in and out on orbiting object
int
Cam_int_fp::cruise_zoom(
   CEvent &e,
   State *&
   )
{
   //XYvec      delta(_te-_tp);
   //double     ratio;

   DEVice_2d *ptr=(DEVice_2d *)e._d;
   CAMptr     cam  (e.view()->cam());
   CAMdataptr data (cam->data());
   XYvec      delta(ptr->delta());
   double     ratio;

   _tp    = ptr->old(); 
   _te    = ptr->cur();
   _move_clock.set();

   CamCruise::cur()->pause();
   if(CamBreathe::cur()) CamBreathe::cur()->pause();

   if (data->persp()) {
      Wvec    movec(data->at() - data->from());

      data->set_center(data->at());
      //BaseCollide::instance()->get_move(.1 * movec.normalized() 
      //                                                                      * (movec.length() * delta[1] * -4));
/*
  Wvec    movec(_down_pt - data->from());

  data->set_center(_down_pt);*/
      // data->translate(.1 * movec.normalized() * (movec.length() * delta[1] * -4));
      data->translate(_speed * movec.normalized() * (movec.length() * delta[1] * -4));

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

   //data->translate(-delta[0]/2 * data->right_v() * ratio);
   //data->set_at(Wline(data->from(), data->at_v()).project(data->center()));
   //data->set_center(data->at());


   /*
     XYvec      delta(_te-_tp);
     double     ratio;

     Wpt     spt  (XYpt(_tp[0],_te[1]));
     Wvec    svec (spt - Wline(data->from(), data->at_v()).project(spt));
     double  sfact(1 + delta[1]);


     data->translate( svec * (1.0 - sfact));
     data->set_width (data->width() * sfact);
     data->set_height(data->height()* sfact);
     ratio = data->height();

     //.2 is temperary to slow it down a bit
     Wvec velocity = .2 * delta[0]/2 * data->at_v() * ratio;
 
     data->translate(velocity);
     data->set_at(Wline(data->from(), data->at_v()).project(data->center()));
     data->set_center(data->at());
   */
   return 0;
}

int
Cam_int_fp::cruise_zoom_up(
   CEvent &e,
   State *&s
   )
{
        
   if(_clock.elapsed_time()<.1 || _move_clock.elapsed_time()> .2)
      {
         CamCruise::cur()->pause();
         if(CamBreathe::cur()) CamBreathe::cur()->unpause();
         //travel(e,s);
      }
   else
      {
         CamCruise::cur()->set_cruise(_tp,_te); 
      }
   return 0;
}


//Cruise Control, moves the camera foward automatically
//towards point of interest
int
Cam_int_fp::cruise(CEvent &e, State *&s) 
{
        

   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
//   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   VIEWptr         view(e.view());
      

   CamCruise::cur() = new CamCruise(cam, data->center());

   view->save_cam();
   view->schedule(&*CamCruise::cur());

   return 0;
}

//Orbit function, schedules the camera to orbit around a point
int
Cam_int_fp::stop_cruise(CEvent &e, State *&s) 
{
   CamCruise::cur()->stop(); 
   CamCruise::cur() = NULL;

   return 0;
}

//stop the breathing animation
int
Cam_int_fp::stop_breathe(CEvent &e, State *&) 
{
   CamBreathe::cur()->stop(); 
   CamBreathe::cur() = NULL;

   return 0;
}



//in orbit state zooms in and out on orbiting object
int
Cam_int_fp::orbit_zoom(
   CEvent &e,
   State *&
   )
{
   DEVice_2d *ptr=(DEVice_2d *)e._d;
   CAMptr     cam  (e.view()->cam());
   CAMdataptr data (cam->data());
   XYvec      delta(ptr->delta());
//   double     ratio;


   XYpt        tp    = ptr->old(); 
   XYpt        te    = ptr->cur();

   double distscale = -100 * (te[1] - tp[1]); //tp.dist(te);

   cam->set_zoom(1);
   cam->set_min(NDCpt(XYpt(-1,-1)));
   cam->data()->changed();

   //move along the view vector, at the normalized length
   Wvec delt = data->at_v().normalized();

   //update camera
   data->translate(distscale * delt);
   data->set_center(data->at());

   return 0;
}

//while in the orbit stage you can change orbit movement
int
Cam_int_fp::orbit_rot(
   CEvent &e, 
   State *&
   ) 
{

   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
   DEVice_2d  *ptr=(DEVice_2d *)e._d;

        

   CamOrbit::cur()->set_orbit(XYpt(0,0),XYpt(0,0)); 
   cam->set_zoom(1);
   cam->set_min(NDCpt(XYpt(-1,-1)));
   cam->data()->changed();

   XYpt        cpt   = data->center();
   double      radsq = sqr(1+fabs(cpt[0])); // squared rad of virtual cylinder
   _tp    = ptr->old(); 
   _te    = ptr->cur();
   _move_clock.set();

   Wvec   op  (_tp[0], 0, 0);             // get start and end X coordinates
   Wvec   oe  (_te[0], 0, 0);             //    of cursor motion
   double opsq = op * op, oesq = oe * oe;
   double lop  = opsq > radsq ? 0 : sqrt(radsq - opsq);
   double loe  = oesq > radsq ? 0 : sqrt(radsq - oesq);
   Wvec   nop  = Wvec(op[0], 0, lop).normalized();
   Wvec   noe  = Wvec(oe[0], 0, loe).normalized();
   double dot  = nop * noe;

   if (fabs(dot) > 0.0001) {
      data->rotate(Wline(data->center(), Wvec::Y()),
                   -2*Acos(dot) * Sign(_te[0]-_tp[0]));

      Wvec   dvec  = data->from() - data->center();
      double rdist = _te[1]-_tp[1];
//      double tdist = Acos(Wvec::Y() * dvec.normalized());

      CAMdata   dd = CAMdata(*data);

      Wline raxe(data->center(),data->right_v());
      data->rotate(raxe, rdist);
      data->set_up(data->from() + Wvec::Y());
      if (data->right_v() * dd.right_v() < 0)
         *data = dd;
   }

   return 0;
}
//while in the orbit stage you can change orbit movement
int
Cam_int_fp::orbit_rot_up(
   CEvent &e, 
   State *&
   ) 
{
   if(_clock.elapsed_time()<.1 || _move_clock.elapsed_time()>.2)
      {
         CamOrbit::cur()->pause();       //pause orbit
         if(CamBreathe::cur()) CamBreathe::cur()->unpause();
      }
   else
      {
         CamOrbit::cur()->set_orbit(_tp,_te); //set the constant X-axis rotation
         if(CamBreathe::cur()) CamBreathe::cur()->pause();
      }
   return 0;
}

//Forward Function, moves the camera foward
int
Cam_int_fp::forward(
   CEvent &e, 
   State *&s
   ) 
{
   if(s==&_cruise)
      {
         CamCruise::cur()->speed(1); 
         return 0;
      }

   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
//   DEVice_2d  *ptr=(DEVice_2d *)e._d;


   VIEWptr         view(e.view());
   RAYhit ray(view->intersect(data->center()));         

   if(!ray.success() || ray.dist() > 1 || ray.dist() < 0)
      {
         cam->set_zoom(1);
         cam->set_min(NDCpt(XYpt(-1,-1)));
         cam->data()->changed();

         //move along the view vector, at the normalized length
         Wvec delt = data->at_v().normalized();

         //update camera
         data->translate(delt);
         data->set_center(data->at());
      }

   
   return 0;
}

//Back Function, moves the camera backwards
int
Cam_int_fp::back(
   CEvent &e, 
   State *&s
   ) 
{ 
   if(s==&_cruise)
      {
         CamCruise::cur()->speed(-1); 
         return 0;
      }


   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
//   DEVice_2d  *ptr=(DEVice_2d *)e._d;

   XYpt        cpt   = (data->from()-data->at_v());
   VIEWptr         view(e.view());
   RAYhit ray(view->intersect(cpt));         

//if(!ray.success() || ray.dist() > 1 || ray.dist() < 0)
//   {
   cam->set_zoom(1);
   cam->set_min(NDCpt(XYpt(-1,-1)));
   cam->data()->changed();

   //move along the view vector, at the normalized length
   Wvec delt = data->at_v().normalized();

   //update camera
   data->translate(-delt);
   data->set_center(data->at());
//   }
   
   return 0;
}

//Left function, moves the camera left
int
Cam_int_fp::left(
   CEvent &e, 
   State *&
   ) 
{

   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
//   DEVice_2d  *ptr=(DEVice_2d *)e._d;

   cam->set_zoom(1);
   cam->set_min(NDCpt(XYpt(-1,-1)));
   cam->data()->changed();

   //move along the right axis, at the normalized length
   Wvec delt = 2*data->right_v().normalized();

   data->translate(-delt);
   //data->set_at(Wline(data->from(), data->at_v()).project(_down_pt));
   data->set_center(data->at());
   
   return 0;
}

//Right function, moves the camera right
int
Cam_int_fp::right(
   CEvent &e, 
   State *&
   ) 
{
  
   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
//   DEVice_2d  *ptr=(DEVice_2d *)e._d;

   cam->set_zoom(1);
   cam->set_min(NDCpt(XYpt(-1,-1)));
   cam->data()->changed();

   //move along the right axis, at the normalized length
   Wvec delt = 2*data->right_v().normalized();

   data->translate(delt);
   //data->set_at(Wline(data->from(), data->at_v()).project(_down_pt));
   data->set_center(data->at());
   
   return 0;
}

//rot function, turns the direction of the camera
int
Cam_int_fp::rot(
   CEvent &e, 
   State *&
   ) 
{
        
   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
   DEVice_2d  *ptr=(DEVice_2d *)e._d;


//cam->set_zoom(1);
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
      data->rotate(Wline(data->from(), Wvec::Y()),
                   -2*Acos(dot) * Sign(te[0]-tp[0]));

      Wvec   dvec  = data->from() - data->center();
      double rdist = te[1]-tp[1];
//      double tdist = Acos(Wvec::Y() * dvec.normalized());

      CAMdata   dd = CAMdata(*data);

      Wline raxe(data->from(),data->right_v());
      data->rotate(raxe, rdist);
      data->set_up(data->from() + Wvec::Y());
      if (data->right_v() * dd.right_v() < 0)
         *data = dd;
   }



   //rot not working when cruising =[
   data->set_center(data->at());
   //VIEWptr         view(e.view());
   //view->save_cam();
   return 0;
}

int
Cam_int_fp::focus(
   CEvent &e,
   State *&s
   ) 
{
   DEVice_buttons *btns=(DEVice_buttons *)e._d;
   DEVice_2d      *ptr=btns->ptr2d();
   VIEWptr         view(e.view());
   CAMptr          cam (view->cam());
   CAMdataptr      data(cam->data());

   RAYhit  r   (view->intersect(ptr->cur()));
   GEOMptr geom(ray_geom(r,GEOM::null));
   if (geom) {
      XYvec    vec  (VEXEL(25,0));
      RAYhit   sil_r(ptr->cur() + vec);
      RAYhit   sil_l(ptr->cur() - vec);
      geom->intersect(sil_r);
      geom->intersect(sil_l);

      if ((r.norm().normalized() * Wvec::Y()) < 0.98 &&
          ( sil_r.success() && !sil_l.success()) ||
          (!sil_r.success() &&  sil_l.success())) {
         Wvec sil_comp;                         // do silhouette focus
         if (sil_r.success())
            sil_comp = -data->right_v() * 6.0 * r.dist();
         else
            sil_comp =  data->right_v() * 6.0 * r.dist();

         Wvec v(sil_comp + r.norm() * 4.0 * r.dist());
         Wpt  newpos(r.surf() + r.dist() * v.normalized());
         newpos[1] = data->from()[1];
         newpos = r.surf() + (newpos - r.surf()).normalized() * r.dist();
         //  if(s==&_cruise)
         //      CamCruise::cur()->
         //else
         //{
    
         new CamFocus(view, newpos, r.surf(), newpos + Wvec::Y(), r.surf(),
                      data->width(), data->height());
      }
      else {
         Wpt  center(r.surf());              // do normal focus
         Wvec norm  (r.norm().normalized());
         if (norm * Wvec::Y() > 0.98)
            norm = -data->at_v();

         Wvec  off   (cross(Wvec::Y(),norm).normalized() * 3);
         Wvec  atv   (data->at_v() - Wvec::Y() * (data->at_v() * Wvec::Y()));
         // Must use 4.0 instead of 4 due to AIX C++ weirdness
         Wvec  newvec(norm*6 + 4.0*Wvec::Y()); 
         if ((center + newvec + off - data->from()).length() > 
             (center + newvec - off - data->from()).length())
            off = newvec - off;
         if (data->persp())
            off = (newvec + off).normalized() * 
//                  (geom->bbox().min() - geom->bbox().max()).length();
               (data->from()-Wline(data->from(),data->at()).project(center)).
               length();

         //       if(s = &_cruise)
         //              {
         //               CamCruise::cur()->set_focus(center + off);
         //              }
         //       else
         //       {
         new CamFocus(view, center + off, center, center + off + Wvec::Y(), 
                      center, data->width(), data->height());
         // }
      }
   } else
      cerr << "Cam_int_fp::focus() - Nothing to focus on!!!\n";

   return 0;
}

int
Cam_int_fp::move(
   CEvent &e, 
   State *&
   )
{
   if (_icon) _icon->set_icon_loc(((DEVice_2d *)e._d)->cur());
   return 0;
}

int
Cam_int_fp::moveup(
   CEvent &, 
   State *&
   )
{
   for (int i = 0; i < _up_obs.num(); i++) 
      _up_obs[i]->reset(_do_reset);  // reset everyone watching us
   _geom = 0;
   _icon = 0;
   return 0;
}

int
Cam_int_fp::iconmove(
   CEvent &e, 
   State *&s
   )
{
   if (_resizing) return _icon->icon_move(e, s);
   return 0;
}

int
Cam_int_fp::iconup(
   CEvent &e, 
   State *&s
   )
{
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
Cam_int_fp::dragup(
   CEvent &e, 
   State *&s
   )
{
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
                from,
                ray.surf(),                 // at
                from + Wvec::Y(),           // up
                ray.surf(),                 // center
                data->width(), data->height());

   return 0;
}

int
Cam_int_fp::up(
   CEvent &e, 
   State *&s
   )
{

   //if the camera is rotating and user taps...then pause orbit
   if(CamOrbit::cur() && _clock.elapsed_time()<.1)
      {
         CamOrbit::cur()->pause();
         if(CamBreathe::cur()) CamBreathe::cur()->unpause();
      }
   //if(CamCruise::cur())
   //      return travel(e,s);

   if(CamBreathe::cur()) CamBreathe::cur()->unpause();
   DEVice_buttons *btns=(DEVice_buttons *)e._d;
   DEVice_2d      *ptr=btns->ptr2d();
   VIEWptr         view(e.view());
   CAMptr          cam (view->cam());

   for (int i = 0; i < _up_obs.num(); i++) 
      _up_obs[i]->reset(_do_reset);  // reset everyone watching us
   if (_camwidg.anchor_displayed()) {
      if (ptr->cur().dist(_camwidg.anchor_pos()) < DOT_DIST)
         {
            focus(e,s);
         }
      reset(1);
   } else if (ptr->cur().dist(_down_pt_2d) < DOT_DIST) {
      RAYhit ray(view->intersect(ptr->cur()));
      if (ray.success()) {
         GEOMptr geom(ray_geom(ray,GEOM::null));
         //geom->set_xform(Wtransf::scaling(2,2,2));
         if (CamOrbit::cur()) {CamOrbit::cur()->set_target(ray.surf());}

         // Create the anchor (blue ball) on the surface:
         _camwidg.display_anchor(ray.surf());
         // If clicked on a mesh, make it the "focus":
         BMESHptr m = gel_to_bmesh(ray.geom());
         if (m)
            BMESH::set_focus(m, dynamic_cast<Patch*>(ray.appear()));
      } else {
         //if (CamCruise::cur()) {CamCruise::cur()->unset_target();}
         // _camwidg.display_anchor(hitp.intersect(ray.screen_point()));
      }
   }
   cam->data()->end_manip();
   return 0;
}
/*
//robcm - travel
int
Cam_int_fp::travel(
CEvent &e, 
State *&s
)
{
DEVice_buttons *btns=(DEVice_buttons *)e._d;
DEVice_2d      *ptr=btns->ptr2d();
VIEWptr         view(e.view());
CAMptr          cam (view->cam());

if(_down_pt_2d == ptr->cur() && _clock2.elapsed_time() < .2) 
{
RAYhit ray(view->intersect(ptr->cur()));
if (ray.success())
CamCruise::cur()->travel(ray.surf());
}
else
{
_clock2.set();
_down_pt_2d = ptr->cur();
}

return 0;
}
*/
int
Cam_int_fp::grow(
   CEvent &e, 
   State *&s
   )
{
   DEVice_2d *ptr=(DEVice_2d *)e._d;
   CAMptr     cam  (e.view()->cam());
   CAMdataptr data (cam->data());
   XYvec      delta(ptr->delta());
//   double     ratio;

   _tp    = ptr->old(); 
   _te    = ptr->cur();

   cout << "grow: " << _te[0]-_tp[0] << endl;
   if (CamBreathe::cur()) CamBreathe::grow(_te[0]-_tp[0]);


   return 0;
}

int
Cam_int_fp::grow_change(
   CEvent &e, 
   State *&s
   )
{
   cout << "_" << endl;
   return 0;
}
//
// reset() - UP observer method
//   Cam_int_fp is an observer of 'UP' events on other objects.
// In particular, Cam_int_fp observes the 'UP' events of ObjManip
// so that the Cam_int_fp can clear its widget after an object
// is dragged.
//
void
Cam_int_fp::reset(int rst)
{
   if (rst)
      _camwidg.undisplay_anchor();
}

// Make a view (camera icon)
void
Cam_int_fp::make_view(
   CXYpt  &curpt
   )
{
   // Create new cam icon
   _icon = CamIcon::create(curpt, _view->cam());
   _geom = 0;
   
   reset(1);
}

int
Cam_int_fp::toggle_buttons(
   CEvent &e, 
   State *&s
   )
{
   BaseJOTapp::button_toggle(e,s);
   return 0;
}

// end of file cam_fp.C
