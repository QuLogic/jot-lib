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
//skybox texture
//by Karol Szerszen

#include "geom/gl_view.hpp"
#include "mesh/uv_data.hpp"
#include "skybox_texture.hpp"
#include "gtex/gl_sphir_tex_coord_gen.hpp"

/*****************************************************************
 * StripTexCoordsCB:
 *
 *   Sets texture coordinates in triangle strip rendering.
 *****************************************************************/
class StripTexCoordsCB : public GLStripCB {
 public:

   GLSphirTexCoordGen _auto_UV;

   //******** TRI STRIPS ********
   virtual void faceCB(CBvert* v, CBface* f) {
      //texture coordinates
      assert(v && f);


      //glTexCoord2dv( _auto_UV.uv_from_vert(v,f).data());
  
      //glTexCoord2dv(f-> tex_coord(v).data());   
      glTexCoord2dv(UVdata::get_uv(v,f).data());
      glVertex3dv(v->loc().data());     // vertex coordinates
   }
};

/*****************************************************************
 * Utilities
 *****************************************************************/
inline string
gradient_filename()
{
   return Config::JOT_ROOT() +
      Config::get_var_str("SKYBOX_TEX_GRADIENT",
                          "nprdata/sky_textures/sky6.png");
}

inline void
init_params(TEXTUREglptr& tex)
{
   // set texture parameters
   if (tex) {
      tex->set_wrap_r(GL_REPEAT);
      tex->set_wrap_s(GL_REPEAT);
      tex->set_wrap_t(GL_REPEAT);
   }
}

/*****************************************************************
 * Skybox_Texture
 *****************************************************************/
Skybox_Texture::Skybox_Texture(Patch* patch) :
   BasicTexture(patch, new StripTexCoordsCB)
{
   CTEXTUREglptr tex = make_shared<TEXTUREgl>(gradient_filename());
   assert(tex);
   set_tex(tex);
   if (!_tex->load_image()) {
      cerr << " Skybox_Texture : load_image failed on "
           << _tex->file() << endl;
   }

   _tex->set_wrap_r(GL_CLAMP);
   _tex->set_wrap_s(GL_CLAMP);
   _tex->set_wrap_t(GL_CLAMP);
}

void 
Skybox_Texture::set_tex(CTEXTUREglptr& tex)
{
   // set the TEXTUREgl to use:
   _tex = tex;
   init_params(_tex);
}

int
Skybox_Texture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);

   // try it with the display list:
   // push GL state before changing things
   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);

   // Set the color
   GL_COL(COLOR::white, alpha()); // GL_CURRENT_BIT
   glDisable(GL_LIGHTING);        // GL_ENABLE_BIT
   set_face_culling();            // GL_ENABLE_BIT

   //put the texture on skybox
   if (_tex)
      _tex->apply_texture();         // GL_TEXTURE_BIT, GL_ENABLE_BIT

   if (!BasicTexture::draw(v)) {
      // ensure this never happens again! get a display list:
      int dl = _dl.get_dl(v, 1, _patch->stamp());
      if (dl) {
         //------------------Building skybox display list START
         glNewList(dl, GL_COMPILE); 
      }

      // draw the triangle strips
      _patch->draw_tri_strips(_cb);
    
      // end the display list here
      if (_dl.dl(v)) {
         //---------------------------Building skybox display list END
         _dl.close_dl(v); 

         // the display list is built; now execute it
         BasicTexture::draw(v);
         GL_VIEW_PRINT_GL_ERRORS(class_name() + "::Draw_gradient");
      }
   }

   // restore gl state:
   glPopAttrib();

   return _patch->num_faces();
}

void 
Skybox_Texture::load_texture(string file_name)
{
   assert(_tex);
   _tex->set_texture(file_name);
    
   if (!_tex->load_image()) {
      cerr << " Skybox_Texture : load_image failed on "
           << _tex->file() << endl;
   }
}
