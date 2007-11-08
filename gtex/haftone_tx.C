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
#include "mesh/mi.H"
#include "haftone_tx.H"
#include "std/stop_watch.H"

#include "gtex/ref_image.H"
#include "gtex/halo_ref_image.H"

const int CORR_RES   = 256; // resolution of the tone correction
const int LOD_RES    =  16; // resolution of the lod correction
const int PROCEDURAL = -1;

static bool debug = Config::get_var_bool("DEBUG_HALFTONE", false);
static bool enable_alpha_transitions =
   Config::get_var_bool("ENABLE_HALFTONE_ALPHA_TRANSITIONS",false);

TAGlist* halftone_layer_t::_lh_tags = 0;

const float PROCEDURAL_DOTS_SIZE = 4.0f;

/*****************************************************************
 * halftone_layer_t:
 *****************************************************************/
void
halftone_layer_t::get_var_locs(int i, GLuint& program) 
{
   layer_base_t::get_var_locs(i,program);

   str_ptr p = str_ptr("layer[") + str_ptr(i);

   get_uniform_loc(p + "].use_tone_correction",
                   _use_tone_correction_loc, program);
   get_uniform_loc(p + "].use_lod_only",
                   _use_lod_only_loc, program);
   get_uniform_loc(p + "].tone_correction_tex",
                   _tone_correction_tex_loc, program);
   get_uniform_loc(p + "].r_function_tex",
                   _r_function_tex_loc, program);
}

inline void
send_var(GLint loc, GLint val)
{
   if (loc >= 0)
      glUniform1i(loc,val);
}

void 
halftone_layer_t::send_to_glsl()  const 
{
   layer_base_t::send_to_glsl();

   send_var(_use_tone_correction_loc,_use_tone_correction);
   send_var(_use_lod_only_loc,_use_lod_only);

   if (_use_tone_correction) {
      send_var (_tone_correction_tex_loc,  _tone_correction_tex_stage );
   }
   if (_use_lod_only) {
      send_var (_r_function_tex_loc,_r_function_tex_stage);
   }

   GL_VIEW::print_gl_errors("halftone_layer_t  send : ");
}

CTAGlist&  
halftone_layer_t::tags() const
{
   if (!_lh_tags) {
      _lh_tags = new TAGlist;
      *_lh_tags += layer_base_t::tags();
      *_lh_tags += new TAG_val<halftone_layer_t,bool>(
         "_use_tone_correction",
         &halftone_layer_t::use_tone_correction
         );
      *_lh_tags += new TAG_val<halftone_layer_t,bool>(
         "_use_lod_only",
         &halftone_layer_t::use_lod_only
         );
   }
   return *_lh_tags;
}

/*****************************************************************
 * Halftone_TX:
 *
 *   Does dot-based halftoning
 *****************************************************************/
GLuint  Halftone_TX::_program(0);
bool    Halftone_TX::_did_init(false);
GLint   Halftone_TX::_origin_loc(-1);
GLint   Halftone_TX::_u_vec_loc(-1);
GLint   Halftone_TX::_v_vec_loc(-1);
GLint   Halftone_TX::_st_loc(-1);
GLint   Halftone_TX::_use_alpha_loc(-1);
GLint   Halftone_TX::_tone_tex_loc(-1);
GLint   Halftone_TX::_halo_tex_loc(-1);
GLint   Halftone_TX::_dims_loc(-1);
GLint   Halftone_TX::_opacity_remap_loc(-1);
GLint   Halftone_TX::_timed_lod_hi(-1);
GLint   Halftone_TX::_timed_lod_lo(-1);

TAGlist* Halftone_TX::_halftone_tags=0;

Halftone_TX::Halftone_TX(Patch* p) :
   GLSLShader_Layer_Base(p,new StripOpCB()),
   _use_alpha_transitions(false)
{
   for (int i=0; i<MAX_LAYERS; i++)
      _layers += new halftone_layer_t();

   // setup a default style here
   _layers[0]->_mode = 2; //procedural dots;
   set_style(0);
}

Halftone_TX::~Halftone_TX()
{
   // layers are deleted in the base class
   gtextures().delete_all();
}

halftone_layer_t*
Halftone_TX::get_layer(int l) const
{
   assert(_layers.valid_index(l));
   return dynamic_cast<halftone_layer_t*>(_layers[l]);
}

