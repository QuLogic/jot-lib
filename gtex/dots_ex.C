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
 * dots_EX.C
 *****************************************************************/
#include <map>

#include "mesh/mi.H"
#include "dots_ex.H"
#include "gtex/glsl_toon.H"
#include "gtex/ref_image.H"

#include "mesh/uv_data.H"
#include "mesh/lmesh.H"
#include "mesh/ledge_strip.H"

#include "mesh/vert_attrib.H"
#include "gl_sphir_tex_coord_gen.H"



static bool debug = Config::get_var_bool("DEBUG_DOTS", false);


//handy dandy storage class
class UV_grad 
{
 public :
   Wvec U_grad;
   Wvec V_grad;
};

//produces uv attribute for a vertex
class UV_attrib  :  public VertAttrib<UVpt,UVvec> {
 public:
        
   UV_attrib() {}

   virtual ~UV_attrib() {}

   virtual  UVpt get_attrib(CBvert* v, CBface* f) {
      assert(v&&f);

      TexCoordGen* tg = f->patch()->tex_coord_gen();
      if (tg)
         return tg->uv_from_vert(v,f);
      else 
         if (UVdata::lookup(f))
            return UVdata::get_uv(v,f);
         else
            return UVpt(0.0, 0.0); 
   }
 protected :
};

//using model's coordinates

class StripTexCoordsCB3 : public GLStripCB {
 public:
   StripTexCoordsCB3() { 
      att_function = new UV_attrib();
      dU_loc =0;
      dV_loc =0;
   }

   virtual ~StripTexCoordsCB3() {
      delete att_function;
   }

   //******** TRI STRIPS ********
   virtual void faceCB(CBvert* v, CBface* f) {
  
      assert(v && f);
      
      glNormal3dv(f->norm().data());

      if (v->has_color()) {
         GL_COL(v->color(), alpha*v->alpha());
      }

      //pass texture coordinates to opengl
      glTexCoord2dv(att_function->get_attrib(v,f).data());

      send_d(v,f);
   
      glVertex3dv(v->loc().data());     // vertex coordinates
   }

   //set uniform var location <- comes from the GLSL texture class
   void set_dU_loc(GLint loc) {
      dU_loc = loc;
   }

   void set_dV_loc(GLint loc) {
      dV_loc = loc;
   }


   //send the texture coord derivatives on its way
   void send_dU(Wvec dU)
      {
         if (dU_loc)
            glVertexAttrib3f(dU_loc, GLfloat(dU[0]),GLfloat(dU[1]),GLfloat(dU[2]));
      }

   void send_dV(Wvec dV)
      {
         if (dV_loc)
            glVertexAttrib3f(dV_loc, GLfloat(dV[0]),GLfloat(dV[1]),GLfloat(dV[2]));
      }

   bool get_valid_grad() { return valid_gradients; }
   void set_valid_grad() { valid_gradients = true; }
   void inv_valid_grad() { valid_gradients = false; }

   void compute_face_gradients(Patch* patch)
      {
         if(valid_gradients) return;

         //face_gradient_map.clear();

         Bface_list faces = patch->faces();

         UV_grad gradient;
         UVvec derivative;

         for(int i=0; i<faces.num(); i++)
            {
               derivative = att_function->dFdx(faces[i]);

               gradient.U_grad[0] = derivative[0]; 
               gradient.V_grad[0] = derivative[1]; 

               derivative = att_function->dFdy(faces[i]);

               gradient.U_grad[1] = derivative[0]; 
               gradient.V_grad[1] = derivative[1]; 

               derivative = att_function->dFdz(faces[i]);

               gradient.U_grad[2] = derivative[0]; 
               gradient.V_grad[2] = derivative[1]; 
         
        

               face_gradient_map[(unsigned int)(faces[i]->index())]= gradient; //uses the face pointer as a key

            }

         valid_gradients = true;
      }

//computes the texture coord derivative on per vertex basis and sends to GLSL program

   void send_d(CBvert* v, CBface* f)
      {
         assert(v && f);   
           
         Bface_list faces = v->get_faces();

         Wvec acc_U(0,0,0);
         Wvec acc_V(0,0,0);

         Wvec U_vec,V_vec;

         UVvec derivative;

         Wvec vertex_normal = v->norm();
         Wvec base_2 = cross(vertex_normal,Wvec(1.0,0.0,0.0)); //use perpend!!
         Wvec base_3 = cross(vertex_normal,base_2);


         base_2 = base_2.normalized();
         base_3 = base_3.normalized();


         for(int i=0; i<faces.num(); i++)
            {
               //getting the previously computed face gradients
               UV_grad grad = face_gradient_map[(unsigned int)(faces[i]->index())];
         
               U_vec = grad.U_grad;
               V_vec = grad.V_grad;

               //rotate to tangent plane

               double U_magnitude = U_vec.length();
               double V_magnitude = V_vec.length();

               U_vec =  Wvec((base_2 * (base_2 * U_vec)) + (base_3 * (base_3 * U_vec)));
               V_vec =  Wvec((base_2 * (base_2 * V_vec)) + (base_3 * (base_3 * V_vec)));

               U_vec = U_vec.normalized() * U_magnitude;
               V_vec = V_vec.normalized() * V_magnitude;

               acc_U += U_vec;
               acc_V += V_vec;

            }

         //just simple average for now
         acc_U = (acc_U / double(faces.num()) );
         acc_V = (acc_V / double(faces.num()) );

         //cerr << " dV = " << acc_V << endl;

         send_dU(acc_U);
         send_dV(acc_V);

      }

