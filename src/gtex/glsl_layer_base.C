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
 *   Layer Base .C
 *****************************************************************/
#include "glsl_layer_base.H"
#include "mesh/patch_blend_weight.H"

/*****************************************************************
 * layer_base_t
 *****************************************************************/
static bool debug = Config::get_var_bool("DEBUG_LAYER_BASE",false);
TAGlist* layer_base_t::_lb_tags = 0;

void         
layer_base_t::get_pattern_name (TAGformat &d)
{
   *d >> _pattern_name;

   // XXX - hack to deal with previous hack where empty file
   //       name was written as "0":
   if (_pattern_name == "0")
      _pattern_name = "";

   if (debug && _pattern_name != "") {
      cerr << "layer_base_t::get_pattern_name: got name: "
           << _pattern_name
           << endl;
   }
}

void 
layer_base_t::put_pattern_name(TAGformat &d) const
{
   if (_pattern_name != "") {
      d.id();
      *d << _pattern_name;
      d.end_id();
   }
}

bool
layer_base_t::get_uniform_loc(Cstr_ptr& var_name, GLint& loc, GLuint& program)
{
   // wrapper for glGetUniformLocation, with error reporting

   loc = glGetUniformLocation(program, **var_name);
   if (loc < 0) {
      cerr << "layer_base_t" << "::get_uniform_loc: error: variable "
           << "\"" << var_name << "\" not found"
           << endl;
      return false;
   }

   return true;
}

void 
layer_base_t::get_var_locs_some(int mode, int i, GLuint& program)
{
   str_ptr p = str_ptr("layer[") + str_ptr(i);
   switch(mode) {
    case 0:  // All
      get_var_locs(i, program);
      break;
    case 1:  // No ink_color
      get_uniform_loc(p + "].mode",          _mode_loc,              program);
      get_uniform_loc(p + "].is_highlight",  _highlight_loc,         program);
      get_uniform_loc(p + "].pattern_scale", _pattern_scale_loc,     program);
      get_uniform_loc(p + "].pattern",       _pattern_tex_stage_loc, program);
      get_uniform_loc(p + "].channel",       _channel_loc,           program);
      break;
    case 2:  // Just pattern and pattern_scale
      get_uniform_loc(p + "].mode",          _mode_loc,              program);
      get_uniform_loc(p + "].pattern_scale", _pattern_scale_loc,     program);
      get_uniform_loc(p + "].pattern",       _pattern_tex_stage_loc, program);
      break;
    default:
      get_var_locs(i, program);
   }
}

void
layer_base_t::get_var_locs(int i, GLuint& program)
{
   //we need to agree on the naming convention for layers in GLSL

   str_ptr p = str_ptr("layer[") + str_ptr(i);

   get_uniform_loc(p + "].mode",          _mode_loc,              program);
   get_uniform_loc(p + "].is_highlight",  _highlight_loc,         program);
   get_uniform_loc(p + "].ink_color",     _ink_color_loc,         program);
   get_uniform_loc(p + "].pattern_scale", _pattern_scale_loc,     program);
   get_uniform_loc(p + "].pattern",       _pattern_tex_stage_loc, program);
   get_uniform_loc(p + "].channel",       _channel_loc,           program);
}

void
layer_base_t::send_to_glsl() const
{
   if(_mode_loc != -1)
      glUniform1i (_mode_loc,_mode);
   
   if (_mode==1 && _pattern_tex_stage_loc != -1)
      glUniform1i (_pattern_tex_stage_loc, _pattern_tex_stage);

   if(_highlight_loc != -1)
      glUniform1i (_highlight_loc, _highlight);

   if(_ink_color_loc != -1)
      glUniform3fv (_ink_color_loc, 1, float3(_ink_color));
   
   if(_pattern_scale_loc != -1)
      glUniform1f (_pattern_scale_loc, _pattern_scale);
   
   if(_channel_loc != -1)
      glUniform1i (_channel_loc, _channel);
}

CTAGlist&
layer_base_t::tags() const
{
   if (!_lb_tags) {
      _lb_tags = new TAGlist;
      
      *_lb_tags += new TAG_meth<layer_base_t>(
         "pattern_name",
         &layer_base_t::put_pattern_name,
         &layer_base_t::get_pattern_name,
         1);

      *_lb_tags += new TAG_val<layer_base_t,int>(
         "mode",
         &layer_base_t::mode
         );
      *_lb_tags += new TAG_val<layer_base_t,bool>(
         "highlight",
         &layer_base_t::highlight
         );
      *_lb_tags += new TAG_val<layer_base_t,float>(
         "pattern_scale",
         &layer_base_t::pattern_scale
         );
      *_lb_tags += new TAG_val<layer_base_t,COLOR>(
         "ink_color",
         &layer_base_t::ink_color
         );
      *_lb_tags += new TAG_val<layer_base_t,GLint>(
         "channel",
         &layer_base_t::channel
         );
   }
   return *_lb_tags;
}

