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
    proxy_texture.H
    ProxyTexture
    -------------------
    Simon Breslav
***************************************************************************/
#ifndef PROXY_TEXTURE_H
#define PROXY_TEXTURE_H

#include "gtex/basic_texture.H"
#include "proxy_surface.H"

#include <string>
#include <vector>

class ToneShader;
class GLSLPaperShader;
class HatchingTexture;
class GLSLSolidShader;
class WireframeTexture;
class DotsShader;
class GLSLHatching;
class HalftoneShader;
class SolidColorTexture;
class HiddenLineTexture;
class SmoothShadeTexture;
class FlatShadeTexture;
class SilFrameTexture;
class Halftone_TX;

class ProxyTexture : public BasicTexture, public CAMobs {
 public:
   //******** MANAGERS ********
   ProxyTexture(Patch* patch = 0);
   virtual ~ProxyTexture();
   
   enum base_textures_t {     
      BASE_SOLID=0,  
      BASE_PAPER,    
      BASE_HATCHING,    
      BASE_HALFTONE,      
      BASE_NUM
   } _base_enum;

   int        get_base() const { return (int)_base_enum; }
   void       set_base(int base);
   GTexture*  get_base_texture()const { return (_base) ? _base : 0; }

   static std::vector<std::string>&  base_names() {
      static std::vector<std::string> b_names;
      if (b_names.empty()) {
         b_names.push_back("Solid");
         b_names.push_back("Paper");
         b_names.push_back("Hatching");
         b_names.push_back("Halftone");
      }
      return b_names;
   }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("ProxyTexture", ProxyTexture*,
                        BasicTexture, CDATA_ITEM*);

   //******** Interface to pattern ********
   ProxySurface*    proxy_surface() const { return _proxy_surface; }
   HatchingTexture* hatching_tex()  const { return _hatching; }
   bool             is_selected()   const { return _is_selected; }
   void             set_selected(bool s)  { _is_selected = s; }
   void             set_draw_samples(bool s) {_draw_samples = s;}
   bool             get_draw_samples()  { return _draw_samples; }

   void             make_old_samples();
   void             clear_old_sampels() { _old_samples.clear(); }

   //******** DATA_ITEM METHODS ********
   virtual DATA_ITEM  *dup() const { return new ProxyTexture;}

   //******** GTexture VIRTUAL METHODS ********
   virtual GTexture_list gtextures() const {
      
      GTexture_list ret;
      ret.add((GTexture*)_tone);
      ret.add((GTexture*)_solid);
      ret.add((GTexture*)_glsl_solid);
      ret.add((GTexture*)_glsl_dots);
      ret.add((GTexture*)_glsl_dots_TX);
      ret.add((GTexture*)_glsl_hatch);
      ret.add((GTexture*)_glsl_halftone);
      // _base may actually point to one of the above GTextures:
      ret.add_uniquely((GTexture*)_base);

      return ret;
   }

   virtual void      set_patch(Patch* p);
   virtual int       draw(CVIEWptr& v);
   virtual int       draw_final(CVIEWptr& v);
   virtual void      request_ref_imgs();    
   virtual int       draw_color_ref(int i);
   void              draw_samples(CVIEWptr& v);
   void              animate_samples();
   PIXEL             get_new_pixel(PIXEL old, double pos);
   void              move_old_samples(double pos);

   //******** CAMobs VIRTUAL METHODS ********
   virtual void notify(CCAMdataptr &data);
   virtual void notify_manip_start(CCAMdataptr &data);
   virtual void notify_manip_end(CCAMdataptr &data);

 private:
   ToneShader*           _tone;
   GTexture*             _base;
   SmoothShadeTexture*   _solid;
   GLSLSolidShader*      _glsl_solid;
   DotsShader*           _glsl_dots;
   Halftone_TX*          _glsl_dots_TX;
   GLSLHatching*         _glsl_hatch;
   HalftoneShader*       _glsl_halftone;
   HatchingTexture*      _hatching;
   ProxySurface*         _proxy_surface;
  
   bool                  _show_sils;
   bool                  _draw_samples;
   bool                  _is_selected;

   PIXEL_list            _tmp_debug_sample;
   vector<DynamicSample> _old_samples;
   PIXEL                 _old_center;
   VEXEL                 _z;
   bool                  _animation_on;
   egg_timer             _timer;
};

#endif  // PROXY_TEXTURE_H

// end of file proxy_texture.H
