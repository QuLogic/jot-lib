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
#include "blur_shader.H"
#include "gtex/ref_image.H"
#include "npr/img_line_shader.H"

static bool debug = Config::get_var_bool("DEBUG_BLUR_SHADER", false);

/**********************************************************************
 * BlurShader:
 *
 *  ...
 **********************************************************************/
GLuint          BlurShader::_program = 0;
bool            BlurShader::_did_init = false;

BlurShader* BlurShader::_instance(0);

BlurShader::BlurShader(Patch* p) : 
   GLSLShader(p, new ImageLineStripCB), 
   _blur_size(2),
   _detail_func(0), 
   _unit_len(0.5),
   _edge_len_scale(0.1),
   _user_depth(0.0),
   _ratio_scale(0.5),
   _global_edge_len(-1)
{
  if(debug){
    cerr<<"Blur Debug's working"<<endl;
  }


  if(GEOM::do_halo_ref())
    GEOM::set_do_halo_ref(false);
}

BlurShader::~BlurShader() 
{
   gtextures().delete_all(); 
}



BlurShader* 
BlurShader::get_instance()
{
   if (!_instance) {
      _instance = new BlurShader();
      assert(_instance);
   }
   return _instance;
}

void 
BlurShader::init_textures()
{
   if(_global_edge_len < 0){
      _global_edge_len = _patch->mesh()->avg_len();
   }
}

bool 
BlurShader::get_variable_locs()
{
   // other variables here as needed...
   ImageLineStripCB* cb = dynamic_cast<ImageLineStripCB*>(_cb);
   GLint loc = glGetAttribLocation(_program,"local_edge_len");
   cb->set_loc(loc);

   get_uniform_loc("tone_map", _tone_tex_loc);
   get_uniform_loc("x_1", _width_loc);
   get_uniform_loc("y_1", _height_loc);
   get_uniform_loc("blur_size", _blur_size_loc);

   get_uniform_loc("detail_func", _detail_func_loc);
   get_uniform_loc("unit_len", _unit_len_loc);
   get_uniform_loc("edge_len_scale", _edge_len_scale_loc);
   get_uniform_loc("ratio_scale", _ratio_scale_loc);
   get_uniform_loc("user_depth", _user_depth_loc);
   get_uniform_loc("global_edge_len", _global_edge_len_loc);
   get_uniform_loc("P_matrix", _proj_der_loc);

   return true;
}

bool
BlurShader::set_uniform_variables() const
{
   // send uniform variable values to the program
   
   if(_patch){
      //tone map variables
      glUniform1i(_tone_tex_loc, ColorRefImage::lookup_raw_tex_unit(0));
      glUniform1f(_width_loc,  1.0/VIEW::peek()->width());
      glUniform1f(_height_loc, 1.0/VIEW::peek()->height());
      glUniform1f(_blur_size_loc, _blur_size);

      glUniform1i(_detail_func_loc, _detail_func);
      glUniform1f(_unit_len_loc, _unit_len);
      glUniform1f(_edge_len_scale_loc, _edge_len_scale);
      glUniform1f(_ratio_scale_loc, _ratio_scale);
      glUniform1f(_user_depth_loc, _user_depth);
      glUniform1f(_global_edge_len_loc, _global_edge_len);
      Wtransf P_matrix =  VIEW::peek()->eye_to_pix_proj();

      glUniformMatrix4fv(_proj_der_loc,1,GL_TRUE /* transpose = true */,(const GLfloat*) float16(P_matrix));

      return true;
   }

   return false;
}



void
BlurShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
   // set the color from the VIEW background color:
   GL_COL(VIEW::peek()->color(), alpha());    // GL_CURRENT_BIT

}

void 
BlurShader::request_ref_imgs()
{
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
}

int 
BlurShader::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ? _patch->get_tex("ToneShader")->draw(VIEW::peek()) : 0;
}
// end of file blur_shader.C
