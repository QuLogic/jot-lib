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
 * binary_image.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "binary_image.H"

static bool debug = Config::get_var_bool("DEBUG_BINARY_IMAGE", false);

/**********************************************************************
 * BinaryImageShader:
 *
 *   Does 1D Toon shading in GLSL.
 **********************************************************************/
GLuint          BinaryImageShader::_program = 0;
bool            BinaryImageShader::_did_init = false;
GLint           BinaryImageShader::_tex_loc = -1;
GLint           BinaryImageShader::_thresh_loc = -1;
BinaryImageShader* BinaryImageShader::_instance(0);

BinaryImageShader::BinaryImageShader(Patch* p) : GLSLShader(p), 
	_binary_threshold(0.8)
{
   set_tex(GtexUtil::toon_name(
      Config::get_var_str("BINARY_IMAGE_FILENAME","warm_spec2_512_2d.png")
      ));
}

BinaryImageShader* 
BinaryImageShader::get_instance()
{
   if (!_instance) {
      _instance = new BinaryImageShader();
      assert(_instance);
   }
   return _instance;
}

// If this isn't declared static, it never gets called!!!!!!!!!
static void
init_params(TEXTUREglptr& tex)
{
   // set parameters for 1D toon shading
   if (tex) {
      tex->set_wrap_s(GL_CLAMP_TO_EDGE);
      tex->set_wrap_t(GL_CLAMP_TO_EDGE);
      tex->set_mipmap(false);
   }
}

void 
BinaryImageShader::set_tex(Cstr_ptr& filename)
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
BinaryImageShader::set_tex(CTEXTUREglptr& tex)
{
   // set the TEXTUREgl to use:
   init_params(_tex = tex);
}

void
BinaryImageShader::init_textures()
{
   // no-op after the first time:
   if (!load_texture(_tex))
      return;
}

void
BinaryImageShader::activate_textures()
{
   activate_texture(_tex);      // GL_ENABLE_BIT
/*
   if (_tex->wrap_r() == GL_CLAMP_TO_EDGE)
      cerr << "wrap r is correct" << endl;
   else
      cerr << "wrap r is incorrect" << endl;

   if (_tex->wrap_s() == GL_CLAMP_TO_EDGE)
      cerr << "wrap s is correct" << endl;
   else
      cerr << "wrap s is incorrect" << endl;

   if (_tex->wrap_t() == GL_CLAMP_TO_EDGE)
      cerr << "wrap t is correct" << endl;
   else
      cerr << "wrap t is incorrect" << endl;
*/
}

bool 
BinaryImageShader::get_variable_locs()
{
   get_uniform_loc("toon_tex", _tex_loc);
   get_uniform_loc("threshold", _thresh_loc);

   // other variables here as needed...

   return true;
}

bool
BinaryImageShader::set_uniform_variables() const
{
   // send uniform variable values to the program
   glUniform1f(_thresh_loc, _binary_threshold);

   if (_tex) {
      glUniform1i(_tex_loc, _tex->get_tex_unit() - GL_TEXTURE0);
      return true;
   }
   return false;
}

void
BinaryImageShader::draw_start(
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
BinaryImageShader::draw_end()
{
   // deactivate program
   get_instance()->deactivate_program();

   // resture gl state
   get_instance()->restore_gl_state();
}

// end of file binary_image.C