/*****************************************************************
 * GLSLShader_Layer_Base:
 *****************************************************************/
TAGlist* GLSLShader_Layer_Base::_tags = 0;

GLSLShader_Layer_Base::GLSLShader_Layer_Base(Patch* p, StripCB* cb) :
   GLSLShader(p, cb),
   _solid(new SolidColorTexture(p, Color::white))
{}

GLSLShader_Layer_Base::~GLSLShader_Layer_Base()
{
   while(!_layers.empty())
      delete _layers.pop();
   gtextures().delete_all();
}

void
GLSLShader_Layer_Base::get_layers_variable_names()
{
   for (int i=0; i<_layers.num(); i++)
      _layers[i]->get_var_locs(i, program());
}

void
GLSLShader_Layer_Base::send_layers_variables() const
{
   for (int i=0; i<_layers.num(); i++)
      _layers[i]->send_to_glsl();
}

bool
GLSLShader_Layer_Base::is_used_tex_stage(int tex_stage)
{
   for (unsigned int i=0; i<_used_texture_stages.size(); i++) {
      if ( tex_stage == _used_texture_stages[i] )
         return true;
   }
   return false;
}

inline GLint
num_tex_units()
{
   // return the number of texture units supported by the GPU.
   // granted, we have reason to believe the number returned 
   // is wrong!

   GLint ret=0;
   //glGetIntegerv(GL_MAX_TEXTURE_UNITS, &ret);
   ret = 31;
   return ret;
}

int 
GLSLShader_Layer_Base::get_free_tex_stage()
{
   // returns the lowest free texture stage, and marks as used

   int tex_stage = TexUnit::REF_HALO+1; //start at this one

   while (is_used_tex_stage(tex_stage)) {
      tex_stage++;
   }

   GLint num_units = num_tex_units();

   _used_texture_stages.push_back(tex_stage);

   if (!(tex_stage < num_units)) {
      cerr << "GLSLShader_Layer_Base::get_free_tex_stage: warning: "
           << "max number of " << num_units
           << " texture units exceeded" << endl;
   } else if (debug) {
      cerr << "GLSLShader_Layer_Base::get_free_tex_stage: "
           << "using texture unit " << tex_stage
           << " of " << num_units << endl;
   }
   return tex_stage;
}

void  
GLSLShader_Layer_Base::free_tex_stage(int tex_stage)
{
   // marks texture stage as unused,
   // noop if pased stage is already unused

   for (unsigned int i=0; i<_used_texture_stages.size(); i++) {
      //finds and removes all matching ints
      if ( tex_stage == _used_texture_stages[i] ) {
         _used_texture_stages[i] =
            _used_texture_stages[_used_texture_stages.size()-1];
         _used_texture_stages.pop_back();
      }
   }
}

void
GLSLShader_Layer_Base::activate_layer_textures()
{
   for (int i=0; i<_layers.num(); i++)
      if ((_layers[i]->_mode == 1) && (_layers[i]->_pattern_tex_stage!=-1))
         if (!activate_texture(_patterns[_layers[i]->_pattern_tex_stage]))
            cerr << "Unable to activate pattern texture for layer "
                 << i << endl;
}

void
GLSLShader_Layer_Base::set_texture_pattern(
   int     layer_num,
   str_ptr dir,
   str_ptr file_name
   )
{
   // screen out the crazies
   assert(_layers.valid_index(layer_num));
   layer_base_t* layer = _layers[layer_num];
   assert(layer);

   // if texture is already set for this layer, do nothing
   if (layer->_pattern_name == file_name) {
      return;
   }

   // free the existing texture if it exists
   if (layer->_mode==1 && layer->_pattern_name != "") {
      // the following frees texture resources if no other layer is using
      // the pattern texture:
      cleanup_unused_textures(layer->_pattern_tex_stage);
      layer->_pattern_tex_stage = -1;
      layer->_mode = 0;
      layer->_pattern_name = "";
   }

   if (file_name == "")
      return;

   // see if this texture is already loaded by another layer
   for (int i=0; i<_layers.num(); i++) {
      assert(_layers[i]);
      if (_layers[i]->_mode==1)
         if (_layers[i]->_pattern_name == file_name) {
            // found one already in use
            layer->_pattern_tex_stage = _layers[i]->_pattern_tex_stage;
            layer->_mode = 1;
            layer->_pattern_name = file_name;
            return;
         }
   }

   // get here if texture is not already loaded
   // by chosen layer or any other

   GLint tex_stage = get_free_tex_stage();

   str_ptr pre_path = Config::JOT_ROOT() + dir;
   str_ptr path = pre_path + file_name;
   _patterns[tex_stage] =
      new TEXTUREgl(path, GL_TEXTURE_2D, GL_TEXTURE0 + tex_stage);
   _patterns[tex_stage]->set_save_img(true);

   if (!_patterns[tex_stage]->load_image()) {
      cerr << class_name() << "::set_texture_pattern: "
           << "invalid pattern texture " << path << endl;

      // fail in a civilized manner
      layer->_mode = 0;
      layer->_pattern_tex_stage = -1;
      layer->_pattern_name = str_ptr("");
      _patterns.erase(tex_stage);
      free_tex_stage(tex_stage);

   } else {
      // succeeded in loading the texture
      layer->_mode = 1;
      layer->_pattern_name = file_name;
      layer->_pattern_tex_stage = tex_stage;
      _patterns[tex_stage]->set_wrap_r(GL_REPEAT);
      _patterns[tex_stage]->set_wrap_s(GL_REPEAT);
      if (debug) {
         cerr << "GLSLShader_Layer_Base::set_texture_pattern: ("
              << class_name() << "): new pattern texture "
              << file_name
              << " at unit " << tex_stage << endl;
      }
   }
}

