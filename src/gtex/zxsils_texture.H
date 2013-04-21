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
 * sils_texture.H:
 **********************************************************************/
#ifndef ZXSILS_TEXTURE_H_IS_INCLUDED
#define ZXSILS_TEXTURE_H_IS_INCLUDED

#include "color_id_texture.H"


/**********************************************************************
 * ZcrossTexture:
 *
 *      Draws silhouette edges as GL line strips.
 **********************************************************************/
class ZcrossTexture : public OGLTexture {
 public:
   //******** MANAGERS ********
   ZcrossTexture(Patch* patch = 0) :
      OGLTexture(patch),
      _color(0,0,0), // XXX g++ 2.97 work around
      _width(2) {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("Zcross Sils", GTexture, CDATA_ITEM *);

   //******** ACCESSORS ********
   GLfloat width()              const   { return _width; }
   void set_width(GLfloat w)            { _width = w; }

   CCOLOR&      color()         const   { return _color; }

   //******** GTexture METHODS ********
   virtual int  set_color(CCOLOR& c)            { _color = c; return 1; }
   virtual bool draws_filled()          const   { return false; }
   virtual int  draw(CVIEWptr& v); 

   //******** RefImageClient METHODS ********
   virtual int draw_vis_ref() {
      // You can pick what you can see ... for SilsTexture you see
      // silhouettes, so that's what we draw into the visibility
      // reference image:
      return ColorIDTexture::draw_edges(&_patch->cur_sils(), 3.0);
   }

   //******** DATA_ITEM METHODS ********
   virtual DATA_ITEM  *dup() const { return new ZcrossTexture; }

 protected:
   COLOR                _color;         // color of sils
   GLfloat              _width;         // width of them
};

#endif // ZXSILS_TEXTURE_H_IS_INCLUDED

/* end of file sils_texture.H */
