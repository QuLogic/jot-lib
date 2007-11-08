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
 * tone_shader.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "tone_shader.H"
#include "mesh/uv_data.H"

const int MAX_OCCLUDERS = 4;

static bool debug = Config::get_var_bool("DEBUG_TONE_SHADER", false);
TAGlist* ToneShader::_tags = 0;
TAGlist* tone_layer_t::_tl_tags = 0;

CTAGlist &
tone_layer_t::tags() const
{
   if (!_tl_tags) {
      _tl_tags = new TAGlist;         

      *_tl_tags += new TAG_val<tone_layer_t,GLint>(
         "is_enabled",
         &tone_layer_t::is_enabled
         );
      *_tl_tags += new TAG_val<tone_layer_t,GLint>(
         "remap_nl",
         &tone_layer_t::remap_nl
         );
      *_tl_tags += new TAG_val<tone_layer_t,GLint>(
         "remap",
         &tone_layer_t::remap
         );
      *_tl_tags += new TAG_val<tone_layer_t,GLint>(
         "backlight",
         &tone_layer_t::backlight
         );
      *_tl_tags += new TAG_val<tone_layer_t,GLfloat>(
         "e0",
         &tone_layer_t::e0
         );
      *_tl_tags += new TAG_val<tone_layer_t,GLfloat>(
         "e1",
         &tone_layer_t::e1
         );
      *_tl_tags += new TAG_val<tone_layer_t,GLfloat>(
         "s0",
         &tone_layer_t::s0
         );
      *_tl_tags += new TAG_val<tone_layer_t,GLfloat>(
         "s1",
         &tone_layer_t::s1
         );
   }
   return *_tl_tags;
}

void
ToneStripCB::faceCB(CBvert* v, CBface* f)
{
   assert(v && f);
   Wvec bNorm;

   //first calculate the abstract(blended) normal

   switch(_blend_type) {
    case ToneStripCB::SMOOTH: {
       // Note: doesn't work
       bNorm = v->get_all_faces().n_ring_faces(2).avg_normal();
    }
      break;
    case ToneStripCB::SPHERIC: {
       BMESH* mesh = v->mesh();
       Wpt c = mesh->get_bb().center();
       bNorm = (v->loc()-c).normalized();
    }
      break;
    case ToneStripCB::ELLIPTIC: {
       BMESH* mesh = v->mesh();
       Wvec c_to_v = v->loc() - mesh->get_bb().center();
       Wvec dim = mesh->get_bb().dim();
       double a = dim[0]*0.5;
       double b = dim[1]*0.5;
       double c = dim[2]*0.5;
       bNorm = Wvec(c_to_v[0]/a, c_to_v[1]/b, c_to_v[2]/c).normalized();
    }
      break;
    case ToneStripCB::CYLINDRIC: {
       BMESH* mesh = v->mesh();
       Wpt c = mesh->get_bb().center();
       Wvec axis;
       Wvec dim = mesh->get_bb().dim();
       if (dim[0]>dim[1] && dim[0]>dim[2])
          axis = dim.X();
       else if (dim[1]>dim[0] && dim[1]>dim[2])
          axis = dim.Y();         
       else 
          axis = dim.Z();         

       Wpt v_proj = c + ((v->loc()-c)*axis) * axis;
       bNorm = (v->loc()-v_proj).normalized();
    }
      break;
    default:
      assert(0);
   }

   //set the blended normal, the regular normal and the vertex point
   glVertexAttrib3f(_norm_loc, bNorm[0], bNorm[1], bNorm[2]); 

   float avg_len = v->avg_edge_len();
   glVertexAttrib1f(_len_loc, avg_len);

   glNormal3dv(f->vert_normal(v).data());
   glTexCoord2dv(UVdata::get_uv(v,f).data());

   glVertex3dv(v->loc().data());

}



/**********************************************************************
 * ToneShader:
 *
 *   Does 1D Toon shading in GLSL.
 **********************************************************************/
GLuint ToneShader::_program = 0;
bool   ToneShader::_did_init = false;
GLint  ToneShader::_tex_loc = -1;
GLint  ToneShader::_tex_2d_loc = -1;
GLint  ToneShader::_is_tex_2d_loc = -1;

