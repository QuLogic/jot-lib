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
 * glsl_xtoon.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "gtex/util.H"
#include "geom/gl_util.H"
#include "mesh/patch.H"
#include "glsl_xtoon.H"

static bool debug = Config::get_var_bool("DEBUG_GLSL_XTOON", false);

/**********************************************************************
 * XtoonStripCB:
 *
 *   Does X-Toon shader in GLSL.  Primarily created to pass in
 *   abstract normals into the gpu per vertex to blend with the 
 *   meshes normals.
 **********************************************************************/

class XToonStripCB : public GLStripCB {
 public:
   XToonStripCB() : _blend_type(1) {}

   void set_loc(GLint L)        { _loc = L;        }
   void set_blendType(int d)    { _blend_type = d; }

   virtual void faceCB(CBvert* v, CBface*);

   // types of normal abstraction:
   enum normal_t { SMOOTH=0, SPHERIC, ELLIPTIC, CYLINDRIC };

 private:

   GLint        _loc;
   int          _blend_type; //Tells which Shape normals should blend to
};
 
void
XToonStripCB::faceCB(CBvert* v, CBface* f)
{
   assert(v && f);
   Wvec bNorm; //Blended Normal

   //first calculate the abstract(blended) normal
   switch(_blend_type) {
    case XToonStripCB::SMOOTH: {
       // Note: doesn't work
       bNorm = v->get_all_faces().n_ring_faces(3).avg_normal();
    }
      break;
    case XToonStripCB::SPHERIC: {
       BMESH* mesh = v->mesh();
       Wpt c = mesh->get_bb().center();
       bNorm = (v->loc()-c).normalized();
    }
      break;
    case XToonStripCB::ELLIPTIC: {
       BMESH* mesh = v->mesh();
       Wvec c_to_v = v->loc() - mesh->get_bb().center();
       Wvec dim = mesh->get_bb().dim();
       double a = dim[0]*0.5;
       double b = dim[1]*0.5;
       double c = dim[2]*0.5;
       bNorm = Wvec(c_to_v[0]/a, c_to_v[1]/b, c_to_v[2]/c).normalized();
    }
      break;
    case XToonStripCB::CYLINDRIC: {
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
   glVertexAttrib3f(_loc, bNorm[0], bNorm[1], bNorm[2]); 
   glNormal3dv(f->vert_normal(v).data());
   glVertex3dv(v->loc().data());
}

/**********************************************************************
 * GLSLXToonShader:
 *
 *   Does X-Toon shader in GLSL.
 **********************************************************************/
GLSLXToonShader::GLSLXToonShader(Patch* p) :
   GLSLShader(p, new XToonStripCB),
   _layer_name("Shader"),
   _use_paper(0),
   _travel_paper(0),
   _transparent(1),
   _annotate(1),
   _color(COLOR::white),
   _alpha(1.0),
   _light_index(-1),
   _light_dir(1),
   _light_cam(1),
   _light_coords(mlib::Wvec(0,0,1)),
   _detail_map(1),
   _target_length(0.03),
   _max_factor(6.968),
   _normals_smoothed(false),
   _normals_elliptic(false),
   _normals_spheric(false),
   _normals_cylindric(false),
   _smooth_factor(0.5),
   _update_curvatures(false),
   _frame_rate(0.0),
   _nb_stat_frames(0),
   _invert_detail(false),       
   _update_uniforms(true),      
   _tex_is_good(false),
   _smoothNormal(1),           
   _smoothDetail(1)             
{
   _tex_name = "nprdata/toon_textures/contrast2_512_2d.png";
   set_tex(_tex_name);
}

void
GLSLXToonShader::set_normals(int i)
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

   XToonStripCB* cb = dynamic_cast<XToonStripCB*>(_cb);
   assert(cb);
   cb->set_blendType(_smoothNormal);

   Patch* p = dynamic_cast<Patch*>(_patch);
   p->changed();
}

void 
GLSLXToonShader::set_tex(Cstr_ptr& filename)
{
   // Set the name of the texture to use
   if (_tex) {
      _tex->set_texture(Config::JOT_ROOT() + filename);
   } else {
      _tex = new TEXTUREgl(Config::JOT_ROOT()+ filename);
      assert(_tex);
      _tex->set_wrap_s(GL_CLAMP_TO_EDGE);
      _tex->set_wrap_t(GL_CLAMP_TO_EDGE);
   }
}

void
GLSLXToonShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask | GL_COLOR_BUFFER_BIT);
   glEnable(GL_BLEND);                                // GL_ENABLE_BIT
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_COLOR_BUFFER_BIT
}

void
GLSLXToonShader::init_textures()
{
   // no-op after the first time:
   if (!load_texture(_tex))
      return;
}

void
GLSLXToonShader::activate_textures()
{
   activate_texture(_tex);      // GL_ENABLE_BIT
}

bool 
GLSLXToonShader::get_variable_locs()
{
   //set values to pass into the vertex shader
   XToonStripCB* cb = dynamic_cast<XToonStripCB*>(_cb);
   GLint normLoc = glGetAttribLocation(_program,"blendNorm");
   cb->set_loc(normLoc);
   cb->set_blendType(_smoothNormal);

   get_uniform_loc("toonTex", _tex_loc);
   get_uniform_loc("detailMap",_Dmap_loc);
   get_uniform_loc("smoothDetail",_Sdtl_loc);
   get_uniform_loc("smoothFactor",_Sfct_loc);
   get_uniform_loc("targetLength", _trgt_loc);
   get_uniform_loc("maxFactor", _Mfct_loc);
   get_uniform_loc("focusPoint", _Fpnt_loc);
   get_uniform_loc("lightIndex",_Lidx_loc);
   get_uniform_loc("lightDir",_Ldir_loc);
   get_uniform_loc("lightCam",_Lcam_loc);
   get_uniform_loc("lightCoords", _Lcoord_loc);

   return true;
}

bool
GLSLXToonShader::set_uniform_variables() const
{
   // send uniform variable values to the program

   glUniform1i(_tex_loc, _tex->get_tex_unit() - GL_TEXTURE0);

   ///////////////////////////
   //Misc Values 
   //detail Map(aka which type of detail value to use)
   glUniform1f(_Dmap_loc,_detail_map);
   //blending detail map
   glUniform1f(_Sdtl_loc,_smoothDetail);
   glUniform1f(_Sfct_loc,_smooth_factor);
   //pass in min and max distance detail values
   glUniform1f(_trgt_loc,_target_length);
   glUniform1f(_Mfct_loc,_max_factor);

   //pass in "focus" point
   Wpt focus = VIEW::peek_cam()->data()->center();
   glUniform4fv(_Fpnt_loc, 1, float4(focus));

   ////////////
   //Lights
   //pass in which light to use
   glUniform1f(_Lidx_loc,_light_index);
   //pass in bool if light is directional or positional
   glUniform1f(_Ldir_loc,_light_dir);
   //pass in the cam frame? (not sure what this is)
   glUniform1f(_Lcam_loc,_light_cam);
   //pass in the light coordinates
   glUniform3fv(_Lcoord_loc, 1, float3(_light_coords));
   return true;
}

// end of file glsl_xtoon.C
