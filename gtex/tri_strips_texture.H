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
 * tri_strips_texture.H:
 **********************************************************************/
#ifndef TRI_STRIPS_TEXTURE_H_IS_INCLUDED
#define TRI_STRIPS_TEXTURE_H_IS_INCLUDED

#include "flat_shade.H"

/**********************************************************************
 * TriStripsTexture:
 *
 *      Used to visualize the pattern of triangle and edge
 *      strips. Draws each triangle strip and edge strip in a
 *      randomly chosen color.  But not too random -- the same
 *      strip will have the same color each time it is drawn.
 *
 *      TriStripsTexture is really just a FlatShadeTexture,
 *      except it uses a specialized strip callback, StripColorCB,
 *      to set distinct colors when starting each strip.
 **********************************************************************/
class StripColorCB : public FlatShadeStripCB {
 protected:

   bool _starting;    // set to true in begin_edges(EdgeStrip* e)

   // _starting is used within an edge strip to set a color
   // randomly on the first edge drawn after starting a new
   // piece of the strip. this is needed because a single
   // edge strip can consist of multiple distinct pieces. to
   // get a consistent, distinct color per *piece* we need
   // to seed the random number generator not with the
   // address of the strip itself, but with the address of
   // the leading edge of each piece in turn.

 public:
   StripColorCB() : _starting(0) {}

   //******** TRI STRIPS ********
   virtual void begin_faces(TriStrip* t) {
      // set a random color per strip
      // choose same color next time for this strip
      srand48((long) t);        
      glColor4d(drand48(), drand48(), drand48(), alpha);
      GLStripCB::begin_faces(t);    // glBegin(GL_TRIANGLE_STRIP);
   }
//    virtual void faceCB(CBvert* v, CBface* f) {
//       // no colors or texture coordinates
//       glNormal3dv(f->norm().data());    // face normals (for flat shading)
//       glVertex3dv(v->loc().data());     // vertex coordinates
//    }

   //******** EDGE STRIPS ********
   virtual void begin_edges(EdgeStrip* e) {
      // get ready to set a random color per
      // each new piece of the edge strip
      _starting = 1;
      GLStripCB::begin_edges(e);    // glBegin(GL_LINE_STRIP); 
   }
   virtual void edgeCB(CBvert* v, CBedge* e) {
      // if this is the beginning of a new piece
      // of the edge strip, set a color
      if (_starting) {
         _starting = 0;         // use it up
         srand48((long) e);     // set the color
         glColor4d(drand48(), drand48(), drand48(), alpha);
      }
      GLStripCB::edgeCB(v,e);
   }

   //******** VERT STRIPS ********
   virtual void begin_verts(VertStrip* v) {
      // set a random color per strip
      // choose same color next time for this strip
      srand48((long) v);        
      glColor4d(drand48(), drand48(), drand48(), alpha);
      GLStripCB::begin_verts(v);    // glBegin(GL_POINTS);
   }
};

/**********************************************************************
 * TriStripsTexture:
 *
 *      It's really just a FlatShadeTexture with a different
 *      triangle strip callback implementation.
 **********************************************************************/
class TriStripsTexture : public FlatShadeTexture {
 public:
   TriStripsTexture(Patch* patch = 0) :
      FlatShadeTexture(patch, new StripColorCB) {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("Show tri-strips", BasicTexture, CDATA_ITEM *);

   //******** GTexture VIRTUAL METHODS ********
 
  // A new color is set inside the display list when each
   // triangle strip is drawn. That's why the display list
   // needs to be invalidated when alpha changes.
   virtual void push_alpha(double a)  {
      FlatShadeTexture::push_alpha(a);
      changed();
   }
   virtual void pop_alpha() {
      FlatShadeTexture::pop_alpha();
      changed();
   }

   // show the crease And border strips in addition to
   // triangle strips:
   virtual int draw(CVIEWptr& v) {
      glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
      glDisable(GL_LIGHTING);                   // GL_ENABLE_BIT
      glLineWidth(5);                           // GL_LINE_BIT
      if (_patch && _patch->cur_creases())
         _patch->cur_creases()->draw(_cb);      // GL_CURRENT_BIT
      if (_patch && _patch->cur_borders())
         _patch->cur_borders()->draw(_cb);      // GL_CURRENT_BIT
      glPopAttrib();
      return FlatShadeTexture::draw(v);
   }

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new TriStripsTexture; }
};

#endif // TRI_STRIPS_TEXTURE_H_IS_INCLUDED

/* end of file tri_strips_texture.H */