GLint  ToneShader::_is_enabled_loc[4] = {-1};
GLint  ToneShader::_remap_nl_loc  [4] = {-1};
GLint  ToneShader::_remap_loc     [4] = {-1};
GLint  ToneShader::_backlight_loc [4] = {-1};
GLint  ToneShader::_e0_loc        [4] = {-1};
GLint  ToneShader::_e1_loc        [4] = {-1};
GLint  ToneShader::_s0_loc        [4] = {-1};
GLint  ToneShader::_s1_loc        [4] = {-1};
GLint  ToneShader::_blend_normal_loc  = -1;
GLint  ToneShader::_ratio_scale_loc  = -1;
GLint  ToneShader::_edge_len_scale_loc  = -1;
GLint  ToneShader::_unit_len_loc  = -1;
GLint  ToneShader::_user_depth_loc  = -1;
GLint  ToneShader::_global_edge_len_loc  = -1;
GLint  ToneShader::_proj_der_loc  = -1;
GLint  ToneShader::_is_reciever_loc = -1;

ToneShader::ToneShader(Patch* p) :
   GLSLShader(p, new ToneStripCB),
   _smoothNormal(1),
   _normals_smoothed(false),
   _normals_elliptic(false),
   _normals_spheric(true),
   _normals_cylindric(false),
   _blend_normal(0),
   _unit_len(0.5),
   _edge_len_scale(0.1),
   _user_depth(0.0),
   _ratio_scale(0.5),
   _global_edge_len(-1.0),
   _tex_2d(0)
{
   set_tex(Config::get_var_str(
              "TONE_SHADER_FILENAME",
              GtexUtil::toon_name("clear-black.png")
              ));

   set_layer(0, 1, 0, 1, 1, 0, 1, 0.5, 1.0);
   set_layer(1, 1, 0, 1, 0, 0, 1, 0.5, 1.0);
   set_layer(2, 1, 0, 1, 0, 0, 1, 0.5, 1.0);
   set_layer(3, 1, 0, 1, 0, 0, 1, 0.5, 1.0);

   set_draw_sils(Config::get_var_bool("TONE_IMG_DRAW_SILS",false));

}

void
ToneShader::set_normals(int i)
{
   _smoothNormal = i;

   if(i==0)
      _normals_smoothed = true;
   else if(i==1)
      _normals_spheric = true;
   else if(i==2)
      _normals_elliptic = true;
   else if(i==3)
      _normals_cylindric = true;

   ToneStripCB* cb = dynamic_cast<ToneStripCB*>(_cb);
   assert(cb);
   cb->set_blendType(_smoothNormal);

   Patch* p = dynamic_cast<Patch*>(_patch);
   p->changed();
}


void 
ToneShader::set_tex(Cstr_ptr& filename)
{
   // Set the name of the texture to use
   if (_tex) {
      _tex->set_texture(filename);
   } else {
      set_tex(new TEXTUREgl(filename));
      assert(_tex);
   }
}

void 
ToneShader::set_tex(CTEXTUREglptr& tex)
{
   // set the TEXTUREgl to use:
   _tex = tex;

   if (_tex) {
      assert(_tex->target() == GL_TEXTURE_2D);
      _tex->set_wrap_s(GL_CLAMP_TO_EDGE);
      _tex->set_wrap_t(GL_CLAMP_TO_EDGE);
      _tex->set_tex_fn(GL_REPLACE);
   }
}

void 
ToneShader::set_tex_2d(CTEXTUREglptr& tex)
{
   // set the TEXTUREgl to use:
   assert(tex->target() == GL_TEXTURE_2D);
   _tex_2d = tex;

   if (_tex_2d) {
      _tex_2d->set_wrap_s(GL_CLAMP_TO_EDGE);
      _tex_2d->set_wrap_t(GL_CLAMP_TO_EDGE);
      _tex_2d->set_tex_fn(GL_REPLACE);
   }
}

void
ToneShader::init_textures()
{
   // no-op after the first time:
   load_texture(_tex);
   load_texture(_tex_2d);

   if(_global_edge_len < 0){
      _global_edge_len = mesh()->avg_len();
   }
}

void
ToneShader::activate_textures()
{
   populate_occluders();
   activate_texture(_tex);      // GL_ENABLE_BIT
}

