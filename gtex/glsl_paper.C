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
 * glsl_paper.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "gtex/ref_image.H"
#include "glsl_paper.H"

static bool debug = Config::get_var_bool("DEBUG_GLSL_PAPER", false);

/*****************************************************************
 * Utilities
 *****************************************************************/
inline str_ptr
paper_tex_name()
{
   return (
      Config::JOT_ROOT() +
      "nprdata/paper_textures/" +
      Config::get_var_str("GLSL_PAPER_FILENAME","basic_paper.png")
      );
}

inline str_ptr
stroke_tex_name()
{
   return (
      Config::JOT_ROOT() +
      "nprdata/stroke_textures/"+
      Config::get_var_str("GLSL_PAPER_TEX_FILENAME","one_d.png")
      );
}

/*****************************************************************
 * GLSLPaperShader:
 *****************************************************************/
GLSLPaperShader* GLSLPaperShader::_instance(0);
TEXTUREglptr     GLSLPaperShader::_paper_tex;
str_ptr          GLSLPaperShader::_paper_tex_name;

// XXX - bogus:
str_ptr          GLSLPaperShader::_paper_tex_filename = "basic_paper.png";

TEXTUREglptr     GLSLPaperShader::_tex;
str_ptr          GLSLPaperShader::_tex_name;
double           GLSLPaperShader::_contrast = 2.0;
GLint            GLSLPaperShader::_paper_tex_loc(-1);
GLint            GLSLPaperShader::_tex_loc(-1);
GLint            GLSLPaperShader::_sample_origin(-1);
GLint            GLSLPaperShader::_sample_u_vec(-1);
GLint            GLSLPaperShader::_sample_v_vec(-1);
GLint            GLSLPaperShader::_st_loc(-1);
GLint            GLSLPaperShader::_tex_width_loc(-1);
GLint            GLSLPaperShader::_tex_height_loc(-1);
GLint            GLSLPaperShader::_contrast_loc(-1);

GLuint           GLSLPaperShader::_program(0);
bool             GLSLPaperShader::_did_init(false);
bool             GLSLPaperShader::_do_texture(false);
/**********************************************************************
 * GLSLPaperShader:
 *
 **********************************************************************/
GLSLPaperShader::GLSLPaperShader(Patch* p) : GLSLShader(p)
{
   set_paper(paper_tex_name());
   set_tex  (stroke_tex_name());
}

GLSLPaperShader* 
GLSLPaperShader::instance() 
{
   if (!_instance)
      _instance = new GLSLPaperShader(0);
   return _instance;      
}

void 
GLSLPaperShader::set_paper(Cstr_ptr& filename)
{
   // Set the name of the paper texture to use

   if (_paper_tex) {
      _paper_tex->set_texture(filename);
   } else {
      _paper_tex = new TEXTUREgl(filename);
      _paper_tex->set_tex_unit(GL_TEXTURE0 + TexUnit::PAPER);
      _paper_tex->set_wrap_s(GL_REPEAT);
      _paper_tex->set_wrap_t(GL_REPEAT);
      assert(_paper_tex);
   }
}

void 
GLSLPaperShader::set_tex(Cstr_ptr& filename)
{
   // name of stroke texture (will be bound to tex unit 3):

   if (_tex) {
      _tex->set_texture(filename);
   } else {
      _tex = new TEXTUREgl(filename);
      _tex->set_tex_unit(GL_TEXTURE0 + TexUnit::APP);
      _tex->set_wrap_s(GL_REPEAT);
      _tex->set_wrap_t(GL_REPEAT);
      assert(_tex);
   }
}

void
GLSLPaperShader::init_textures()
{
   // no-op after the first time:
   if (!load_texture(_paper_tex))
      return;

   if (!load_texture(_tex))
      return;
}

void
GLSLPaperShader::activate_textures()
{
   activate_texture(_paper_tex);        // GL_ENABLE_BIT
   activate_texture(_tex);              // GL_ENABLE_BIT

   // XXX - why does paper shader need this?
   ColorRefImage::activate_tex_unit(0); // GL_ENABLE_BIT
}

bool 
GLSLPaperShader::get_variable_locs()
{
   get_uniform_loc("paper_tex", _paper_tex_loc);
 
   if(_do_texture){      
      get_uniform_loc("texture", _tex_loc);
   }
   get_uniform_loc("origin",    _sample_origin);
   get_uniform_loc("u_vec",     _sample_u_vec);
   get_uniform_loc("v_vec",     _sample_v_vec);

   get_uniform_loc("st",        _st_loc);

   get_uniform_loc("tex_width", _tex_width_loc);
   get_uniform_loc("tex_height",_tex_height_loc);
   
   get_uniform_loc("contrast",  _contrast_loc);

   
   return true;
}

