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
#include "base_ref_image.H"

map<VIEWimpl*,BaseVisRefImage*> BaseVisRefImage::_hash;
BaseVisRefImageFactory*         BaseVisRefImage::_factory = nullptr;

BaseVisRefImage* 
BaseVisRefImage::lookup(CVIEWptr& v) 
{
   // Used instead of a public constructor. Lookup the BaseVisRefImage
   // associated w/ a view.  (If the view could know about this type,
   // we would probably just store the it on the view.)

   if (!v) {
      err_msg("BaseVisRefImage::lookup: error -- view is nil");
      return nullptr;
   }

   // hash on the view implementation rather than the view itself
   map<VIEWimpl*,BaseVisRefImage*>::iterator it;
   VIEWimpl *key = v->impl();
   it = _hash.find(key);
   BaseVisRefImage *ret = nullptr;
   if (it != _hash.end())
      ret = it->second;
   else if (_factory && (ret = _factory->produce(v)))
      _hash[key] = ret;

   return ret;
}
