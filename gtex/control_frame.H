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
 * control_frame.H:
 **********************************************************************/
#ifndef CONTROL_FRAME_H_IS_INCLUDED
#define CONTROL_FRAME_H_IS_INCLUDED

#include "disp/colors.H"
#include "color_id_texture.H"

/**********************************************************************
 * ControlFrameTexture:
 **********************************************************************/
class ControlFrameTexture : public BasicTexture {
 public:

   //******** MANAGERS ********

   ControlFrameTexture(Patch* patch = 0, CCOLOR& col = Color::blue_pencil_d) :
      BasicTexture(patch, new GLStripCB),
      _color(col),
      _strip(0) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS2("ControlFrame", BasicTexture, CDATA_ITEM *);

   //******** ACCESSORS ********

   int     set_color(CCOLOR& c)         { _color = c; return 1; }
   CCOLOR& color()              const   { return _color; }

   //******** GTexture METHODS ********

   virtual bool draws_filled() const  { return false; }
   virtual int  draw(CVIEWptr& v); 

   //******** RefImageClient METHODS ********

   virtual int draw_vis_ref() {
      cerr << "ControlFrameTexture::draw_vis_ref: not implemented" << endl;
      return 0;
   }

   //******** DATA_ITEM METHODS ********

   virtual DATA_ITEM  *dup() const { return new ControlFrameTexture; }

 protected:
   COLOR        _color;         // color to use
   EdgeStrip*   _strip;         // edge strip of control mesh edges

   //******** UTILITY METHODS ********

   void draw_selected_faces();
   void draw_selected_edges();
   void draw_selected_verts();

   // Draw helper -- draws control curves from level k,
   // relative to the control Patch:
   void draw_level(CVIEWptr& v, int k);

   // Builds the edge strip for level k:
   bool build_strip(int k);
};

#endif // CONTROL_FRAME_H_IS_INCLUDED

/* end of file control_frame.H */
