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
#include "control_line.H"

ControlLineTexture::ControlLineTexture(Patch* patch) :
   OGLTexture(patch),
   _solid(new SolidColorTexture(patch, Color::white)),
   _toon(new ToonTexture_1D(patch)),
   _controlframe(new ControlFrameTexture(patch)),
   _sils(new SilsTexture(patch)) 
{
   static const bool SHOW_BORDERS =
      Config::get_var_bool("CTRL_LINE_SHOW_BORDERS",true);

   _toon->set_tex_name("nprdata/toon_textures/clear-gray-keyline.png");
   _toon->set_color(Color::white);

   _controlframe->set_color(Color::blue_pencil_d);
   _sils        ->set_color(Color::blue_pencil_d);
   _sils        ->set_draw_borders(SHOW_BORDERS);
}

ControlLineTexture::~ControlLineTexture() 
{
   gtextures().delete_all();
}

void 
ControlLineTexture::draw_filled_tris(double alpha) 
{
   _solid->draw_with_alpha(alpha);
   _toon->draw_with_alpha(alpha);
}

void 
ControlLineTexture::draw_non_filled_tris(double alpha) 
{
   _controlframe->draw_with_alpha(alpha);
   _sils->draw_with_alpha(alpha);
}

inline bool
do_solid(BMESH* m)
{
   // when secondary faces are shown, we want to see thru
   // the outer layers, so don't draw the solid color texture
   return m && !m->show_secondary_faces();
}

int 
ControlLineTexture::draw(CVIEWptr& v) 
{
   if (_ctrl)
      return _ctrl->draw(v);
   if (do_solid(mesh())) {
      _solid->set_color(_patch->color());
      _solid->draw(v);
   }

   // save polygon offset set:
   glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);

   // use reduced offset, or else the toon pixels have
   // bug-fights with the basecoat pixels.
   GL_VIEW::init_polygon_offset(0.5,0.5);  // GL_ENABLE_BIT

   // draw the toon as mainly transparent overlay:
   _toon->draw(v);

   // restore previous polygon offset settings:
   glPopAttrib();

   // save the lines for draw final

   return _patch->num_faces();
}

int 
ControlLineTexture::draw_final(CVIEWptr& v) 
{
   if (_ctrl)
      return _ctrl->draw_final(v);

   // draw final tends not to use the z-buffer, but we need it:
   glPushAttrib(GL_ENABLE_BIT);
   glEnable(GL_DEPTH_TEST);
   _controlframe->draw(v);
   _sils->draw(v);
   glPopAttrib();

   return _patch->num_faces();
}

int 
ControlLineTexture::draw_vis_ref() 
{
   _solid->draw_vis_ref();
   _sils ->draw_vis_ref();
   return _patch->num_faces();
}

// end of file control_line.C
