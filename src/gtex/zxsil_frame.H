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
 * sil_frame.H:
 **********************************************************************/
#ifndef ZXSIL_FRAME_H_IS_INCLUDED
#define ZXSIL_FRAME_H_IS_INCLUDED

#include "zxsils_texture.H"
#include "creases_texture.H"

/**********************************************************************
 * SilFrameTexture:
 **********************************************************************/
class ZcrossFrameTexture : public OGLTexture {
 public:
   //******** MANAGERS ********
   ZcrossFrameTexture(Patch* patch = 0) :
      OGLTexture(patch),
      _creases(new CreasesTexture(patch)),
      _zxsils(new ZcrossTexture(patch)) {}

   virtual ~ZcrossFrameTexture() { gtextures().delete_all(); }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("ZX Sil Frame", OGLTexture, CDATA_ITEM *);

   //******** ACCESSORS ********
   void set_sil_width(GLfloat w)    { _zxsils->set_width(w); }
   void set_sil_color(CCOLOR& c)    { _zxsils->set_color(c); }
   void set_crease_width(GLfloat w) { _creases->set_width(w); }
   void set_crease_color(CCOLOR& c) { _creases->set_color(c); }

   //******** GTexture METHODS ********

   virtual GTexture_list gtextures() const {
      return GTexture_list(_creases, _zxsils);
   }

   virtual int set_color(CCOLOR& c) {
      set_sil_color(c);
      set_crease_color((c * 0.9) + (COLOR::white * 0.1));
      return 1;
   }

   virtual bool draws_filled() const { return false; }

   virtual int draw(CVIEWptr& v) {
      if (_ctrl)
	return _ctrl->draw(v);
      _creases->draw(v);
      _zxsils->draw(v);
      
      return 1;
   }

   //******** RefImageClient METHODS ********
   virtual int draw_vis_ref() {
      // XXX - draw creases?
      return _zxsils->draw_vis_ref();
   }

   //******** DATA_ITEM METHODS ********
   virtual DATA_ITEM  *dup() const { return new ZcrossFrameTexture; }

 protected:
   CreasesTexture*      _creases;
   ZcrossTexture*         _zxsils;
};

#endif // ZXSIL_FRAME_H_IS_INCLUDED

/* end of file sil_frame.H */
