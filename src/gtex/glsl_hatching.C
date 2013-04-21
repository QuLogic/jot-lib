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
 * glsl_hatching.C
 *****************************************************************/
#include "glsl_hatching.H"
#include "gtex/glsl_toon.H"
#include "gtex/tone_shader.H"
#include "gtex/ref_image.H"
static bool debug = Config::get_var_bool("DEBUG_GLSL_HATCHING", false);

inline ToneShader*
get_tone_shader(Patch* p)
{
   return new ToneShader(p);
}

HatchingStyle::HatchingStyle(int l1, int l2, int l3, int l4)
{
   set_style(l1, l2, l3, l4);
}

void
HatchingStyle::set_style(int l1, int l2, int l3, int l4)
{
   int s[4] = {l1, l2, l3, l4};
   for (int i=0; i < HatchingStyle::LAYER_NUMBER; ++i) {
      int index = i*3;
      switch (s[i]) {
       case 1:
         visible[i] = 1;
         color[index] =  149.0f;
         color[index+1] = 47.0f;
         color[index+2] = 7.0f;
         line_spacing[i] = 15.0f;
         line_width[i] = 0.8f;
         do_paper[i] =  1;
         angle[i] = 45.0f;
         perlin_amp[i] = 10.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  0.0f;
         highlight[i] =  0;
         channel[i] = 0;
         break;
       case 2:
         visible[i] = 1;
         color[index] =  6.0f;
         color[index+1] =  90.0f;
         color[index+2] = 126.0f;
         line_spacing[i] = 5.5f;
         line_width[i] = 0.5f;
         do_paper[i] =  1;
         angle[i] = 30.0f;
         perlin_amp[i] = 10.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  0.0f;
         highlight[i] =  0;
         channel[i] = 0;
         break;
       case 3:
         visible[i] = 1;
         color[index] =  234.0f;
         color[index+1] =  137.0f;
         color[index+2] = 33.0f;
         line_spacing[i] = 8.5f;
         line_width[i] = 0.5f;
         do_paper[i] =  1;
         angle[i] = 45.0f;
         perlin_amp[i] = 6.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  -0.2f;
         highlight[i] =  0;
         channel[i] = 0;
         break;
       case 4:
         visible[i] = 1;
         color[index] =  234.0f;
         color[index+1] =  137.0f;
         color[index+2] = 33.0f;
         line_spacing[i] = 8.5f;
         line_width[i] = 0.5f;
         do_paper[i] =  1;
         angle[i] = -65.0f;
         perlin_amp[i] = 6.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  -0.2f;
         highlight[i] =  0;
         channel[i] = 0;
         break;
       case 5:
         visible[i] = 1;
         color[index] =  36.0f;
         color[index+1] =  16.0f;
         color[index+2] = 5.0f;
         line_spacing[i] = 10.5f;
         line_width[i] = .85f;
         do_paper[i] =  1;
         angle[i] = 77.0f;
         perlin_amp[i] = 2.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  0.0f;
         highlight[i] =  0;
         channel[i] = 0;
         break;
       case 6:
         visible[i] = 1;
         color[index] =  36.0f;
         color[index+1] =  16.0f;
         color[index+2] = 5.0f;
         line_spacing[i] = 10.5f;
         line_width[i] = 0.8f;
         do_paper[i] =  1;
         angle[i] = 35.0f;
         perlin_amp[i] = 2.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  0.0f;
         highlight[i] =  0;
         channel[i] = 1;
         break;
       case 7:
         visible[i] = 1;
         color[index] =  70.0f;
         color[index+1] =  67.0f;
         color[index+2] = 31.0f;
         line_spacing[i] = 12.5f;
         line_width[i] = 0.5f;
         do_paper[i] =  1;
         angle[i] = 35.0f;
         perlin_amp[i] = 7.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  0.0f;
         highlight[i] =  0;
         channel[i] = 0;
         break;
       case 8:
         visible[i] = 1;
         color[index] =  63.0f;
         color[index+1] =  48.0f;
         color[index+2] = 9.0f;
         line_spacing[i] = 8.0f;
         line_width[i] = 0.7f;
         do_paper[i] =  1;
         angle[i] = 47.0f;
         perlin_amp[i] = 7.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  0.0f;
         highlight[i] =  0;
         channel[i] = 0;
         break;
       case 9:
         visible[i] = 1;
         color[index] =  255.0f;
         color[index+1] =  255.0f;
         color[index+2] = 255.0f;
         line_spacing[i] = 8.0f;
         line_width[i] = 0.7f;
         do_paper[i] =  1;
         angle[i] = 57.0f;
         perlin_amp[i] = 7.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  -0.2f;
         highlight[i] =  1;
         channel[i] = 1;
         break;
       case 10:
         visible[i] = 1;
         color[index] =  63.0f;
         color[index+1] =  48.0f;
         color[index+2] = 9.0f;
         line_spacing[i] = 8.5f;
         line_width[i] = 0.7f;
         do_paper[i] =  1;
         angle[i] = -57.0f;
         perlin_amp[i] = 5.0f;
         perlin_freq[i] = 0.015f;
         min_tone[i] =  0.4f;
         highlight[i] =  0;
         channel[i] = 2;
         break;
       case 11:
         visible[i] = 1;
         color[index] =  185.0f;
         color[index+1] =  56.0f;
         color[index+2] = 34.0f;
         line_spacing[i] = 10.0f;
         line_width[i] = 0.7f;
         do_paper[i] =  1;
         angle[i] = 35.0f;
         perlin_amp[i] = 7.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  0.0f;
         highlight[i] =  0;
         channel[i] = 0;
         break;
       case 12:
         visible[i] = 1;
         color[index] =  246.0f;
         color[index+1] =  160.0f;
         color[index+2] = 73.0f;
         line_spacing[i] = 10.0f;
         line_width[i] = 0.7f;
         do_paper[i] =  1;
         angle[i] = 75.0f;
         perlin_amp[i] = 7.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  0.0f;
         highlight[i] =  0;
         channel[i] = 11;
         break;
       case 13:
         visible[i] = 1;
         color[index] =  55.0f;
         color[index+1] =  73.0f;
         color[index+2] = 147.0f;
         line_spacing[i] = 10.0f;
         line_width[i] = 0.7f;
         do_paper[i] =  1;
         angle[i] = 75.0f;
         perlin_amp[i] = 7.0f;
         perlin_freq[i] = 0.01f;
         min_tone[i] =  0.0f;
         highlight[i] =  0;
         channel[i] = 2;
         break;
       default:
         visible[i] = 0;
         color[index] =  0.0f;
         color[index+1] =  0.0f;
         color[index+2] =  0.0f;
         line_spacing[i] = 0.0f;
         line_width[i] = 0.0f;
         do_paper[i] =  0;
         angle[i] = 0.0f;
         perlin_amp[i] = 0.0f;
         perlin_freq[i] = 0.0f;
         min_tone[i] =  0.0f;
         highlight[i] =  0;
         channel[i] = 0;
         break;
      }

   }
}