bool 
ToneShader::get_variable_locs()
{
   ToneStripCB* cb = dynamic_cast<ToneStripCB*>(_cb);
   GLint normLoc = glGetAttribLocation(_program,"blendNorm");
   cb->set_norm_loc(normLoc);
   cb->set_blendType(_smoothNormal);

   GLint lenLoc = glGetAttribLocation(_program,"local_edge_len");
   cb->set_len_loc(lenLoc);

   get_uniform_loc("blend_normal", _blend_normal_loc);
   get_uniform_loc("unit_len", _unit_len_loc);
   get_uniform_loc("edge_len_scale", _edge_len_scale_loc);
   get_uniform_loc("ratio_scale", _ratio_scale_loc);
   get_uniform_loc("user_depth", _user_depth_loc);

   get_uniform_loc("toon_tex", _tex_loc);
   get_uniform_loc("tex_2d", _tex_2d_loc);
   get_uniform_loc("is_tex_2d", _is_tex_2d_loc);

   for (int i=0; i<4; i++) {
      str_ptr p = str_ptr("layer[") + str_ptr(i);
      get_uniform_loc(p + "].is_enabled", _is_enabled_loc[i]);
      get_uniform_loc(p + "].remap_nl",   _remap_nl_loc  [i]);
      get_uniform_loc(p + "].remap",      _remap_loc     [i]);
      get_uniform_loc(p + "].backlight",  _backlight_loc [i]);
      get_uniform_loc(p + "].e0",         _e0_loc        [i]);
      get_uniform_loc(p + "].e1",         _e1_loc        [i]);
      get_uniform_loc(p + "].s0",         _s0_loc        [i]);
      get_uniform_loc(p + "].s1",         _s1_loc        [i]);
   }

   for (int i=0; i<_occluders.num(); i++) {
      str_ptr p = str_ptr("occluder[") + str_ptr(i);
      get_uniform_loc(p+"].is_active",_occluders[i]._is_active_loc);
      get_uniform_loc(p+"].xf",       _occluders[i]._xf_loc);
      get_uniform_loc(p+"].softness", _occluders[i]._softness_loc);
   }

   get_uniform_loc("is_reciever",_is_reciever_loc);
   get_uniform_loc("global_edge_len", _global_edge_len_loc);
   get_uniform_loc("P_matrix", _proj_der_loc);

   return true;
}

inline double
max_component(CWvec& v)
{
   return max(max(fabs(v[0]),fabs(v[1])),fabs(v[2]));
}

void 
ToneShader::populate_occluders()
{
   _occluders.clear();
   if (mesh()->reciever()) {
      // world to eye space:
      Wtransf V = VIEW::peek()->world_to_eye();  
             
      // populate the occluder array
      BMESH_list meshes = BMESH_list(DRAWN);
      for (int i=0; i<meshes.num() && _occluders.num() < MAX_OCCLUDERS; i++) {
         BMESHptr m = meshes[i];
         if (mesh() == m || ! m->occluder())
            continue;

         BBOX bb = m->get_bb();

         // map canonical sphere to ellipsoid in mesh object space.
         // take scale factors from bounding box dimensions:
         Wtransf S = Wtransf::scaling(m->shadow_scale()*bb.dim()/2);
         // translate ellipsoid center to bounding box center:
         Wtransf T1 = Wtransf::translation(bb.center());

         // object space to world:
         Wtransf M = m->xform();

         // optionally raise the shadow (in world space).
         // use shadow offset times bounding box "radius":
         double y_offset = m->shadow_offset()*bb.dim().length()/2;
         // account for object-to-world scaling:
         y_offset *= max_component(M.get_scale());
         Wtransf T2 = Wtransf::translation(Wvec::Y()*y_offset);

         OccluderData o;
         o._is_active = true;
         o._shadow_xf = (V*T2*M*T1*S).inverse(); // eye to canonical

         Wvec bbdim = bb.dim();
         o._softness =
            m->shadow_softness() * max(max(bbdim[0],bbdim[1]),bbdim[2]);
         _occluders += o;
      }
   }
   // fill out the rest of the array w/ disabled occluders:
   while (_occluders.num() < MAX_OCCLUDERS)
      _occluders += OccluderData();
}

