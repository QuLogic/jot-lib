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
/**********************************************************************
 * npr_view.H:
 **********************************************************************/
#include "disp/colors.H"
#include "wnpr/view_ui.H"
#include "geom/world.H"
#include "gtex/ref_image.H"
#include "gtex/control_line.H"
#include "gtex/aux_ref_image.H"
#include "gtex/buffer_ref_image.H"
#include "gtex/glsl_shader.H"
#include "gtex/glsl_xtoon.H"
#include "gtex/glsl_toon.H"
#include "gtex/tone_shader.H"
#include "gtex/glsl_halo.H"
#include "gtex/glsl_marble.H"
#include "gtex/halftone_shader.H"
#include "gtex/halftone_shader_ex.H"
#include "gtex/hatching_tx.H"
#include "gtex/painterly.H"
#include "gtex/haftone_tx.H"
#include "gtex/dots_ex.H"
#include "gtex/dots.H"
#include "gtex/glsl_hatching.H"
#include "gtex/zxsil_frame.H"
#include "gtex/paper_effect.H"
#include "gtex/glsl_normal.H"
#include "gtex/blur_shader.H"

#include "mesh/lpatch.H"
#include "pattern/pattern_texture.H"
#include "proxy_pattern/proxy_texture.H"
#include "proxy_pattern/hatching_texture.H"
#include "std/config.H"

#include "skybox_texture.H"
#include "zxedge_stroke_texture.H"
#include "zxkey_line.H"
#include "npr_texture.H"
#include "ffstexture.H"
#include "feature_stroke_texture.H"
#include "npr_view.H"

#include "binary_image.H"
#include "img_line.H"
#include "img_line_shader.H"
#include "simple_img_line_shader.H"

using namespace mlib;

#ifdef WIN32
#include <process.h> // for getpid
#endif

static class DECODERSnpr {
 public:
   DECODERSnpr() {
      DECODER_ADD(ZXedgeStrokeTexture);
      DECODER_ADD(ZkeyLineTexture);
      DECODER_ADD(NPRTexture);
      DECODER_ADD(FeatureStrokeTexture);
      DECODER_ADD(FFSTexture);
      DECODER_ADD(PatternTexture);  
      DECODER_ADD(ProxyTexture); 
      DECODER_ADD(HatchingTexture);
      DECODER_ADD(GLSLMarbleShader);
      DECODER_ADD(Skybox_Texture);
      DECODER_ADD(ImgLineTexture);
      DECODER_ADD(BinaryImageShader);
      DECODER_ADD(ImageLineShader);
      DECODER_ADD(BlurShader);
      DECODER_ADD(SimpleImageLineShader);
   }
} DECODERSnpr_static;

int NPRview::_draw_flag    = NPRview::SHOW_NONE;
int NPRview::_capture_flag = 0;

