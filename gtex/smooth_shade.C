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
/*!
 *  \file smooth_shade.C
 *  \brief Contains the implementation of the SmoothShadeTexture GTexture and
 *  related classes.
 *
 *  \sa smooth_shade.H
 *
 */

#include "geom/texturegl.H"
#include "mesh/uv_data.H"

#include "smooth_shade.H"

/**********************************************************************
 * SmoothShadeStripCB:
 **********************************************************************/
void 
SmoothShadeStripCB::faceCB(CBvert* v, CBface* f) 
{
   using mlib::Wvec;
   // normal
   Wvec n;
   glNormal3dv(f->vert_normal(v,n).data());

   if (v->has_color())
      GL_COL(v->color(), alpha*v->alpha());

   // texture coords
   if (do_texcoords) {
      // the patch has a texture... try to find
      // appropriate texture coordinates...

      // use patch's TexCoordGen if possible,
      // otherwise use the texture coordinates stored
      // on the face (if any):
      TexCoordGen* tg = f->patch()->tex_coord_gen();
      if (tg)
         glTexCoord2dv(tg->uv_from_vert(v,f).data());
      else if (UVdata::lookup(f))
         glTexCoord2dv(UVdata::get_uv(v,f).data());
   }

   // vertex coords
   glVertex3dv(v->loc().data());
}

/**********************************************************************
 * SmoothShadeTexture:
 **********************************************************************/
int
SmoothShadeTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);
   _cb->alpha = alpha();

   // XXX - dumb hack
   check_patch_texture_map();

   // this is a no-op unless needed:
   // (don't put it inside display list creation):
   _patch->apply_texture();

   // set gl state (lighting, shade model)
   glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

   glEnable(GL_LIGHTING);                       // GL_LIGHTING_BIT
   glShadeModel(GL_SMOOTH);                     // GL_LIGHTING_BIT

//    // XXX - for debugging
//    glEnable(GL_DEPTH_TEST);

   // set color (affects GL_CURRENT_BIT):
   GL_COL(_patch->color(), alpha()); // GL_CURRENT_BIT
   
   // set material:
   GL_MAT_COLOR(GL_FRONT_AND_BACK, GL_AMBIENT, _patch->ambient_color(), alpha());
   GL_MAT_COLOR(GL_FRONT_AND_BACK, GL_DIFFUSE, _patch->color(), alpha());
   GL_MAT_COLOR(GL_FRONT_AND_BACK, GL_SPECULAR, _patch->specular_color(), alpha());
   glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, static_cast<GLfloat>(_patch->shininess()));

   // execute display list if it's valid:
   if (BasicTexture::dl_valid(v)) {
      BasicTexture::draw(v);

      // restore gl state:
      glPopAttrib();

      return _patch->num_faces();
   }


   // try to generate a display list
   int dl = _dl.get_dl(v, 1, _patch->stamp());
   if (dl)
      glNewList(dl, GL_COMPILE);

   // set up face culling for closed surfaces
   if (!set_face_culling()) 
      glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);  // GL_LIGHTING_BIT
   
   SmoothShadeStripCB *smooth_cb = dynamic_cast<SmoothShadeStripCB*>(_cb);

   // turn on texturing if any
   if (_patch->has_texture()){
      if (smooth_cb) smooth_cb->enable_texcoords();
   } else {
      if(smooth_cb) smooth_cb->disable_texcoords();
   }

   // draw the triangle strips
   _patch->draw_tri_strips(_cb);

   // end the display list here
   if (_dl.dl(v)) {
      _dl.close_dl(v);

      // the display list is built; now execute it
      BasicTexture::draw(v);
   }

   // restore gl state:
   glPopAttrib();

   return _patch->num_faces();
}

/* end of file smooth_shade.C */
