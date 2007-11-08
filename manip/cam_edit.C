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

#include "manip/cam_edit.H"

using namespace mlib;

/*****************************************************************
 *
 *  Cam_int_edit :
 *****************************************************************/
Cam_int_edit::Cam_int_edit(
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
   Event  toggle(NULL, Evd('Y', KEYD));

   _cam_rot    += Arc(move_ev, Cb(&Cam_int_edit::rot));
   _cam_rot    += Arc(up_ev,   Cb(&Cam_int_edit::up,    (State *)-1));
   _cam_rot    += Arc(up2_ev,  Cb(&Cam_int_edit::up,    (State *)-1));
   _cam_pan    += Arc(move_ev, Cb(&Cam_int_edit::pan));
   _cam_pan    += Arc(up_ev,   Cb(&Cam_int_edit::up,    (State *)-1));
   _cam_pan    += Arc(down2_ev,Cb(&Cam_int_edit::noop,  &_cam_rot));
   _cam_zoom   += Arc(move_ev, Cb(&Cam_int_edit::zoom));
   _cam_zoom   += Arc(up_ev,   Cb(&Cam_int_edit::up,    (State *)-1));
   _cam_zoom   += Arc(down2_ev,Cb(&Cam_int_edit::noop,  &_cam_rot));
   _cam_choose += Arc(down2_ev,Cb(&Cam_int_edit::noop,  &_cam_rot));
   _cam_choose += Arc(move_ev, Cb(&Cam_int_edit::choose));
   _cam_choose += Arc(up_ev,   Cb(&Cam_int_edit::up,    (State *)-1));
   _cam_choose.set_name("Cam_int_edit::cam_choose");
   ////////////////////////////
   //Z Rotation
   ////////////////////////////
   
   _rot_z_move += Arc(move_ev, Cb(&Cam_int_edit::rot_z));
   _rot_z_move += Arc(up_ev,   Cb(&Cam_int_edit::up,    &_rot_z));
   _rot_z      += Arc(down_ev,   Cb(&Cam_int_edit::edit_down,  &_rot_z_move));
   _rot_z      += Arc(down2_ev,   Cb(&Cam_int_edit::down2));
   
   ////////////////////////////
   //Y Rotation
   ////////////////////////////
   _ymove      += Arc(move_ev, Cb(&Cam_int_edit::rot_y));
   _ymove      += Arc(up_ev,   Cb(&Cam_int_edit::up,    &_rot_y));
   _rot_y      += Arc(down_ev,   Cb(&Cam_int_edit::edit_down,  &_ymove));
   _rot_y      += Arc(down2_ev,   Cb(&Cam_int_edit::down2));
   ////////////////////////////
   //X Rotation
   ////////////////////////////
   _rot_x_move += Arc(move_ev, Cb(&Cam_int_edit::rot_x));
   _rot_x_move += Arc(up_ev,   Cb(&Cam_int_edit::up,    &_rot_x));
   _rot_x      += Arc(down_ev,   Cb(&Cam_int_edit::edit_down,  &_rot_x_move));
   _rot_x      += Arc(down2_ev,   Cb(&Cam_int_edit::down2));
   /////////////////////////////
   //Scale
   //////////////////////////
   _scale_move += Arc(move_ev, Cb(&Cam_int_edit::scale));
   _scale_move += Arc(up_ev,   Cb(&Cam_int_edit::up,    &_scale));
   _scale      += Arc(down_ev,   Cb(&Cam_int_edit::edit_down,  &_scale_move));
   _scale      += Arc(down2_ev,   Cb(&Cam_int_edit::down2));
   _scale      += Arc(toggle,   Cb(&Cam_int_edit::toggle_buttons));


   /////////////////////////////
   //ScaleX
   //////////////////////////
   _scalex_move += Arc(move_ev, Cb(&Cam_int_edit::scale_x));
   _scalex_move += Arc(up_ev,   Cb(&Cam_int_edit::up,    &_scalex));
   _scalex      += Arc(down_ev,   Cb(&Cam_int_edit::edit_down,  &_scalex_move));
   _scalex      += Arc(down2_ev,   Cb(&Cam_int_edit::down2));


   /////////////////////////////
   //ScaleY
   //////////////////////////
   _scaley_move += Arc(move_ev, Cb(&Cam_int_edit::scale_y));
   _scaley_move += Arc(up_ev,   Cb(&Cam_int_edit::up,    &_scaley));
   _scaley      += Arc(down_ev,   Cb(&Cam_int_edit::edit_down,  &_scaley_move));
   _scaley      += Arc(down2_ev,   Cb(&Cam_int_edit::down2));


   /////////////////////////////
   //ScaleZ
   //////////////////////////
   _scalez_move += Arc(move_ev, Cb(&Cam_int_edit::scale_z));
   _scalez_move += Arc(up_ev,   Cb(&Cam_int_edit::up,    &_scalez));
   _scalez      += Arc(down_ev,   Cb(&Cam_int_edit::edit_down,  &_scalez_move));
   _scalez      += Arc(down2_ev,   Cb(&Cam_int_edit::down2));


   _entry      += Arc(down_ev, Cb(&Cam_int_edit::down,  &_cam_choose));
   _entry      += Arc(down2_ev,   Cb(&Cam_int_edit::down2));


   _but_trans  += Arc(move_ev      , Cb(&Cam_int_edit::pan2,  &_but_trans));
   _but_trans  += Arc(trans_up_ev  , Cb(&Cam_int_edit::noop,    (State *)-1));
   _but_rot    += Arc(move_ev      , Cb(&Cam_int_edit::rot2,  &_but_rot));
   _but_rot    += Arc(rot_up_ev    , Cb(&Cam_int_edit::noop,    (State *)-1));
   _but_zoom   += Arc(move_ev      , Cb(&Cam_int_edit::zoom2, &_but_zoom));
   _but_zoom   += Arc(zoom_up_ev   , Cb(&Cam_int_edit::noop,    (State *)-1));

//     _phys       += Arc(trans_up_ev  , Cb(&Cam_int_edit::physup, (State *)-1));
//     _phys       += Arc(move_ev      , Cb(&Cam_int_edit::physmove));

// XXX - ?
//   _entry2     += Arc(trans_down_ev, Cb(&Cam_int_edit::physdown,&_phys));
   _entry2     += Arc(trans_down_ev, Cb(&Cam_int_edit::predown,&_but_trans));
   _entry2     += Arc(rot_down_ev,   Cb(&Cam_int_edit::predown,&_but_rot));
   _entry2     += Arc(zoom_down_ev,  Cb(&Cam_int_edit::predown,&_but_zoom));

}



