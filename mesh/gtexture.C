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
/*****************************************************************
 * gtexture.C
 *****************************************************************/
#include "mesh/gtexture.H"
#include "mesh/patch.H"
#include "mlib/points.H"

Cstr_ptr GTexture::_type_name("GTEXTURE");
Cstr_ptr GTexture::_begin_tag(str_ptr("#BEGIN ") + _type_name + str_ptr("\n"));
Cstr_ptr GTexture::_end_tag  (str_ptr("\n#END ") + _type_name + str_ptr("\n"));

// GTexture is not serialized into jot files, so an empty TAGlist is created
CTAGlist &
GTexture::tags() const
{
   static TAGlist* basic_tags = 0;

   if (!basic_tags) {
      basic_tags = new TAGlist(0);
   }
   return *basic_tags;
}

GTexture::GTexture(Patch* p, StripCB* cb) :
   _patch(p),
   _cb(cb),
   _ctrl(0)
{
   changed(); 
}

void 
GTexture::set_patch(Patch* p) 
{
   _patch = p;
   gtextures().set_patch(p);
   changed();
}

double 
GTexture::alpha() const 
{
   // include alpha from the patch, if any:
   double a = _patch ? _patch->transp() : 1.0;
   return a * (_alphas.empty() ? 1.0 : _alphas.last());
}

// end of file gtexture.C