void
NPRview::set_view(CVIEWptr &v)
{
   VIEWimpl::set_view(v);

   if (Config::get_var_bool("ENABLE_FFS",false)) {
      v->add_rend_type(ControlLineTexture::static_name());
      if (Config::get_var_bool("ADD_FFS_TEXTURE",false)){
         v->add_rend_type(FFSTexture::static_name());
         v->add_rend_type("FFSTexture2");
      }
   }
   if (Config::get_var_bool("ENABLE_MARBLE",false)) {
      v->add_rend_type(GLSLMarbleShader::static_name());
   }
   if (Config::get_var_bool("ENABLE_PATTERN",false)) {
      v->add_rend_type(PatternTexture::static_name());
   }
   if (Config::get_var_bool("ENABLE_HALFTONE_EX",false)) {
      v->add_rend_type(HalftoneShaderEx::static_name());
   }
  if (Config::get_var_bool("ENABLE_DOTS_EX",false)) {
      v->add_rend_type(DotsShader_EX::static_name());
  }
  if (Config::get_var_bool("ENABLE_HALFTONE_TX",true)) {
      v->add_rend_type(Halftone_TX::static_name());
   }
   if (Config::get_var_bool("ENABLE_HATCHING_TX",true)) {
      v->add_rend_type(HatchingTX::static_name());
      v->add_rend_type(Painterly::static_name());
   }
   if (Config::get_var_bool("ENABLE_PROXY",true)) {
      v->add_rend_type(ProxyTexture::static_name());
      v->add_rend_type(HatchingTexture::static_name());
   }
   if (Config::get_var_bool("ENABLE_SKYBOX_TEX",false)) {
      v->add_rend_type(Skybox_Texture::static_name());
   }
   if (Config::get_var_bool("ENABLE_GLSL_LIGHTING",true)) {
      v->add_rend_type(GLSLLightingShader::static_name());
   }
   if (Config::get_var_bool("ENABLE_GLSL_TEST",false)) {
      v->add_rend_type(GLSLShader::static_name());
   }
   if (Config::get_var_bool("ENABLE_XTOON_SHADER",true)) {
      v->add_rend_type(GLSLXToonShader::static_name());
   }
   if (Config::get_var_bool("ENABLE_GLSL_TOON_SHADER",true)) {
      v->add_rend_type(GLSLToonShader::static_name());
   }
   if (Config::get_var_bool("ENABLE_TONE_SHADER",true)) {
      v->add_rend_type(ToneShader::static_name());
   }
   if (Config::get_var_bool("ENABLE_GLSL_HALO_SHADER",false)) {
	   v->add_rend_type(GLSLHaloShader::static_name());
   }
   if (Config::get_var_bool("ENABLE_IMG_LINE_TEXTURE",false)) {
      v->add_rend_type(ImgLineTexture::static_name());
   }
   if (Config::get_var_bool("ENABLE_BINARY_SHADER",false)) {
      v->add_rend_type(BinaryImageShader::static_name());
   }
   if (Config::get_var_bool("ENABLE_GLSL_NORMAL_SHADER",false)) {
      v->add_rend_type(GLSLNormalShader::static_name());
   }
   if (Config::get_var_bool("ENABLE_IMAGE_LINE_SHADER",false)) {
      v->add_rend_type(ImageLineShader::static_name());
   }
   if (Config::get_var_bool("ENABLE_SIMPLE_IMAGE_LINE_SHADER",false)) {
      v->add_rend_type(SimpleImageLineShader::static_name());
   }
   if (Config::get_var_bool("ENABLE_BLUR_SHADER",false)) {
      v->add_rend_type(BlurShader::static_name());
   }
   if (Config::get_var_bool("DEBUG_PATCH_BLEND_WEIGHTS",false)) {
      v->add_rend_type(SolidColorTexture::static_name());
   }
   bool minimal = Config::get_var_bool("JOT_MINIMAL_RENDER_STYLES",false);

   if (!minimal) {
      // not usually used:
      if (Config::get_var_bool("ADD_ZX_FRAME",false))
         v->add_rend_type(ZcrossFrameTexture::static_name());
      if (Config::get_var_bool("ADD_ZX_KEYLINE",false))
         v->add_rend_type(ZkeyLineTexture::static_name());
      if (Config::get_var_bool("ADD_ZX_STROKE",false))
         v->add_rend_type(ZXedgeStrokeTexture::static_name());
   }
   v->add_rend_type(GTexture::static_name());
}

void
NPRview::set_size(int w, int h, int x, int y)
{
   GL_VIEW::set_size(w,h,x,y);
   RefImage::view_resize(_view); // move to GL_VIEW?
}

static bool started_drawing = false;

void   
NPRview::set_rendering(Cstr_ptr &)
{
   // XXX - should use observers for rendering changes

   // crashes w/out the check for 'started_drawing'
   if (started_drawing && VisRefImage::lookup(_view))
      VisRefImage::lookup(_view)->force_dirty();

   if (started_drawing && BufferRefImage::lookup(_view))
      BufferRefImage::lookup(_view)->force_dirty();
}

