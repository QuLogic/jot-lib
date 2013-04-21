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
 * dots_tx_pattern.C
 *****************************************************************/
#include "gtex/ref_image.H"
#include "gtex/halo_ref_image.H"
#include "glui/glui.h"
#include "mesh/mi.H"
#include "std/stop_watch.H"

#include "hatching_tx.H"

TAGlist* HatchingTX::_hatching_tags = 0;

GLuint   HatchingTX::_program(0);
bool     HatchingTX::_init(false);
GLint    HatchingTX::_origin_loc(-1);
GLint    HatchingTX::_u_vec_loc(-1);
GLint    HatchingTX::_v_vec_loc(-1);
GLint    HatchingTX::_st_loc(-1);
GLint    HatchingTX::_tone_tex_loc(-1);
GLint    HatchingTX::_dims_loc(-1);
GLint    HatchingTX::_halo_tex_loc(-1);
GLint    HatchingTX::_opacity_remap_loc(-1);

static bool debug = Config::get_var_bool("DEBUG_HATCHING",false);

/**********************************************************************
 * Hatching_TX:
 **********************************************************************/
HatchingTX::HatchingTX(Patch* p) :
   GLSLShader_Layer_Base(p, new StripOpCB())
{
   for (int i=0; i<MAX_LAYERS; i++)
      _layers += new layer_hatching_t();
}

HatchingTX::~HatchingTX()
{
}

void 
HatchingTX::init_textures() 
{
   init_default(0);
}

void
HatchingTX::init_default(int layer)
{
   // sets the hatching and paper textures for the given layer,
   // but only if they are currently not assigned textures.

   assert(_layers.valid_index(layer));
   str_ptr pat = get_name(_layers[layer]->_pattern_name, "hatch1.png");
   assert(get_layer(layer));
   str_ptr pap = get_name(get_layer(layer)->_paper_name, "basic_paper.png");
   init_layer(layer, pat, pap); // no-op if already set
}

void          
HatchingTX::init_layer(int l, Cstr_ptr& pattern, Cstr_ptr& paper)
{
   set_texture_pattern(l, pattern);
   set_paper_texture(l,paper); 
}

void 
HatchingTX::set_paper_texture(int layer_num, str_ptr file_name)
{
   // screen out the crazies
   assert(_layers.valid_index(layer_num));
   layer_hatching_t* layer = get_layer(layer_num);
   assert(layer);

   // if texture is already set for this layer, do nothing
   if (layer->_paper_name == file_name) {
      return;
   }

   // free the existing paper texture if it exists
   if (layer->_mode==1 && layer->_paper_name != "") {
      // the following frees texture resources if no other layer is using
      // the paper texture:
      cleanup_unused_paper_textures(layer->_paper_tex);
      layer->_paper_tex = -1;
      layer->_mode = 0;
      layer->_paper_name = "";
   }

   if (file_name == "")
      return;

   // see if this texture is already loaded by another layer
   for (int i=0; i<_layers.num(); i++) {
      layer_hatching_t* layer_i = get_layer(i);
      assert(layer_i);
      if (layer_i->_mode==1)
         if (layer_i->_paper_name == file_name) {
            // found one already in use
            layer->_paper_tex = layer_i->_paper_tex;
            layer->_mode = 1;
            layer->_paper_name = file_name;
            return;
         }
   }

   // get here if texture is not already loaded
   // by chosen layer or any other

   GLint tex_stage = get_free_tex_stage();

   str_ptr pre_path = Config::JOT_ROOT() + "nprdata/paper_textures/";
   str_ptr path = pre_path + file_name;
   _paper_textures[tex_stage] =
      new TEXTUREgl(path, GL_TEXTURE_2D, GL_TEXTURE0 + tex_stage);
   _paper_textures[tex_stage]->set_save_img(true);

   if (!_paper_textures[tex_stage]->load_image()) {
      cerr << class_name() << "::set_paper_texture: "
           << "invalid paper texture " << path << endl;

      // fail in a civilized manner
      layer->_mode = 0;
      layer->_paper_tex = -1;
      layer->_paper_name = str_ptr("");
      _paper_textures.erase(tex_stage);
      free_tex_stage(tex_stage);

   } else {
      // succeeded in loading the texture
      layer->_mode = 1;
      layer->_paper_name = file_name;
      layer->_paper_tex = tex_stage;
      _paper_textures[tex_stage]->set_wrap_r(GL_REPEAT);
      _paper_textures[tex_stage]->set_wrap_s(GL_REPEAT);
      if (debug) {
         cerr << "HatchingTX::set_paper_texture"
              << ": new paper texture " << file_name
              << " at unit " << tex_stage << endl;
      }
   }  
}

