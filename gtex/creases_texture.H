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
 * creases_texture.H:
 **********************************************************************/
#ifndef CREASES_TEXTURE_H_IS_INCLUDED
#define CREASES_TEXTURE_H_IS_INCLUDED

#include "basic_texture.H"

/**********************************************************************
 * CreasesTexture:
 **********************************************************************/
class CreasesTexture : public BasicTexture {
 public:
   //******** MANAGERS ********
   CreasesTexture(Patch* patch = 0) :
      BasicTexture(patch),
      _width(1),
      _color(0,0,0) // XXX g++ 2.97 workaround
      {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("Creases", BasicTexture, CDATA_ITEM *);

   //******** ACCESSORS ********
   GLfloat      width()         const   { return _width; }
   CCOLOR&      color()         const   { return _color; }
   void         set_width(GLfloat w)    { _width = w; }
   int          set_color(CCOLOR& c)    { _color = c; return 1; }

   //******** GTexture METHODS ********
   virtual int draw(CVIEWptr& v); 

   //******** DATA_ITEM METHODS ******** 
   virtual DATA_ITEM  *dup() const { return new CreasesTexture; }

 protected:
   GLfloat      _width;
   COLOR        _color;
};

#endif // CREASES_TEXTURE_H_IS_INCLUDED

/* end of file creases_texture.H */

