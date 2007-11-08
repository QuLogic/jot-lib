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
 * control_line.H:
 **********************************************************************/
#ifndef CONTROL_LINE_H_IS_INCLUDED
#define CONTROL_LINE_H_IS_INCLUDED

#include "geom/gl_view.H"

#include "solid_color.H"
#include "toon_texture_1D.H" //less buggy toon shader
#include "control_frame.H"
#include "sils_texture.H"

/**********************************************************************
 * ControlLineTexture:
 **********************************************************************/
class ControlLineTexture : public OGLTexture {
 public:

   //******** MANAGERS ********

   ControlLineTexture(Patch* patch = 0);

   virtual ~ControlLineTexture();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Control Line", ControlLineTexture*,
                        OGLTexture, CDATA_ITEM*);

   //******** GTexture VIRTUAL METHODS ********

   virtual GTexture_list gtextures() const {
      return GTexture_list(_solid, _toon, _controlframe, _sils);
   }

   virtual void draw_filled_tris(double alpha);
   virtual void draw_non_filled_tris(double alpha);

   virtual int draw(CVIEWptr& v);
   virtual int draw_final(CVIEWptr& v);

   //******** RefImageClient METHODS ********

   virtual int draw_vis_ref();

   //******** DATA_ITEM VIRTUAL METHODS ********

   virtual DATA_ITEM  *dup() const { return new ControlLineTexture; }

 protected:
   SolidColorTexture*   _solid;
   ToonTexture_1D*      _toon;
   ControlFrameTexture* _controlframe;
   SilsTexture*         _sils;
};

#endif // CONTROL_LINE_H_IS_INCLUDED

// end of file control_line.H