int
Cam_int_edit::predown(
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
  
   data->start_manip();
   return 0;
}

int
Cam_int_edit::down(
   CEvent &e, 
   State *&s
   )
{
   DEVice_buttons *btns = (DEVice_buttons *)e._d;
   DEVice_2d      *ptr  = btns->ptr2d();
   VIEWptr         view(e.view());

   int             x, y; view->get_size(x,y);
   PIXEL           curpix(ptr->cur());
   XYpt            curpt (curpix[0]/x*2-1, curpix[1]/y*2-1);

   predown(e,s);
   
   //if the user clicks at the edge of the screen rotate
   if (fabs(curpt[0]) > .85 || fabs(curpt[1]) > .9)
      {
         s = &_cam_rot;
         return 0;
      }

   // } 

   return 0;
}


int
Cam_int_edit::down2(
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
      //////////////////
      //Scale Button
      //////////////////
      if(r.geom()->name() == "scale")
         {
            BaseJOTapp::deactivate_button();
            if(s == &_scale)
               s = (State *)-1;
            else
               {
                  BaseJOTapp::activate_button("scale");
                  s = &_scale;
               }
         }
      //////////////////
      //ScaleX Button
      //////////////////
      if(r.geom()->name() == "scalex")
         {
            BaseJOTapp::deactivate_button();
            if(s == &_scalex)
               s = (State *)-1;
            else
               {
                  BaseJOTapp::activate_button("scalex");
                  s = &_scalex;
               }
         }
      //////////////////
      //ScaleY Button
      //////////////////
      if(r.geom()->name() == "scaley")
         {
            BaseJOTapp::deactivate_button();
            if(s == &_scaley)
               s = (State *)-1;
            else
               {
                  BaseJOTapp::activate_button("scaley");
                  s = &_scaley;
               }
         }
      //////////////////
      //ScaleZ Button
      //////////////////
      if(r.geom()->name() == "scalez")
         {
            BaseJOTapp::deactivate_button();
            if(s == &_scalez)
               s = (State *)-1;
            else
               {
                  BaseJOTapp::activate_button("scalez");
                  s = &_scalez;
               }
         }
      //////////////////
      //Rotate Button
      //////////////////
      if(r.geom()->name() == "rotateX")
         {
            BaseJOTapp::deactivate_button();
            if(s == &_rot_x)
               s = (State *)-1;
            else
               {
                  BaseJOTapp::activate_button("rotateX");
                  s = &_rot_x;
               }
         }
      //////////////////
      //RotateY Button
      //////////////////
      if(r.geom()->name() == "rotateY")
         {
            BaseJOTapp::deactivate_button();
            if(s == &_rot_y)
               s = (State *)-1;
            else
               {
                  BaseJOTapp::activate_button("rotateY");
                  s = &_rot_y;
               }
         }
      //////////////////
      //Rotate Button
      //////////////////
      if(r.geom()->name() == "rotateZ")
         {
            BaseJOTapp::deactivate_button();
            if(s == &_rot_z)
               s = (State *)-1;
            else
               {
                  BaseJOTapp::activate_button("rotateZ");
                  s = &_rot_z;
               }
         }
      /////////////////
      //Eye Button
      ////////////////
      else if(r.geom()->name() == "eye_button")
         {
            s = (State *)-1;
            BaseJOTapp::cam_switch(e,s);
         }
   }
   return 0;
}


