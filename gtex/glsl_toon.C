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
 * glsl_toon.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "glsl_toon.H"

static bool debug = Config::get_var_bool("DEBUG_GLSL_TOON", false);

/**********************************************************************
 * GLSLToonShader:
 *
 *   Does 1D Toon shading in GLSL.
 **********************************************************************/
GLuint          GLSLToonShader::_program = 0;
bool            GLSLToonShader::_did_init = false;
GLint           GLSLToonShader::_tex_loc = -1;
GLSLToonShader* GLSLToonShader::_instance(0);

GLSLToonShader::GLSLToonShader(Patch* p) : GLSLShader(p)
{
   set_tex(Config::get_var_str(
              "GLSL_TOON_FILENAME",
              GtexUtil::toon_name("clear-black.png")
              ));
}

GLSLToonShader* 
GLSLToonShader::get_instance()
{
   if (!_instance) {
      _instance = new GLSLToonShader();
      assert(_instance);
   }
   return _instance;
}

void 
GLSLToonShader::set_tex(Cstr_ptr& filename)
{
   // Set the name of the texture to use
   if (_tex) {
      _tex->set_texture(filename);
   } else {
      set_tex(new TEXTUREgl(filename));
      assert(_tex);
   }
}

void 
GLSLToonShader::set_tex(CTEXTUREglptr& tex)
{
   // set the TEXTUREgl to use:
   _tex = tex;

   if (_tex) {
      assert(_tex->target() == GL_TEXTURE_2D);
      _tex->set_tex_unit(GL_TEXTURE0); // XXX - don't need...
      _tex->set_wrap_s(GL_CLAMP_TO_EDGE);
      _tex->set_wrap_t(GL_CLAMP_TO_EDGE);
      _tex->set_tex_fn(GL_REPLACE);
      bool debug=false;
      if (debug) {
         cerr << "GLSLToonShader::set_tex: file: " << _tex->file()
              << ", tex unit " << _tex->get_raw_unit() << endl;
      }
   }
}

void
GLSLToonShader::init_textures()
{
   // no-op after the first time:
   load_texture(_tex);
}

void
GLSLToonShader::activate_textures()
{
   activate_texture(_tex);      // GL_ENABLE_BIT
}

bool 
GLSLToonShader::get_variable_locs()
{
   get_uniform_loc("toon_tex", _tex_loc);

   if (debug) {
      cerr << "GLSLToonShader::get_variable_locs: toon tex loc: "
           << _tex_loc << endl;
   }

   // other variables here as needed...

   return true;
}

bool
GLSLToonShader::set_uniform_variables() const
{
   // send uniform variable values to the program

   if (_tex) {
      GLint unit = _tex->get_raw_unit();
      // XXX - debug:
//      unit = VIEW::stamp() % 4; // spray numbers, hope one sticks
      if (0 && debug) {
         cerr << "GLSLToonShader::set_uniform_variables: toon tex loc: "
              << _tex_loc << ", unit: " << unit << endl;
      }
      glUniform1i(_tex_loc, unit);
      return true;
   }
   return false;
}

void
GLSLToonShader::draw_start(
   TEXTUREglptr toon_tex,       // the 1D toon texture
   CCOLOR& base_color,          // base color for triangles
   double  base_alpha,          // base alpha for triangles
   bool    do_culling           // enable backface culling
   )
{
   // init program
   get_instance()->init();
 
   // set toon texture on static instance:
   get_instance()->set_tex(toon_tex);

   // set gl state
   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
   if (do_culling) glEnable (GL_CULL_FACE);     // GL_ENABLE_BIT
   else            glDisable(GL_CULL_FACE);
   GL_COL(base_color, base_alpha);              // GL_CURRENT_BIT

   // activate texture
   get_instance()->activate_textures();

   // activate program
   get_instance()->activate_program();

   // set uniform variables
   get_instance()->set_uniform_variables();
}

void
GLSLToonShader::draw_end()
{
   // deactivate program
   get_instance()->deactivate_program();

   // resture gl state
   get_instance()->restore_gl_state();
}

// end of file glsl_toon.C
