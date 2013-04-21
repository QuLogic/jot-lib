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
* glsl_marble.C
*****************************************************************/
#include "gtex/gl_extensions.H"
#include "glsl_marble.H"

static bool debug = Config::get_var_bool("DEBUG_GLSL_MARBLE", false);

/**********************************************************************
* Utilities
**********************************************************************/
inline str_ptr 
tex_path()
{
   return Config::JOT_ROOT() + "nprdata/toon_textures/"; 
}

inline str_ptr
toon_name(Cstr_ptr& name)
{
   return tex_path() + name;
}

/**********************************************************************
* GLSLMarbleShader:
*
*   Does 1D Toon shading in GLSL.
**********************************************************************/
GLSLMarbleShader::GLSLMarbleShader(Patch* p) :
GLSLShader(p, new VertNormStripCB)
{
   set_tex(toon_name(
      Config::get_var_str("GLSL_TOON_FILENAME","clear-black.png")
      )); 
   
   _perlin=0;
   _perlin_generator= Perlin::get_instance();
   if (!_perlin_generator)
      _perlin_generator= new Perlin();
}

void 
GLSLMarbleShader::set_tex(Cstr_ptr& filename)
{
   // Set the name of the texture to use

   if (_tex) {
      _tex->set_texture(filename);
   } else {
      _tex = new TEXTUREgl(filename);

      // set parameters
      _tex->set_wrap_s(GL_CLAMP_TO_EDGE);
      _tex->set_wrap_t(GL_CLAMP_TO_EDGE);
      assert(_tex);
   }


}

void
GLSLMarbleShader::init_textures()
{
   // no-op after the first time:



    _perlin = _perlin_generator->create_perlin_texture3();

   if (!load_texture(_tex))
      return;
}

void
GLSLMarbleShader::activate_textures()
{
   activate_texture(_tex);      // GL_ENABLE_BIT
   if(_perlin)
      activate_texture(_perlin);

}

bool 
GLSLMarbleShader::get_variable_locs()
{
   get_uniform_loc("toon_tex", _tex_loc);
   get_uniform_loc("sampler3D_perlin", _perlin_loc);
  

   // other variables here as needed...

   return true;
}

bool
GLSLMarbleShader::set_uniform_variables() const
{
   // send uniform variable values to the program

   glUniform1i(_tex_loc, _tex->get_tex_unit() - GL_TEXTURE0);
   if(_perlin)
      glUniform1i(_perlin_loc, _perlin->get_tex_unit() - GL_TEXTURE0);


   return true;
}

// end of file glsl_marble.C