bool
Halftone_TX::get_variable_locs()
{
   get_uniform_loc("origin", _origin_loc);
   get_uniform_loc("u_vec",  _u_vec_loc);
   get_uniform_loc("v_vec",  _v_vec_loc);
   get_uniform_loc("st",     _st_loc);
   get_uniform_loc("op_remap",    _opacity_remap_loc);
   get_uniform_loc("timed_lod_hi", _timed_lod_hi);
   get_uniform_loc("timed_lod_lo", _timed_lod_lo);

   // tone map variables
   get_uniform_loc("tone_tex",   _tone_tex_loc);
   get_uniform_loc("halo_tex",   _halo_tex_loc);
   get_uniform_loc("dims",       _dims_loc);

   // XXX - hack for demoing alpha transitions.
   //       to save instructions in the shader this is normally
   //       not enabled
   if (enable_alpha_transitions)
      get_uniform_loc("use_alpha_transitions",_use_alpha_loc);

   get_layers_variable_names();

   //query the vertex attribute loacations
   //part of the patch blending
   StripOpCB* s = dynamic_cast<StripOpCB*>(cb());
   if (s) {
      s->set_opacity_attr_loc(glGetAttribLocation(program(),"opacity_attr"));
      s->set_patch(_patch);
   }

   return true;
}

float
Halftone_TX::get_abs_scale(int l)
{
   halftone_layer_t* layer = get_layer(l);
   assert(layer);
   if (layer->_mode==1) {
      assert(layer->_pattern_tex_stage != -1);
      return _patterns[layer->_pattern_tex_stage]->image().width();
   }
   return PROCEDURAL_DOTS_SIZE;
}

bool
Halftone_TX::set_uniform_variables() const
{
   if (!_patch) return 0;
   assert(_patch);

   glUniform1f(_opacity_remap_loc, (1.0/mesh()->get_blend_remap_value()));

   //tone map variables
   send_var(_tone_tex_loc, ColorRefImage::lookup_raw_tex_unit(0));
   send_var(_halo_tex_loc, HaloRefImage::lookup_raw_tex_unit());
   
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

   // XXX - hack for demoing alpha transitions.
   //       to save instructions in the shader this is normally
   //       not enabled
   if (enable_alpha_transitions)
      send_var(_use_alpha_loc, _use_alpha_transitions);

   if (_patch->get_do_lod()) {
      glUniform1f (_st_loc, float(_patch->lod_t()));
      glUniform1f (_timed_lod_hi, float(_patch->timed_lod_hi()));
      glUniform1f (_timed_lod_lo, float(_patch->timed_lod_lo()));
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
Halftone_TX::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
   
   if (_solid)
      GL_COL(get_base_color(), 1.0); 
  
   glEnable(GL_BLEND); 
   glBlendFunc(GL_ONE, GL_SRC_ALPHA);
   glDepthFunc(GL_LEQUAL);
}

void
Halftone_TX::draw_triangles()
{
   StripOpCB* s = dynamic_cast<StripOpCB*>(cb());
   assert(s);

   if (debug) {
      cerr << "Halftone_TX::draw_triangles" << endl;
   }

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

   } else {
      // patch = null means skip blending, send weight = 1 always
      s->set_patch(0);
   }

   // now draw the interior (with triangle strips):
   _patch->draw_tri_strips(_cb);
}

int 
Halftone_TX::draw(CVIEWptr& v)
{
   return _solid->draw(v);
//    return GLSLShader::draw(v);
}

int 
Halftone_TX::draw_final(CVIEWptr& v)
{
   return GLSLShader::draw(v);
//    return 0;
}

void
Halftone_TX::request_ref_imgs()
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
Halftone_TX::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ?
      _patch->get_tex(ToneShader::static_name())->draw(VIEW::peek()) :
      0;
}

void
Halftone_TX::activate_layer_textures()
{
   GLSLShader_Layer_Base::activate_layer_textures();

   for (int i=0; i<_layers.num(); i++) {
      //main tone correction map
      halftone_layer_t* layer = get_layer(i);
      if (layer->_use_tone_correction) {
         
         if (!activate_texture(
                tone_correction_maps[layer->_tone_correction_tex_stage])
            )
            cerr << class_name()
                 << ": can't activate tone correction texture at stage: "
                 << layer->_tone_correction_tex_stage << endl;
      }

      // forward R function
      if (layer->_use_lod_only) {
         if (!activate_texture(r_function_maps[layer->_r_function_tex_stage]))
            cerr << class_name()
                 << ": can't activate R function texture at stage: "
                 << layer->_r_function_tex_loc << endl;
      }
   }
}

