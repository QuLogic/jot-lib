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
 * fader_texture.C
 **********************************************************************/
#include "geom/gl_view.H"
#include "mesh/patch.H"
#include "fader_texture.H"

/**********************************************************************
 * FaderTexture:
 *
 *      Fades from one texture to another over a given time interval.
 **********************************************************************/
int
FaderTexture::draw(CVIEWptr& v)
{
   // get the opacity of the fading texture:
   double a = fade(v->frame_time());

   // put the base GTexture in charge of itself:
   _base->set_ctrl(0);

   // are we done?
   if (a <= 0) {
      // our job is done
      int ret = _base->draw(v); // draw normally
      delete this;              // kill ourselves
      return ret;
   }

   // If the fading-out texture doesn't draw the patch
   // triangles in filled mode, then all parts of the
   // fading-in texture need to use alpha = 1 - a.
   //
   // Otherwise, the fading-in texture can draw its filled
   // triangles (if any) with alpha = 1. In any case,
   // those parts of the fading-in texture that don't
   // correspond to filled triangles of the patch should
   // use alpha = 1 - a.

   // set GL state for blending the two
   glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
   glEnable(GL_BLEND);                                  // GL_ENABLE_BIT
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   // GL_COLOR_BUFFER_BIT

   // draw the base with alpha = 1 - a (where needed):
   _base->draw_filled_tris(_fader->draws_filled() ? 1.0 : 1-a);
   _fader->draw_with_alpha(a);
   _base->draw_non_filled_tris(1-a);

   // re-assert control of the base texture for next time:
   _base->set_ctrl(this);

   // restore state:
   glPopAttrib();

   return _patch->num_faces();
}
   
/* end of file fader_texture.C */