void
NPRview::draw_setup()
{
   PaperEffect::delayed_activate();

   // If an AuxRefImage exists, let's update it...
   // But not the first time round -- goes wacky
   // for some obscure gl reason...
   AuxRefImage *aux = AuxRefImage::lookup(_view);
   if (started_drawing && aux && VIEW::stamp()>=0) aux->update();

   started_drawing = true; // don't ask =(

   // If a BufferRefImage exists, is observing AND isn't dirty,
   // then we *shouldn't* need any other ref-image either...
   BufferRefImage *buf = BufferRefImage::lookup(_view);
   if (!(buf && buf->is_observing() && !buf->need_update())) {
      RefImage::update_all(_view);
   }

   GL_VIEW::draw_setup();
}

void
NPRview::distribute_pixels_to_patches()
{
   // On demand: iterate over the pixels of the ID image,
   // distributing pixels to each patch. Only does work the first
   // time it is called for a frame:

   // XXX - 
   //   we're not checking that the ID image has been updated 
   //   -- we assume it has been. might want to fix...

   if (_distribute_pixels_stamp == VIEW::stamp())
      return;
   _distribute_pixels_stamp = VIEW::stamp();

   Bsimplex*     sim = 0;
   Patch*          p = 0;
   IDRefImage* idref = id_ref();

   for (unsigned int id=0; id<idref->max(); id++) {
      if ((sim = idref->simplex(id)) &&
          (p = get_ctrl_patch(sim)))
         p->add_pixel(id);
   }
}