void
Halftone_TX::activate_textures()
{
   HaloRefImage::activate_tex_unit();
   activate_layer_textures();
}

void
Halftone_TX::set_style(int s)
{
   switch (s) {
    case 0:
      set_procedural_pattern(0,0);
      _layers[0]->_ink_color = COLOR(0.0,0.0,0.0);
      break;
    case 1:
      set_texture_pattern(0,"pattern5.png");
      _layers[0]->_ink_color = COLOR(0.5,1.0,0.5);
      break;
    default:
      set_procedural_pattern(0,0);
      _layers[0]->_ink_color = COLOR(1.0,0.1,0.1);
      break;
   }

   /*
     switch (s) {
     case 0:
     _current_pattern = "";

     //set_color(93, 50, 23);

     _use_correction = false;
     _lod_only_corr = false;
     _tex_scale = 18.0f/PROCEDURAL_DOTS_SIZE;

     _layer[0]._is_enabled=true;
     _layer[1]._is_enabled=false;
     _layer[2]._is_enabled=false;
     _layer[3]._is_enabled=false;

     _base_color_valid = false;

     break;

     case 1:

     // colors from paul cadmus...

     // other good choices:
     //   p-blobby-2-ht.png
     //   big_rough-ht.png
     //   brushed_and_teased-ht.png
     //   combs-ht.png
     //   craqulure-ht.png
     //   cross_fibre-ht.png
     //   finger_print-ht.png
     _current_pattern = str_ptr("finger_print-ht.png");

     _use_correction = false;
     _lod_only_corr = false;
     _tex_scale = 1.0f;

     // main dark layer:
     _layer[0] = layer_t(true, false, Color::color_ub( 37,  31,  33));
     _tone_shader->set_layer(
     0,     // layer 0
     1,     // enabled
     1,     // remap nl
     2,     // use smoothstep
     1,     // use backlight
     0.2,   // e0
     0.7);  // e1

     // white highlight layer:
     _layer[1] = layer_t(true, true,  Color::color_ub(192, 189, 208));
     _tone_shader->set_layer(
     1,     // layer 1
     1,     // enabled
     0,     // don't remap nl
     2,     // use smoothstep
     0,     // no backlight
     0.75f, // e0
     0.98f);// s1

     // bluish highlight layer:
     _layer[2] = layer_t(true, true,  Color::color_ub(172, 170, 191));
     _tone_shader->set_layer(
     2,     // layer 2
     1,     // enabled
     0,     // don't remap nl
     2,     // use smoothstep
     0,     // no backlight
     0.75f, // e0
     0.98f);// e1

     // orange highlight layer:
     _layer[3] = layer_t(true, true,  Color::color_ub(144, 130, 129));
     _tone_shader->set_layer(
     3,     // layer 3
     1,     // enabled
     0,     // don't remap nl
     2,     // use smoothstep
     0,     // no backlight
     0.75f, // e0
     0.98f);// e1

     // mid-tone background, slightly lighter patch
     VIEW::peek()->set_color(Color::color_ub(106, 109, 128));
     _base_color = Color::color_ub(144, 143, 157);
     _base_color_valid = true;

     break;

     case 2:

     break;

     case 3:
     _current_pattern = str_ptr("pattern5.png");

     //set_color(23, 93, 50);

     _use_correction = false;
     _lod_only_corr = false;
     _tex_scale = 35.0f/128.0f;

     _layer[0]._is_enabled=true;
     _layer[1]._is_enabled=false;
     _layer[2]._is_enabled=false;
     _layer[3]._is_enabled=false;

     _base_color_valid = false;

     break;

     case 4:
     _current_pattern = "";

     //set_color(70, 35, 117);

     _use_correction = false;
     _lod_only_corr = false;
     _tex_scale = 16.0f/PROCEDURAL_DOTS_SIZE;

     _layer[0]._is_enabled=true;
     _layer[1]._is_enabled=
     _layer[2]._is_enabled=
     _layer[3]._is_enabled=false;

     _base_color_valid = false;

     break;

     case 5:

     // colors from durer

     _current_pattern = str_ptr("finger_print-ht.png");

     _use_correction = false;
     _lod_only_corr = false;
     _tex_scale = 1.0f;

     // main dark layer:
     _layer[0] = layer_t(true, false, Color::color_ub(105,  97,  94));
     _tone_shader->set_layer(
     0,     // layer 0
     1,     // enabled
     0,     // don't remap nl
     1,     // use toon shader
     1);    // use backlight

     // highlight layer:
     _layer[1] = layer_t(true, true,  Color::color_ub(255, 253, 250));
     _tone_shader->set_layer(
     1,     // layer 1
     1,     // enabled
     0,     // don't remap nl
     2,     // use smoothstep
     0,     // no backlight
     0.7f,  // e0
     0.95f);// e1

     _layer[2]._is_enabled =
     _layer[3]._is_enabled = false;

     // mid-tone background:
     VIEW::peek()->set_color(Color::color_ub(197, 189, 186));
     _base_color_valid = false;

     break;

     case 6:
     // colors from paul cadmus, "Portrait of Ralph McWilliams"

     // other good choices:
     //   p-blobby-2-ht.png
     //   big_rough-ht.png
     //   brushed_and_teased-ht.png
     //   combs-ht.png
     //   craqulure-ht.png
     //   cross_fibre-ht.png
     //   finger_print-ht.png
     _current_pattern = str_ptr("finger_print-ht.png");

     _use_correction = false;
     _lod_only_corr = false;
     _tex_scale = 1.0f;

     // main dark layer:
     _layer[0] = layer_t(true, false, Color::color_ub( 60,  43,   0));
     _tone_shader->set_layer(
     0,     // layer 0
     1,     // enabled
     1,     // remap nl
     2,     // use smoothstep
     1,     // use backlight
     0.2,   // e0
     0.7);  // e1

     // white highlight layer:
     _layer[1] = layer_t(true, true,  Color::color_ub(253, 247, 251));
     _tone_shader->set_layer(
     1,     // layer 1
     1,     // enabled
     0,     // don't remap nl
     2,     // use smoothstep
     0,     // no backlight
     0.7f,  // e0
     0.95f, // e1
     0.8f,  // s0
     1.0f); // s1

     // bluish highlight layer:
     _layer[2] = layer_t(true, true,  Color::color_ub(122, 132, 107));
     _tone_shader->set_layer(
     2,     // layer 2
     1,     // enabled
     0,     // don't remap nl
     2,     // use smoothstep
     0,     // no backlight
     0.7f,  // e0
     0.95f);// e1

     _layer[3]._is_enabled = false;

     // mid-tone background:
     VIEW::peek()->set_color(Color::color_ub(149, 131,  49));
     _base_color_valid = false;

     break;

     case 7:
     // colors from roy lichtenstein, "Girl with ball"

     _current_pattern = str_ptr("big_rough-ht.png");

     _use_correction = false;
     _lod_only_corr = false;
     _tex_scale = 1.0f;

     // main dark layer:
     _layer[0] = layer_t(true, false, Color::color_ub( 44,  39,  71));
     _tone_shader->set_layer(
     0,     // layer 0
     1,     // enabled
     0,     // don't remap nl
     1,     // use toon ramp 
     1);    // e1

     // white highlight layer:
     _layer[1] = layer_t(true, true,  Color::color_ub(208, 199, 200));
     _tone_shader->set_layer(
     1,     // layer 1
     1,     // enabled
     0,     // don't remap nl
     2,     // use smoothstep
     0,     // no backlight
     0.6f,  // e0
     0.95f);// e1

     _layer[2]._is_enabled = 
     _layer[3]._is_enabled = false;

     // yellow background
     VIEW::peek()->set_color(Color::color_ub(189, 149,  28));

     // pinkish patch:
     _base_color = Color::color_ub(197, 171, 156);
     _base_color_valid = true;

     break;

     default:
     _current_pattern = "";

     //set_color(0, 10, 25);

     _use_correction = false;
     _lod_only_corr = false;
     _tex_scale = 25.0f/PROCEDURAL_DOTS_SIZE;

     _layer[0]._is_enabled=true;
     _layer[1]._is_enabled=false;
     _layer[2]._is_enabled=false;
     _layer[3]._is_enabled=false;
     }

     load_halftone_texture(_current_pattern);
     _UI->Update_UI();

   */
}

