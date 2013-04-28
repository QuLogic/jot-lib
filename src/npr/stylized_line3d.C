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
#include "disp/ray.H"
#include "geom/gl_util.H"
#include "geom/world.H"
#include "gtex/ref_image.H"
#include "mesh/mi.H"    

#include "ffstexture.H"
#include "stylized_line3d.H"

using mlib::CWpt_list;

inline bool 
is_stylized() 
{
   return VIEW::peek()->rendering() == FFSTexture::static_name() ||
      VIEW::peek()->rendering() == "FFSTexture2";
}

StylizedLine3D::StylizedLine3D(CWpt_list& pts) :
   LINE3D(pts),
   _got_style(false),
   _stroke_3d(new WpathStroke(NULL))
{
}

StylizedLine3D::~StylizedLine3D() 
{
   delete _stroke_3d;
}

void 
StylizedLine3D::get_style ()
{
   if (_got_style)
      return;

   // get line3d style from ffs_style file:
   _stroke_3d->get_style("nprdata/ffs_style/line3d.ffs");

   _got_style = true;
}

int
StylizedLine3D::draw(CVIEWptr& v) 
{
   if (is_stylized())
      return 0;
   return LINE3D::draw(v);
}

int
StylizedLine3D::draw_final(CVIEWptr& v)
{     
   // draw stylized stoke only if it;s FFSTexture
   if (is_stylized()) {
      get_style();
      return _stroke_3d->draw(v);
   }
   return 0;
}

void
StylizedLine3D::request_ref_imgs()
{
   if (is_stylized())
      IDRefImage::schedule_update();
}

int
StylizedLine3D::draw_id_ref_pre1()
{
   if (is_stylized()){
      _stroke_3d->set_polyline(_pts);
      return _stroke_3d->draw_id_ref_pre1();
   } else {
      return 0;
   } 
}

int
StylizedLine3D::draw_id_ref_pre2()
{
   if (is_stylized())
      return _stroke_3d->draw_id_ref_pre2();
   else
      return 0;
}

int
StylizedLine3D::draw_id_ref_pre3()
{
   if (is_stylized())
      return _stroke_3d->draw_id_ref_pre3();
   else
      return 0;
}

int
StylizedLine3D::draw_id_ref_pre4()
{
   if (is_stylized())
      return _stroke_3d->draw_id_ref_pre4();
   else
      return 0;
}

// end of file line3d.C