int
NPRview::draw_background() 
{
   bool do_pattern =  Config::get_var_bool("ENABLE_PATTERN",false);
    
   if(!do_pattern) {
      GL_VIEW::print_gl_errors("NPRview::draw_background [Start] - ");
   
      // First see if the texture needs updating

      str_ptr     tex_name = _view->get_bkg_file();
      TEXTUREptr  tex_ptr  = _view->get_bkg_tex();
   
      double width=0.0, height=0.0;
        
      if ((tex_name != NULL_STR) && (tex_ptr == NULL)) {
         Image image(tex_name);

         if (!image.empty()) {
            TEXTUREglptr t = new TEXTUREgl();
            t->set_save_img(true);
            t->set_image(image.copy(),image.width(),image.height(),image.bpp());
            width = image.width();
            height = image.height();
            err_mesg(ERR_LEV_INFO, 
                     "NPRview::draw_background() - Loaded new background: w=%d h=%d bpp=%u", 
                     image.width(), image.height(), image.bpp());

            tex_ptr = t;
            _view->set_bkg_tex(t);
         } else {
            err_mesg(ERR_LEV_ERROR, "NPRview::draw_background() - *****ERROR***** Failed loading background: '%s'", **tex_name);
            err_mesg(ERR_LEV_INFO, "NPRview::draw_background() - Resetting background.");

            // Clear the name (and tex ptr)
            _view->set_bkg_file(NULL_STR);

            //Make sure the GUI shows this
            if (ViewUI::is_vis(_view)) 
               ViewUI::update(_view);
         }
      } else if (tex_name == NULL_STR) {
         assert(tex_ptr == NULL);
      }

      // Now draw bkg if it's textured and/or needs papering

      if ((tex_ptr == NULL) || (!_view->get_use_paper()) || (_view->get_alpha() == 1.0))
         return 0;

      GL_VIEW::print_gl_errors("NPRview::draw_background [Middle] - ");

      // Now update the texture's matrix for this window
      // such that NDC->UV
   
      double ax = _view->aspect_x();
      double ay = _view->aspect_y();
    
      Wtransf tf;

      if (tex_ptr != NULL) {
         // Aspect ratio of texture
         double ta = ((double)tex_ptr->image().width())/((double)tex_ptr->image().height());
         // Aspect ratio of window
         double wa = ax/ay;
         double r = wa/ta;

         //Put origin in lower left
         tf = Wtransf(Wpt(ax,ay,0)); 
               
         if (r<1.0)
            tf = Wtransf::scaling(0.5/(ay*ta), 0.5/ay, 1) * tf;
         else
            tf = Wtransf::scaling(0.5/ax, 0.5*ta/ax, 1) * tf;

         _view->set_bkg_tf(tf);
      }

      glPushAttrib(
         GL_DEPTH_BUFFER_BIT |
         GL_CURRENT_BIT      |  
         GL_ENABLE_BIT       | 
         GL_COLOR_BUFFER_BIT | 
         GL_TEXTURE_BIT);

      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadMatrixd(VIEW::peek()->ndc_proj().transpose().matrix());

      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();

      glDisable(GL_LIGHTING);                      // GL_LIGHTING_BIT     or GL_ENABLE_BIT
      glDepthMask(GL_FALSE);                       // GL_DEPTH_BUFFER_BIT or GL_ENABLE_BIT
   
      GL_COL(_view->color(), _view->get_alpha());  // GL_CURRENT_BIT

      if (tex_ptr!=NULL) {
         glEnable(GL_TEXTURE_2D);                  // GL_TEXTURE_BIT      or GL_ENABLE_BIT
         tex_ptr->apply_texture(&tf);              // GL_TEXTURE_BIT
      }      

      glEnable(GL_BLEND);                          // GL_COLOR_BUFFER_BIT or GL_ENABLE_BIT
      if (PaperEffect::is_alpha_premult())
         glBlendFunc(GL_ONE, GL_ZERO);             // GL_COLOR_BUFFER_BIT
      else
         glBlendFunc(GL_SRC_ALPHA, GL_ZERO);       // GL_COLOR_BUFFER_BIT

      PaperEffect::begin_paper_effect(_view->get_use_paper());

      NDCpt a(ax,-ay), b(ax,ay), c(-ax,ay), d(-ax,-ay);
   
  
      glBegin(GL_QUADS);
      PaperEffect::paper_coord(a.data());    glTexCoord2dv(a.data());    glVertex2dv(a.data());
      PaperEffect::paper_coord(b.data());    glTexCoord2dv(b.data());    glVertex2dv(b.data());
      PaperEffect::paper_coord(c.data());    glTexCoord2dv(c.data());    glVertex2dv(c.data());
      PaperEffect::paper_coord(d.data());    glTexCoord2dv(d.data());    glVertex2dv(d.data());
      glEnd();

      PaperEffect::end_paper_effect(_view->get_use_paper());

      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();

      glMatrixMode(GL_PROJECTION);
      glPopMatrix();

  
      glPopAttrib();

      GL_VIEW::print_gl_errors("NPRview::draw_background [End] - ");
   } else {

      GL_VIEW::print_gl_errors("NPRview::draw_background [Start] - ");
   
      str_ptr     tex_name = _view->get_bkg_file();
      TEXTUREptr  tex_ptr  = _view->get_bkg_tex();
      double width=0.0, height=0.0;
        
      if ((tex_name != NULL_STR) && (tex_ptr == NULL)) {
         Image image(tex_name);

         if (!image.empty()) {
            TEXTUREglptr t = new TEXTUREgl();
            t->set_save_img(true);
            t->set_image(image.copy(),image.width(),image.height(),image.bpp());
              
            err_mesg(ERR_LEV_INFO, 
                     "NPRview::draw_background() - Loaded new background: w=%d h=%d bpp=%u", 
                     image.width(), image.height(), image.bpp());

            tex_ptr = t;
            _view->set_bkg_tex(t);
         } else {
            err_mesg(ERR_LEV_ERROR, "NPRview::draw_background() - *****ERROR***** Failed loading background: '%s'", **tex_name);
            err_mesg(ERR_LEV_INFO, "NPRview::draw_background() - Resetting background.");

            // Clear the name (and tex ptr)
            _view->set_bkg_file(NULL_STR);

            //Make sure the GUI shows this
            if (ViewUI::is_vis(_view)) 
               ViewUI::update(_view);
         }
      } else if (tex_name == NULL_STR) {
         assert(tex_ptr == NULL);
      }
   
      glPushAttrib(
         GL_DEPTH_BUFFER_BIT |
         GL_CURRENT_BIT      |  
         GL_ENABLE_BIT       | 
         GL_COLOR_BUFFER_BIT | 
         GL_TEXTURE_BIT);
   
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
   
      //glLoadMatrixd(VIEW::peek()->ndc_proj().transpose().matrix());

      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();

      glDisable(GL_LIGHTING);                      // GL_LIGHTING_BIT     or GL_ENABLE_BIT
      glDepthMask(GL_FALSE);                       // GL_DEPTH_BUFFER_BIT or GL_ENABLE_BIT
   
      GL_COL(_view->color(), _view->get_alpha());  // GL_CURRENT_BIT
   
      Wtransf tf;
      tf = Wtransf(Wpt(1.0,1.0,0)); 
         
      if (tex_ptr!=NULL) {
         glEnable(GL_TEXTURE_2D);                  // GL_TEXTURE_BIT      or GL_ENABLE_BIT
         _view->set_bkg_tf(tf);
         tex_ptr->apply_texture(&tf);              // GL_TEXTURE_BIT
        
         width = (double)tex_ptr->image().width();
         height = (double)tex_ptr->image().height();
       
         double s_width = (double)_view->width();
         double s_height = (double)_view->height();
       
         double w_r = width/s_width;
         double h_r = height/s_height;
         w_r = 2 * w_r - 1;
         h_r = 2 * h_r - 1;
       
    
        
         glEnable (GL_BLEND);                      // GL_COLOR_BUFFER_BIT or GL_ENABLE_BIT
         if (PaperEffect::is_alpha_premult())
            glBlendFunc(GL_ONE, GL_ZERO);             // GL_COLOR_BUFFER_BIT
         else
            glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
       
         NDCpt a(-1.0, -1.0), b(w_r, -1.0), c(w_r, h_r), d(-1.0, h_r);
         PaperEffect::begin_paper_effect(_view->get_use_paper());
         glBegin(GL_QUADS);  
         PaperEffect::paper_coord(a.data()); glTexCoord2d(0.0, 0.0);    glVertex2dv(a.data());
         PaperEffect::paper_coord(b.data()); glTexCoord2d(1.0, 0.0);    glVertex2dv(b.data());
         PaperEffect::paper_coord(c.data()); glTexCoord2d(1.0, 1.0);    glVertex2dv(c.data());
         PaperEffect::paper_coord(d.data()); glTexCoord2d(0.0, 1.0);    glVertex2dv(d.data());
         glEnd();
         PaperEffect::end_paper_effect(_view->get_use_paper());
      }
  
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();

      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();

      glPopAttrib();
      GL_VIEW::print_gl_errors("NPRview::draw_background [End] - ");
   }

   return 2;
}

