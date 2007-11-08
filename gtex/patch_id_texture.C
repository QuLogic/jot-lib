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
 * patch_id_texture.C
 **********************************************************************/


#include "patch_id_texture.H"
#include "ref_image.H"

/**********************************************************************
 * PatchIDStripCB:
 **********************************************************************/
void 
PatchIDStripCB::faceCB(CBvert* v, CBface* f)
{
   uint rgba = IDRefImage::key_to_rgba(uint(f->patch()));
   glColor4ubv((GLubyte*)&rgba);

   glVertex3dv(v->loc().data());
}

/*****************************************************************
 * PatchIDTexture
 *****************************************************************/
PatchIDTexture::PatchIDTexture(Patch* patch, GLenum pmode) :
   BasicTexture(patch, new PatchIDStripCB),
   _width(5),
   _polygon_mode(pmode),
   _black(new SolidColorTexture(patch, COLOR::black)) 
{
   _black->set_alpha(0);
}

int
PatchIDTexture::draw(CVIEWptr& v)
{
   // filled triangles -- regular state
   // do backface culling for closed meshes
   glPushAttrib( GL_LIGHTING_BIT |       // shade model
                GL_CURRENT_BIT |        // current color
                GL_ENABLE_BIT);         // lighting, culling, blending

   glShadeModel(GL_FLAT);               // GL_LIGHTING_BIT
   glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
   glDisable(GL_BLEND);                 // GL_ENABLE_BIT
   set_face_culling();                               // GL_ENABLE_BIT

   // draw triangles and/or polyline strips
   // use a display list if it's valid:

   _patch->draw_tri_strips(_cb); 

   // restore gl state:
   glPopAttrib();


   return _patch->num_faces();
}



/* end of file color_id_texture.C */

