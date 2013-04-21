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
 * color_id_texture.C
 **********************************************************************/


#include "color_id_texture.H"
#include "ref_image.H"

/**********************************************************************
 * ColorIDStripCB:
 **********************************************************************/
void 
ColorIDStripCB::vertCB(CBvert* v)
{
   uint rgba = IDRefImage::key_to_rgba(v->key());
   glColor4ubv((GLubyte*)&rgba);

   glVertex3dv(v->loc().data());
}

void
ColorIDStripCB::edgeCB(CBvert* v, CBedge* e)
{
   uint rgba = IDRefImage::key_to_rgba(e->key());
   glColor4ubv((GLubyte*)&rgba);

   glVertex3dv(v->loc().data());
}

void 
ColorIDStripCB::faceCB(CBvert* v, CBface* f)
{
   uint rgba = IDRefImage::key_to_rgba(f->key());
   glColor4ubv((GLubyte*)&rgba);

   glVertex3dv(v->loc().data());
}

/*****************************************************************
 * ColorIDTexture
 *****************************************************************/
ColorIDTexture::ColorIDTexture(Patch* patch, GLenum pmode) :
   BasicTexture(patch, new ColorIDStripCB),
   _width(5),
   _polygon_mode(pmode),
   _black(new SolidColorTexture(patch, COLOR::black))
{
   _black->set_alpha(0);
}

void 
ColorIDTexture::push_gl_state(GLbitfield mask)
{
   // set attributes for this mode
   glPushAttrib(mask |
                GL_LIGHTING_BIT |       // shade model
                GL_CURRENT_BIT |        // current color
                GL_ENABLE_BIT);         // lighting, culling, blending

   glShadeModel(GL_FLAT);               // GL_LIGHTING_BIT
   glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
   glDisable(GL_BLEND);                 // GL_ENABLE_BIT
}

int
ColorIDTexture::draw(CVIEWptr& v)
{
   switch (_polygon_mode) {
    case GL_FILL:
      // filled triangles -- regular state
      // do backface culling for closed meshes
      push_gl_state(0);
      set_face_culling();                               // GL_ENABLE_BIT
    brcase GL_LINE:
      // wireframe triangles
      push_gl_state(GL_LINE_BIT | GL_POLYGON_BIT);
      glLineWidth(_width);                              // GL_LINE_BIT
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);        // GL_POLYGON_BIT
      glDisable(GL_CULL_FACE);                          // GL_ENABLE_BIT
    brcase GL_POINT:
      // drawing points for some strange reason
      push_gl_state(GL_POINT_BIT | GL_POLYGON_BIT);
      glPointSize(_width);                              // GL_POINT_BIT
      glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);       // GL_POLYGON_BIT
      glDisable(GL_CULL_FACE);                          // GL_ENABLE_BIT
    brdefault:
      err_msg( "ColorIDTexture::draw: error: unknown mode: %d", _polygon_mode);
      push_gl_state(0);
   }

   // draw triangles and/or polyline strips
   // use a display list if it's valid:
   if (!BasicTexture::draw(v)) {

      int dl = _dl.get_dl(v, 1, _patch->stamp());
      if (dl)
         glNewList(dl, GL_COMPILE);

      _patch->draw_tri_strips(_cb); 

      // restore gl state:
      glPopAttrib();

      // end the display list here
      if (_dl.dl(v)) {
         _dl.close_dl(v);

         // the display list is built; now execute it
         BasicTexture::draw(v);
      }
   }

   return _patch->num_faces();
}

int
ColorIDTexture::draw_edges(EdgeStrip* edges, GLfloat width, bool z_test)
{
   if (!edges)
      return 0;

   // need one of these for this static method:
   ColorIDStripCB  cb;

   // set regular state plus line width:
   push_gl_state(GL_LINE_BIT);
   if (!z_test)
      glDisable(GL_DEPTH_TEST); // GL_ENABLE_BIT
   glLineWidth(width);          // GL_LINE_BIT

   // draw edge strip
   edges->draw(&cb);

   // restore state
   glPopAttrib();

   // what do they want us to return?
   return edges->num();
}

int
ColorIDTexture::draw_verts(VertStrip* verts, GLfloat width, bool z_test)
{
   if (!verts)
      return 0;

   // need one of these for this static method:
   ColorIDStripCB  cb;

   // set regular state plus point size:
   push_gl_state(GL_POINT_BIT);
   if (!z_test)
      glDisable(GL_DEPTH_TEST); // GL_ENABLE_BIT
   glPointSize(width);          // GL_POINT_BIT

   // draw vert strip
   verts->draw(&cb);

   // restore state
   glPopAttrib();

   // what do they want us to return?
   return verts->num();
}

/* end of file color_id_texture.C */

