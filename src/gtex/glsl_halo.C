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
 * glsl_halo.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "glsl_halo.H"

static bool debug = Config::get_var_bool("DEBUG_GLSL_HALO", false);

/**********************************************************************
 * GLSLHaloShader:
 *
 *   Does Halo shading in GLSL.
 **********************************************************************/
GLuint          GLSLHaloShader::_program = 0;
GLint           GLSLHaloShader::_pixel_width_loc = -1;
//GLint           GLSLHaloShader::_distance_loc = -1;
bool            GLSLHaloShader::_did_init = false;
GLSLHaloShader* GLSLHaloShader::_instance(0);


GLSLHaloShader::GLSLHaloShader(Patch* p) :
   GLSLToonShader(p)
{

   set_tex(GtexUtil::toon_name(
      Config::get_var_str("GLSL_HALO_FILENAME","halo_white.png")
      ));
}

GLSLHaloShader* 
GLSLHaloShader::get_instance()
{
   if (!_instance) {
      _instance = new GLSLHaloShader();
      assert(_instance);
   }
   return _instance;
}


bool 
GLSLHaloShader::get_variable_locs()
{
   // gets HaloShader-specific uniform variable locations (from "halo2.vp")


   get_uniform_loc("pixel_width", _pixel_width_loc);
//   get_uniform_loc("camera_distance", _distance_loc);

   // other variables here as needed...

   return GLSLToonShader::get_variable_locs();
}

bool
GLSLHaloShader::set_uniform_variables() const
{
   // send uniform variable values to the program


   // The following 7 lines are used when halo size is computed by distance
   // rather than in terms of pixels.  It is retained but commented out in case
   // somebody decides at any point that this was a good idea.
//   Wpt e = VIEW::peek()->eye();
//   Wpt c = _patch->mesh()->get_bb().center();
//   GLfloat distance;
//   distance = e.dist(c);
//   distance = (distance < 20 ? 20 : distance);
//   distance = (distance > 900 ? 900 : distance);
//   glUniform1f(_distance_loc, distance);

   // declare array to hold info, get info from viewport; what we want is
   // vp[2]: the width of the screen in pixels
   GLint vp[4];
   glGetIntegerv(GL_VIEWPORT, vp);

   glUniform1f(_pixel_width_loc, vp[2]/2);

  
   return GLSLToonShader::set_uniform_variables();
}

void GLSLHaloShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}



// end of file glsl_halo.C
