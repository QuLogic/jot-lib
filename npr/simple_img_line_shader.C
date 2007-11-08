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
 * img_line_shader.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "simple_img_line_shader.H"
#include "gtex/ref_image.H"
#include "gtex/patch_id_texture.H"
#include "gtex/color_id_texture.H"

static bool debug = Config::get_var_bool("DEBUG_IMAGE_LINE_SHADER", false);

/**********************************************************************
 * SimpleImageLineShader:
 *
 *  ...
 **********************************************************************/

GLuint          SimpleImageLineShader::_program = 0;
bool            SimpleImageLineShader::_did_init = false;

SimpleImageLineShader* SimpleImageLineShader::_instance(0);

SimpleImageLineShader::SimpleImageLineShader(Patch* p) : 
   GLSLShader(p),
   _line_width(2.0),
   _curv_opacity_ctrl(1),
   _dist_opacity_ctrl(1),
   _moving_factor(0.3),
   _tone_opacity_ctrl(1),
   _tone(0)
{
   if(debug){
      cerr<<"SimpleImageLineShader Debug's working"<<endl;
   }

   _tone = new MLToneShader(p);
   _tone->_layer[0]._remap = 0;

   _curv_threshold[0] = 0.01;
   _curv_threshold[1] = 0.007;


   if(GEOM::do_halo_ref())
      GEOM::set_do_halo_ref(false);

}

SimpleImageLineShader::~SimpleImageLineShader() 
{
   gtextures().delete_all(); 
}


SimpleImageLineShader* 
SimpleImageLineShader::get_instance()
{
   if (!_instance) {
      _instance = new SimpleImageLineShader();
      assert(_instance);
   }
   return _instance;
}


bool 
SimpleImageLineShader::get_variable_locs()
{
   // other variables here as needed...
   //
   //
   get_uniform_loc("tone_map", _tone_tex_loc);
   get_uniform_loc("x_1",  _width_loc);
   get_uniform_loc("y_1", _height_loc);
   get_uniform_loc("upper_curv_thd", _curv_loc[0]);
   get_uniform_loc("lower_curv_thd", _curv_loc[1]);
   get_uniform_loc("half_width", _line_width_loc);

   get_uniform_loc("curv_opacity_ctrl", _curv_opacity_ctrl_loc);
   get_uniform_loc("dist_opacity_ctrl", _dist_opacity_ctrl_loc);
   get_uniform_loc("moving_factor", _moving_factor_loc);
   get_uniform_loc("tone_opacity_ctrl", _tone_opacity_ctrl_loc);


   return true;
}

void 
SimpleImageLineShader::init_textures()
{

}

bool
SimpleImageLineShader::set_uniform_variables() const
{
   // send uniform variable values to the program
   
   if(_patch){
      //tone map variables
      glUniform1i(_tone_tex_loc, ColorRefImage::lookup_raw_tex_unit(1));
      glUniform1f(_width_loc,  1.0/VIEW::peek()->width());
      glUniform1f(_height_loc, 1.0/VIEW::peek()->height());
      glUniform1f(_curv_loc[0], _curv_threshold[0]);
      glUniform1f(_curv_loc[1], _curv_threshold[1]);
      glUniform1f(_line_width_loc, _line_width);

      glUniform1i(_curv_opacity_ctrl_loc, _curv_opacity_ctrl);
      glUniform1f(_moving_factor_loc, _moving_factor);
      glUniform1i(_dist_opacity_ctrl_loc, _dist_opacity_ctrl);
      glUniform1i(_tone_opacity_ctrl_loc, _tone_opacity_ctrl);

      return true;
   }

   return false;
}



void
SimpleImageLineShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
   // set the color from the VIEW background color:
   GL_COL(VIEW::peek()->color(), alpha());    // GL_CURRENT_BIT
   glEnable(GL_BLEND);                                // GL_ENABLE_BIT
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_COLOR_BUFFER_BIT

}

void 
SimpleImageLineShader::request_ref_imgs()
{

   IDRefImage::schedule_update(VIEW::peek(), false, false, true);
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
   ColorRefImage::schedule_update(1, false, true);
}

int
SimpleImageLineShader::draw(CVIEWptr& v)
{
  return 0;
}

int
SimpleImageLineShader::draw_final(CVIEWptr& v)
{
   return GLSLShader::draw(v);
}

int 
SimpleImageLineShader::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ? _tone->draw(VIEW::peek()) : _patch->get_tex("BlurShader")->draw(VIEW::peek());
    
}

int 
SimpleImageLineShader::draw_id_ref() 
{
   PatchIDTexture* tex = get_tex<PatchIDTexture>(_patch);
   if (tex) {
      // XXX - should draw black if the ID image isn't used 
      //       by this GTexture. we used to be able to find
      //       that out via use_ref_image(), but that's been
      //       changed. for now, drawing IDs (not black)...
      return tex->draw(VIEW::peek());
   }

   return 0;
}

// end of file img_line_shader.C
