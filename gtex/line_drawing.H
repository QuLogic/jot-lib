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
#ifndef LINE_DRAWING_H_IS_INCLUDED
#define LINE_DRAWING_H_IS_INCLUDED

/*!
 *  \file line_drawing.H
 *  \brief Contains the definitions of the classes that implement the
 *  "Line Drawing" rendering style GTexture.
 *
 *  \sa line_drawing.C
 *
 */

#include "gtex/basic_texture.H"
#include "gtex/solid_color.H"

/*!
 *  \brief GTexture that implements the "Line Drawing" rendering style.
 *
 */
class LineDrawingTexture : public BasicTexture {
   
 public:
   
   //! \name Constructors
   //@{
      
   LineDrawingTexture(Patch* patch = 0, StripCB* cb=0);
      
   ~LineDrawingTexture();
      
   //@}
      
   //! \name RTTI Related
   //@{
      
   DEFINE_RTTI_METHODS3("Line Drawing", LineDrawingTexture*,
                        BasicTexture, CDATA_ITEM *);
      
   //@}
      
   //! \name GTexture Virtual Methods
   //@{
      
   virtual int draw(CVIEWptr& v); 
      
   //@}
      
   //! \name DATA_ITEM Virtual Methods
   //@{
      
   virtual DATA_ITEM  *dup() const { return new LineDrawingTexture; }
      
   //@}
      
   //! \name Style Parameter Accessors
   //@{
      
   static bool get_draw_in_color()
      { return draw_in_color; }
   static void set_draw_in_color(bool draw_in_color_in)
      { draw_in_color = draw_in_color_in; }
   static bool get_draw_contours()
      { return draw_contours; }
   static void set_draw_contours(bool draw_contours_in)
      { draw_contours = draw_contours_in; }
   static bool get_draw_sugcontours()
      { return draw_sugcontours; }
   static void set_draw_sugcontours(bool draw_sugcontours_in)
      { draw_sugcontours = draw_sugcontours_in; }
   static float get_sugcontour_thresh()
      { return sugcontour_thresh; }
   static void set_sugcontour_thresh(float sugcontour_thresh_in)
      { sugcontour_thresh = sugcontour_thresh_in; }
      
   //@}
   
 private:
   
   SolidColorTexture *solid_color_texture;
      
   static bool draw_in_color;       //!< Draw contours in color?
   static bool draw_contours;       //!< Draw contours (a.k.a. silhouettes)?
   static bool draw_sugcontours;    //!< Draw suggestive contours?
   static float sugcontour_thresh;  //!< Suggestive contour threshold

};

#endif // LINE_DRAWING_H_IS_INCLUDED