bool
GLSLPaperShader::set_uniform_variables() const
{
   //XXX - a hack, but I don't care...:D
   GLSLPaperShader* me = (GLSLPaperShader*)this; 
   glUniform1i(me->get_uniform_loc("paper_tex"), _paper_tex->get_tex_unit() - GL_TEXTURE0);
   
   if(_do_texture){      
      glUniform1i(me->get_uniform_loc("texture"), _tex->get_tex_unit() - GL_TEXTURE0);
   }
  
   //XXX - should be the size of the paper texture!?
   double tex_size = 350;
   glUniform1f(me->get_uniform_loc("tex_width"),  float(tex_size));
   glUniform1f(me->get_uniform_loc("tex_height"), float(tex_size));
   //glUniform2iv(_dims_loc, 1, _paper_tex->image().dimensions().data());

   glUniform1f(me->get_uniform_loc("contrast"),   float(_contrast));

   if (!_patch)
     return true;

   if (_patch->get_use_visibility_test()) {
      _patch->update_dynamic_samples(IDVisibilityTest());
   } else {
      _patch->update_dynamic_samples();
   }

   glUniform2fv(me->get_uniform_loc("origin"), 1, float2(_patch->sample_origin()));

   if (_patch->get_do_lod()) {
      glUniform1f (me->get_uniform_loc("st"), float(_patch->lod_t()));
      glUniform2fv(me->get_uniform_loc("u_vec"),  1, float2(_patch->lod_u()));
      glUniform2fv(me->get_uniform_loc("v_vec"),  1, float2(_patch->lod_v()));
  
   } else {
      glUniform1f (me->get_uniform_loc("st"), float(0));
      glUniform2fv(me->get_uniform_loc("u_vec"),  1, float2(_patch->sample_u_vec()));
      glUniform2fv(me->get_uniform_loc("v_vec"),  1, float2(_patch->sample_v_vec()));
   }
 
   return true;
}

void
GLSLPaperShader::request_ref_imgs()
{
   assert(_patch);
   if (_patch->get_use_visibility_test()) {
      IDRefImage::schedule_update();
   }
}


/**********************************************************************
 * Static Functions
 **********************************************************************/

TEXTUREglptr    
GLSLPaperShader::get_texture()
{
   if (!_paper_tex) {
      // create the TEXTUREgl and check for errors:
      _paper_tex = new TEXTUREgl(_paper_tex_name);
      assert(_paper_tex);
   } else if (_paper_tex->is_valid()) {
      return _paper_tex;
   } else if (_paper_tex->load_attempt_failed()) {
      // we previously tried to load this texture and failed.
      // no sense trying again with the same filename.
      return 0;
   }

   return _paper_tex;

}

void    
GLSLPaperShader::begin_glsl_paper(Patch* p)
{
   GL_VIEW::print_gl_errors("GLSLPaperShader::begin_glsl_paper: start");

   GLSLPaperShader* paper = GLSLPaperShader::instance();
   paper->set_patch(p);

   if (!paper->init())
      return;

   GL_VIEW::print_gl_errors("GLSLPaperShader::begin_glsl_paper: init");
  
   // Load textures (if any) and set their parameters. 
   paper->init_textures();
    GL_VIEW::print_gl_errors("GLSLPaperShader::begin_glsl_paper: init_textures");

   
    // call glPushAttrib() and set desired state 
   paper->set_gl_state();
   // call glPushAttrib() and set desired state 
   //glPushAttrib(GL_ENABLE_BIT);

   // activate textures, if any:
   paper->activate_textures(); // GL_ENABLE_BIT
   GL_VIEW::print_gl_errors("GLSLPaperShader::begin_glsl_paper: activate_textures");

   // activate program:
   paper->activate_program();
   GL_VIEW::print_gl_errors("GLSLPaperShader::begin_glsl_paper: activate_program");

     
   // send values to uniform variables:
   paper->set_uniform_variables();

   GL_VIEW::print_gl_errors("GLSLPaperShader::begin_glsl_paper: end");
}

void
GLSLPaperShader::end_glsl_paper()
{
   GL_VIEW::print_gl_errors("GLSLPaperShader::end_glsl_paper: start");
   GLSLPaperShader* paper = GLSLPaperShader::instance();  
   paper->deactivate_program();

   // restore gl state:
   paper->restore_gl_state();
   // restore GL state
   //glPopAttrib();
  
   paper->set_patch(0);
   GL_VIEW::print_gl_errors("GLSLPaperShader::end_glsl_paper: end");
}

// end of file glsl_paper.C
