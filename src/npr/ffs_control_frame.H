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
 * ffs_control_frame.H:
 **********************************************************************/
#ifndef FFS_CONTROL_FRAME_H_IS_INCLUDED
#define FFS_CONTROL_FRAME_H_IS_INCLUDED

#include "gtex/control_frame.H"

/**********************************************************************
 * ControlFrameTexture:
 **********************************************************************/
class FFSControlFrameTexture : public ControlFrameTexture {
 public:

   //******** MANAGERS ********

   FFSControlFrameTexture(Patch* patch = 0, CCOLOR& col = COLOR::blue) :
      ControlFrameTexture(patch, col)
      {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS2("FFSControlFrame", BasicTexture, CDATA_ITEM *);

   //******** GTexture METHODS ********

   virtual bool draws_filled() const  { return false; }
   virtual int  draw(CVIEWptr& v); 

   //******** DATA_ITEM METHODS ********

   virtual DATA_ITEM  *dup() const { return new FFSControlFrameTexture; }

 protected:     

   //******** UTILITY METHODS ********

   // Draw helper -- draws control curves from level k,
   // relative to the control Patch:
   void draw_level(CVIEWptr& v, int k);    
};

#endif // CONTROL_FRAME_H_IS_INCLUDED

// end of file control_frame.H