 protected :
   UV_attrib* att_function;
   GLint    dU_loc;
   GLint    dV_loc;

   map<unsigned int,UV_grad> face_gradient_map;
   bool valid_gradients;

};








inline GTexture*
get_toon_shader(Patch* p)
{
   GLSLToonShader* ret = new GLSLToonShader(p);
   ret->set_tex(Config::JOT_ROOT() + "nprdata/toon_textures/clear-black.png");
   return ret;
}

/*****************************************************************
 * DotsShader_EX:
 *
 *   Does dot-based halftoning using texture coordinates 
 *****************************************************************/
GLuint  DotsShader_EX::_program(0);
bool    DotsShader_EX::_did_init(false);
GLint   DotsShader_EX::_origin_loc(-1);
GLint   DotsShader_EX::_u_vec_loc(-1);
GLint   DotsShader_EX::_v_vec_loc(-1);
GLint   DotsShader_EX::_st_loc(-1);
GLint   DotsShader_EX::_style_loc(-1);
GLint   DotsShader_EX::_tone_tex_loc(-1);
GLint   DotsShader_EX::_width_loc(-1);
GLint   DotsShader_EX::_height_loc(-1);

DotsShader_EX::DotsShader_EX(Patch* p) :
   GLSLShader(p,new StripTexCoordsCB3()),
   _tone_shader(0),
   _style(0)
{
   set_tone_shader(get_toon_shader(p));
   ((StripTexCoordsCB3*)cb())->inv_valid_grad();
}

DotsShader_EX::~DotsShader_EX() 
{
   gtextures().delete_all(); 
}

void
DotsShader_EX::set_tone_shader(GTexture* g)
{
   if (g == _tone_shader)
      return;
   delete _tone_shader;
   _tone_shader = g;
   changed();
}

bool 
DotsShader_EX::get_variable_locs()
{
   get_uniform_loc("origin", _origin_loc);
   
  
   get_uniform_loc("style",  _style_loc);

   // tone map variables
   get_uniform_loc("tone_map", _tone_tex_loc);
   get_uniform_loc("width",  _width_loc);
   get_uniform_loc("height", _height_loc);

   //inverse projection matrix
   get_uniform_loc("P_inverse", _proj_der_loc);

   //query the vertex attribute loacations
   
   GLint tmp;
   tmp = glGetAttribLocation(program(),"dU_XYZ");
   ((StripTexCoordsCB3*)cb())->set_dU_loc(tmp);

   cerr << "dU_loc = " << tmp <<endl;

   tmp = glGetAttribLocation(program(),"dV_XYZ");
   ((StripTexCoordsCB3*)cb())->set_dV_loc(tmp);
   
   cerr << "dV_loc = " << tmp <<endl;

   return true;
}

bool
DotsShader_EX::set_uniform_variables() const
{
   assert(_patch);   

   if (!_patch->tex_coord_gen() ) {
      _patch->set_tex_coord_gen(new GLSphirTexCoordGen);      
   }

   //tone map variables
   glUniform1i(_tone_tex_loc, ColorRefImage::lookup_raw_tex_unit(0));
   glUniform1i(_width_loc,  VIEW::peek()->width());
   glUniform1i(_height_loc, VIEW::peek()->height());

   _patch->update_dynamic_samples(); // XXX - isn't this bogus?
   glUniform2fv(_origin_loc, 1, float2(_patch->sample_origin()));
   glUniform1i (_style_loc, _style);





   Wtransf P_matrix =  VIEW::peek()->wpt_to_pix_proj();
 
   //make sure that the matrix derivative is taken at the correct Z value 

  
   //cerr << "Projection Matrix : " << endl << P_matrix << endl;

   //GLSL stroes matrices in column major order
   //while jot's mlib stores matrices in row major order
   //therefore a transpose is needed

   glUniformMatrix4fv(_proj_der_loc,1,GL_TRUE /* transpose = true */,(const GLfloat*) float16(P_matrix.inverse())/*temp*/);




   return true;
}

GTexture_list 
DotsShader_EX::gtextures() const
{
   return GTexture_list(_tone_shader);
}

void
DotsShader_EX::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
   // set the color from the VIEW background color:
   GL_COL(VIEW::peek()->color(), alpha());    // GL_CURRENT_BIT
}

void 
DotsShader_EX::request_ref_imgs()
{
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
}

int 
DotsShader_EX::draw_color_ref(int i)
{
   if (i != 0)
      return 0;

   ((StripTexCoordsCB3*)cb())->compute_face_gradients(patch());
   
   // Draw the reference image for tone.
   return _tone_shader->draw(VIEW::peek());
}


void
DotsShader_EX::set_patch(Patch* p)
{
   //invalidates the gradient map for new patch 
   ((StripTexCoordsCB3*)cb())->inv_valid_grad();
   return GLSLShader::set_patch(p);
}
void 
DotsShader_EX::changed()
{
   ((StripTexCoordsCB3*)cb())->inv_valid_grad();
   GLSLShader::changed();
}

// end of file dots_EX.C
