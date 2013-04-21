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
 * key_line.H:
 **********************************************************************/
#ifndef KEY_LINE_H_IS_INCLUDED
#define KEY_LINE_H_IS_INCLUDED

#include "basic_texture.H"
#include "glsl_toon.H" 

class SolidColorTexture;
class GLSLToonShader;
class SilFrameTexture;
/**********************************************************************
 * KeyLineTexture:
 *
 *    Render Patch silhouettes with a solid background color
 *
 **********************************************************************/
class KeyLineTexture : public OGLTexture {
 public:

   //******** MANAGERS ********

   KeyLineTexture(Patch* patch = 0);
   virtual ~KeyLineTexture();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Key Line", KeyLineTexture*, OGLTexture, CDATA_ITEM *);

   //******** STATICS ********

   // Whether to show hidden lines or not:
   static void set_show_hidden(bool show) { _show_hidden = show; }

   static bool toggle_show_hidden_lines() {
      return (_show_hidden = !_show_hidden);
   }

   //******** ACCESSORS ********

   SolidColorTexture* solidcolor() { return _solid; }
   GLSLToonShader*    toon()       { return _toon; }
   SilFrameTexture*   sil_frame()  { return _sil_frame;  }

   virtual int set_color(CCOLOR& c);
   int set_sil_color(CCOLOR& c);

   // Set the toon shader to use (or null if none should be used).
   // The toon shader is owned by this GTexture; its destructor
   // will delete the toon shader:
   void set_toon(GLSLToonShader*);

   //******** GTexture VIRTUAL METHODS ********

   virtual GTexture_list gtextures() const;

   virtual void draw_filled_tris(double alpha);
   virtual void draw_non_filled_tris(double alpha);

   virtual int draw(CVIEWptr& v); 

   //******** DATA_ITEM METHODS ********

   virtual DATA_ITEM  *dup() const { return new KeyLineTexture; }

 protected:
   SolidColorTexture* _solid;         // draws filled triangles, no shading
   GLSLToonShader*    _toon;          // toon shader, goes on top of solid color
   SilFrameTexture*   _sil_frame;     // draws silhouettes and creases
   static bool        _show_hidden;   // if true, show hidden lines (dotted)

   // Internal method for drawing the hidden lines:
   int draw_hidden(CVIEWptr& v);
};

#endif // KEY_LINE_H_IS_INCLUDED

// end of file key_line.H

