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
/**************************************************************************
 * haltone_shader_ex.C
 *************************************************************************/
#include <map>

#include "halftone_shader_ex.H"
#include "gtex/gl_extensions.H"
#include "gtex/util.H"
#include "gtex/glsl_toon.H"             // Temp for testing ref images.
#include "gtex/ref_image.H"
#include "mesh/patch.H"

#include "mesh/uv_data.H"
#include "mesh/lmesh.H"
#include "mesh/ledge_strip.H"
#include "mesh/vert_attrib.H"

#include "gl_sphir_tex_coord_gen.H"



class UV_grad 
{
public :
   Wvec U_grad;
   Wvec V_grad;
};

class UV_attrib  :  public VertAttrib<UVpt,UVvec>
{
public:
	
	UV_attrib()
	{

	}

	virtual ~UV_attrib()
	{
		
	}

	virtual  UVpt get_attrib(CBvert* v, CBface* f)
	{
	  assert(v&&f);

	  TexCoordGen* tg = f->patch()->tex_coord_gen();
      if (tg)
         return tg->uv_from_vert(v,f);
      else 
		 if (UVdata::lookup(f))
           return UVdata::get_uv(v,f);
		 else
			return UVpt(0.0,0.0); 
	}

protected :

};



//using model's texture coordinates as primary source
//or generate automatic spherical mapping 

class StripTexCoordsCB2 : public GLStripCB {
 public:
	 StripTexCoordsCB2() 
	 { 
		att_function = new UV_attrib();
		dU_loc =0;
		dV_loc =0;

	 }

	 virtual ~StripTexCoordsCB2()
	 {
		 delete att_function;
	 }

   //******** TRI STRIPS ********
   virtual void faceCB(CBvert* v, CBface* f) {
  
      assert(v && f);
      
	  glNormal3dv(f->norm().data());

	  if (v->has_color()) 
	  {
			GL_COL(v->color(), alpha*v->alpha());
	  }
	   

	 //pass texture coordinates to opengl
	  glTexCoord2dv(att_function->get_attrib(v,f).data());

	  send_d(v,f);
	 
   
      glVertex3dv(v->loc().data());     // vertex coordinates
   }

 


//set uniform var location <- comes from the GLSL texture class
   void set_dU_loc(GLint loc)
   {
	   dU_loc = loc;
   }

