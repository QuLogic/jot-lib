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
/**********************************************************************
 * patch_pen.C:
 **********************************************************************/
#include "gtex/patch_color.H"

#include "patch_pen.H"

static bool debug = Config::get_var_bool("DEBUG_PATCH_PEN", false);

/*****************************************************************
 * CHANGE_PATCH_CMD:
 *****************************************************************/
CHANGE_PATCH_CMD::CHANGE_PATCH_CMD(Patch* p, CBface_list& faces) :
   // just take faces that are not already in the new patch:
   _faces(faces.filter(BfaceFilter() + !PatchFaceFilter(p))),
   _old_patches(_faces),
   _new_patch(p) 
{
   assert(_new_patch);
}

void
CHANGE_PATCH_CMD::add(Bface* f)
{
   // skip if null face or already in the new patch:
   if (!f || f->patch() == _new_patch)
      return;
   _faces       += f;
   _old_patches += f->patch();
   if (is_done()) {
      _new_patch->add(f);
      _new_patch->mesh()->changed(BMESH::PATCHES_CHANGED);
   }
}

bool 
CHANGE_PATCH_CMD::doit() 
{
   if (is_done())
      return true;

   _new_patch->add(_faces);
   _new_patch->mesh()->changed(BMESH::PATCHES_CHANGED);

   return COMMAND::doit();      // update state in COMMAND
}

bool 
CHANGE_PATCH_CMD::undoit() 
{
   if (!is_done())
      return true;

   assert(_old_patches.num() == _faces.num());
   for (int i=0; i<_old_patches.num(); i++) {
      _old_patches[i]->add(_faces[i]);
      _old_patches[i]->mesh()->changed(BMESH::PATCHES_CHANGED);
   }
   return COMMAND::undoit();    // update state in COMMAND
}

/*****************************************************************
 * PatchPen
 *****************************************************************/
PatchPen::PatchPen(CGEST_INTptr &gest_int, CEvent &d, CEvent &m, CEvent &u) :
   Pen(str_ptr("patch"), gest_int, d, m, u),
   _prev_rendering("")
{
   // Set up GESTURE FSA (First matched will be executed)

   // catch garbage strokes early and reject them
   _draw_start += DrawArc(new GarbageGuard,  drawCB(&PatchPen::garbage_cb));
   _draw_start += DrawArc(new TapGuard,      drawCB(&PatchPen::tap_cb));
   _draw_start += DrawArc(new StrokeGuard,   drawCB(&PatchPen::stroke_cb));
}

void
PatchPen::activate(State* s) 
{
   Pen::activate(s);
   if (_view) {
      _prev_rendering = _view->rendering();
      _view->set_rendering(PatchColorTexture::static_name());
   }
}

bool
PatchPen::deactivate(State* s)
{
   if (_view && _view->rendering() == PatchColorTexture::static_name()) {
      _view->set_rendering(_prev_rendering);
   }
   return Pen::deactivate(s);
}

void
PatchPen::notify_down()
{
   _cmd = 0; // start a new command
   Bface* f = cur_face();
   if (f) {
      _cmd = new CHANGE_PATCH_CMD(f->patch());
      WORLD::add_command(_cmd); // calls _cmd->doit()
   } else {
      err_adv(debug, "PatchPen::notify_down: did not hit mesh");
   }
}

void
PatchPen::notify_move()
{
   if (_cmd)
      _cmd->add(cur_face());
}

int
PatchPen::garbage_cb(CGESTUREptr&, DrawState*&)
{
   return 0;
}

int
PatchPen::tap_cb(CGESTUREptr& tap, DrawState*&)
{
   // if the user taps a face, start a new patch and add the face:
   Bface* f = cur_face();
   if (f) {
      assert(f->mesh());
      WORLD::add_command(
         new CHANGE_PATCH_CMD(f->mesh()->new_patch(), Bface_list(f))
         );
   } else {
      err_adv(debug, "PatchPen::tap_cb: did not hit mesh");
   }
   return 0;
}

int
PatchPen::stroke_cb(CGESTUREptr& stroke, DrawState*&)
{
   if (!(_cmd && stroke && stroke->is_closed()))
      return 0;

   // Add all faces enclosed by the stroke.

   // XXX - TODO: make it smarter so it only reaches faces
   //       via a surface walk starting at first face and
   //       remaining inside the lasso.

   Patch* p = _cmd->patch();
   assert(p && p->mesh());
   CBface_list& faces = p->mesh()->faces();
   CPIXEL_list& lasso = stroke->pts();
   for (int i=0; i<faces.num(); i++) {
      if (faces[i]->patch() != p) {
         for (int j=1; j<=3; j++) {
            if (lasso.contains(faces[i]->v(j)->pix())) {
               _cmd->add(faces[i]);
               j = 3; // break the loop
            }
         }
      }
   }
   return 0;
}

// end of file patch_pen.C
