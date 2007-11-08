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
 * HaloBlurShader.C
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "halo_blur_shader.H"
#include "gtex/ref_image.H"

static bool debug = Config::get_var_bool("DEBUG_BLUR_SHADER", false);

/**********************************************************************
 * HaloBlurShader:
 *
 *  ...
 **********************************************************************/
GLuint          HaloBlurShader::_program = 0;
bool            HaloBlurShader::_did_init = false;

HaloBlurShader* HaloBlurShader::_instance(0);

HaloBlurShader::HaloBlurShader(Patch* p) : GLSLShader(p)
{
  if(debug){
    cerr<<"Halo Blur Debug's working"<<endl;
  }

  _input_tex = 0;
  _kernel_size = 20; 

}

HaloBlurShader::~HaloBlurShader() 
{
   gtextures().delete_all(); 
}



HaloBlurShader* 
HaloBlurShader::get_instance()
{
   if (!_instance) {
      _instance = new HaloBlurShader();
      assert(_instance);
   }
   return _instance;
}

bool 
HaloBlurShader::get_variable_locs()
{
   // other variables here as needed...

   get_uniform_loc("input_map", _input_tex_loc);
   get_uniform_loc("x_size", _width_loc);
   get_uniform_loc("y_size", _height_loc);
   get_uniform_loc("direction", _direction_loc);
   get_uniform_loc("kernel_size", _kernel_size_loc);

   return true;
}

bool
HaloBlurShader::set_uniform_variables() const
{
   // send uniform variable values to the program
   

   if(_input_tex){
      //tone map variables
      glUniform1i(_input_tex_loc, _input_tex->get_raw_unit());
      glUniform1f(_width_loc,  VIEW::peek()->width());
      glUniform1f(_height_loc, VIEW::peek()->height());
      glUniform1i(_direction_loc, 0);
      glUniform1i(_kernel_size_loc,_kernel_size);

      return true;
   }

   return false;
}



void
HaloBlurShader::set_gl_state(GLbitfield mask) const
{
   GLSLShader::set_gl_state(mask);
   // set the color from the VIEW background color:
   GL_COL(VIEW::peek()->color(), alpha());    // GL_CURRENT_BIT

}

int
HaloBlurShader::draw(CVIEWptr& v)
{
   if (!_input_tex) return 0;
   
   GL_VIEW::print_gl_errors(class_name() + "::draw: start");

   // Ensure program is loaded:
   if (!init())
      return 0;
   assert(program());

   // Load textures (if any) and set their parameters.
   // This is a no-op after the first time.
   //
   // Q: Why not do it in the constructor?
   // A: Some textures get created but not drawn, so we
   //    prefer to wait until we're sure we need the textures
   //    before requesting resources from OpenGL.
   init_textures();

   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: init textures");

   // call glPushAttrib() and set desired state 
   set_gl_state(GL_TRANSFORM_BIT);
   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: push attrib");

   //SPECIAL SETUP FOR FILTER TEXTURE


   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT

   // prevents depth testing AND writing:
   glDisable(GL_DEPTH_TEST);    // GL_ENABLE_BIT

   // so it doesn't bleed stuff from the front buffer
   // through the low alpha areas:
   glDisable(GL_BLEND);         // GL_ENABLE_BIT

   // load identity for model matrix
   glMatrixMode(GL_MODELVIEW);  // GL_TRANSFORM_BIT
   glPushMatrix();
   glLoadIdentity();

   // set up to draw in XY coords:
   glMatrixMode(GL_PROJECTION); // GL_TRANSFORM_BIT
   glPushMatrix();
   glLoadMatrixd(VIEW::peek()->xypt_proj().transpose().matrix());

   _input_tex->apply_texture();

   // activate textures, if any:
   activate_textures(); // GL_ENABLE_BIT
   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: activate textures");

   // activate program:
   activate_program();
   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: activate program");

   // query variable locations and store the results:
   get_variable_locs();
  
   
   // send values to uniform variables:
   set_uniform_variables();
   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: set uniform variables");

          //SEPARABLE FILTER 2-PASS 
   
          const GLfloat t = 1;
          GLenum unit = _input_tex->get_tex_unit();
          
          //DRAW A QUAD COVERING THE ENTIRE SCREEN ****HORIZONTAL PASS

          glColor4f(1, 1, 1, 1);       // GL_CURRENT_BIT
          glBegin(GL_QUADS);
        // draw vertices in CCW order starting at bottom left:
         glMultiTexCoord2f(unit, 0, 0); glVertex2f(-t, -t);
         glMultiTexCoord2f(unit, 1, 0); glVertex2f( t, -t);
         glMultiTexCoord2f(unit, 1, 1); glVertex2f( t,  t);
         glMultiTexCoord2f(unit, 0, 1); glVertex2f(-t,  t);
         
         glEnd();

         //read back for the second pass
         
         glReadBuffer(GL_BACK);

         // Copies the frame buffer into a texture in gpu texture memory.
         glCopyTexImage2D(
            GL_TEXTURE_2D,  // The target to which the image data will be changed.
             0,              // Level of detail, 0 is base detail
             GL_RGBA,        // internal format
             0,              // x-coord of lower left corner of the window
             0,              // y-coord of lower left corner of the window
             _input_tex->image().width(),         // texture width
             _input_tex->image().height(),        // texture height
             0);  

         //change directions
         glUniform1i(_direction_loc, 1);

         //DRAW A QUAD COVERING THE ENTIRE SCREEN ***VERTICAL PASS
          glColor4f(1, 1, 1, 1);       // GL_CURRENT_BIT
          glBegin(GL_QUADS);
        // draw vertices in CCW order starting at bottom left:
         glMultiTexCoord2f(unit, 0, 0); glVertex2f(-t, -t);
         glMultiTexCoord2f(unit, 1, 0); glVertex2f( t, -t);
         glMultiTexCoord2f(unit, 1, 1); glVertex2f( t,  t);
         glMultiTexCoord2f(unit, 0, 1); glVertex2f(-t,  t);
         
         glEnd();
 

   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: draw triangles");

   //RESTORE PROPER PROJECTION

  // restore projection matrix
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();

   // restore modelview matrix
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();


   // restore gl state:
   restore_gl_state();

   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: pop attrib");

   deactivate_program();

   GL_VIEW::print_gl_errors(class_name() + "::draw: end");

   return 0;
}


// end of file blur_shader.C