bool
NPRview::antialias_check()
{
   if (_view->win()->accum_red_bits() && _view->win()->accum_green_bits() &&
       _view->win()->accum_blue_bits() && _view->win()->accum_alpha_bits())
      return true;
   else
      return false;

}

int
NPRview::draw_frame(
   CAMdata::eye  e
   )
{
   int tris = 0;

   BufferRefImage *buf = BufferRefImage::lookup(_view);

   int mode = _view->get_antialias_mode();
   int jitters = (!_view->get_antialias_enable())?(1):(VIEW::get_jitter_num(mode));

   for (int i=0; i<jitters; i++) {
      if (jitters>1) {
         _view->set_jitter(mode,i);
         if (i>0)
            GL_VIEW::draw_setup();
      }

      if ((buf)&&(buf->is_observing())) {
         if (buf->need_update()) {
            tris = draw_background();
            tris += GL_VIEW::draw_frame(e);
            tris += GL_VIEW::draw_objects(buf->list(), e);
         } else {
            buf->draw_img(); // XXX - should use texture memory image
            glDisable(GL_DEPTH_TEST);
            tris = GL_VIEW::draw_objects(buf->list(), e);
            glEnable(GL_DEPTH_TEST);
         }
      } else {
         tris = draw_background();
         tris += GL_VIEW::draw_frame(e);
      }

      if (jitters>1) {
         if (i==0)
            glAccum(GL_LOAD,1.0f/jitters);
         else
            glAccum(GL_ACCUM,1.0f/jitters);
      }
   }

   if (jitters>1) {
      _view->set_jitter(-1,-1);
      glAccum (GL_RETURN,1.0);
   }

   return tris;
}