void
Halftone_TX::build_correction_tex(
   TEXTUREglptr correction_map,
   TEXTUREglptr source
   )
{
   stop_watch total_timer;

   // the histograms are fixed size of 256 buckets
   // and there is one for each of LOD_RES LOD values

   uint* histogram_array = new uint[256 * LOD_RES];

   uint sample_count = 0;  //gets value from the function call below
   build_pattern_histogram(histogram_array,sample_count,source);

   // constructing the error correction table

   if (debug)
      cerr << "Halftone_TX::build_correction_tex: "
           << "Building the tone response map ... " << endl;

   stop_watch inv_r_timer;
   // correction texture
   for (int i_lod=0; i_lod<LOD_RES; i_lod++) {

      //fills in a row of inverse R fot this lod value
      compute_inverse_R(
         i_lod,
         sample_count,
         histogram_array,
         &(correction_map->image())
         );
   }

   if (debug) {
      cerr << " Inverse R build time : "
           << inv_r_timer.elapsed_time() << endl;
   }

   if (debug) {
      cerr << "Total time ellapsed : "
           << total_timer.elapsed_time() << endl;
   }

   delete[] histogram_array;
}

void
Halftone_TX::build_r_function_tex(
   TEXTUREglptr r_function_map,
   TEXTUREglptr source
   )
{
   uint* histogram_array = new uint[256 * LOD_RES];

   uint sample_count = 0;  //gets value from the function call below
   build_pattern_histogram(histogram_array,sample_count,source);

   // constructing the error correction table

   if (debug)
      cerr << "Halftone_TX::build_correction_tex: "
           << "Building the tone response map ... " << endl;


   // forward R texture
   compute_forward_R(
      sample_count,
      histogram_array,
      &(r_function_map->image())
      );

   if (debug) {
      r_function_map->image().write_png("test_r_function.png");
   }

   delete[] histogram_array;
}