/**********************************************************************
 * GLSLHatching:
 *
 *   Draws evenly spaced 2D lines that vary width and pressure,
 *   rendered with paper effect. Looks like pencil.
 **********************************************************************/
GLuint GLSLHatching::_program(0);
bool   GLSLHatching::_did_init(false);
GLint  GLSLHatching::_tone_tex_loc(-1);
GLint  GLSLHatching::_width_loc(-1);
GLint  GLSLHatching::_height_loc(-1);
GLint  GLSLHatching::_perlin_loc(-1);
GLint GLSLHatching::_visible_loc(-1);       //is this layer visible
GLint GLSLHatching::_color_loc(-1);
GLint GLSLHatching::_line_spacing_loc(-1);
GLint GLSLHatching::_line_width_loc(-1);
GLint GLSLHatching::_do_paper_loc(-1);
GLint GLSLHatching::_angle_loc(-1);
GLint GLSLHatching::_perlin_amp_loc(-1);
GLint GLSLHatching::_perlin_freq_loc(-1);
GLint GLSLHatching::_min_tone_loc(-1);
GLint GLSLHatching::_highlight_loc(-1);
GLint GLSLHatching::_channel_loc(-1);

GLSLHatching::GLSLHatching(Patch* p) : GLSLPaperShader(p), _tone_shader(0)
{
   set_tone_shader(get_tone_shader(p));

   _perlin = 0;
   _perlin_generator= Perlin::get_instance();
   if (!_perlin_generator)
      _perlin_generator= new Perlin();

   _style = new HatchingStyle(1, 0, 0, 0);
}

GLSLHatching::~GLSLHatching()
{
   gtextures().delete_all();
}

void
GLSLHatching::set_style(int l1, int l2, int l3, int l4)
{
   /*
     _tone_shader->set_layer(
     0,     // layer 0
     1,     // enabled
     1,     // remap nl
     2,     // use smoothstep
     1,     // use backlight
     0.2,   // e0
     0.7);  // e1
     _tone_shader->set_layer(
     1,     // layer 1
     1,     // enabled
     0,     // don't remap nl
     2,     // use smoothstep
     0,     // no backlight
     0.7f,  // e0
     0.95f, // e1
     0.8f,  // s0
     1.0f); // s1
     _tone_shader->set_layer(
     2,     // layer 2
     1,     // enabled
     0,     // don't remap nl
     2,     // use smoothstep
     0,     // no backlight
     0.7f,  // e0
     0.95f);// e1
   */
   _style->set_style(l1, l2, l3, l4);

}

