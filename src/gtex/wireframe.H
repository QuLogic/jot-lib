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
 * wireframe.H:
 **********************************************************************/
#ifndef WIREFRAME_H_IS_INCLUDED
#define WIREFRAME_H_IS_INCLUDED

#include "color_id_texture.H"

/**********************************************************************
 * WireframeTexture:
 *
 *      Draws unlit triangles in wireframe --
 *      i.e. glPolygonMode(GL_LINE)
 **********************************************************************/
class WireframeTexture : public BasicTexture {
 public:
   //******** MANAGERS ********
   WireframeTexture(Patch* patch = 0, CCOLOR& col = default_line_color()) :
      BasicTexture(patch, new GLStripCB),
      _width(0.5),
      _color(col),
      _color_id(new ColorIDTexture(patch, GL_LINE)),
      _quad_diagonals_in_dl(0) {
      _color_id->set_width(5);
   }
   virtual ~WireframeTexture() { gtextures().delete_all(); }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3(
      "Wireframe", WireframeTexture*, BasicTexture, CDATA_ITEM*
      );

   //******** STATICS ********

   static bool show_quad_diagonals() { return _show_quad_diagonals; }
   static void toggle_show_quad_diagonals() {
      _show_quad_diagonals = !_show_quad_diagonals;
   }

   static CCOLOR& default_line_color() { return _default_line_color; }
   static void    set_default_line_color(CCOLOR& color);

   //******** ACCESSORS ********

   CCOLOR& color()              const   { return _color; }
   float width()                const   { return _width; }
   void  set_width(float w)             { _width = w;    }

   //******** GTexture METHODS ********

   virtual GTexture_list gtextures() const { return GTexture_list(_color_id); }

   virtual int  set_color(CCOLOR& c)    { _color = c; return 1; }
   virtual bool draws_filled()  const;
   virtual int  draw(CVIEWptr& v); 

   // only here for debugging; mostly a no-op:
   virtual void request_ref_imgs();

   //******** RefImageClient METHODS ********
   virtual int draw_vis_ref() { return _color_id->draw(VIEW::peek()); }

   //******** DATA_ITEM METHODS ********
   virtual DATA_ITEM  *dup() const { return new WireframeTexture; }

 protected:
   float                _width;
   COLOR                _color;         // color to use
   ColorIDTexture*      _color_id;      // for drawing visibility
   bool                 _quad_diagonals_in_dl;
   static bool          _show_quad_diagonals;
   static COLOR         _default_line_color;

   // for debugging d2d samples:
   void draw_d2d_samples();
};

#endif // WIREFRAME_H_IS_INCLUDED

// end of file wireframe.H
