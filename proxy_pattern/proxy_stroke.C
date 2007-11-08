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
/***************************************************************************
    proxy_stroke.H
    
    ProxyStroke
        -Stroke used to render Proxy3dStroke(proxy_group.c) in 2d
        -Derives from BaseStroke
        -Has check_vert_visibility Basestroke method to use visibility 
         check: if there is a face on the ref image(thus visible) that
       intersects with the location of the vertex, then draw it.
    -------------------
    Simon Breslav
    Fall 2004
***************************************************************************/

#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
#pragma warning(disable: 4786)
#endif

#include "gtex/ref_image.H" 
#include "mesh/uv_data.H" 

#include "proxy_stroke.H"
#include "proxy_surface.H"
#include "hatching_texture.H"
#include "gtex/glsl_paper.H"

static int foo = DECODER_ADD(ProxyStroke);

bool ProxyStroke::taggle_vis = false;
ProxyStroke::ProxyStroke() 
{   
   _vis_type = VIS_TYPE_SCREEN | VIS_TYPE_SUBCLASS;
   //_verts.set_proto(new ProxyVertexData);
   //_draw_verts.set_proto(new ProxyVertexData);
   //_refine_verts.set_proto(new ProxyVertexData);
}

int 
ProxyStroke::draw(CVIEWptr& v)
{
   //cerr << "ProxyStroke::draw " <<  _ps->center() << endl;       
   return BaseStroke::draw(v);
}
/////////////////////////////////////
// draw_start()
/////////////////////////////////////
void
ProxyStroke::draw_start()
{
   // Push affected state:
   glPushAttrib(
      GL_CURRENT_BIT            |
      GL_ENABLE_BIT             |
      GL_COLOR_BUFFER_BIT       |
      GL_TEXTURE_BIT
      );

   glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

   // Set state for drawing strokes:
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT
   glDisable(GL_CULL_FACE);     // GL_ENABLE_BIT
   if (!_use_depth)  
      glDisable(GL_DEPTH_TEST); // GL_ENABLE_BIT
   glEnable(GL_BLEND);          // GL_ENABLE_BIT
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_COLOR_BUFFER_BIT

   // Enable or disable texturing:
   if (_tex) {
      glEnable(GL_TEXTURE_2D);  // GL_ENABLE_BIT
      _tex->apply_texture();    // GL_TEXTURE_BIT
   } else {
      glDisable(GL_TEXTURE_2D); // GL_ENABLE_BIT
   }

   glEnableClientState(GL_VERTEX_ARRAY); //GL_CLIENT_VERTEX_ARRAY_BIT)
   glEnableClientState(GL_COLOR_ARRAY);  //GL_CLIENT_VERTEX_ARRAY_BIT)
   if (_tex)
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);//GL_CLIENT_VERTEX_ARRAY_BIT)

   // Set projection and modelview matrices for drawing in NDC:
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixd(VIEW::peek()->ndc_proj().transpose().matrix());

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   // Cache view related info:
   if (_stamp != VIEW::stamp()) {
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
   
   GLSLPaperShader::begin_glsl_paper(_p);  
}

/////////////////////////////////////
// draw_end()
/////////////////////////////////////
void
ProxyStroke::draw_end()
{
   GLSLPaperShader::end_glsl_paper();

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   glPopClientAttrib();

   glPopAttrib();
}

BaseStroke* 
ProxyStroke::copy() const
{
   ProxyStroke *s = new ProxyStroke;
   assert(s);
   s->copy(*this);
   return s;
}
void                    
ProxyStroke::copy(CProxyStroke& s)
{
   BaseStroke::copy(s);
}