void
GLSLShader_Layer_Base::cleanup_unused_textures(int tex_stage)
{
   if (tex_stage == -1)
      return;

   // cleanup unused textures
   for (int i=0; i<_layers.num(); i++) {
      if (_layers[i]->_pattern_tex_stage == tex_stage) {
         // something else is still using this texture stage
         return;
      }
   }
   // nothing else is using this texture
   _patterns.erase(tex_stage);
   free_tex_stage(tex_stage);
}

void
GLSLShader_Layer_Base::disable_layer(int layer)
{
   assert(_layers.valid_index(layer));
   _layers[layer]->_mode = 0;
   _layers[layer]->_pattern_name = str_ptr("");
   GLint tex_stage =  _layers[layer]->_pattern_tex_stage;
   _layers[layer]->_pattern_tex_stage = -1;
   cleanup_unused_textures(tex_stage);
}

void
GLSLShader_Layer_Base::set_procedural_pattern(int layer, int pattern_id)
{
   // must asign layer object in the derived constructor
   assert(_layers.valid_index(layer) && _layers[layer]);

   _layers[layer]->_mode = 2 + pattern_id;
   _layers[layer]->_pattern_name = str_ptr("");

   GLint tex_stage = _layers[layer]->_pattern_tex_stage;
   _layers[layer]->_pattern_tex_stage= -1;

   cleanup_unused_textures(tex_stage);
}

CTAGlist&
GLSLShader_Layer_Base::tags() const
{
   if (!_tags) {
      // start with base class tags
      _tags = new TAGlist;
      *_tags = GLSLShader::tags();
     
      *_tags += new TAG_meth<GLSLShader_Layer_Base>(
         "tone_shader",
         &GLSLShader_Layer_Base::put_tone_shader,
         &GLSLShader_Layer_Base::get_tone_shader,
         1);

      *_tags += new TAG_meth<GLSLShader_Layer_Base>(
         "base_shader",
         &GLSLShader_Layer_Base::put_base_shader,
         &GLSLShader_Layer_Base::get_base_shader,
         1);        
  
   }
   return *_tags;
}

void
GLSLShader_Layer_Base::put_layer(TAGformat &d) const
{   
   for(int i=0; i<_layers.num(); ++i) {
      assert(_layers[i]);
      // skip disabled layers:
      if (_layers[i]->_mode != 0) {
         d.id();
         *d << i;
         _layers[i]->format(*d);
         d.end_id();
      }
   }
}

void 
GLSLShader_Layer_Base::get_tone_shader(TAGformat &d)
{
   str_ptr str;
   *d >> str;

   if ((str != ToneShader::static_name())) {
      cerr << class_name()
           << "::get_tone_shader: error: unexpected shader name: "
           << str
           << endl;
      return;
   }
   assert(get_tex<ToneShader>(_patch));
   get_tex<ToneShader>(_patch)->decode(*d);
}

void
GLSLShader_Layer_Base::put_tone_shader(TAGformat &d) const
{
   d.id();
   assert(get_tex<ToneShader>(_patch));
   get_tex<ToneShader>(_patch)->format(*d);
   d.end_id();
}

void 
GLSLShader_Layer_Base::get_base_shader(TAGformat &d)
{
   str_ptr str;
   *d >> str;

   if ((str != SolidColorTexture::static_name())) {
      cerr << class_name()
           << "::get_base_shader: error: unexpected shader name: "
           << str
           << endl;
      return;
   }
   assert(_solid);
   _solid->decode(*d);
}

void
GLSLShader_Layer_Base::put_base_shader(TAGformat &d) const
{
   if (_solid) {
      d.id();
      _solid->format(*d);
      d.end_id();
   }
}

/*****************************************************************
 * StripOpCB::faceCB
 *****************************************************************/
void 
StripOpCB::faceCB(CBvert* v, CBface* f) 
{
   if (_patch && _patch->mesh()->get_do_patch_blend()) {
      send_opacity_attr(PatchBlendWeight::get_weight(_patch, v));
   }else{
      send_opacity_attr(1.0);
   }
   glNormal3dv(f->vert_normal(v).data());
   glVertex3dv(v->loc().data());
}

// glsl_layer_base.C
