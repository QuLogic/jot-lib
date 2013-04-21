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
 * dots.C
 *****************************************************************/
#include "mesh/mi.H"
#include "dots.H"
#include "gtex/tone_shader.H"
#include "gtex/ref_image.H"

static bool debug = Config::get_var_bool("DEBUG_DOTS", false);

inline GTexture*
get_tone_shader(Patch* p)
{
   ToneShader* ret = new ToneShader(p);
   ret->set_tex(Config::JOT_ROOT() + "nprdata/toon_textures/clear-black.png");
   return ret;
}

/*****************************************************************
 * DotsShader:
 *
 *   Does dot-based halftoning
 *****************************************************************/
GLuint  DotsShader::_program(0);
bool    DotsShader::_did_init(false);
GLint   DotsShader::_origin_loc(-1);
GLint   DotsShader::_u_vec_loc(-1);
GLint   DotsShader::_v_vec_loc(-1);
GLint   DotsShader::_st_loc(-1);
GLint   DotsShader::_style_loc(-1);
GLint   DotsShader::_tone_tex_loc(-1);
GLint   DotsShader::_width_loc(-1);
GLint   DotsShader::_height_loc(-1);

DotsShader::DotsShader(Patch* p) :
   GLSLShader(p),
   _tone_shader(0),
   _style(0)
{
   set_tone_shader(get_tone_shader(p));
}

DotsShader::~DotsShader() 
{
   gtextures().delete_all(); 
}

void
DotsShader::set_tone_shader(GTexture* g)
{
   if (g == _tone_shader)
      return;
   delete _tone_shader;
   _tone_shader = g;
   changed();
}

bool 
DotsShader::get_variable_locs()
{
   get_uniform_loc("origin", _origin_loc);
   get_uniform_loc("u_vec",  _u_vec_loc);
   get_uniform_loc("v_vec",  _v_vec_loc);
   get_uniform_loc("st",     _st_loc);
   get_uniform_loc("style",  _style_loc);

   // tone map variables
   get_uniform_loc("tone_map", _tone_tex_loc);
   get_uniform_loc("width",  _width_loc);
   get_uniform_loc("height", _height_loc);

   return true;
}

bool
DotsShader::set_uniform_variables() const
{
   assert(_patch);   

   //tone map variables
   glUniform1i(_tone_tex_loc, ColorRefImage::lookup_raw_tex_unit(0));
   glUniform1i(_width_loc,  VIEW::peek()->width());
   glUniform1i(_height_loc, VIEW::peek()->height());

   if (_patch->get_use_visibility_test()) {
      _patch->update_dynamic_samples(IDVisibilityTest());
   } else {
      _patch->update_dynamic_samples();
   }
   
   glUniform2fv(_origin_loc, 1, float2(_patch->sample_origin()));
   glUniform1i (_style_loc, _style);

   if (_patch->get_do_lod()) {
      glUniform1f (_st_loc, float(_patch->lod_t()));
      glUniform2fv(_u_vec_loc,  1, float2(_patch->lod_u()));
      glUniform2fv(_v_vec_loc,  1, float2(_patch->lod_v()));
   } else {
      glUniform1f (_st_loc, float(0));
      glUniform2fv(_u_vec_loc,  1, float2(_patch->sample_u_vec()));
      glUniform2fv(_v_vec_loc,  1, float2(_patch->sample_v_vec()));
   }

   return true;
}

GTexture_list 
DotsShader::gtextures() const
{
   return GTexture_list(_tone_shader);
}

void
DotsShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
   // set the color from the VIEW background color:
   GL_COL(VIEW::peek()->color(), alpha());    // GL_CURRENT_BIT
}

void 
DotsShader::request_ref_imgs()
{
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
   assert(_patch);
   if (_patch->get_use_visibility_test()) {
      IDRefImage::schedule_update();
   }
}

int 
DotsShader::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ? _tone_shader->draw(VIEW::peek()) : 0;
}

// end of file dots.C
