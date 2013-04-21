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
 * painterly.C
 *****************************************************************/
#include "mesh/mi.H"
#include "std/stop_watch.H"
#include "gtex/ref_image.H"
#include "gtex/halo_ref_image.H"
#include "glui/glui.h"

#include "painterly.H"

TAGlist* Painterly::_painterly_tags = 0;

GLuint   Painterly::_program(0);
bool     Painterly::_init(false);
GLint    Painterly::_origin_loc(-1);
GLint    Painterly::_u_vec_loc(-1);
GLint    Painterly::_v_vec_loc(-1);
GLint    Painterly::_st_loc(-1);
GLint    Painterly::_tone_tex_loc(-1);
GLint    Painterly::_halo_tex_loc(-1);
GLint    Painterly::_dims_loc(-1);
GLint    Painterly::_paper_tex_loc(-1);
GLint    Painterly::_opacity_remap_loc(-1);
GLint    Painterly::_doing_sil_loc(-1);

static bool debug = Config::get_var_bool("DEBUG_PAINTERLY",false);

/**********************************************************************
 * Hatching_TX:
 **********************************************************************/
Painterly::Painterly(Patch* p) :
   GLSLShader_Layer_Base(p, new StripOpCB()),     
   _paper_tex(0),
   _paper_name("")
{
   for (int i=0; i<MAX_LAYERS; i++)
      _layers += new layer_paint_t();

   set_draw_sils(true);
   set_sil_width(3.0f);
}

Painterly::~Painterly()
{
}

void 
Painterly::init_textures() 
{
   init_default(0);
}

void
Painterly::init_default(int layer)
{
   // sets the hatching and paper textures for the given layer,
   // but only if they are currently not assigned textures.

   assert(_layers.valid_index(layer));
   str_ptr pat = get_name(_layers[layer]->_pattern_name, "paint1.png");
   assert(get_layer(layer));
   str_ptr pap = get_name(_paper_name, "basic_paper.png");
   init_layer(layer, pat, pap); // no-op if already set
}

void          
Painterly::init_layer(int l, Cstr_ptr& pattern, Cstr_ptr& paper)
{
   set_paper_texture(paper); 
   set_texture_pattern(l, pattern);  //finger_print-ht.png hatch.png
}

bool
Painterly::get_variable_locs()
{
   get_uniform_loc("paper_tex",   _paper_tex_loc);
   get_uniform_loc("origin",      _origin_loc);
   get_uniform_loc("u_vec",       _u_vec_loc);
   get_uniform_loc("v_vec",       _v_vec_loc);
   get_uniform_loc("st",          _st_loc);
   get_uniform_loc("op_remap",    _opacity_remap_loc);

   // tone map variables
   get_uniform_loc("tone_tex",    _tone_tex_loc);
   get_uniform_loc("halo_tex",    _halo_tex_loc);
   get_uniform_loc("tone_dims",   _dims_loc);
   get_uniform_loc("doing_sil",   _doing_sil_loc);

   get_layers_variable_names();

   // query the vertex attribute loacations
   StripOpCB* ocb = dynamic_cast<StripOpCB*>(cb());
   ocb->set_opacity_attr_loc(glGetAttribLocation(program(),"opacity_attr"));
   ocb->set_patch(_patch);

   return true;
}

bool
Painterly::set_uniform_variables() const
{
   if (!_patch) return 0;
   assert(_patch);
   
   glUniform1i(_paper_tex_loc, _paper_tex);

   glUniform1f(_opacity_remap_loc, (1.0/mesh()->get_blend_remap_value()));
 
   //tone map variables
   glUniform1i(_tone_tex_loc, ColorRefImage::lookup_raw_tex_unit(0));  
   glUniform1i(_halo_tex_loc, HaloRefImage::lookup_raw_tex_unit());

   GLint dims[2];
   int w, h;
   VIEW::peek_size(w,h);
   dims[0] = w;
   dims[1] = h;
   glUniform2iv(_dims_loc, 1, dims);

   if (_patch->get_use_visibility_test()) {
      _patch->update_dynamic_samples(IDVisibilityTest());
   } else {
      _patch->update_dynamic_samples();
   }

   glUniform2fv(_origin_loc, 1, float2(_patch->sample_origin()));

   if (_patch->get_do_lod()) {
      glUniform1f (_st_loc, float(_patch->lod_t()));
      glUniform2fv(_u_vec_loc,  1, float2(_patch->lod_u()));
      glUniform2fv(_v_vec_loc,  1, float2(_patch->lod_v()));
   } else {
      glUniform1f (_st_loc, float(0));
      glUniform2fv(_u_vec_loc,  1, float2(_patch->sample_u_vec()));
      glUniform2fv(_v_vec_loc,  1, float2(_patch->sample_v_vec()));
   }

   send_layers_variables();

   return true;
}

