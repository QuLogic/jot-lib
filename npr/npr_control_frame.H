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
 * npr_control_frame.H:
 **********************************************************************/
#ifndef NPR_CONTROL_FRAME_H_IS_INCLUDED
#define NPR_CONTROL_FRAME_H_IS_INCLUDED

#include "gtex/basic_texture.H"

class BaseStroke;

/**********************************************************************
 * NPRControlFrameTexture:
 **********************************************************************/
class NPRControlFrameTexture : public BasicTexture {
 private:
   //******** STATIC MEMBER VARIABLES ********
   static bool          _show_box;
   static bool          _show_wire;
 public:
   //******** STATIC METHODS ********
   static void          next_select_mode();

   //******** MEMBER VARIABLES ********
 protected:
   COLOR                _color;         
   double               _alpha;
   EdgeStrip*           _strip;         
   ARRAY<BaseStroke*>   _strokes;

 public:
   //******** MANAGERS ********
   NPRControlFrameTexture(Patch* patch = 0, CCOLOR& col = COLOR::blue);
   virtual ~NPRControlFrameTexture();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("NPRControlFrame", BasicTexture, CDATA_ITEM *);

   //******** ACCESSORS ********
   int     set_color(CCOLOR& c)           { _color = c; return 1; }
   CCOLOR& get_color() const              { return _color;        }
   void    set_alpha(double a)            { _alpha = a;           }
   double  get_alpha() const              { return _alpha;        }

   //******** GTexture METHODS ********
   virtual bool draws_filled() const      { return false;         }
   virtual int  draw(CVIEWptr& v); 
   virtual int  draw_final(CVIEWptr& v);

   //******** RefImageClient METHODS ********
   virtual int draw_vis_ref() 
   {
      cerr << "NPRControlFrameTexture::draw_vis_ref: not implemented" << endl;
      return 0;
   }

   //******** DATA_ITEM METHODS ********
   virtual DATA_ITEM  *dup() const { return new NPRControlFrameTexture; }

};

#endif // NPR_CONTROL_FRAME_H_IS_INCLUDED

/* end of file npr_control_frame.H */
