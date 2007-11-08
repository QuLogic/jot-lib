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
 * msld.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "gtex/ref_image.H"
#include "npr/binary_image.H"
#include "npr/msld.H"

static bool debug = Config::get_var_bool("DEBUG_MSLD", false);

inline GTexture*
get_tone_shader(Patch* p)
{
   BinaryImageShader* ret = new BinaryImageShader(p);
   ret->set_tex(Config::JOT_ROOT() + "nprdata/toon_textures/warm_spec2_512_2d.png");

   return ret;
}

/**********************************************************************
 * MSLDStripCB:
 *
 **********************************************************************/

class MSLDStripCB : public GLStripCB {
 public:
   MSLDStripCB() {}

   void set_locs(GLint loc[5])        { 
     pdir1_attrib_loc = loc[0];
     pdir2_attrib_loc = loc[1];
     k1_attrib_loc = loc[2];
     k2_attrib_loc = loc[3];
     dcurv_tensor_attrib_loc = loc[4];
   }

   virtual void faceCB(CBvert* v, CBface*);

 private:

   GLint pdir1_attrib_loc, pdir2_attrib_loc;
   GLint k1_attrib_loc, k2_attrib_loc;
   GLint dcurv_tensor_attrib_loc;
};
 
void
MSLDStripCB::faceCB(CBvert* v, CBface* f)
{
   assert(v && f);

   // Send curvature data as vertex attributes:
   
   Wvec pdir1 = v->pdir1();
   Wvec pdir2 = v->pdir2();

   glVertexAttrib3fARB(pdir1_attrib_loc,
                       static_cast<GLfloat>(pdir1[0]),
                       static_cast<GLfloat>(pdir1[1]),
                       static_cast<GLfloat>(pdir1[2]));
   glVertexAttrib3fARB(pdir2_attrib_loc,
                       static_cast<GLfloat>(pdir2[0]),
                       static_cast<GLfloat>(pdir2[1]),
                       static_cast<GLfloat>(pdir2[2]));
                       
   glVertexAttrib1fARB(k1_attrib_loc, static_cast<GLfloat>(v->k1()));
   glVertexAttrib1fARB(k2_attrib_loc, static_cast<GLfloat>(v->k2()));
   
   double *dcurv = &(v->dcurv_tensor().dcurv[0]);
   
   glVertexAttrib4fARB(dcurv_tensor_attrib_loc,
                       static_cast<GLfloat>(dcurv[0]),
                       static_cast<GLfloat>(dcurv[1]),
                       static_cast<GLfloat>(dcurv[2]),
                       static_cast<GLfloat>(dcurv[3]));

   glNormal3dv(f->vert_normal(v).data());
   glVertex3dv(v->loc().data());

}


/**********************************************************************
 * MSLDShader:
 *
 *  ...
 **********************************************************************/
GLuint          MSLDShader::_program = 0;
bool            MSLDShader::_did_init = false;
//GLint   MSLDShader::_tone_tex_loc(-1);
//GLint   MSLDShader::_width_loc(-1);
//GLint   MSLDShader::_height_loc(-1);

MSLDShader* MSLDShader::_instance(0);

//MSLDShader::MSLDShader(Patch* p) : GLSLShader(p, new MSLDStripCB), 
MSLDShader::MSLDShader(Patch* p) : GLSLShader(p), _tone_shader(0)
{
  if(debug){
    cerr<<"MSLD Debug's working"<<endl;
  }

  set_tone_shader(get_tone_shader(p));
}

MSLDShader::~MSLDShader() 
{
   gtextures().delete_all(); 
}

void
MSLDShader::set_tone_shader(GTexture* g)
{
   if (g == _tone_shader)
      return;
   delete _tone_shader;
   _tone_shader = g;
   changed();
}

MSLDShader* 
MSLDShader::get_instance()
{
   if (!_instance) {
      _instance = new MSLDShader();
      assert(_instance);
   }
   return _instance;
}

bool 
MSLDShader::get_variable_locs()
{
  // get_uniform_loc("toon_tex", _tex_loc);

   // other variables here as needed...
/*   MSLDStripCB* cb = dynamic_cast<MSLDStripCB*>(_cb);
   GLint loc[5];

   loc[0] = glGetAttribLocationARB(_program, "pdir1");
   loc[1] = glGetAttribLocationARB(_program, "pdir2");
   loc[2] = glGetAttribLocationARB(_program, "k1");
   loc[3] = glGetAttribLocationARB(_program, "k2");
   loc[4] = glGetAttribLocationARB(_program, "dcurv_tensor");

   cb->set_locs(loc);
   get_uniform_loc("scthresh", scthresh_uniform_loc);
   get_uniform_loc("feature_size", feature_size_uniform_loc);
*/

   get_uniform_loc("tone_map", _tone_tex_loc);
   get_uniform_loc("x_1",  _width_loc);
   get_uniform_loc("y_1", _height_loc);

   return true;
}

bool
MSLDShader::set_uniform_variables() const
{
   // send uniform variable values to the program
   
   if(_patch){
      //tone map variables
      glUniform1i(_tone_tex_loc, ColorRefImage::lookup_raw_tex_unit(0));
      glUniform1fARB(_width_loc,  1.0/VIEW::peek()->width());
      glUniform1fARB(_height_loc, 1.0/VIEW::peek()->height());

/*      glUniform1fARB(feature_size_uniform_loc,
                  static_cast<GLfloat>(_patch->mesh()->curvature()->feature_size()));

      glUniform1fARB(scthresh_uniform_loc, 0.01);*/
      return true;
   }

   return false;
}

GTexture_list 
MSLDShader::gtextures() const
{
   return GTexture_list(_tone_shader);
}

void
MSLDShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
//   glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
   // set the color from the VIEW background color:
   GL_COL(VIEW::peek()->color(), alpha());    // GL_CURRENT_BIT

}

void 
MSLDShader::request_ref_imgs()
{
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
}

int 
MSLDShader::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ? _tone_shader->draw(VIEW::peek()) : 0;
}

// end of file msld.C