void
NPRview::swap_buffers()
{
   // Draw a given reference image (for debugging):
   static bool count_strokes = Config::get_var_bool("COUNT_STROKES",false);


   switch (_draw_flag) {
    case SHOW_VIS_ID:
      VisRefImage::lookup(_view)->draw_img();
    brcase SHOW_ID_REF:
        id_ref()->draw_img();
    brcase SHOW_COLOR_REF0:
        color_ref(0)->draw_img();
    brcase SHOW_COLOR_REF1:
        color_ref(1)->draw_img();
    brcase SHOW_TEX_MEM0:
        color_ref(0)->draw_tex();
    brcase SHOW_TEX_MEM1:
        color_ref(1)->draw_tex();
    brcase SHOW_BUFFER:
        BufferRefImage::lookup(_view)->draw_img();
    brcase SHOW_AUX:
        AuxRefImage::lookup(_view)->draw_img();
    brcase SHOW_HALO:
        halo_ref()->draw_output();

    brdefault:
        ;
   }
   if (_capture_flag) {
      _capture_flag = 0;

      const int max_num = 10000;

      str_ptr fname_base = str_ptr("big_grab");
      str_ptr fname_suffix = str_ptr(".png");

      str_ptr fname;

      char buf[10];


      // XXX - hacked
      //
      //   We try to open "fname_base + XXX + fanme_suffix"
      //   and continue to increment XXX until we fail
      //   meaning that the file does not exist and
      //   we can safely use that filename.
      //   The loop terminates after max_num just in case
      //   some voodoo happens and we loop forever...

      int i;
      for (i=1; i< max_num; i++) {
         sprintf(buf,"%05d",i);
         fname = fname_base + buf + fname_suffix;
         FILE *fp = fopen(**fname,"r");

         if (!fp) break;

         fclose(fp);
      }

      if (i != max_num) {
         _view->screen_grab(1, fname);
         err_mesg(ERR_LEV_ERROR, "NPRView::swap_buffers() - Wrote grab to: '%s'", **fname);
      } else {
         err_mesg(ERR_LEV_ERROR,
                  "NPRView::swap_buffers() - Couldn't get a free output filename for grab!!");
      }
   }

   GL_VIEW::swap_buffers();

   err_adv(count_strokes, "Strokes Drawn: %d" , BaseStroke::get_strokes_drawn()); 
}

inline void
show_message(Cstr_ptr& s)
{
   cerr << s << endl;
   WORLD::message(s);
}

void
NPRview::next_ref_img() 
{
   _draw_flag = (_draw_flag + 1) % NUM_REF_IMGS;
   switch (_draw_flag) {
      // use err_msg since the on-screen messages get obscured by the
      // displayed images themselves
    case SHOW_NONE:
      show_message("NPRview: Not displaying any reference image");
      break;
    case SHOW_VIS_ID:
      show_message("NPRview: Displaying VisRefImage");
      break;
    case SHOW_ID_REF:
      show_message("NPRview: Displaying IDRefImage");
      break;
    case SHOW_COLOR_REF0:
      show_message("NPRview: Displaying ColorRefImage0");
      break;
    case SHOW_COLOR_REF1:
      show_message("NPRview: Displaying ColorRefImage1");
      break;
    case SHOW_TEX_MEM0:
      show_message("NPRview: Displaying ColorRefImage0 (texture memory)");
      break;
    case SHOW_TEX_MEM1:
      show_message("NPRview: Displaying ColorRefImage1 (texture memory)");
      break;
    case SHOW_BUFFER:
      show_message("NPRview: Displaying BufferRefImage");
      break;
    case SHOW_AUX:
      show_message("NPRview: Displaying AuxRefImage");
      break;
    case SHOW_HALO:
       show_message("NPRview: Displaying HaloRefImage");
       break;
    default:
      // Should never come here
      assert(0);
   }
}

// end of file npr_view.C
