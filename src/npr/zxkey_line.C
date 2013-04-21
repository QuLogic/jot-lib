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
 * zkey_line.C
 **********************************************************************/


#include "geom/gl_view.H"
#include "gtex/solid_color.H"
#include "mesh/patch.H"

#include "zxedge_stroke_texture.H"
#include "zxkey_line.H"

/**********************************************************************
 * ZkeyLineTexture:
 **********************************************************************/
ZkeyLineTexture::ZkeyLineTexture(Patch* patch) :
   OGLTexture(patch)
{
   // Set up the base coat as a SolidColorTexture that tracks
   // the background color of the view:
   SolidColorTexture* solid = new SolidColorTexture(patch);
   solid->set_track_view_color(true);
   _base_coat = solid;

   // And get a silhouette stroke texture for the hard job:
   _zx_stroke = new ZXedgeStrokeTexture(patch);
}

ZkeyLineTexture::~ZkeyLineTexture()
{
   delete _base_coat;
   delete _zx_stroke;
}

int
ZkeyLineTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);
   // We draw the strokes in the final pass so they don't get
   // occluded by other objects.

   return _base_coat->draw(v);
}

void 
ZkeyLineTexture::request_ref_imgs()
{
   IDRefImage::schedule_update();
}

int
ZkeyLineTexture::draw_final(CVIEWptr& v) 
{ 
   _zx_stroke->draw(v);
   return 1;
}

int
ZkeyLineTexture::draw_id_ref() 
{ 
   _base_coat->draw_id_ref();
   _zx_stroke->draw_id_ref();

   return 1;
}

int
ZkeyLineTexture::set_sil_color(CCOLOR& c)
{
   _zx_stroke->set_color(c);
   return 1;
}

void
ZkeyLineTexture::set_patch(Patch* p)
{
   GTexture::set_patch(p);
   _base_coat->set_patch(p);
   _zx_stroke->set_patch(p);
}

void 
ZkeyLineTexture::push_alpha(double a)  
{
   _base_coat->push_alpha(a);
   _zx_stroke->push_alpha(a);
}

void 
ZkeyLineTexture::pop_alpha() 
{
   _base_coat->pop_alpha();
   _zx_stroke->pop_alpha();
}

void 
ZkeyLineTexture::draw_filled_tris(double alpha) 
{
   _base_coat->draw_with_alpha(alpha);
}

void 
ZkeyLineTexture::draw_non_filled_tris(double alpha) 
{
   _zx_stroke->draw_with_alpha(alpha);
}

/* end of file key_line.C */