   void set_dV_loc(GLint loc)
   {
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


//compute face gradients and store them inside the map
   void compute_face_gradients(Patch* patch)
   {
      if(valid_gradients) return;

      face_gradient_map.clear();

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
         
         face_gradient_map[int(faces[i])]= gradient; //uses the face pointer as a key

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

         UV_grad grad = face_gradient_map[int(faces[i])];
         
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
	GLint	 dU_loc;
	GLint	 dV_loc;

   map<int,UV_grad> face_gradient_map;
   bool valid_gradients;

};



/**********************************************************************
 * HalftoneShaderEx:
 *
 *   Doesn't do anything yet.
 **********************************************************************/
HalftoneShaderEx::HalftoneShaderEx(Patch* patch) :
   GLSLShader(patch, new StripTexCoordsCB2() ),
   m_toon_tone_shader(new GLSLToonShader(patch)),
   _style_loc(-1),
   _style(0)
{
   // Specify the texture we'd like to use.
   str_ptr tex_name = "nprdata/toon_textures/clear-black.png";
   m_toon_tone_shader->set_tex(Config::JOT_ROOT() + tex_name);

   _perlin=0;
   _perlin_generator= Perlin::get_instance();
   if (!_perlin_generator)
      _perlin_generator= new Perlin();

   
   ((StripTexCoordsCB2*)cb())->inv_valid_grad();

}



bool 
HalftoneShaderEx::get_variable_locs()
{

   //get_uniform_loc("lod", _lod_loc);
   
   //make sure it has at least an automatic generator
   if (!_patch->tex_coord_gen() )
   {
       _patch->set_tex_coord_gen(new GLSphirTexCoordGen);      
   }


   get_uniform_loc("tone_map", m_tex_loc);
   get_uniform_loc("width", m_width);
   get_uniform_loc("height", m_height);
   get_uniform_loc("sampler2D_perlin", _perlin_loc);
   get_uniform_loc("style", _style_loc);
   
   get_uniform_loc("P_inverse", _proj_der_loc);

   GLint tmp;

   //query the vertex attribute loacations

   tmp = glGetAttribLocation(program(),"dU_XYZ");
   ((StripTexCoordsCB2*)cb())->set_dU_loc(tmp);

   cerr << "dU_loc = " << tmp <<endl;

   tmp = glGetAttribLocation(program(),"dV_XYZ");
   ((StripTexCoordsCB2*)cb())->set_dV_loc(tmp);
   
   cerr << "dV_loc = " << tmp <<endl;

   return true;
}

bool
HalftoneShaderEx::set_uniform_variables() const
{
   assert(_patch);
   _patch->update_dynamic_samples();
    
   // glUniform1f (_lod_loc, float(0));
   

   glUniform1i (m_tex_loc, m_texture->get_tex_unit() - GL_TEXTURE0);
   glUniform1i (m_width, VIEW::peek()->width());
   glUniform1i (m_height, VIEW::peek()->height());
   if(_perlin)
      glUniform1i(_perlin_loc, _perlin->get_tex_unit() - GL_TEXTURE0);
   
   glUniform1i(_style_loc, _style);


   //send the derivative of the transformation matrix
   //under construction ... seems to be working now


   
   Wtransf P_matrix =  VIEW::peek()->wpt_to_pix_proj();

   //make sure that the matrix derivative is taken at the correct Z value 

  
   //cerr << "Projection Matrix : " << endl << P_matrix << endl;

   //GLSL stroes matrices in column major order
   //while jot's mlib stores matrices in row major order
   //therefore a transpose is needed

   glUniformMatrix4fv(_proj_der_loc,1,GL_TRUE /* transpose = true */,(const GLfloat*) float16(P_matrix.inverse())/*temp*/);


   return true;
}

void 
HalftoneShaderEx::init_textures()
{
   // This may seem a little odd, but since the texture is in texture
   // memory, we need to look it up from the reference image.
   // XXX - still seems bogus! :)
   m_texture = ColorRefImage::lookup_texture(0);
   _perlin = _perlin_generator->create_perlin_texture2();
}

void
HalftoneShaderEx::activate_textures()
{
   activate_texture(m_texture);      // GL_ENABLE_BIT
   if(_perlin)
      activate_texture(_perlin);

}

GTexture_list HalftoneShaderEx::gtextures() const
{
   return GTexture_list(m_toon_tone_shader);
}

void
HalftoneShaderEx::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
   // set the color from the VIEW background color:
   GL_COL(VIEW::peek()->color(), alpha());    // GL_CURRENT_BIT
}

// ******** GTexture VIRTUAL METHODS ********

void 
HalftoneShaderEx::request_ref_imgs()
{
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
}

int 
HalftoneShaderEx::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ? m_toon_tone_shader->draw(VIEW::peek()) : 0;
}

int 
HalftoneShaderEx::draw(CVIEWptr &v)
{   
   //setup the UV gradients
   ((StripTexCoordsCB2*)cb())->compute_face_gradients(patch());
   //use the base draw
   return GLSLShader::draw(v);
}

void
HalftoneShaderEx::set_patch(Patch* p)
{
   //invalidates the gradient map for new patch 
   ((StripTexCoordsCB2*)cb())->inv_valid_grad();
   return GLSLShader::set_patch(p);
}
void 
HalftoneShaderEx::changed()
{
   ((StripTexCoordsCB2*)cb())->inv_valid_grad();
   GLSLShader::changed();
}


// End of file halftone_shader_EX.C
