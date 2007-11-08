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
 * creases_texture.C
 **********************************************************************/


#include "geom/gl_view.H"
#include "creases_texture.H"

/**********************************************************************
 * CreasesTexture:
 **********************************************************************/
int
CreasesTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);

   // set line width, init line smoothing, and push attributes:
   GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*_width), GL_CURRENT_BIT);

   GL_COL(_color, alpha());     // GL_CURRENT_BIT
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT

   // draw creases (not view-dependent)
   if (!BasicTexture::draw(v)) {

      int dl = _dl.get_dl(v, 1, _patch->stamp());
      if (dl)
         glNewList(dl, GL_COMPILE);

      _patch->draw_crease_strips(_cb);

      // restore gl state:
      GL_VIEW::end_line_smooth();

      // end the display list here
      if (_dl.dl(v)) {
         _dl.close_dl(v);

         // the display list is built; now execute it
         BasicTexture::draw(v);
      }
   }   

   return 1;
}
   
/* end of file creases_texture.C */
