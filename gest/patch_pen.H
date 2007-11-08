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
 * patch_pen.H:
 *****************************************************************/
#ifndef PATCH_PEN_H_IS_INCLUDED
#define PATCH_PEN_H_IS_INCLUDED

#include "geom/command.H"
#include "gtex/ref_image.H"
#include "mesh/bmesh.H"
#include "pen.H"

/*****************************************************************
 * CHANGE_PATCH_CMD
 *
 *   Change a set of faces from one patch to another.  The
 *   faces can come from different patches, and will be moved
 *   to the same patch. When the command is undone, each face
 *   is moved back to its original patch.
 *
 *****************************************************************/
MAKE_PTR_SUBC(CHANGE_PATCH_CMD,COMMAND);
class CHANGE_PATCH_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   CHANGE_PATCH_CMD(Patch* p, CBface_list& faces=Bface_list());

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3(
      "CHANGE_PATCH_CMD", CHANGE_PATCH_CMD*, COMMAND, CCOMMAND*
      );

   //******** ACCESSORS ********

   // The patch that faces are moved to:
   Patch* patch() const { return _new_patch; }

   // Add additional faces to the command 
   // (if the command has been executed, each newly added
   // face is moved to the target patch immediately):
   void add(Bface* f);

   //******** COMMAND VIRTUAL METHODS ********

   // This moves the faces to the new patch
   virtual bool doit();

   // This puts the faces back in the original patch
   virtual bool undoit();

 protected:
   //******** MEMBER DATA ********
   Bface_list   _faces;         // faces to move to the new patch
   Patch_list   _old_patches;   // the old patches (one per face)
   Patch*       _new_patch;     // the new patch
};

/*****************************************************************
 * PatchPen
 *
 * User manual:
 *
 *      Tap on a triangle to start a new patch.
 *
 *      Draw a stroke starting on a triangle T to move all faces
 *      under the stroke to T's patch.
 *
 *      If the stroke is closed (ends near its beginning), all
 *      triangles that lie partially or fully inside the stroke
 *      (in screen space) are moved to T's patch.
 *
 *      Undo/redo are supported.
 *****************************************************************/
class PatchPen : public Pen {
 public:

   //******** MANAGERS ********
   PatchPen(CGEST_INTptr &gest_int, CEvent &d, CEvent &m, CEvent &u);
   ~PatchPen() {}

   //******** GESTURE CALLBACK METHODS ********

   virtual int  garbage_cb(CGESTUREptr& gest, DrawState*&);
   virtual int  tap_cb    (CGESTUREptr& gest, DrawState*&);
   virtual int  stroke_cb (CGESTUREptr& gest, DrawState*&);

   //******** Pen VIRTUAL METHODS ********

   virtual void activate(State *);
   virtual bool deactivate(State *);

   //******** GestObs METHODS ********

   virtual void notify_down();
   virtual void notify_move();

 protected:
   //******** MEMBER DATA ********
   // rendering style before activating pen:
   str_ptr              _prev_rendering; 
   CHANGE_PATCH_CMDptr  _cmd; // command that does all the work

   //******** UTILITIES ********

   //! For creating callbacks to use with GESTUREs:
   typedef CallMeth_t<PatchPen,GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) { return new draw_cb_t(this,m); }

   // Return the control face currently under the cursor:
   Bface* cur_face() const {
      return VisRefImage::get_ctrl_face(get_ptr_position());
   }
};

#endif // PATCH_PEN_H_IS_INCLUDED

// end of file patch_pen.H
