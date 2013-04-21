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

#include "outline_stroke.H"
#include "gtex/glsl_paper.H"

static bool use_glsl_paper = Config::get_var_bool("ENABLE_GLSL_PAPER",true);

static int ols = DECODER_ADD(OutlineStroke);

TAGlist*            OutlineStroke::_os_tags = 0;

CTAGlist &
OutlineStroke::tags() const
{
   if (!_os_tags) {
      _os_tags = new TAGlist;
      *_os_tags += BaseStroke::tags();    

      *_os_tags += new TAG_val<OutlineStroke,double>(
         "orig_mesh_size",
         &OutlineStroke::original_mesh_size_);
   }
   return *_os_tags;
}


// Subclass this to change offset LOD, etc.
void   
OutlineStroke::apply_offset(BaseStrokeVertex* v, BaseStrokeOffset* o)
{
   assert(_offsets->get_pix_len() > 0);

   if(_press_vary_width)
      v->_width = (float)o->_press;
   else
      v->_width = 1.0f;

   if(_press_vary_alpha)
      v->_alpha = (float)o->_press;
   else
      v->_alpha = 1.0f;

   double scale = _scale * min(1.0f,_offset_stretch);

   v->_loc = v->_base_loc + v->_dir * (scale * o->_len);
}

/////////////////////////////////////
// draw_start()
/////////////////////////////////////
void
OutlineStroke::draw_start()
{

   GL_VIEW::print_gl_errors("OutlineStroke::draw_start: start");

   if(!use_glsl_paper)
      return BaseStroke::draw_start();
  
     // Push affected state:
   glPushAttrib(
      GL_CURRENT_BIT            |
      GL_ENABLE_BIT             |
      GL_COLOR_BUFFER_BIT       |
      GL_TEXTURE_BIT
      );

   glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

   // Set state for drawing strokes:
   glDisable(GL_LIGHTING);												// GL_ENABLE_BIT
   glDisable(GL_CULL_FACE);											// GL_ENABLE_BIT
   if (!_use_depth)  
      glDisable(GL_DEPTH_TEST);										// GL_ENABLE_BIT
   glEnable(GL_BLEND);													// GL_ENABLE_BIT
	//if (PaperEffect::is_alpha_premult())
	//	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);				// GL_COLOR_BUFFER_BIT
	//else
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		// GL_COLOR_BUFFER_BIT

   // Enable or disable texturing:
   if (_tex ) // && (!_debug))
   {
      glEnable(GL_TEXTURE_2D);  // GL_ENABLE_BIT
      _tex->apply_texture();    // GL_TEXTURE_BIT
   }
   else
   {
      glDisable(GL_TEXTURE_2D); // GL_ENABLE_BIT
   }

   //if (!_debug)
   //{
      glEnableClientState(GL_VERTEX_ARRAY);                    //GL_CLIENT_VERTEX_ARRAY_BIT)
      glEnableClientState(GL_COLOR_ARRAY);                     //GL_CLIENT_VERTEX_ARRAY_BIT)
      if (_tex) glEnableClientState(GL_TEXTURE_COORD_ARRAY);   //GL_CLIENT_VERTEX_ARRAY_BIT)

      //if (GLExtensions::gl_arb_multitexture_supported())
      //{
      //   glClientActiveTextureARB(GL_TEXTURE1_ARB); 
      //   glEnableClientState(GL_TEXTURE_COORD_ARRAY);          //GL_CLIENT_VERTEX_ARRAY_BIT)
      //   glClientActiveTextureARB(GL_TEXTURE0_ARB); 
      //}
  // }
   GL_VIEW::print_gl_errors("OutlineStroke::draw_start: glEnableClientState");


   // Set projection and modelview matrices for drawing in NDC:
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixd(VIEW::peek()->ndc_proj().transpose().matrix());

   GL_VIEW::print_gl_errors("OutlineStroke::draw_start: GL_PROJECTION");

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   GL_VIEW::print_gl_errors("OutlineStroke::draw_start: GL_MODELVIEW");

   // Cache view related info:
   if (_stamp != VIEW::stamp()) 
   {
      _stamp = VIEW::stamp();
      int w, h;
      VIEW_SIZE(w, h);
      _scale = (float)VIEW::pix_to_ndc_scale();
      
      _max_x = w*_scale/2;
      _max_y = h*_scale/2;
      
      _cam = VIEW::peek_cam()->data()->from();
      _cam_at_v = VIEW::peek_cam()->data()->at_v();
      
      _strokes_drawn = 0;
   }

  GLSLPaperShader::set_do_texture(true);
  GLSLPaperShader::begin_glsl_paper(_patch);  
  GLSLPaperShader::set_do_texture(false);
}


/////////////////////////////////////
// draw_end()
/////////////////////////////////////
void
OutlineStroke::draw_end()
{
  //if(!use_glsl_paper)
  //    return BaseStroke::draw_end();

   GLSLPaperShader::end_glsl_paper();
   
  
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();

   glPopClientAttrib();

   glPopAttrib();
}
