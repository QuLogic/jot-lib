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
#include "img_line_shader.H"
#include "gtex/ref_image.H"
#include "gtex/patch_id_texture.H"
#include "gtex/color_id_texture.H"

static bool debug = Config::get_var_bool("DEBUG_IMAGE_LINE_SHADER", false);
void
ImageLineStripCB::faceCB(CBvert* v, CBface* f)
{
   assert(v && f);

   double avg_len = v->avg_edge_len();

   glVertexAttrib1f(_loc, avg_len);
   glNormal3dv(f->vert_normal(v).data());
   glVertex3dv(v->loc().data());
}

/**********************************************************************
 * ImageLineShader:
 *
 *  ...
 **********************************************************************/
GLuint          ImageLineShader::_program = 0;
bool            ImageLineShader::_did_init = false;

ImageLineShader* ImageLineShader::_instance(0);

ImageLineShader::ImageLineShader(Patch* p) : 
   GLSLShader(p, new ImageLineStripCB), 
   _line_width(2.0),
   _detail_func(0), 
   _unit_len(0.5),
   _edge_len_scale(0.1),
   _user_depth(0.0),
   _ratio_scale(0.5),
   _debug_shader(0),
   _line_mode(1),
   _draw_mode(0),
   _confidence_mode(1),
   _alpha_offset(0.0),
   _silhouette_mode(1),
   _draw_silhouette(0),
   _global_edge_len(-1),
   _highlight_control(1.0),
   _light_control(1.0),
   _tapering_mode(1),
   _moving_factor(0.75),
   _ht_width_control(1.0),
   _tone_effect(1),
   _dark_color(Color::black),
   _highlight_color(Color::white),
   _light_color(Color::white)
{
   if(debug){
      cerr<<"ImageLineShader Debug's working"<<endl;
   }

   _tone = new MLToneShader(p);
   _tone->_layer[0]._remap = 0;
   _basecoat = new BasecoatShader(p);

   _curv_threshold[0] = 0.01;
   _curv_threshold[1] = 0.007;


   if(GEOM::do_halo_ref())
      GEOM::set_do_halo_ref(false);

}

ImageLineShader::~ImageLineShader() 
{
   gtextures().delete_all(); 
}



ImageLineShader* 
ImageLineShader::get_instance()
{
   if (!_instance) {
      _instance = new ImageLineShader();
      assert(_instance);
   }
   return _instance;
}


bool 
ImageLineShader::get_variable_locs()
{
   // other variables here as needed...
   //
   //
   ImageLineStripCB* cb = dynamic_cast<ImageLineStripCB*>(_cb);
   GLint loc = glGetAttribLocation(_program,"local_edge_len");
   cb->set_loc(loc);

   get_uniform_loc("tone_map", _tone_tex_loc);
   get_uniform_loc("id_map", _id_tex_loc);
   get_uniform_loc("x_1",  _width_loc);
   get_uniform_loc("y_1", _height_loc);
   get_uniform_loc("curv_thd0", _curv_loc[0]);
   get_uniform_loc("curv_thd1", _curv_loc[1]);
   get_uniform_loc("line_width", _line_width_loc);

   get_uniform_loc("dk_color",  _dark_color_loc);
   get_uniform_loc("ht_color",  _highlight_color_loc);
   get_uniform_loc("lt_color",  _light_color_loc);

   get_uniform_loc("detail_func", _detail_func_loc);
   get_uniform_loc("unit_len", _unit_len_loc);
   get_uniform_loc("edge_len_scale", _edge_len_scale_loc);
   get_uniform_loc("ratio_scale", _ratio_scale_loc);
   get_uniform_loc("user_depth", _user_depth_loc);
   get_uniform_loc("debug", _debug_shader_loc);
   get_uniform_loc("line_mode", _line_mode_loc);
   get_uniform_loc("confidence_mode", _confidence_mode_loc);
   get_uniform_loc("silhouette_mode", _silhouette_mode_loc);
   get_uniform_loc("draw_silhouette", _draw_silhouette_loc);
   get_uniform_loc("alpha_offset", _alpha_offset_loc);
   get_uniform_loc("global_edge_len", _global_edge_len_loc);
   get_uniform_loc("P_matrix", _proj_der_loc);
   get_uniform_loc("highlight_control", _highlight_control_loc);
   get_uniform_loc("light_control", _light_control_loc);
   get_uniform_loc("tapering_mode", _tapering_mode_loc);
   get_uniform_loc("moving_factor", _moving_factor_loc);
   get_uniform_loc("tone_effect", _tone_effect_loc);
   get_uniform_loc("ht_width_control", _ht_width_control_loc);

   return true;
}