void
Halftone_TX::build_pattern_histogram(
   uint* histogram,
   uint &sample_count,
   TEXTUREglptr source
   )
{
   // hardcoded to 256x256  array of histograms for each lod value

   assert(histogram);

   stop_watch hist_timer;

   if (debug) {
      cerr << " Constructing histogram array ... ";
   }

   for (int i=0 ; i<(256*LOD_RES); i++)
      histogram[i] = 0;

   uint X_tex;
   uint Y_tex;
   if (source) {
      X_tex = source->get_size()[0];
      Y_tex = source->get_size()[1];
   } else {
      //for procedural pattern we use maximum sampling resolution
      X_tex = 256;
      Y_tex = 256;
   }

   sample_count = X_tex * Y_tex;

   // building the regular histogram for each  LOD value;
   for (int i_lod=0; i_lod<LOD_RES; i_lod++) {
      double f_lod = double(i_lod)/double(LOD_RES-1);  // 0.0~1.0

      for (uint x=0; x<X_tex; x++)
         for (uint y=0; y<Y_tex; y++) {

            double sample;
            if (source) {
               //sample the halftone texture
               double sample_hi =
                  source->image().pixel_r((x*2)%X_tex, (y*2) % Y_tex);
               double sample_lo =
                  source->image().pixel_r(x, y);

               sample =  min(255.0, interp(sample_lo,sample_hi,f_lod));

            } else {
               //sample procedural dots
               double sample_hi =
                  procedural_dots((double(x)/double(X_tex-1)) * 2.0 ,
                                  (double(y)/double(Y_tex-1)) * 2.0);
               double sample_lo =
                  procedural_dots(double(x)/double(X_tex-1),
                                  double(y)/double(Y_tex-1));

               sample =  min(255.0, interp(sample_lo,sample_hi,f_lod)*255.0);
            }

            uint bucket = uint(sample + 0.5 );
            histogram[256 * i_lod + bucket]+= 1;
         }
   }

   if (debug) {
      cerr << "Histogram built time : "
           << hist_timer.elapsed_time() << endl;
   }
}

