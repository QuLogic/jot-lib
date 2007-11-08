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
/*****************************************************************
 * xform_pen.H:
 *****************************************************************/
#ifndef XFORM_PEN_H_IS_INCLUDED
#define XFORM_PEN_H_IS_INCLUDED

#include "geom/command.H"
#include "gtex/ref_image.H"
#include "mesh/bmesh.H"
#include "gest/pen.H"

/*****************************************************************
 * XformPen
 *
 * User manual:
 *
 *      Double tap a mesh to activate the floor widget and
 *      attach floor to mesh. Use middle button to manipulate
 *      floor / 3D cursor to change mesh xform.
 *****************************************************************/
class XformPen : public Pen {
 public:

   //******** MANAGERS ********
   XformPen(CGEST_INTptr &gest_int, CEvent &d, CEvent &m, CEvent &u);
   virtual ~XformPen();

   //******** GESTURE CALLBACK METHODS ********

   virtual int     garbage_cb(CGESTUREptr& gest, DrawState*&);
   virtual int         tap_cb(CGESTUREptr& gest, DrawState*&);
   virtual int  double_tap_cb(CGESTUREptr& gest, DrawState*&);
   virtual int      stroke_cb(CGESTUREptr& gest, DrawState*&);
   virtual int      cancel_cb(CGESTUREptr& gest, DrawState*&);

   //******** Pen VIRTUAL METHODS ********

 protected:
   BMESHptr     _mesh;

   //******** UTILITIES ********

   //! For creating callbacks to use with GESTUREs:
   typedef CallMeth_t<XformPen,GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) { return new draw_cb_t(this,m); }

   // Return the control face currently under the cursor:
   Bface* cur_face() const {
      return VisRefImage::get_ctrl_face(get_ptr_position());
   }
};

#endif // XFORM_PEN_H_IS_INCLUDED

// end of file xform_pen.H
