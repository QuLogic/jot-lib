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
    proxy_texture.C
   -------------------
    Simon Breslav
    Fall 2004
***************************************************************************/
#include "disp/colors.H"
#include "geom/gl_view.H"
#include "geom/world.H"
#include "stroke/base_stroke.H"
#include "stroke/outline_stroke.H"

#include "gtex/solid_color.H"
#include "gtex/sil_frame.H"
#include "gtex/hidden_line.H"
#include "gtex/tone_shader.H"
#include "gtex/glsl_paper.H"
#include "gtex/glsl_solid.H"

#include "gtex/ref_image.H"
#include "gtex/wireframe.H"
#include "gtex/flat_shade.H"
#include "gtex/smooth_shade.H"

#include "proxy_stroke.H"
#include "proxy_surface.H"
#include "proxy_texture.H"
#include "hatching_texture.H"
#include "gtex/dots.H"
#include "gtex/haftone_tx.H"
#include "gtex/glsl_hatching.H"
#include "gtex/halftone_shader.H"
#include "gtex/sil_frame.H"

#include "npr/feature_stroke_texture.H"
#include "npr/sil_and_crease_texture.H"

static bool    debug = Config::get_var_bool("DEBUG_PROXY_TEXTURE",false);
static double  animation_dur = 2.0;

/**********************************************************************
 * ProxyTexture:
 **********************************************************************/
ProxyTexture::ProxyTexture(Patch* patch) :
   BasicTexture(patch, new GLStripCB),
   _tone(new ToneShader(patch)),
   _base(new FlatShadeTexture(patch)),
   _solid(new SmoothShadeTexture(patch)),
   _glsl_solid(new GLSLSolidShader(patch)),
   _glsl_dots(new DotsShader(patch)),
   _glsl_dots_TX(new Halftone_TX(patch)),
   _glsl_hatch(new GLSLHatching(patch)),
   _glsl_halftone(new HalftoneShader(patch)),
   _hatching(new HatchingTexture(patch)),
   _proxy_surface(new ProxySurface(patch)),
   _show_sils(true),
   _draw_samples(false),
   _animation_on(false),
   _timer(0)
{
  

   _hatching->set_proxy_texture(this);
 
   set_base(0);
   
}

ProxyTexture::~ProxyTexture()
{
   delete _proxy_surface;
   delete _hatching;
   gtextures().delete_all();
}

void
ProxyTexture::request_ref_imgs()
{
   // request color ref image 0 in texture memory
   ColorRefImage::schedule_update(0, false, true);
}

void
ProxyTexture::set_patch(Patch* p)
{
   GTexture::set_patch(p); // sets the patch on gtextures() list
   if (_proxy_surface)
      _proxy_surface->set_patch(p);

   VIEW::peek()->set_color(COLOR(1.0,1.0,1.0));
}

void
ProxyTexture::draw_samples(CVIEWptr& v)
{
   const vector<DynamicSample>& samples = _patch->get_samples();
   Wpt_list pts;
   for (uint i=0; i < samples.size(); ++i) {
      if (samples[i].get_weight() > 0.0)
         pts += samples[i].get_pos();
   }
   GL_VIEW::draw_pts(pts,Color::red,1.0,10);
}

void
ProxyTexture::make_old_samples()
{
   _old_samples.clear();
   _tmp_debug_sample.clear();
   const vector<DynamicSample>& samples = _patch->get_samples();
   _old_center = _patch->get_sample_center();
   _old_samples = samples;
}

int
ProxyTexture::draw(CVIEWptr& v)
{


   // For early development and debugging, draw the patch in
   // hidden line style.  For this, we can just use the existing
   // HiddenLineTexture stored on the Patch.
   //
   // XXX - should replace this with some kind of "base coat",
   //       and when that happens the base coat texture should
   //       belong to this ProxyTexture, instead of being stored
   //       on the patch.
   //assert(get_tex<HiddenLineTexture>(_patch));
   //int ret = get_tex<HiddenLineTexture>(_patch)->draw(v);



   // Also for early development and debugging, draw the proxy mesh in
   // orange wireframe with fat lines.

   patch()->update_dynamic_samples();

   GL_VIEW::print_gl_errors("ProxyTexture::draw: update_dynamic_samples");

   int ret = _base->draw(v);

   GL_VIEW::print_gl_errors("ProxyTexture::draw: _base->draw");

   if (_show_sils) {      
   }



   //assert(_proxy_surface);
   //_proxy_surface->update_proxy_surface();
   //assert(_proxy_surface->proxy_mesh());


   //if(!_hatching->patch()){
   //   assert(_proxy_surface->proxy_mesh()->npatches() == 1);
   //   Patch* p = _proxy_surface->proxy_mesh()->patches().first();
   //   assert(p);
   //   _hatching->set_patch(p);
   //}
   //_hatching->draw(v);


   return ret;
}

