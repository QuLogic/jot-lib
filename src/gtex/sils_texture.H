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
#ifndef SILS_TEXTURE_H_IS_INCLUDED
#define SILS_TEXTURE_H_IS_INCLUDED

#include "color_id_texture.H"

/**********************************************************************
 * SilsTexture:
 *
 *      Draws silhouette edges as GL line strips.
 **********************************************************************/
class SilsTexture : public OGLTexture {
 public:
   //******** MANAGERS ********
   SilsTexture(Patch* patch = 0) :
      OGLTexture(patch),
      _color(0,0,0), // XXX g++ 2.97 work around
      _width(2),
      _draw_borders(true) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Sils", SilsTexture*, OGLTexture, CDATA_ITEM *);

   //******** ACCESSORS ********

   GLfloat width()              const   { return _width; }
   void set_width(GLfloat w)            { _width = w; }

   CCOLOR&      color()         const   { return _color; }

   bool draw_borders()          const   { return _draw_borders; }
   void set_draw_borders(bool b)        { _draw_borders = b; }
   
   //******** GTexture METHODS ********

   virtual int  set_color(CCOLOR& c)            { _color = c; return 1; }
   virtual bool draws_filled()          const   { return false; }
   virtual int  draw(CVIEWptr& v); 

   //******** RefImageClient METHODS ********

   virtual int draw_vis_ref();

   //******** DATA_ITEM METHODS ********

   virtual DATA_ITEM  *dup() const { return new SilsTexture; }

 protected:
   COLOR        _color;         // color of sils
   GLfloat      _width;         // width of them
   bool         _draw_borders;  // if true, draw borders and sils (default is true)
};

#endif // SILS_TEXTURE_H_IS_INCLUDED

/* end of file sils_texture.H */
