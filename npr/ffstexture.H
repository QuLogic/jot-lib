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
 * ffstexture.H
 **********************************************************************/
#ifndef FFSTEXTURE_H_IS_INCLUDED
#define FFSTEXTURE_H_IS_INCLUDED

#include "npr/npr_texture.H"
#include "std/config.H"

class FFSControlFrameTexture;

class FFSTexture : public NPRTexture  {
 public: 

   //******** MANAGERS ********

   FFSTexture(Patch* patch = 0);
      
   virtual ~FFSTexture();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS2("FFSTexture", NPRTexture, CDATA_ITEM *);

   //******** DATA_ITEM METHODS ********

   virtual DATA_ITEM  *dup() const { return new FFSTexture;}
    
   //******** GTexture VIRTUAL METHODS ********

   virtual void set_patch(Patch* p);
   virtual int  draw(CVIEWptr& v);
   virtual int  draw_final(CVIEWptr& v);
   
   void         get_ffs_data_file ();  
   static void  set_location(str_ptr loc) { location = loc; }  

 private:
   bool                     _got_ffs_file;
   FFSControlFrameTexture*  _controlframe;
   static str_ptr           location;
};

#endif // FFSTEXTURE_H_IS_INCLUDED

// end of file ffstexture.H