void
Painterly::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
   
   if(_solid)
      GL_COL(get_base_color(), 1.0); 

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_SRC_ALPHA);
}

void
Painterly::request_ref_imgs()
{
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
   HaloRefImage::schedule_update(false,true);
   assert(_patch);
   if (_patch->get_use_visibility_test()) {
      IDRefImage::schedule_update();
   }
}

int
Painterly::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ? _patch->get_tex("ToneShader")->draw(VIEW::peek()) : 0;
}

void
Painterly::activate_layer_textures()
{
   GLSLShader_Layer_Base::activate_layer_textures(); 
  
}

void
Painterly::activate_textures()
{ 
   HaloRefImage::activate_tex_unit();
   if ((_paper_tex != -1) && (_paper_name != "")){
      if(_paper_textures[_paper_tex]){
         if (!activate_texture(_paper_textures[_paper_tex]))
            cerr << "Unable to activate paper texture at texture stage : "
                 << _paper_tex << endl;
      }
   }
   activate_layer_textures();
}

void 
Painterly::set_texture_pattern(int layer, str_ptr file_name)
{
   GLSLShader_Layer_Base::set_texture_pattern(
      layer,"nprdata/painterly_textures/",file_name
      );
}

void
Painterly::cleanup_unused_paper_textures(int tex_stage)
{
   if (tex_stage == -1)
      return;

   // cleanup unused textures
   if ((_paper_tex==tex_stage) && (tex_stage!=-1)) {
      // something else is still using this texture stage
      return;
   }
   _paper_textures.erase(tex_stage);
   free_tex_stage(tex_stage);
}

void
Painterly::set_paper_texture(str_ptr file_name)
{
   // if texture is already set, do nothing:
   if (_paper_name == file_name)
      return;

   if (debug)
      cerr << "Painterly::set_paper_texture: " << file_name << endl;   

   if (_paper_name != "") {
      GLint tex_stage = _paper_tex;
      _paper_tex = -1;
      cleanup_unused_paper_textures(tex_stage);
   } 

   GLint tex_stage = get_free_tex_stage();

   str_ptr pre_path = Config::JOT_ROOT() + "nprdata/paper_textures/";
   str_ptr path = pre_path + file_name;

   _paper_textures[tex_stage] =
      new TEXTUREgl(path, GL_TEXTURE_2D, GL_TEXTURE0 + tex_stage);
   _paper_textures[tex_stage]->set_save_img(true);

   if (!_paper_textures[tex_stage]->load_image()) {
      cerr << "Painterly::set_paper_texture: "
           << "Invalid paper texture " << path << endl;
      _paper_tex = -1;
      _paper_name = str_ptr("");
      _paper_textures.erase(tex_stage);
      free_tex_stage(tex_stage);
   } else {
      _paper_name = file_name;
      _paper_textures[tex_stage]->set_wrap_r(GL_REPEAT);
      _paper_textures[tex_stage]->set_wrap_s(GL_REPEAT);
      _paper_tex = tex_stage;
      if (debug) {
         cerr << "Painterly::set_paper_texture: "
              << "new paper texture " << file_name
              << " at unit " << tex_stage << endl;
      }
   }
}

int 
Painterly::draw(CVIEWptr& v)
{
   return _solid->draw(v);
}
int 
Painterly::draw_final(CVIEWptr& v)
{
   return GLSLShader::draw(v);
}

void
Painterly::draw_triangles()
{
   StripOpCB* s = dynamic_cast<StripOpCB*>(cb());
   assert(s);

   bool do_blend = (mesh()->get_do_patch_blend() && mesh()->npatches() > 1);
   if (do_blend) {      
      // make sure blending weights are up-to-date:
      // (no-op if they are up-to-date):
      mesh()->update_patch_blend_weights(); 

      // the strip cb needs to know which patch to use:
      s->set_patch(patch()->cur_patch());

      // draw the outer n-ring (no triangle strips):
      int n_ring_size = mesh()->patch_blend_smooth_passes() + 1;
      _patch->draw_n_ring_triangles(n_ring_size, s, true);

      // now draw the interior (with triangle strips):
      _patch->draw_tri_strips(_cb);
   } else {
      
      // patch = null means skip blending, send weight = 1 always
      s->set_patch(0);
      _patch->draw_tri_strips(_cb);
   }
}

void 
Painterly::draw_sils()
{
   glUniform1i  (_doing_sil_loc, 1);
   glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   GLSLShader::draw_sils();
   glPopAttrib();
   glUniform1i  (_doing_sil_loc, 0);
}

