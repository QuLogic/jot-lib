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
 * glsl_normal.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "glsl_normal.H"
#include "gtex/glsl_toon.H"
#include "gtex/ref_image.H"

static bool debug = Config::get_var_bool("DEBUG_GLSL_NORMAL", false);


/**********************************************************************
 * GLSLNormalShader:
 *
 *  ...
 **********************************************************************/
GLuint          GLSLNormalShader::_program = 0;
bool            GLSLNormalShader::_did_init = false;

GLSLNormalShader* GLSLNormalShader::_instance(0);

GLSLNormalShader::GLSLNormalShader(Patch* p) : GLSLShader(p)
{
  if(debug){
    cerr<<"GLSL_NORMAL Debug's working"<<endl;
  }
}

GLSLNormalShader* 
GLSLNormalShader::get_instance()
{
   if (!_instance) {
      _instance = new GLSLNormalShader();
      assert(_instance);
   }
   return _instance;
}

bool 
GLSLNormalShader::get_variable_locs()
{
   // other variables here as needed...

   return true;
}

bool
GLSLNormalShader::set_uniform_variables() const
{
   // send uniform variable values to the program
   
   if(_patch){
      return true;
   }

   return false;
}

// end of file glsl_normal.C