void 
ImageLineShader::init_textures()
{
   if(_global_edge_len < 0){
      _global_edge_len = _patch->mesh()->avg_len();
   }

}

bool
ImageLineShader::set_uniform_variables() const
{
   // send uniform variable values to the program
   
   if(_patch){
      //tone map variables
      glUniform1i(_tone_tex_loc, ColorRefImage::lookup_raw_tex_unit(1));
      IDRefImage::activate_tex_unit(VIEW::peek());
      glUniform1i(_id_tex_loc, IDRefImage::lookup_raw_tex_unit());
      glUniform1f(_width_loc,  1.0/VIEW::peek()->width());
      glUniform1f(_height_loc, 1.0/VIEW::peek()->height());
      glUniform1f(_curv_loc[0], _curv_threshold[0]);
      glUniform1f(_curv_loc[1], _curv_threshold[1]);
      glUniform1f(_line_width_loc, _line_width);
      glUniform1i(_detail_func_loc, _detail_func);
      glUniform1f(_unit_len_loc, _unit_len);
      glUniform1f(_edge_len_scale_loc, _edge_len_scale);
      glUniform1f(_ratio_scale_loc, _ratio_scale);
      glUniform1f(_user_depth_loc, _user_depth);

      glUniform3fv (_dark_color_loc, 1, float3(_dark_color));
      glUniform3fv (_highlight_color_loc, 1, float3(_highlight_color));
      glUniform3fv (_light_color_loc, 1, float3(_light_color));

      glUniform1i(_debug_shader_loc, _debug_shader);
      glUniform1i(_line_mode_loc, _line_mode);
      glUniform1i(_confidence_mode_loc, _confidence_mode);
      glUniform1i(_silhouette_mode_loc, _silhouette_mode);
      glUniform1i(_draw_silhouette_loc, _draw_silhouette);
      glUniform1f(_alpha_offset_loc, _alpha_offset);
      glUniform1f(_highlight_control_loc, _highlight_control);
      glUniform1f(_light_control_loc, _light_control);
      glUniform1f(_global_edge_len_loc, _global_edge_len);
      glUniform1f(_moving_factor_loc, _moving_factor);
      glUniform1i(_tapering_mode_loc, _tapering_mode);
      glUniform1i(_tone_effect_loc, _tone_effect);
      glUniform1f(_ht_width_control_loc, _ht_width_control);

      Wtransf P_matrix =  VIEW::peek()->eye_to_pix_proj();

      glUniformMatrix4fv(_proj_der_loc,1,GL_TRUE /* transpose = true */,(const GLfloat*) float16(P_matrix));

      return true;
   }

   return false;
}



void
ImageLineShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
   // set the color from the VIEW background color:
   GL_COL(VIEW::peek()->color(), alpha());    // GL_CURRENT_BIT
   glEnable(GL_BLEND);                                // GL_ENABLE_BIT
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_COLOR_BUFFER_BIT

}

void 
ImageLineShader::request_ref_imgs()
{

   IDRefImage::schedule_update(VIEW::peek(), false, false, true);
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
   ColorRefImage::schedule_update(1, false, true);
}

int
ImageLineShader::draw(CVIEWptr& v)
{
   if(_draw_mode == 0)
      return _basecoat->draw(v);
   else if(_draw_mode == 1){
      return _tone->draw(v);
   }
   else{
      return _patch->get_tex("BlurShader")->draw(v);
   }
}

int
ImageLineShader::draw_final(CVIEWptr& v)
{
   if(_line_mode && _draw_mode == 0)
      return GLSLShader::draw(v);
   else
      return 0;
}

int 
ImageLineShader::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ? _tone->draw(VIEW::peek()) : _patch->get_tex("BlurShader")->draw(VIEW::peek());
    
}

int 
ImageLineShader::draw_id_ref() 
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

int 
ImageLineShader::draw_vis_ref() 
{
   ColorIDTexture* tex = get_tex<ColorIDTexture>(_patch);
   if (tex) {
      // XXX - should draw black if the ID image isn't used 
      //       by this GTexture. we used to be able to find
      //       that out via use_ref_image(), but that's been
      //       changed. for now, drawing IDs (not black)...
      return tex->draw(VIEW::peek());
   }

   return 0;
}

double
ImageLineShader::get_ndcz_bounding_box_size()
{
   if(!_patch)
      return 0.0;

   mlib::NDCZpt min_pt, max_pt;
   _patch->mesh()->get_bb().ndcz_bounding_box(_patch->obj_to_ndc(), min_pt, max_pt);
//    cerr << "bb is " <<  min_pt <<  " " << max_pt << endl;

   return (max_pt-min_pt).planar_length();
}

// end of file img_line_shader.C