bool
ToneShader::set_uniform_variables() const
{
   // send uniform variable values to the program

   assert(_tex);
   glUniform1i(_blend_normal_loc, _blend_normal);
   glUniform1f(_unit_len_loc, _unit_len);
   glUniform1f(_edge_len_scale_loc, _edge_len_scale);
   glUniform1f(_ratio_scale_loc, _ratio_scale);
   glUniform1f(_user_depth_loc, _user_depth);

   glUniform1i(_tex_loc, _tex->get_raw_unit());
   if(_tex_2d){
      glUniform1i(_is_tex_2d_loc, 1);
      glUniform1i(_tex_2d_loc, _tex_2d->get_raw_unit());
   }
   else{
      glUniform1i(_is_tex_2d_loc, 0);
      glUniform1i(_tex_2d_loc, 0);
   }

   // send layer info:
   for (int i=0; i<4; i++) {
      glUniform1i(_is_enabled_loc[i], _layer[i]._is_enabled);
      glUniform1i(_remap_nl_loc  [i], _layer[i]._remap_nl);
      glUniform1i(_remap_loc     [i], _layer[i]._remap);
      glUniform1i(_backlight_loc [i], _layer[i]._backlight);
      glUniform1f(_e0_loc        [i], _layer[i]._e0);
      glUniform1f(_e1_loc        [i], _layer[i]._e1);
      glUniform1f(_s0_loc        [i], _layer[i]._s0);
      glUniform1f(_s1_loc        [i], _layer[i]._s1);
   }

   // send shadow info:
   glUniform1i(_is_reciever_loc, mesh()->reciever());

   // send occluder data
   for (int i=0; i<_occluders.num(); i++) {
      glUniform1i(_occluders[i]._is_active_loc,
                  _occluders[i]._is_active);
      glUniformMatrix4fv(_occluders[i]._xf_loc,
                         1,
                         GL_TRUE,
                         float16(_occluders[i]._shadow_xf));
      glUniform1f(_occluders[i]._softness_loc,
                  _occluders[i]._softness);
   }

   glUniform1f(_global_edge_len_loc, _global_edge_len);

   Wtransf P_matrix =  VIEW::peek()->eye_to_pix_proj();

   // GL_TRUE means transpose the result
   glUniformMatrix4fv(_proj_der_loc,1,GL_TRUE,float16(P_matrix));

   return true;
}

void
ToneShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
   // let's not use the patch color, ok?
   // (at least until after the deadline)
   GL_COL(COLOR::white, alpha());    // GL_CURRENT_BIT

}

int
ToneShader::draw_final(CVIEWptr& v) 
{
   // visualize the bounding box, for debugging shadows
   if (Config::get_var_bool("TONE_SHADER_SHOW_BBOX",false)) {
      GL_VIEW::init_line_smooth(2.0, GL_CURRENT_BIT);
      glDisable(GL_LIGHTING);
      glColor4fv(float4(Color::white));
      Wpt_list p;
      mesh()->get_bb().points(p);
      glBegin(GL_LINES); {
         glVertex3dv(p[0].data());
         glVertex3dv(p[1].data());
         glVertex3dv(p[1].data());
         glVertex3dv(p[6].data());
         glVertex3dv(p[6].data());
         glVertex3dv(p[2].data());
         glVertex3dv(p[2].data());
         glVertex3dv(p[0].data());
         glVertex3dv(p[0].data());
         glVertex3dv(p[3].data());
         glVertex3dv(p[1].data());
         glVertex3dv(p[5].data());

         glVertex3dv(p[6].data());
         glVertex3dv(p[7].data());
         glVertex3dv(p[2].data());
         glVertex3dv(p[4].data());
         glVertex3dv(p[3].data());
         glVertex3dv(p[5].data());
         glVertex3dv(p[5].data());
         glVertex3dv(p[7].data());
         glVertex3dv(p[7].data());
         glVertex3dv(p[4].data());
         glVertex3dv(p[4].data());
         glVertex3dv(p[3].data());
      }
      glEnd();
      GL_VIEW::end_line_smooth();
   }
   return 0;
}

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
ToneShader::tags() const
{
   if (!_tags) {
      _tags = new TAGlist;
      *_tags += GLSLShader::tags();
      *_tags += new TAG_meth<ToneShader>(
         "layer",
         &ToneShader::put_layer,
         &ToneShader::get_layer,
         1);
      
   }
   return *_tags;
}

void         
ToneShader::get_layer (TAGformat &d)
{
   int i;
   *d >> i;
   
   str_ptr str;
   *d >> str;

   if ((str != tone_layer_t::static_name())) 
      return;
   
   _layer[i].decode(*d);
}

void
ToneShader::put_layer (TAGformat &d) const
{   
   for(int i=0; i < 4; ++i){
      d.id();
      *d << i;
      _layer[i].format(*d);
      d.end_id();
   }   
}

// end of file tone_shader.C