void //fills in one row of the inverse_R function
Halftone_TX::compute_inverse_R(
   uint i_lod,
   uint sample_count,
   uint* histogram_array,
   Image* image
   )
{
   double old_R = 0.0;
   double old_f_tone = 0.0;

   for (int i_tone=0; i_tone<CORR_RES; i_tone++) {

      double f_tone = double(i_tone) / double(CORR_RES-1); // 0.0~1.0

      //computes the R first
      //in order to take the smooth step into account
      //the procedure steps trough all heigths found in the
      //pattern, than it does the smooth step on each bucket
      //and multiplies by the total count of pixels in that bucket

      double R = 0.0;
      for (int h=0; h< 256; h++) {
         double tested_h = double(h)/double(255); //0~1

         R+= double (histogram_array[256 * i_lod + int(tested_h * 255.0)]) *
            smooth_step(-0.1,0.1, f_tone - tested_h) ;
      }

      R =  R / double (sample_count);

      //function inversion
      //this part is still a little iffy in my opinion

      bool filled = false;
      for (int i =  int (old_R * CORR_RES); i < int(R * CORR_RES); i++) {
         double f_i =
            double(i - int(old_R * CORR_RES))/double(int(R * CORR_RES)-1);
         image->set_grey(min(CORR_RES - 1, i),
                         i_lod,
                         int((interp(old_f_tone, f_tone, f_i)*255.0) + 0.5));
         filled = true;
      }

      if (filled) {
         old_f_tone = f_tone;
         old_R = R;
      }
   }

   //fixing the end, this hack puts a white pixel at the end
   image->set_grey( CORR_RES-1, i_lod, 255);
}

void
Halftone_TX::compute_forward_R(
   uint sample_count,
   uint* histogram_array,
   Image* image
   )
{
   stop_watch r_timer;

   for (int i_tone=0; i_tone<CORR_RES; i_tone++) {

      double f_tone = double(i_tone) / double(CORR_RES-1); // 0.0~1.0

      //computes the R
      //in order to take the smooth step into account
      //the procedure steps trough all heigths found in the
      //pattern, than it does the smooth step on each bucket
      //and multiplies by the total count of pixels in that bucket

      double R = 0.0;
      for (int h=0; h< 256; h++) {
         double tested_h = double(h)/double(255); //0~1

         R+= double (histogram_array[ int(tested_h * 255.0)]) *
            smooth_step(-0.1,0.1, f_tone - tested_h) ;

      }

      R =  R / double (sample_count);

      image->set_grey(i_tone,0, int (R*255.0));
   }

   image->set_grey(0,0,0);
   image->set_grey(CORR_RES,0,255);

   if (debug) {
      cerr << " R build time : "
           << r_timer.elapsed_time() << endl;
   }
}

double
Halftone_TX::smooth_step(double edge0, double edge1, double x)
{
   double t = (x-edge0)/(edge1-edge0);
   t = clamp( t, 0.0 , 1.0 );
   return t*t*(3.0-2.0*t);
}

double
Halftone_TX::procedural_dots(double x, double y)
{
   if (x>1.0) x = x - 1.0;
   if (y>1.0) y = y - 1.0;

   const double A = 0.4;
   return ((0.5 + A * cos(x*TWO_PI)) + (0.5 + A * cos(y*TWO_PI)))/2;
}

void
Halftone_TX::toggle_tone_correction(int layer_num)
{
   halftone_layer_t* layer = get_layer(layer_num);
   assert(layer);

   if (layer->_use_tone_correction) {
      disable_tone_correction(layer_num);
   } else {
      enable_tone_correction(layer_num);
   }
}

void 
Halftone_TX::toggle_lod_only_correction(int layer_num)
{
   halftone_layer_t* layer = get_layer(layer_num);
   assert(layer);

   if (layer->_use_lod_only) {
      disable_lod_only_correction(layer_num);
   } else {
      enable_lod_only_correction(layer_num);
   }
}

