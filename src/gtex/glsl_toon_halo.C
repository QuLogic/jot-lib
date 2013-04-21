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
 * glsl_toon_halo.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "glsl_toon_halo.H"

static bool debug = Config::get_var_bool("DEBUG_GLSL_TOON", false);

/**********************************************************************
 * GLSLToonShaderHalo:
 *
 *   Does 1D Toon shading in GLSL.
 **********************************************************************/
GLSLToonShaderHalo::GLSLToonShaderHalo(Patch* p) :
   GLSLShader(p, new VertNormStripCB)
{
   set_tex(GtexUtil::toon_name(
      Config::get_var_str("HALO_TEX","halo_white2.png")
      ));
}

void 
GLSLToonShaderHalo::set_tex(Cstr_ptr& filename)
{
   // Set the name of the texture to use
   if (_tex) {
      _tex->set_texture(filename);
   } else {
      _tex = new TEXTUREgl(filename);
      assert(_tex);
   }
}

void
GLSLToonShaderHalo::init_textures()
{
   // no-op after the first time:
   if (!load_texture(_tex))
      return;

   // set parameters
   // XXX - we're calling this every frame, but it's light-weight
   _tex->set_wrap_s(GL_CLAMP_TO_EDGE);
   _tex->set_wrap_t(GL_CLAMP_TO_EDGE);  
}

void
GLSLToonShaderHalo::activate_textures()
{
   activate_texture(_tex);      // GL_ENABLE_BIT
}

bool 
GLSLToonShaderHalo::get_variable_locs()
{
   if (!get_uniform_loc("toon_tex", _tex_loc))
      return false;

   // other variables here as needed...
   return true;
}

bool
GLSLToonShaderHalo::set_uniform_variables() const
{
   // send values to the program

   glUniform1i(_tex_loc, _tex->get_tex_unit() - GL_TEXTURE0);
   return true;
}

// end of file glsl_toon_halo.C
