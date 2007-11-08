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
    pattern_texture.H
 ***************************************************************************/
#ifndef PATTERN_TEXTURE_H
#define PATTERN_TEXTURE_H

#include "std/config.H"
#include "mesh/patch.H"
#include "gtex/basic_texture.H"
#include "gtex/smooth_shade.H"

class PatternTexture : public BasicTexture, public CAMobs  {
public: 
   //******** MANAGERS ********
   PatternTexture(Patch* patch = 0);      
   virtual ~PatternTexture();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("PatternTexture", BasicTexture, CDATA_ITEM *);

   //******** DATA_ITEM METHODS ********
   static TAGlist     *_pt_tags;
   virtual DATA_ITEM  *dup() const { return new PatternTexture;}
   virtual CTAGlist&    tags() const;
   
   //******** GTexture VIRTUAL METHODS ********
   virtual void set_patch(Patch* p); 
   virtual int  draw(CVIEWptr& v);
   virtual int  draw_final(CVIEWptr& v);    
   virtual int  draw_id_ref();
   virtual int  draw_color_ref(int i);
   virtual void request_ref_imgs();
   
   void    set_base_coler(CCOLOR c)         { if(_base) _base->set_color(c); }
   
   
   //******** CAMobs VIRTUAL METHODS ********
   virtual void notify(CCAMdataptr &data){}
   virtual void notify_manip_start(CCAMdataptr &data){}
   virtual void notify_manip_end(CCAMdataptr &data){}
   //********Interface to pattern ********
   //ProxySurface*        proxy_surface() {return _proxy_surface; }
private:
   //ProxySurface*          _proxy_surface;
   SmoothShadeTexture*     _base;
 
};

#endif  // PATTERN_TEXTURE_H

/* end of file pattern_texture.H */
