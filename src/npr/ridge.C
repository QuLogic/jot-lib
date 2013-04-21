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
 * ridge.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "gtex/ref_image.H"
#include "ridge.H"

static bool debug = Config::get_var_bool("DEBUG_RIDGE", false);

/**********************************************************************
 * RidgeShader:
 *
 *  ...
 **********************************************************************/
GLuint          RidgeShader::_program = 0;
bool            RidgeShader::_did_init = false;

RidgeShader* RidgeShader::_instance(0);

RidgeShader::RidgeShader(Patch* p) : GLSLShader(p), 
	_start_offset(5), _end_offset(35), 
	_curv_threshold(0.25), _dist_threshold(0.6), _vari_threshold(0.05),
	_dark_threshold(0.8), _bright_threshold(0.0)
{
  if(debug){
    cerr<<"Ridge Debug's working"<<endl;
  }


  if(GEOM::do_halo_ref())
    GEOM::set_do_halo_ref(false);
}

RidgeShader::~RidgeShader() 
{
   gtextures().delete_all(); 
}



RidgeShader* 
RidgeShader::get_instance()
{
   if (!_instance) {
      _instance = new RidgeShader();
      assert(_instance);
   }
   return _instance;
}

bool 
RidgeShader::get_variable_locs()
{
   // other variables here as needed...

   get_uniform_loc("tone_map", _tone_tex_loc);
   get_uniform_loc("x_1",  _width_loc);
   get_uniform_loc("y_1", _height_loc);
   get_uniform_loc("curv_thd", _curv_loc);
   get_uniform_loc("dist_thd", _dist_loc);
   get_uniform_loc("vari_thd", _vari_loc);
   get_uniform_loc("start_offset", _start_offset_loc);
   get_uniform_loc("end_offset", _end_offset_loc);
   get_uniform_loc("dark_thd", _dark_loc);
   get_uniform_loc("bright_thd", _bright_loc);

   return true;
}

bool
RidgeShader::set_uniform_variables() const
{
   // send uniform variable values to the program
   
   if(_patch){
      //tone map variables
      glUniform1i(_tone_tex_loc, ColorRefImage::lookup_raw_tex_unit(0));
      glUniform1f(_width_loc,  1.0/VIEW::peek()->width());
      glUniform1f(_height_loc, 1.0/VIEW::peek()->height());
      glUniform1f(_curv_loc, _curv_threshold);
      glUniform1f(_dist_loc, _dist_threshold);
      glUniform1f(_vari_loc, _vari_threshold);
      glUniform1i(_start_offset_loc, _start_offset);
      glUniform1i(_end_offset_loc, _end_offset);
      glUniform1f(_dark_loc, _dark_threshold);
      glUniform1f(_bright_loc, _bright_threshold);

      return true;
   }

   return false;
}



void
RidgeShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
   // set the color from the VIEW background color:
   GL_COL(VIEW::peek()->color(), alpha());    // GL_CURRENT_BIT

}

void 
RidgeShader::request_ref_imgs()
{
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
}

int 
RidgeShader::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ? _patch->get_tex("ToneShader")->draw(VIEW::peek()) : 0;
}
// end of file ridge.C