int 
Cam_int_edit::choose(
   CEvent &e,
   State *&s
   )
{
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
   /* if we got this far, we actually have a valid choice, so save the camera */   VIEWptr         view(e.view());
   view->save_cam();

   return 0;
}



int
Cam_int_edit::zoom2(
   CEvent &e,
   State *&
   )
{
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
Cam_int_edit::rot2(
   CEvent &e,
   State *&
   )
{
   CAMptr  cam(e.view()->cam());

   cam->set_zoom(1);
   cam->set_min(NDCpt(XYpt(-1,-1)));
   cam->data()->changed();

   return 0;
}

int
Cam_int_edit::pan2(
   CEvent &e,
   State *&
   )
{
   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   CAMptr      cam  (e.view()->cam());
   PIXEL       curpt(ptr->cur());

   cam->set_min(cam->min() + NDCvec(ptr->delta()));
   cam->data()->changed();
   ptr->set_cur(curpt);
   return 0;
}

int
Cam_int_edit::pan(
   CEvent &e,
   State *&
   )
{
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
Cam_int_edit::zoom(
   CEvent &e,
   State *&
   )
{
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
Cam_int_edit::rot(
   CEvent &e, 
   State *&
   ) 
{
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

int
Cam_int_edit::up(
   CEvent &e, 
   State *&s
   )
{
//   DEVice_buttons *btns=(DEVice_buttons *)e._d;
//   DEVice_2d      *ptr=btns->ptr2d();
   VIEWptr         view(e.view());
   CAMptr          cam (view->cam());

   _model = NULL;
   cam->data()->end_manip();
   return 0;
}


///////////////////////////////////////////////////
//scale: scales the selected model 
//              based on mouse movement
///////////////////////////////////////////////////
int  
Cam_int_edit::scale   
(CEvent &e,
 State *&s)
{
   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   
   if(_model == NULL)
      return 0;

   XYpt tp   = ptr->old(); 
   XYpt te  = ptr->cur();

   double Scale;
   double dist = te[1]-tp[1];

   //rotate the Y-axis
   if(dist < 0)
      Scale = 1-(abs(dist));
   else
      Scale = 1+(abs(dist));

   Wtransf xf = _model->obj_to_world() * Wtransf::scaling(Scale,Scale,Scale);

   _model->set_xform(xf);
 
   return 0;

}



///////////////////////////////////////////////////
//scaleX: scales the selected model 
//              based on mouse movement in the x direction
///////////////////////////////////////////////////
int  
Cam_int_edit::scale_x   
(CEvent &e,
 State *&s)
{
   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   
   if(_model == NULL)
      return 0;

   XYpt tp   = ptr->old(); 
   XYpt te  = ptr->cur();

   double Scale;
   double dist = te[1]-tp[1];

   //rotate the Y-axis
   if(dist < 0)
      Scale = 1-(abs(dist));
   else
      Scale = 1+(abs(dist));

   Wtransf xf = _model->obj_to_world() * Wtransf::scaling(Scale,1,1);

   _model->set_xform(xf);
 
   return 0;

}


///////////////////////////////////////////////////
//scale_y: scales the selected model 
//              based on mouse movement
///////////////////////////////////////////////////
int  
Cam_int_edit::scale_y   
(CEvent &e,
 State *&s)
{
   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   
   if(_model == NULL)
      return 0;

   XYpt tp   = ptr->old(); 
   XYpt te  = ptr->cur();

   double Scale;
   double dist = te[1]-tp[1];

   //rotate the Y-axis
   if(dist < 0)
      Scale = 1-(abs(dist));
   else
      Scale = 1+(abs(dist));

   Wtransf xf = _model->obj_to_world() * Wtransf::scaling(1,Scale,1);

   _model->set_xform(xf);
 
   return 0;

}


///////////////////////////////////////////////////
//scale_z: scales the selected model 
//              based on mouse movement
///////////////////////////////////////////////////
int  
Cam_int_edit::scale_z   
(CEvent &e,
 State *&s)
{
   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   
   if(_model == NULL)
      return 0;

   XYpt tp   = ptr->old(); 
   XYpt te  = ptr->cur();

   double Scale;
   double dist = te[1]-tp[1];

   //rotate the Y-axis
   if(dist < 0)
      Scale = 1-(abs(dist));
   else
      Scale = 1+(abs(dist));

   Wtransf xf = _model->obj_to_world() * Wtransf::scaling(1,1,Scale);

   _model->set_xform(xf);
 
   return 0;

}

///////////////////////////////////////////////////
//rot_z: rotates selected model around the z-axis
///////////////////////////////////////////////////
int  
Cam_int_edit::rot_z   
(CEvent &e,
 State *&s)
{
   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   
   //return if you didn't select a model to edit
   if(_model == NULL)
      return 0;

   XYpt tp  = ptr->old(); 
   XYpt te  = ptr->cur();

   //get a angle based on mouse movement(up/down)
   double angle = (te[0]-tp[0])*3;

   //rotate on the X-axis
   Wtransf xf = _model->obj_to_world() * Wtransf::rotation(Wvec::Z(), angle);
   _model->set_xform(xf);
   return 0;
}


///////////////////////////////////////////////////
//rot_y: rotates selected model around the y-axis
///////////////////////////////////////////////////
int  
Cam_int_edit::rot_y   
(CEvent &e,
 State *&s)
{
   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   
   //return if you didn't select a model to edit
   if(_model == NULL)
      return 0;

   XYpt tp  = ptr->old(); 
   XYpt te  = ptr->cur();

   //get a angle based on mouse movement(up/down)
   double angle = (te[0]-tp[0])*3;

   //rotate on the X-axis
   Wtransf xf = _model->obj_to_world() * Wtransf::rotation(Wvec::Y(), angle);
   _model->set_xform(xf);
   return 0;
}

///////////////////////////////////////////////////
//rot_x: rotates selected model around the x-axis
///////////////////////////////////////////////////
int  
Cam_int_edit::rot_x   
(CEvent &e,
 State *&s)
{
   CAMptr      cam (e.view()->cam());
   CAMdataptr  data(cam->data());
   DEVice_2d  *ptr=(DEVice_2d *)e._d;
   
   //return if you didn't select a model to edit
   if(_model == NULL)
      return 0;

   XYpt tp  = ptr->old(); 
   XYpt te  = ptr->cur();

   //get a angle based on mouse movement(up/down)
   double angle = (te[1]-tp[1])*3;

   //rotate on the X-axis
   Wtransf xf = _model->obj_to_world() * Wtransf::rotation(Wvec::X(), angle);
   _model->set_xform(xf);
   return 0;
}

////////////////////////////////////////////////////
//edit_down: grabs the model that is clicked on for
//                   editing
/////////////////////////////////////////////////////
int 
Cam_int_edit::edit_down (
   CEvent &e,
   State *&s)
{
   DEVice_buttons *btns = (DEVice_buttons *)e._d;
   DEVice_2d      *ptr  = btns->ptr2d();

   _view = e.view();

   CAMdataptr  data(_view->cam()->data());
   RAYhit      r   (_view->intersect(ptr->cur()));
   if (r.success()) 
      _model = (ray_geom(r,GEOM::null));
   return 0;
                
}

int
Cam_int_edit::toggle_buttons(
   CEvent &e, 
   State *&s
   )
{
   BaseJOTapp::button_toggle(e,s);
   return 0;
}

// cam_edit.C