bool  
ProxyStroke::check_vert_visibility(CBaseStrokeVertex &v)
{    
   if(taggle_vis)     
      return true;

   static IDRefImage* id_ref       = 0;
   static uint        id_ref_stamp = UINT_MAX;
   // cache id ref image for the current frame
    
   if (id_ref_stamp != VIEW::stamp()) {
      IDRefImage::set_instance(VIEW::peek());
      id_ref = IDRefImage::instance();     

      id_ref_stamp = VIEW::stamp();
   }
   CBface* f = id_ref->intersect(v._base_loc);
   if(!f) return false;
   
   return true;
}

// ***************************************
// ProxyUVStroke 
// ***************************************
ProxyUVStroke::ProxyUVStroke(
   CUVpt_list& pts,
   ProxySurface* ps,
   BaseStroke* proto) 
{   
   _uv_pts = pts;
   _stroke = new ProxyStroke();
   //_stroke->set_color(COLOR::black);
   //_stroke->set_width(10.0);
   //_stroke->set_alpha(0.5);
   //_stroke->set_contrast(.55);
   //_stroke->set_brightness(.5);
   //_stroke->set_offset_stretch(32);
   //_stroke->set_angle(32);
   //_stroke->set_texture("nprdata/stroke_textures/one_d.png");  //bestest_dotline one_d
   //_stroke->set_paper("nprdata/paper_textures/basic_paper.png");
   _stroke->copy(*proto);
   _ps = ps;
   _proto = proto;
}


void 
ProxyUVStroke::draw_start()
{
   // Set state for drawing simple antialiased line strips:
   /*
     const GLfloat width = 2.0f;          // line width
     const COLOR   color = Color::black;  // line color
     const double  alpha = 0.8;           // line alpha
     GL_VIEW::init_line_smooth(width, GL_CURRENT_BIT);
     glDisable(GL_LIGHTING);      // GL_ENABLE_BIT
     glDisable(GL_DEPTH_TEST);    // GL_ENABLE_BIT
     glDisable(GL_TEXTURE_2D);    // GL_ENABLE_BIT
     GL_COL(color,alpha);         // GL_CURRENT_BIT
   */
   assert(_ps);
   if(_stroke){
      _stroke->set_patch(_ps->patch());
      _stroke->draw_start();
   }
}

void 
ProxyUVStroke::draw_end()
{
   if(_stroke)
      _stroke->draw_end();
   //GL_VIEW::end_line_smooth();
}

void 
ProxyUVStroke::draw(CVIEWptr& v)
{
   if(_stroke && _stroke->num_verts() > 2)
      _stroke->draw(v);

   //if (!_ps || !_face)
   //   return;

   //stroke_setup(_face);
   
   //glBegin(GL_LINE_STRIP);   
   //for(int i=0; i < _pts.num(); ++i) {
   //   glVertex2dv((NDCZpt(_pts[i])).data());
   //}
   //glEnd();
}

void 
ProxyUVStroke::stroke_setup(Bface* face, bool selected)
{       
   ProxyStroke::taggle_vis = (selected) ? true : false;
   
   // Lets figure out the smallest uv point of the face
   UVpt base_uv;
   _stroke->clear();

   if(_ps->baseUVpt(face, base_uv)) {
  
      for(int i=0; i < _uv_pts.num(); ++i) {
         Wpt pt = _ps->getWptfromUV(base_uv + _uv_pts[i]);
         CNDCZpt p = NDCZpt(pt);

     
         static ColorRefImage* col_ref = 0;
         static uint     col_ref_stamp = UINT_MAX;
         // cache col ref image for the current frame
         if (col_ref_stamp != VIEW::stamp()) {
            col_ref = ColorRefImage::lookup(0, VIEW::peek());
            col_ref_stamp = VIEW::stamp();
         }
         COLOR col = col_ref->color(p);
//     double x = clamp(1.0 - col.luminance(), 0.0,1.0);
      

         double w = (selected) ? 1.0 : 1.0;
         double a = (selected) ? 1.0 : 1.0;

         _stroke->add(p, w, a, true);
      }
   } else {
      cerr << "ProxyUVStroke::stroke_setup baseUVpt not found" << endl;
   }
}

// proxy_stroke.C