bool
HatchingTX::get_variable_locs()
{
   //get_uniform_loc("base_color",  _base_color_loc);
   get_uniform_loc("origin",      _origin_loc);
   get_uniform_loc("u_vec",       _u_vec_loc);
   get_uniform_loc("v_vec",       _v_vec_loc);
   get_uniform_loc("st",          _st_loc);

   // tone map variables
   get_uniform_loc("tone_tex",    _tone_tex_loc);
   get_uniform_loc("halo_tex",   _halo_tex_loc);
   get_uniform_loc("tone_dims",   _dims_loc);
   get_uniform_loc("op_remap",    _opacity_remap_loc);

   get_layers_variable_names();

   //query the vertex attribute loacations
   StripOpCB* s = dynamic_cast<StripOpCB*>(cb());
   if (s) {
      s->set_opacity_attr_loc(glGetAttribLocation(program(),"opacity_attr"));
      s->set_patch(patch()->cur_patch());
   }

   return true;
}

bool
HatchingTX::set_uniform_variables() const
{
   if (!_patch) return 0;
   assert(_patch);

   glUniform1f(
      _opacity_remap_loc, 1.0f/_patch->mesh()->get_blend_remap_value()
      );

   // tone map variables
   glUniform1i(_tone_tex_loc, ColorRefImage::lookup_raw_tex_unit(0));
   glUniform1i(_halo_tex_loc,  HaloRefImage::lookup_raw_tex_unit());

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
HatchingTX::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

   if (_solid)
      GL_COL(get_base_color(), 1.0);    // GL_CURRENT_BIT

   glEnable(GL_BLEND);                  // GL_ENABLE_BIT
   glBlendFunc(GL_ONE, GL_SRC_ALPHA);   // GL_COLOR_BUFFER_BIT
}

void
HatchingTX::request_ref_imgs()
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
HatchingTX::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ? _patch->get_tex("ToneShader")->draw(VIEW::peek()) : 0;
}

void
HatchingTX::activate_layer_textures()
{
   GLSLShader_Layer_Base::activate_layer_textures();

   for (int i=0; i<_layers.num(); i++) {
      layer_hatching_t* layer = get_layer(i);
      if ((layer->_mode == 1) &&
          (layer->_paper_tex != -1) &&
          (layer->_paper_name != "")) {
         if (_paper_textures[layer->_paper_tex]) {
            if (!activate_texture(_paper_textures[ layer->_paper_tex]))
               cerr << class_name() << ":: unable to activate paper texture "
                    << "at texture stage: " << layer->_paper_tex << endl;
         }
      }
   }
}

void
HatchingTX::activate_textures()
{     
   HaloRefImage::activate_tex_unit();
   activate_layer_textures();
}

void 
HatchingTX::set_texture_pattern(int layer, str_ptr file_name)
{
   GLSLShader_Layer_Base::set_texture_pattern(
      layer,"nprdata/hatching_textures/",file_name
      );
}

void
HatchingTX::cleanup_unused_paper_textures(int tex_stage)
{
   if (tex_stage == -1)
      return;

   // cleanup unused textures
   for (int i=0; i<_layers.num(); i++) {
      if (get_layer(i)->_paper_tex == tex_stage) {
         return; // texture is still in use
      }
   }
   // tex stage is not in use
   _paper_textures.erase(tex_stage);
   free_tex_stage(tex_stage);
}