int
ProxyTexture::draw_final(CVIEWptr& v)
{
   int n=0;

   if (_draw_samples) {
      // cerr << "drawing samples" << endl;

      if (!_old_samples.empty()) {

         if (_animation_on) {
            if (!_timer.expired()) {
               move_old_samples((animation_dur - _timer.remaining()) / animation_dur);
            } else {
               move_old_samples(1.0);
               _animation_on = false;
            }
         }
         Wpt_list tmp;



         for (uint i=0; i < _old_samples.size(); ++i) {
            if (_old_samples[i].get_weight() > 0.0)
               tmp += (!_tmp_debug_sample.empty()) ?
                  Wpt(XYpt(_tmp_debug_sample[i])) :
                  Wpt(XYpt(_old_samples[i].get_pix()));
         }
         GL_VIEW::draw_pts(tmp,Color::blue1,1.0,10);
      }
      draw_samples(v);
   }

   //Draw direction line
   if(_patch){
      CWpt_list pts = _patch->get_direction_stroke();
      GL_VIEW::draw_wpt_list(pts, Color::blue4,1.0, 5.0, false);    

      VEXEL dir = _patch->get_direction_vec();
      VEXEL dir2 = _patch->get_z()*100;
      PIXEL start = _patch->get_sample_center();
      PIXEL end = start + dir;
      PIXEL end2 = start + dir2;
      
      Wpt w_s = Wpt(XYpt(start));
      Wpt w_e = Wpt(XYpt(end));
      Wpt w_e2 = Wpt(XYpt(end2));
      

      ARRAY<Wline>  lines;
      
      Wline line(w_s, w_e);
      Wline line2(w_s, w_e2);

      //lines += line;
      lines += line2;

      GL_VIEW::draw_lines(lines,Color::red2,0.5,2.0,false);
   }

   return n;
}

void
ProxyTexture::notify_manip_start(CCAMdataptr &data)
{}

void
ProxyTexture::notify_manip_end(CCAMdataptr &data)
{}

void
ProxyTexture::notify(CCAMdataptr &data)
{}

void
ProxyTexture::set_base(int base)
{
   // XXX assigning to _base may be a memory leak, if _base was
   //     previously allocated and now should be deleted.
   //     should fix this...

   _base_enum = (base_textures_t)base;
   switch(base) {
     case BASE_SOLID:
        _base = _solid;
        break;
     case BASE_PAPER:
         _base = _glsl_solid;
         GLSLPaperShader::set_contrast(2.0);
        break;
     case BASE_HATCHING:
        _base = _glsl_hatch;
        break;
     case BASE_HALFTONE:
        _base = _glsl_dots_TX;
        break;
      default:               
        break;
   }
   bool debug = true;
   if (debug) {
      cerr << "ProxyTexture::set_base: using "
           << (_base ? _base->class_name() : "null")
           << endl;
   }
}

int
ProxyTexture::draw_color_ref(int i)
{
   // Draw the reference image for tone.
   return (i == 0) ? _base->draw_color_ref(i) : 0;
}

void
ProxyTexture::animate_samples()
{
   cerr << "Animated Samples" << endl;
   _z = _patch->get_z_dynamic_samples(_old_samples);
   _timer.reset(animation_dur);
   _animation_on = true;
}

inline VEXEL
cmult(CVEXEL& a, CVEXEL& b)
{
   // complex number multiplication:
   return VEXEL(a[0]*b[0] - a[1]*b[1], a[0]*b[1] + a[1]*b[0]);
}

PIXEL
ProxyTexture::get_new_pixel(PIXEL old, double pos)
{
   PIXEL o = _old_center;
   PIXEL n = interp(o, _patch->get_sample_center(), pos);
   VEXEL z = interp(VEXEL(1,0), _z , pos);

   PIXEL newp = PIXEL(n + cmult(old - o, z));
   return newp;
}

void
ProxyTexture::move_old_samples(double pos)
{
   _tmp_debug_sample.clear();   
   for(uint i=0; i < _old_samples.size(); ++i)
      {        
         _tmp_debug_sample += get_new_pixel(_old_samples[i].get_pix(), pos);
      }
}

/* end of file proxy_texture.C */
