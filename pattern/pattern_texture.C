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
/***************************************************************************
    pattern_texture.C
 ***************************************************************************/

#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
#pragma warning(disable: 4786)
#endif

#include "disp/colors.H"
#include "geom/gl_view.H"
#include "geom/world.H"
#include "mesh/ledge_strip.H"
#include "mesh/bfilters.H"
#include "gtex/hidden_line.H"
#include "mesh/uv_data.H"
#include "stroke/base_stroke.H"

//#include "pattern_stroke.H"
#include "pattern_texture.H"

TAGlist *       PatternTexture::_pt_tags = 0;
static bool debug = Config::get_var_bool("DEBUG_TEXTURE",false);

/**********************************************************************
 * PatternTexture:
 **********************************************************************/
PatternTexture::PatternTexture(Patch* patch) :
      BasicTexture(patch, new GLStripCB)
{
   //_proxy_surface = new ProxySurface(patch);
   _base =  new SmoothShadeTexture(patch);
}

PatternTexture::~PatternTexture()
{
}

void
PatternTexture::set_patch(Patch* p) {
   
   GTexture::set_patch(p);
   _base->set_patch(p);

   
   //_proxy_surface->set_patch(p);

   //VIEW::peek()->set_color(COLOR(.72,.725,.56)); 
   VIEW::peek()->cam()->data()->add_cb(this);   
}
    
int
PatternTexture::draw(CVIEWptr& v)
{
    _base->draw(v);
   //_proxy_surface->draw(v);
  return 0;
}

int
PatternTexture::draw_final(CVIEWptr& v)
{
    _base->draw_final(v);
   //GTexture::draw_final(v);
  // _proxy_surface->draw_final(v);
 
   return 0;     
}

int
PatternTexture::draw_id_ref()
{ 
   int n=0; 
  // cerr << "PatternTexture::draw_id_ref" << endl; 
   n +=  draw_id_triangles(true, true);//_base->draw(VIEW::peek());//draw_id_triangles(true, true); // //draw_id_triangles(true, true); 
   return n;
}

int
PatternTexture::draw_color_ref(int i)
{
   return (i == 0) ? _base->draw(VIEW::peek()) : 0;
}

void 
PatternTexture::request_ref_imgs()
{
   // to update this correctly we need to know what '3' means...
//   return (ref_img_t)(3); 
} 


CTAGlist &
PatternTexture::tags() const
{
   if (!_pt_tags) {
      _pt_tags = new TAGlist;
   }
   return *_pt_tags;
}

/* end of file pattern_texture.C */