void
HatchingTX::draw_triangles()
{
   StripOpCB* s = dynamic_cast<StripOpCB*>(cb());
   assert(s);

   // XXX - should also check if patch blending is enabled in the GUI.
   //       for now, just check if there is more than 1 patch:
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

int 
HatchingTX::draw(CVIEWptr& v)
{
   return _solid->draw(v);
}
int 
HatchingTX::draw_final(CVIEWptr& v)
{
   return GLSLShader::draw(v);
}

//------------------I/O-------------------
/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingTX::tags() const
{
   if (!_hatching_tags) {
      _hatching_tags = new TAGlist;

      // start with base class tags
      *_hatching_tags = GLSLShader_Layer_Base::tags();

      *_hatching_tags += new TAG_meth<HatchingTX>(
         "layer",
         &GLSLShader_Layer_Base::put_layer, // base class puts layers
         &HatchingTX::get_layer,
         1);
   }
   return *_hatching_tags;
}

void
HatchingTX::get_layer(TAGformat &d)
{
   int i;
   *d >> i;
   
   str_ptr str;
   *d >> str;

   if ((str != layer_hatching_t::static_name())) {
      cerr << class_name()
           << "::get_layer: error: unexpected layer type: "
           << str
           << endl;
      return;
   }

   layer_hatching_t* layer = get_layer(i);
   assert(layer);
   layer->decode(*d);

   // Initialize the layer's textures: set the layer's texture
   // names to "", then call init_layer with the desired names:
   str_ptr pat = layer->_pattern_name;
   str_ptr pap = layer->_paper_name;
   layer->_pattern_name = "";
   layer->_paper_name = "";
   init_layer(i, pat, pap);
}

/**********************************************************************
 * layer_hatching_t
 **********************************************************************/
TAGlist* layer_hatching_t::_lh_tags = 0;

CTAGlist &
layer_hatching_t::tags() const
{
   if (!_lh_tags) {
      _lh_tags = new TAGlist;
      *_lh_tags += layer_base_t::tags();
     
      *_lh_tags += new TAG_meth<layer_hatching_t>(
         "paper_name",
         &layer_hatching_t::put_paper_name,
         &layer_hatching_t::get_paper_name,
         1);

      *_lh_tags += new TAG_val<layer_hatching_t,float>(
         "angle",
         &layer_hatching_t::angle
         );
      *_lh_tags += new TAG_val<layer_hatching_t,float>(
         "paper_contrast",
         &layer_hatching_t::paper_contrast
         );
      *_lh_tags += new TAG_val<layer_hatching_t,float>(
         "paper_scale",
         &layer_hatching_t::paper_scale
         );    
      *_lh_tags += new TAG_val<layer_hatching_t,float>(
         "tone_push",
         &layer_hatching_t::tone_push
         );    
   }
   return *_lh_tags;
}

void
layer_hatching_t::get_paper_name(TAGformat &d)
{ 
   *d >> _paper_name;

   // XXX - hack to deal with previous hack where empty file
   //       name was written as "0":
   if (_paper_name == "0")
      _paper_name = "";

   if (debug && _paper_name != "") {
      cerr << "layer_hatching_t::get_paper_name: got name: "
           << _paper_name
           << endl;
   }
}

void 
layer_hatching_t::put_paper_name (TAGformat &d) const
{
   if (_paper_name != "") {
      d.id();
      *d << _paper_name;
      d.end_id();
   }
}

layer_hatching_t::layer_hatching_t() :
   layer_base_t(),
   _paper_name(""),   
   _angle(45.0),
   _angle_loc(-1),
   _paper_contrast(1.6), 
   _paper_contrast_loc(-1),  
   _paper_tex(0),
   _paper_tex_loc(-1),
   _paper_scale(25.5),
   _paper_scale_loc(-1),
   _tone_push(0.0),
   _tone_push_loc(-1)     
{ 
}

void 
layer_hatching_t::get_var_locs(int i, GLuint& program) 
{
   layer_base_t::get_var_locs(i,program);

   str_ptr p = str_ptr("layer[") + str_ptr(i);
   get_uniform_loc(p + "].angle",          _angle_loc,          program);
   get_uniform_loc(p + "].paper_contrast", _paper_contrast_loc, program);
   get_uniform_loc(p + "].paper_tex",      _paper_tex_loc,      program);
   get_uniform_loc(p + "].paper_scale",    _paper_scale_loc,    program);
   get_uniform_loc(p + "].tone_push",      _tone_push_loc,      program); 
}

void 
layer_hatching_t::send_to_glsl()  const 
{
   layer_base_t::send_to_glsl();

   glUniform1f  (_angle_loc,_angle);
   glUniform1f  (_paper_contrast_loc, _paper_contrast);
   glUniform1i  (_paper_tex_loc, _paper_tex);
   glUniform1f  (_paper_scale_loc, _paper_scale);
   glUniform1f  (_tone_push_loc, _tone_push);  
   GL_VIEW::print_gl_errors("layer_hatching_t::send_to_glsl() : ");
}

// end of file hatching_tx.C