CTAGlist &
Painterly::tags() const
{
   if (!_painterly_tags) {
      _painterly_tags = new TAGlist;
     
      // start with base class tags
      *_painterly_tags = GLSLShader_Layer_Base::tags();

      *_painterly_tags += new TAG_meth<Painterly>(
         "paper_name",
         &Painterly::put_paper_name,
         &Painterly::get_paper_name,
         1);
      *_painterly_tags += new TAG_meth<Painterly>(
         "layer",
         &GLSLShader_Layer_Base::put_layer, // base class puts layers
         &Painterly::get_layer,
         1);
   }
   return *_painterly_tags;
}

void         
Painterly::get_layer(TAGformat &d)
{
   int i;
   *d >> i;
   
   str_ptr str;
   *d >> str;

   if ((str != layer_paint_t::static_name())) {
      cerr << class_name()
           << "::get_layer: error: unexpected layer type: "
           << str
           << endl;
      return;
   }

   layer_paint_t* layer = get_layer(i);
   assert(layer);
   layer->decode(*d);
   
   // Initialize the layer's texture: set the layer's texture name
   // to "", then call set_texture_pattern with the desired name:
   str_ptr pat = layer->_pattern_name;
   layer->_pattern_name = "";
   set_texture_pattern(i, pat);
}

void         
Painterly::get_paper_name (TAGformat &d)
{
   str_ptr pap = "";
   *d >> pap;  

   // XXX - hack to deal w/ removed hack that would write empty
   //       paper name to file as "0". can remove this hack when
   //       no longer using files written with old hack:
   if (pap == "0")
      pap = "";

   set_paper_texture(pap); 
}

void 
Painterly::put_paper_name (TAGformat &d) const
{
   if (_paper_name != "") {
      d.id();
      *d << _paper_name;
      d.end_id();
   }
}

/**********************************************************************
 * layer_paint_t
 **********************************************************************/
TAGlist* layer_paint_t::_lh_tags = 0;

layer_paint_t::layer_paint_t() :
   layer_base_t(),      
   _angle(45.0),
   _angle_loc(-1),
   _paper_contrast(1.6), 
   _paper_contrast_loc(-1),    
   _paper_scale(25.5),
   _paper_scale_loc(-1),
   _tone_push(0.0),
   _tone_push_loc(-1)     
{
}

void 
layer_paint_t::get_var_locs(int i, GLuint& program) 
{
   if(i == 0){
      layer_base_t::get_var_locs_some(2,i,program);
      str_ptr p = str_ptr("layer[") + str_ptr(i);
      get_uniform_loc(p + "].angle",          _angle_loc,          program);
      get_uniform_loc(p + "].paper_contrast", _paper_contrast_loc, program);   
      get_uniform_loc(p + "].paper_scale",    _paper_scale_loc,    program);
   } else {
      layer_base_t::get_var_locs_some(1,i,program);
      str_ptr p = str_ptr("layer[") + str_ptr(i);
      get_uniform_loc(p + "].angle",          _angle_loc,          program);
      get_uniform_loc(p + "].paper_contrast", _paper_contrast_loc, program);   
      get_uniform_loc(p + "].paper_scale",    _paper_scale_loc,    program);
      get_uniform_loc(p + "].tone_push",      _tone_push_loc,      program); 
   }
}

void 
layer_paint_t::send_to_glsl()  const 
{
   layer_base_t::send_to_glsl();

   if(_angle_loc != -1)
      glUniform1f  (_angle_loc,_angle);

   if(_paper_contrast_loc != -1)
      glUniform1f  (_paper_contrast_loc, _paper_contrast); 
   
   if(_paper_scale_loc != -1)
      glUniform1f  (_paper_scale_loc, _paper_scale);
   
   if(_tone_push_loc != -1)
      glUniform1f  (_tone_push_loc, _tone_push);  
}

CTAGlist &
layer_paint_t::tags() const
{
   if (!_lh_tags) {
      _lh_tags = new TAGlist;
      *_lh_tags += layer_base_t::tags();
     
      *_lh_tags += new TAG_val<layer_paint_t,float>(
         "angle",
         &layer_paint_t::angle
         );
      *_lh_tags += new TAG_val<layer_paint_t,float>(
         "paper_contrast",
         &layer_paint_t::paper_contrast
         );
      *_lh_tags += new TAG_val<layer_paint_t,float>(
         "paper_scale",
         &layer_paint_t::paper_scale
         );    
      *_lh_tags += new TAG_val<layer_paint_t,float>(
         "tone_push",
         &layer_paint_t::tone_push
         );    
   }
   return *_lh_tags;
}

// end of file painterly.C