void
Halftone_TX::enable_tone_correction(int layer_num)
{
   //see tone correction for this layer's texture exists

   halftone_layer_t* layer = get_layer(layer_num);
   assert(layer);

   if (layer->_use_tone_correction) {
      disable_tone_correction(layer_num);
   }

   if (layer->_mode !=1 && layer->_mode !=2) {
      cerr << "Halftone_TX::enable_tone_correction"
           << ": unknown layer mode, can't setup tone correction " << endl;
      return;
   }

   // look if the tone correction map has been already created for
   // this layer's texture stage somewhere else
   bool used = false;
   for (int i=0; i<_layers.num(); i++) {
      if (i == layer_num)
         continue;
      halftone_layer_t* layer_i = get_layer(i);
      assert(layer_i);
      
      if (layer_i->_use_tone_correction &&
          layer_i->_pattern_tex_stage == layer->_pattern_tex_stage) {
         used = true;
         layer->_tone_correction_tex_stage =
            layer_i->_tone_correction_tex_stage;
         layer->_use_tone_correction = true;
         break;
      }
   }

   if (!used) {
      //create tone correction map for this stage

      GLint tex_stage = get_free_tex_stage();

      if (debug)
         cerr << endl
              << "Creating tone correction map at tex stage " << tex_stage
              << " for source at " << layer->_pattern_tex_stage << endl;

      tone_correction_maps[tex_stage] = new TEXTUREgl(
         "", GL_TEXTURE_2D, GL_TEXTURE0 + tex_stage
         );

      tone_correction_maps[tex_stage]->set_save_img(true);
      tone_correction_maps[tex_stage]->set_wrap_r(GL_CLAMP_TO_EDGE);
      tone_correction_maps[tex_stage]->set_wrap_s(GL_CLAMP_TO_EDGE);
      tone_correction_maps[tex_stage]->image().resize(CORR_RES,LOD_RES,4);

      //bug test, if update fails than it should be filled with white

      for (int x=0; x<CORR_RES; x++) {
         for (int y=0; y<LOD_RES; y++) {
            tone_correction_maps[tex_stage]->image().set_grey(x, y, 255);
         }
      }
       
      if (layer->_mode == 1)
         build_correction_tex(
            tone_correction_maps[tex_stage],
            _patterns[layer->_pattern_tex_stage]
            );

      if (layer->_mode == 2)
         build_correction_tex(tone_correction_maps[tex_stage],0);

      layer->_use_tone_correction = true;
      layer->_tone_correction_tex_stage = tex_stage;

      if (debug) {
         if (!tone_correction_maps[tex_stage]->image().
             write_png("test_error_texture.png"))
            cerr << "Unable to write the debug file test_error_texture.png "
                 << endl;
      }
   }
}

void
Halftone_TX::enable_lod_only_correction(int layer_num)
{
   halftone_layer_t* layer = get_layer(layer_num);
   assert(layer);

   if (layer->_use_lod_only) {
      disable_lod_only_correction(layer_num);
   }

   enable_tone_correction(layer_num); //need this first

   if (layer->_mode !=1 && layer->_mode !=2) {
      cerr << "Unknown layer mode, can't setup tone correction " << endl;
      return;
   }

   // look if the r function map has been already creaed for
   // this layer's texture stage somewhere else
   bool used = false;
   for (int i=0; i<_layers.num(); i++) {
      halftone_layer_t* layer_i = get_layer(layer_num);
      if (layer_i->_use_lod_only &&
          layer_i->_pattern_tex_stage == layer->_pattern_tex_stage) {
         used = true;
         layer->_r_function_tex_stage = layer_i->_r_function_tex_stage;
         layer->_use_lod_only = true;
         if (debug)
            cerr << "Layer " << i
                 << " already has this texture so we will share "
                 << endl;
         break;
      }
   }

   if (!used) {
      //create r function map for this stage
      GLint tex_stage = get_free_tex_stage();

      r_function_maps[tex_stage] = new TEXTUREgl(
         "", GL_TEXTURE_2D, GL_TEXTURE0 + tex_stage
         );

      r_function_maps[tex_stage]->set_save_img();
      r_function_maps[tex_stage]->set_wrap_r(GL_CLAMP_TO_EDGE);
      r_function_maps[tex_stage]->set_wrap_s(GL_CLAMP_TO_EDGE);

      // it's a 1D texture really:
      r_function_maps[tex_stage]->image().resize(CORR_RES,1,4);
      
      if (layer->_mode == 1)
         build_r_function_tex(r_function_maps[tex_stage],
                              _patterns[layer->_pattern_tex_stage]);
      if (layer->_mode == 2)
         build_r_function_tex(r_function_maps[tex_stage],0);
      
      layer->_r_function_tex_stage = tex_stage;
      layer->_use_lod_only = true;
   }
}

