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
 * multi_lights_tone.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "tone_shader.H"
#include "multi_lights_tone.H"

static bool debug = Config::get_var_bool("DEBUG_MULTI_LIGHTS_TONE", false);
TAGlist* MLToneShader::_tags = 0;

/**********************************************************************
 * MLToneShader:
 *
 *   Does 1D Toon shading in GLSL.
 **********************************************************************/
GLuint MLToneShader::_program = 0;
bool   MLToneShader::_did_init = false;
GLint  MLToneShader::_tex_loc = -1;
GLint  MLToneShader::_tex_2d_loc = -1;
GLint  MLToneShader::_is_tex_2d_loc = -1;

GLint  MLToneShader::_is_enabled_loc = -1;
GLint  MLToneShader::_remap_nl_loc   = -1;
GLint  MLToneShader::_remap_loc      = -1; 
GLint  MLToneShader::_backlight_loc  = -1;
GLint  MLToneShader::_e0_loc         = -1;
GLint  MLToneShader::_e1_loc         = -1;
GLint  MLToneShader::_s0_loc         = -1;
GLint  MLToneShader::_s1_loc         = -1;
GLint  MLToneShader::_blend_normal_loc  = -1;
GLint  MLToneShader::_unit_len_loc  = -1;
GLint  MLToneShader::_edge_len_scale_loc  = -1;
GLint  MLToneShader::_ratio_scale_loc  = -1;
GLint  MLToneShader::_user_depth_loc  = -1;
GLint  MLToneShader::_global_edge_len_loc  = -1;
GLint  MLToneShader::_proj_der_loc  = -1;
GLint  MLToneShader::_show_channel_loc  = -1;

MLToneShader::MLToneShader(Patch* p) :
   ToneShader(p), _show_channel(0)
{
   set_tex(Config::get_var_str(
              "MULTI_LIGHTS_TONE_FILENAME",
              GtexUtil::toon_name("clear-black.png")
              ));

   set_draw_sils(Config::get_var_bool("BASECOAT_IMG_DRAW_SILS",false));
}


void
MLToneShader::init_textures()
{
   // no-op after the first time:
   load_texture(_tex);
   load_texture(_tex_2d);

   if(_tex_2d)
      _tex_2d->apply_texture();

   if(_global_edge_len < 0){
      _global_edge_len = _patch->mesh()->avg_len();
   }
}

void
MLToneShader::activate_textures()
{
   activate_texture(_tex);      // GL_ENABLE_BIT
}

bool 
MLToneShader::get_variable_locs()
{
   ToneStripCB* cb = dynamic_cast<ToneStripCB*>(_cb);
   GLint normLoc = glGetAttribLocation(_program,"blendNorm");
   cb->set_norm_loc(normLoc);
   cb->set_blendType(_smoothNormal);

   GLint lenLoc = glGetAttribLocation(_program,"local_edge_len");
   cb->set_len_loc(lenLoc);

   get_uniform_loc("blend_normal", _blend_normal_loc);
   get_uniform_loc("unit_len", _unit_len_loc);
   get_uniform_loc("edge_len_scale", _edge_len_scale_loc);
   get_uniform_loc("ratio_scale", _ratio_scale_loc);
   get_uniform_loc("user_depth", _user_depth_loc);

   get_uniform_loc("toon_tex", _tex_loc);
   get_uniform_loc("tex_2d", _tex_2d_loc);
   get_uniform_loc("is_tex_2d", _is_tex_2d_loc);

   get_uniform_loc("is_enabled", _is_enabled_loc);
   get_uniform_loc("remap_nl",   _remap_nl_loc  );
   get_uniform_loc("remap",      _remap_loc     );
   get_uniform_loc("backlight",  _backlight_loc );
   get_uniform_loc("e0",         _e0_loc        );
   get_uniform_loc("e1",         _e1_loc        );
   get_uniform_loc("s0",         _s0_loc        );
   get_uniform_loc("s1",         _s1_loc        );
   get_uniform_loc("global_edge_len", _global_edge_len_loc);
   get_uniform_loc("P_matrix", _proj_der_loc);
   get_uniform_loc("show_channel", _show_channel_loc);

   return true;
}

bool
MLToneShader::set_uniform_variables() const
{
   // send uniform variable values to the program

   assert(_tex);
   glUniform1i(_blend_normal_loc, _blend_normal);
   glUniform1f(_user_depth_loc, _user_depth);
   glUniform1f(_unit_len_loc, _unit_len);
   glUniform1f(_edge_len_scale_loc, _edge_len_scale);
   glUniform1f(_ratio_scale_loc, _ratio_scale);

   glUniform1i(_tex_loc, _tex->get_raw_unit());
   if(_tex_2d){
      glUniform1i(_is_tex_2d_loc, 1);
      glUniform1i(_tex_2d_loc, _tex_2d->get_raw_unit());
   }
   else{
      glUniform1i(_is_tex_2d_loc, 0);
      glUniform1i(_tex_2d_loc, 0);
   }

   glUniform1i(_is_enabled_loc, _layer[0]._is_enabled);
   glUniform1i(_remap_nl_loc  , _layer[0]._remap_nl);
   glUniform1i(_remap_loc     , _layer[0]._remap);
   glUniform1i(_backlight_loc , _layer[0]._backlight);
   glUniform1f(_e0_loc        , _layer[0]._e0);
   glUniform1f(_e1_loc        , _layer[0]._e1);
   glUniform1f(_s0_loc        , _layer[0]._s0);
   glUniform1f(_s1_loc        , _layer[0]._s1);
   glUniform1f(_global_edge_len_loc, _global_edge_len);
   glUniform1i(_show_channel_loc, _show_channel);

   Wtransf P_matrix =  VIEW::peek()->eye_to_pix_proj();

   glUniformMatrix4fv(_proj_der_loc,1,GL_TRUE /* transpose = true */,(const GLfloat*) float16(P_matrix));


   return true;
}

void
MLToneShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
   // let's not use the patch color, ok?
   // (at least until after the deadline)
   GL_COL(COLOR::white, alpha());    // GL_CURRENT_BIT

   // set material:
   GL_MAT_COLOR(GL_FRONT_AND_BACK, GL_AMBIENT, _patch->ambient_color(), alpha());
   GL_MAT_COLOR(GL_FRONT_AND_BACK, GL_DIFFUSE, _patch->color(), alpha());
   GL_MAT_COLOR(GL_FRONT_AND_BACK, GL_SPECULAR, _patch->specular_color(), alpha());
   glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, static_cast<GLfloat>(_patch->shininess()));
}

// end of file multi_lights_tone.C
