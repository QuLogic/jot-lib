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
/**************************************************************************
 * haltone_shader.C
 *************************************************************************/

#include "gtex/gl_extensions.H"
#include "gtex/util.H"
#include "gtex/glsl_toon.H"             // Temp for testing ref images.
#include "gtex/ref_image.H"
//#include "geom/gl_util.H"
#include "mesh/patch.H"
#include "halftone_shader.H"

/**********************************************************************
 * HalftoneShader:
 *
 *   Doesn't do anything yet.
 **********************************************************************/
HalftoneShader::HalftoneShader(Patch* patch) :
   GLSLShader(patch),
   m_toon_tone_shader(new GLSLToonShader(patch)),
   _style_loc(-1),
   _style(0)
{
   // Specify the texture we'd like to use.
   str_ptr tex_name = "nprdata/toon_textures/clear-black.png";
   m_toon_tone_shader->set_tex(Config::JOT_ROOT() + tex_name);

   _perlin=0;
   _perlin_generator= Perlin::get_instance();
   if (!_perlin_generator)
      _perlin_generator= new Perlin();

}

bool 
HalftoneShader::get_variable_locs()
{
   get_uniform_loc("origin", m_origin_loc);
   

   get_uniform_loc("u_vec",  m_u_vec_loc);
   get_uniform_loc("v_vec",  m_v_vec_loc);
   get_uniform_loc("lod", _lod_loc);
   
   get_uniform_loc("tone_map", m_tex_loc);
   get_uniform_loc("width", m_width);
   get_uniform_loc("height", m_height);
   get_uniform_loc("sampler2D_perlin", _perlin_loc);
   get_uniform_loc("style", _style_loc);

   return true;
}

bool
HalftoneShader::set_uniform_variables() const
{
   assert(_patch);
   _patch->update_dynamic_samples();

   glUniform2fv(m_origin_loc, 1, float2(_patch->sample_origin()));
  
  if (_patch->get_do_lod()) {
      glUniform1f (_lod_loc, float(_patch->lod_t()));
      glUniform2fv(m_u_vec_loc,  1, float2(_patch->lod_u()));
      glUniform2fv(m_v_vec_loc,  1, float2(_patch->lod_v()));
   } else {
      glUniform1f (_lod_loc, float(0));
      glUniform2fv(m_u_vec_loc,  1, float2(_patch->sample_u_vec()));
      glUniform2fv(m_v_vec_loc,  1, float2(_patch->sample_v_vec()));
   }


   glUniform1i (m_tex_loc, m_texture->get_tex_unit() - GL_TEXTURE0);
   glUniform1i (m_width, VIEW::peek()->width());
   glUniform1i (m_height, VIEW::peek()->height());
     if(_perlin)
      glUniform1i(_perlin_loc, _perlin->get_tex_unit() - GL_TEXTURE0);
   
   glUniform1i(_style_loc, _style);

   return true;
}

void 
HalftoneShader::init_textures()
{
   // This may seem a little odd, but since the texture is in texture
   // memory, we need to look it up from the reference image.
   // XXX - still seems bogus! :)
   m_texture = ColorRefImage::lookup_texture(0);
   _perlin = _perlin_generator->create_perlin_texture2();
}

void
HalftoneShader::activate_textures()
{
   activate_texture(m_texture);      // GL_ENABLE_BIT
   if(_perlin)
      activate_texture(_perlin);

}

GTexture_list HalftoneShader::gtextures() const
{
   return GTexture_list(m_toon_tone_shader);
}

void
HalftoneShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
   // set the color from the VIEW background color:
   GL_COL(VIEW::peek()->color(), alpha());    // GL_CURRENT_BIT
}

// ******** GTexture VIRTUAL METHODS ********

void 
HalftoneShader::request_ref_imgs()
{
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
}

int 
HalftoneShader::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ? m_toon_tone_shader->draw(VIEW::peek()) : 0;
}

// End of file halftone_shader.C