void 
Halftone_TX::disable_tone_correction(int layer_num)
{
   halftone_layer_t* layer = get_layer(layer_num);
   assert(layer);

   //disable tone correction
   if(!layer->_use_tone_correction)
      return;

   layer->_use_tone_correction=false;
   GLint tex_stage = layer->_tone_correction_tex_stage;

   bool used = false;
   for (int i=0; i<_layers.num(); i++) {
      halftone_layer_t* layer_i = get_layer(i);
      if (layer_i->_use_tone_correction &&
          layer_i->_tone_correction_tex_stage == tex_stage)
         used = true;
   }

   if (!used) {
      if (debug)
         cerr << "Recycling texture stage " << tex_stage
              << " back to unused and mopping up the details.. " << endl;
      free_tex_stage(tex_stage);
      tone_correction_maps.erase(tex_stage);
      layer->_tone_correction_tex_stage = 0;
   } else {
      layer->_tone_correction_tex_stage = 0;
   }
}

void 
Halftone_TX::disable_lod_only_correction(int layer_num)
{
   halftone_layer_t* layer = get_layer(layer_num);
   assert(layer);

   if(!layer->_use_lod_only)
      return;
      
   // flip off tone correction
   assert(layer->_use_tone_correction);
   toggle_tone_correction(layer_num);

   // disable lod only correction
   layer->_use_lod_only=false;
   GLint tex_stage = layer->_r_function_tex_stage;

   bool used = false;
   for (int i=0; i<_layers.num(); i++) {
      halftone_layer_t* layer_i = get_layer(i);
      if (layer_i->_use_lod_only &&
          layer_i->_r_function_tex_stage == tex_stage)
         used = true;
   }

   if (!used) {
      if (debug)
         cerr << "Recycling texture stage " << tex_stage
              << " back to unused and mopping up the details.. " << endl;
      free_tex_stage(tex_stage);
      tone_correction_maps.erase(tex_stage);
      layer->_r_function_tex_stage = 0;
   } else {
      layer->_r_function_tex_stage = 0;
   }
}

void 
Halftone_TX::set_texture_pattern(int layer, str_ptr file_name)
{
   GLSLShader_Layer_Base::set_texture_pattern(
      layer,"nprdata/haftone_textures/",file_name
      );
   disable_lod_only_correction(layer);
   disable_tone_correction(layer);
}

void 
Halftone_TX::set_procedural_pattern(int layer, int pattern_id)
{
   GLSLShader_Layer_Base::set_procedural_pattern( layer, pattern_id);

   disable_lod_only_correction(layer);
   disable_tone_correction(layer);
}

//------------------I/O-------------------
/////////////////////////////////////
// tags()  
/////////////////////////////////////

CTAGlist&  
Halftone_TX::tags() const
{
   if (!_halftone_tags) {
      _halftone_tags = new TAGlist;
     
      // start with base class tags
      *_halftone_tags = GLSLShader_Layer_Base::tags();

      *_halftone_tags += new TAG_meth<Halftone_TX>(
         "halftone_layer",
         &GLSLShader_Layer_Base::put_layer, // base class puts layers
         &Halftone_TX::get_layer,
         1);
   }
   return *_halftone_tags;
}

void         
Halftone_TX::get_layer(TAGformat &d)
{
   int i;
   *d >> i;
   
   str_ptr str;
   *d >> str;

   if ((str != halftone_layer_t::static_name())) {
      cerr << class_name()
           << "::get_layer: error: unexpected layer type: "
           << str
           << endl;
      return;
   }

   halftone_layer_t* layer = get_layer(i);
   assert(layer);
   layer->decode(*d);

   // save the name here so we can load the texture properly
   // in set_texture_pattern(), below:
   str_ptr pattern_name = layer->_pattern_name;
   layer->_pattern_name = "";

   // save the parameters because pattern loading resets these
   bool use_corr = layer->_use_tone_correction;
   bool lod_only = layer->_use_lod_only;
   layer->_use_tone_correction=false;
   layer->_use_lod_only=false;

   if (layer->_mode == 1) {
      set_texture_pattern(i, pattern_name);
   } else if (layer->_mode == 2) {
      set_procedural_pattern(i,0);
   }
     
   if (layer->_mode != 0) {
      if (use_corr) {
         enable_tone_correction(i);
      }
      if (lod_only) {
         enable_lod_only_correction(i);
      }
   }
}

// end of file halftone_tx.C
