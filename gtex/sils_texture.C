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
 * sils_texture.C
 **********************************************************************/
#include "std/config.H"
#include "geom/gl_view.H"
#include "sils_texture.H"

/**********************************************************************
 * SilsTexture:
 **********************************************************************/
int
SilsTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);

   if (!_patch)
      return 0;

   static bool antialias = Config::get_var_bool("ANTIALIAS_SILS",true);
   if (antialias) {
      // push attributes, enable line smoothing, and set line width
      GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*_width),
                                GL_CURRENT_BIT);
   } else {
      // push attributes and set line width
      glPushAttrib(GL_LINE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
      glLineWidth(float(v->line_scale()*_width)); // GL_LINE_BIT
   }

   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT
   GL_COL(_color, alpha());     // GL_CURRENT_BIT

   if (_draw_borders)
      _patch->cur_sils().draw(_cb);
   else
      _patch->cur_sils().get_filtered(!BorderEdgeFilter()).draw(_cb);
   
   // restore gl state:
   glPopAttrib();

   return 1;
}
   
int 
SilsTexture::draw_vis_ref() 
{
   // You can pick what you can see ... for SilsTexture you see
   // silhouettes, so that's what we draw into the visibility
   // reference image:

   if (!_patch)
      return 0;

   if (_draw_borders)
      return ColorIDTexture::draw_edges(&_patch->cur_sils(), 3.0);

   EdgeStrip sils = _patch->cur_sils().get_filtered(!BorderEdgeFilter());
   return ColorIDTexture::draw_edges(&sils, 3.0);
}

/* end of file sils_texture.C */
