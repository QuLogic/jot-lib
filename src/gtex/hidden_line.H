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
 * hidden_line.H:
 **********************************************************************/
#ifndef HIDDEN_LINE_H_IS_INCLUDED
#define HIDDEN_LINE_H_IS_INCLUDED

#include "geom/gl_view.H"

#include "solid_color.H"
#include "wireframe.H"

/**********************************************************************
 * HiddenLineTexture:
 **********************************************************************/
class HiddenLineTexture : public OGLTexture {
 public:

   //******** MANAGERS ********

   HiddenLineTexture(Patch* patch = 0) :
      OGLTexture(patch),
      _solidcolor(new SolidColorTexture(patch, COLOR::white)),
      _wireframe( new WireframeTexture (patch, COLOR::black)) {}

   virtual ~HiddenLineTexture() { gtextures().delete_all(); }

   //******** RUN-TIME TYPD ID ********

   DEFINE_RTTI_METHODS3("Hidden Line", HiddenLineTexture*,
                        OGLTexture, CDATA_ITEM *);

   //******** GTexture VIRTUAL METHODS ********

   virtual GTexture_list gtextures() const {
      return GTexture_list(_solidcolor, _wireframe);
   }

   virtual void draw_filled_tris(double alpha) {
      _solidcolor->draw_with_alpha(alpha);
   }
   virtual void draw_non_filled_tris(double alpha) {
      _wireframe->draw_with_alpha(alpha);
   }

   // draw the solid stuff first, all patches agree
   // (see draw_final):
   virtual int draw(CVIEWptr& v) {
      if (_ctrl)
         return _ctrl->draw(v);

      return _solidcolor->draw(v);
   }

   // draw the wireframe stuff last, only after
   // every patch has drawn solid. avoid ugly
   // blending artifacts that way.
   virtual int draw_final(CVIEWptr& v) {
      if (_ctrl)
         return _ctrl->draw_final(v);

      // draw final tends not to use the z-buffer, but we need it:
      glPushAttrib(GL_ENABLE_BIT);
      glEnable(GL_DEPTH_TEST);
      _wireframe->draw(v);
      glPopAttrib();
      return _patch->num_faces();
   }

   int  set_wire_color(CCOLOR& c) { return _wireframe->set_color(c); }
   CCOLOR& wire_color()              const{ return _wireframe->color(); }
   
   virtual int  set_color(CCOLOR& c)    { return _solidcolor->set_color(c); }
   CCOLOR& color()              const   { return _solidcolor->get_color(); }

   void  set_width(float w)             { _wireframe->set_width(w); }
   float width()              const     { return _wireframe->width(); }

   //******** DATA_ITEM VIRTUAL METHODS ********

   virtual DATA_ITEM  *dup() const { return new HiddenLineTexture; }

 protected:
   SolidColorTexture* _solidcolor;
   WireframeTexture*  _wireframe;
};

#endif // HIDDEN_LINE_H_IS_INCLUDED

// end of file hidden_line.H