void
GLSLHatching::set_tone_shader(ToneShader* g)
{
   if (g == _tone_shader)
      return;
   delete _tone_shader;
   _tone_shader = g;
   changed();
}

bool
GLSLHatching::get_variable_locs()
{
   GLSLPaperShader::get_variable_locs();

   // tone map variables
   get_uniform_loc("tone_map", _tone_tex_loc);
   get_uniform_loc("width",    _width_loc);
   get_uniform_loc("height",   _height_loc);

   get_uniform_loc("sampler2D_perlin", _perlin_loc);

   get_uniform_loc("visible", _visible_loc);       //is this layer visible
   get_uniform_loc("color", _color_loc);
   get_uniform_loc("line_spacing", _line_spacing_loc);
   get_uniform_loc("line_width", _line_width_loc);
   get_uniform_loc("do_paper", _do_paper_loc);
   get_uniform_loc("angle", _angle_loc);
   get_uniform_loc("perlin_amp", _perlin_amp_loc);
   get_uniform_loc("perlin_freq", _perlin_freq_loc);
   get_uniform_loc("min_tone", _min_tone_loc);
   get_uniform_loc("highlight", _highlight_loc);
   get_uniform_loc("channel", _channel_loc);

   return true;
}

void
GLSLHatching::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);

   // set the color from the VIEW background color:
   GL_COL(VIEW::peek()->color(), alpha());    // GL_CURRENT_BIT
}

bool
GLSLHatching::set_uniform_variables() const
{
   GLSLPaperShader::set_uniform_variables();

   VIEWptr view = VIEW::peek();

   //tone map variables
   glUniform1i(_tone_tex_loc, ColorRefImage::lookup_raw_tex_unit(0));
   glUniform1i(_width_loc,  view->width());
   glUniform1i(_height_loc, view->height());

   // XXX - compile error on next line:
   // error: invalid conversion from 'int*' to 'const GLint*'
//   glUniform1iv( _visible_loc, HatchingStyle::LAYER_NUMBER, _style->visible);       //is this layer visible

   glUniform3fv( _color_loc, HatchingStyle::LAYER_NUMBER, _style->color);

   glUniform1fv( _line_spacing_loc, HatchingStyle::LAYER_NUMBER,_style->line_spacing);
   glUniform1fv( _line_width_loc, HatchingStyle::LAYER_NUMBER, _style->line_width);
   // XXX - compile error on next line:
   // error: invalid conversion from 'int*' to 'const GLint*'
//   glUniform1iv( _do_paper_loc, HatchingStyle::LAYER_NUMBER, _style->do_paper);
   glUniform1fv( _angle_loc, HatchingStyle::LAYER_NUMBER, _style->angle);
   glUniform1fv( _perlin_amp_loc, HatchingStyle::LAYER_NUMBER, _style->perlin_amp);
   glUniform1fv( _perlin_freq_loc, HatchingStyle::LAYER_NUMBER, _style->perlin_freq);
   glUniform1fv( _min_tone_loc, HatchingStyle::LAYER_NUMBER, _style->min_tone);
   // XXX - compile error on next line:
   // error: invalid conversion from 'int*' to 'const GLint*'
//   glUniform1iv( _highlight_loc, HatchingStyle::LAYER_NUMBER, _style->highlight);
   // XXX - compile error on next line:
   // error: invalid conversion from 'int*' to 'const GLint*'
//   glUniform1iv( _channel_loc, HatchingStyle::LAYER_NUMBER, _style->channel);

   if (_perlin)
      glUniform1i(_perlin_loc, _perlin->get_tex_unit() - GL_TEXTURE0);
   else
      glUniform1i(_perlin_loc, 0 );


   return true;
}


void
GLSLHatching::init_textures()
{
   GLSLPaperShader::init_textures();
   _perlin = _perlin_generator->create_perlin_texture2();

}

void
GLSLHatching::activate_textures()
{
   GLSLPaperShader::activate_textures();
   if (_perlin)
      activate_texture(_perlin);
}


void
GLSLHatching::request_ref_imgs()
{
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
}

int
GLSLHatching::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0 && _tone_shader) ? _tone_shader->draw(VIEW::peek()) : 0;
}

GTexture_list
GLSLHatching::gtextures() const
{
   return _tone_shader ? GTexture_list(_tone_shader) : GTexture_list();
}

// end of file glsl_hatching.C
