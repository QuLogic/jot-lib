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
 * patch_id_texture.H:
 **********************************************************************/
#ifndef PATCH_ID_TEXTURE_H_IS_INCLUDED
#define PATCH_ID_TEXTURE_H_IS_INCLUDED

#include "solid_color.H"

/**********************************************************************
 * PatchIDStripCB:
 **********************************************************************/
class PatchIDStripCB : public GLStripCB {
 public:

   virtual void faceCB(CBvert* v, CBface* f);
};

/**********************************************************************
 * PatchIDTexture:
 **********************************************************************/
class PatchIDTexture : public BasicTexture {
 protected:
   GLfloat              _width;         // used when drawing lines
   GLenum               _polygon_mode;  // filled or wireframe?
   SolidColorTexture*   _black;         // optional solid black texture

 public:
   //******** MANAGERS ********
   PatchIDTexture(Patch* patch = 0, GLenum pmode = GL_FILL);
   virtual ~PatchIDTexture() { gtextures().delete_all(); }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Patch ID", PatchIDTexture*, BasicTexture, CDATA_ITEM*);

   //******** ACCESSORS ********

   void set_polygon_mode(GLenum pmode)  { _polygon_mode = pmode; changed(); }
   void set_width(GLfloat w)            { _width = w; }

   //******** VIRTUAL METHODS ********

   virtual GTexture_list gtextures() const { return GTexture_list(_black); }
   virtual int  draw_black(CVIEWptr& v) { return _black->draw(v); }

   //******** GTexture METHODS ********
   virtual int  draw(CVIEWptr& v); 

   //******** DATA_ITEM METHODS ********
   virtual DATA_ITEM  *dup() const { return new PatchIDTexture; }

};

#endif // PATCH_ID_TEXTURE_H_IS_INCLUDED

/* end of file color_id_texture.H */
