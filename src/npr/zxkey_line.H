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
 * zxkey_line.H:
 **********************************************************************/
#ifndef ZZXKEY_LINE_H_IS_INCLUDED
#define ZZXKEY_LINE_H_IS_INCLUDED

#include "gtex/basic_texture.H"

class ZXedgeStrokeTexture;

class ZkeyLineTexture : public OGLTexture {
 public:
   //******** MANAGERS ********
   ZkeyLineTexture(Patch* patch = 0);
   virtual ~ZkeyLineTexture();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("ZX Key Line", OGLTexture, CDATA_ITEM *);

   //******** ACCESSORS ********
   ZXedgeStrokeTexture* zx_stroke()     { return _zx_stroke;  }
   GTexture*            base_coat()     { return _base_coat; }

   int set_sil_color(CCOLOR& c);

   //******** GTexture VIRTUAL METHODS ********
   virtual void set_patch(Patch* p);

   virtual void push_alpha(double a);
   virtual void pop_alpha();

   virtual void draw_filled_tris(double alpha);
   virtual void draw_non_filled_tris(double alpha);

   virtual int draw(CVIEWptr& v); 
  
   virtual void request_ref_imgs();
   virtual int  draw_id_ref();
   virtual int  draw_final(CVIEWptr&);

  //******** DATA_ITEM METHODS ********
   virtual DATA_ITEM  *dup() const { return new ZkeyLineTexture; }

 protected:
   GTexture*            _base_coat;
   ZXedgeStrokeTexture* _zx_stroke;
};

#endif // ZZXKEY_LINE_H_IS_INCLUDED

/* end of file zxkey_line.H */

