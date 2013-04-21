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
#ifndef JOT_CAM_EDIT_H
#define JOT_CAM_EDIT_H

#include "disp/cam.H"
#include "manip/manip.H"
#include "manip/cam_pz.H"
#include "base_jotapp/base_jotapp.H"


/*****************************************************************
 * Cam_int_edit:
 *
 *      A jot camera interactor that can edit(scale, rotate) models
 *****************************************************************/
class Cam_int_edit : public Interactor<Cam_int_edit,Event,State>,
                     public UPobs {
 private:
   GEOMptr _model;

 protected:
   double        _dtime;
   double        _dist;
   PIXEL         _start_pix;
   XYpt          _scale_pt;
   Wpt           _down_pt;
   XYpt          _down_pt_2d;
   VIEWptr       _view;
   State         _cam_pan, _cam_zoom, _cam_rot, _cam_choose, _cam_drag;
   State         _move_view, _icon_click;
   State         _scale, _scale_move, _rotate;
   State         _scalex, _scaley, _scalez,_scalex_move,_scaley_move,_scalez_move;
   State         _rot_x, _rot_y, _rot_z;
   State         _rot_x_move, _rot_y_move, _rot_z_move;
   State         _ymove;


   State         _entry2, _but_trans, _but_rot, _but_zoom, _but_drag, _phys;

   ARRAY<UPobs*> _up_obs;
   int           _do_reset;
   GEOMptr       _geom;
   bool          _resizing;


 public:
   void add_up_obs(UPobs *o)   { _up_obs += o; }
   virtual int  predown(CEvent &e,State *&s);
   virtual int  down   (CEvent &e,State *&s);
   virtual int  down2  (CEvent &e,State *&s);
   virtual int  choose (CEvent &e,State *&s);
   virtual int  rot    (CEvent &e,State *&s);
   virtual int  zoom   (CEvent &e,State *&s);
   virtual int  pan    (CEvent &e,State *&s);
   virtual int  rot2   (CEvent &e,State *&s);
   virtual int  zoom2  (CEvent &e,State *&s);
   virtual int  pan2   (CEvent &e,State *&s);

   virtual int  rot_z   (CEvent &e,State *&s);
   virtual int  rot_y   (CEvent &e,State *&s);
   virtual int  rot_x   (CEvent &e,State *&s);
   virtual int  scale   (CEvent &e,State *&s);
   virtual int  scale_x   (CEvent &e,State *&s);
   virtual int  scale_y   (CEvent &e,State *&s);
   virtual int  scale_z   (CEvent &e,State *&s);
   virtual int  edit_down (CEvent &e,State *&s);

   virtual int  up     (CEvent &e,State *&s);
   virtual  void reset(int rst) {}
   virtual int  noop   (CEvent & ,State *&) { return 0; }

   virtual int  toggle_buttons  (CEvent &e,State *&s);

   virtual ~Cam_int_edit() {}
   Cam_int_edit(CEvent &d, CEvent &m, CEvent &u, CEvent &d2, CEvent &u2, 
                CEvent &dt, CEvent &dr, CEvent &dz, 
                CEvent &ut, CEvent &ur, CEvent &uz);

   State   *entry2    () { return &_entry2; }
};


#endif // JOT_CAM_EDIT_H
