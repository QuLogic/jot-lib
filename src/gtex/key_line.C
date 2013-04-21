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
 * key_line.C
 **********************************************************************/
#include "disp/colors.H"
#include "geom/gl_view.H"
#include "mesh/patch.H"

#include "key_line.H"
#include "solid_color.H"
#include "sil_frame.H"

bool KeyLineTexture::_show_hidden = false;

inline COLOR
patch_color(Patch* p, CCOLOR& default_color = Color::white)
{
   return (p && p->has_color()) ? p->color() : default_color;
}

/**********************************************************************
 * KeyLineTexture:
 **********************************************************************/
KeyLineTexture::KeyLineTexture(Patch* patch) :
   OGLTexture(patch),
   _solid(new SolidColorTexture(patch, patch_color(patch))),
   _toon(new GLSLToonShader(patch)),
   _sil_frame(new SilFrameTexture(patch))
{
   _toon->set_tex(
      Config::JOT_ROOT() + "nprdata/toon_textures/clear-gray-keyline.png"
      );

   float w = (float)Config::get_var_dbl("KEYLINE_SIL_WIDTH", 2.0);
   _sil_frame->set_sil_width(w);
   _sil_frame->set_color(Color::black);
   _sil_frame->set_sil_alpha(0.5);
   _sil_frame->set_crease_alpha(0.5);
}

KeyLineTexture::~KeyLineTexture()
{
   gtextures().delete_all();
}

void
KeyLineTexture::set_toon(GLSLToonShader* t)
{
   if (t == _toon) return;
   delete _toon;
   _toon = t;
}

GTexture_list 
KeyLineTexture::gtextures() const 
{
   // compiler gripes stupidly if we use: const_cast<GTexture*>
   GTexture_list ret(
      (GTexture*)(_solid),      
      (GTexture*)(_sil_frame)
      );
   if (_toon) {
      ret.add((GTexture*)(_toon));
   }
   return ret;
}

int
KeyLineTexture::set_color(CCOLOR& c)
{
   // create a lighter shade for the base coat,
   // and a darker shade for the toon shading:
   _solid->set_color(interp(c, Color::white, 0.5));
   if (_toon)
      _toon->set_color(interp(c, Color::black, 0.5));

   return 1;
}

int
KeyLineTexture::set_sil_color(CCOLOR& c)
{
   _sil_frame->set_color(c);
   return 1;
}

int
KeyLineTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);

   glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
   glDepthFunc(GL_LEQUAL);

   if (_patch->has_color())
      set_color(_patch->color());

   _solid->draw(v);
   if (_toon)
      _toon->draw(v);

   if (_show_hidden)
      draw_hidden(v);

   _sil_frame->draw(v);

   glPopAttrib();

   return _patch->num_faces();
}

int
KeyLineTexture::draw_hidden(CVIEWptr& v)
{
   // Save OpenGL state:
   glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);

   // Set new OpenGL state:
   glLineStipple(1,0x00ff);     // GL_LINE_BIT
   glEnable(GL_LINE_STIPPLE);   // GL_ENABLE_BIT
   glDisable(GL_DEPTH_TEST);    // GL_ENABLE_BIT

   // Find current line widths and colors...
   COLOR s_col = _sil_frame->sil_color();
   COLOR c_col = _sil_frame->crease_color();
   GLfloat s_width = _sil_frame->sil_width();
   GLfloat c_width = _sil_frame->crease_width();

   // Draw thinner, lighter lines:
   _sil_frame->set_sil_width(GLfloat(s_width * 0.75));
   _sil_frame->set_crease_width(GLfloat(c_width * 0.75));
   _sil_frame->set_sil_color(interp(s_col, COLOR::white, 0.3));
   _sil_frame->set_crease_color(interp(c_col, COLOR::white, 0.3));

   // Draw silhouettes and creases in dotted style, no depth testing:
   _sil_frame->draw(v);

   // Restore line widths and colors:
   _sil_frame->set_sil_width(s_width);
   _sil_frame->set_crease_width(c_width);
   _sil_frame->set_sil_color(s_col);
   _sil_frame->set_crease_color(c_col);

   // Restore old OpenGL state:
   glPopAttrib();

   return _patch->num_faces();
}

void 
KeyLineTexture::draw_filled_tris(double alpha) 
{
   _solid->draw_with_alpha(alpha);
   if (_toon)
      _toon->draw_with_alpha(alpha);
}

void 
KeyLineTexture::draw_non_filled_tris(double alpha) 
{
   _sil_frame->draw_with_alpha(alpha);
}

// end of file key_line.C
