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
 * ffstexture.C
 **********************************************************************/
#include "npr_texture.H"
#include "ffstexture.H"
#include "ffs_control_frame.H"

str_ptr FFSTexture::location = "nprdata/ffs_style/ffs_texture.ffs";

FFSTexture::FFSTexture(Patch* patch) :
   NPRTexture(patch),
   _got_ffs_file(false),
   _controlframe(new FFSControlFrameTexture(patch))   
{
   _controlframe->set_color(COLOR(0.24,0.103488,0.0672));
}

FFSTexture::~FFSTexture()
{
   delete _controlframe;
}

void 
FFSTexture::set_patch(Patch* p) 
{
   NPRTexture::set_patch(p);
   _controlframe->set_patch(p);
}    

int
FFSTexture::draw(CVIEWptr& v)
{
   if(!_got_ffs_file){      
      get_ffs_data_file ();
      _got_ffs_file = true;
   }
   NPRTexture::draw(v);

   if(v->rendering() != "FFSTexture2")
       _controlframe->draw(v);
   return 0;
}

int
FFSTexture::draw_final(CVIEWptr& v)
{
   NPRTexture::draw_final(v); 
   //_controlframe->draw_with_alpha(.7);
   return 0;
}

void
FFSTexture::get_ffs_data_file ()
{
   str_ptr str;
   // Get the style file    
   
   str = Config::JOT_ROOT() + location;    

   _data_file = str;   

   fstream fin;
   fin.open(**(str),ios::in);
   if (!fin){
      err_mesg(ERR_LEV_ERROR, "FFSTexture - Could not find the file %s", **str);
      //XXX - TO DO : Load some default values....           
   } else {       
      
      STDdstream s(&fin);
      s >> str;
      if (str != FFSTexture::static_name())
         err_mesg(ERR_LEV_ERROR, "FFSTexture - No FFSTexture found in the file");
      else {       
        decode(s);       
      }      
   }  
}
