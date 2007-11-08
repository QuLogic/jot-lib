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
//SKY_BOX stroke texture
// by Karol Szerszen

#ifndef SKYBOX_TEXTURE_H_IS_INCLUDED
#define SKYBOX_TEXTURE_H_IS_INCLUDED

#include "geom/texturegl.H"    // for ConventionalTexture
#include "gtex/basic_texture.H"

/*****************************************************************
 * Skybox_Texture:
 *
 *  Draws a textured Patch without lighting. Designed for
 *  use with the skybox, but could be renamed and used for
 *  other purposes as well.
 *****************************************************************/
class Skybox_Texture : public BasicTexture {
 public:
   //******** MANAGERS ********
   Skybox_Texture(Patch* patch =0);
   virtual ~Skybox_Texture() {} 

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("SkyBox_Texture", Skybox_Texture*,
                        BasicTexture, CDATA_ITEM *);

   // set the TEXTUREgl to use:
   void set_tex(CTEXTUREglptr& tex);

   //******** GTexture VIRTUAL METHODS ********
 
   //draw faces using 2d texture according to 
   //texture coordinates provided by the triangle strip callback
   //disable light
   virtual int draw(CVIEWptr& v); 

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new Skybox_Texture; }

   virtual void load_texture(str_ptr file_name);

 protected:

   //******** Member Variables ********

   //stores the 2d texture to be used
   TEXTUREglptr _tex;   
};

#endif // SKYBOX_TEXTURE_H_IS_INCLUDED

// end of file skybox_texture.H
