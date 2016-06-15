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
#ifndef GEOM_OBS_H_HAS_BEEN_INCLUDED
#define GEOM_OBS_H_HAS_BEEN_INCLUDED

#include "geom/geom.hpp"
#include <map>
#include <set>

//--------------------------------------------
// XFORMobs -
//   An object that can be notified when some
// other object is transformed
//--------------------------------------------
class XFORMobs;
typedef set<XFORMobs*>       XFORMobs_list;
typedef const XFORMobs_list CXFORMobs_list;
typedef const XFORMobs      CXFORMobs;
class XFORMobs {
 public:
   enum STATE { START = 0, // Start of manipulation
                MIDDLE,    // Middle of manipulation
                END,       // End of manipulation
                PRIMARY,   // top-level calls to set_xform/mult_by
                GRAB,      // Start of spreadsheet drag & drop
                DRAG,      // Spreadsheet drag 
                DROP,      // Spreadsheet drop
                NET,       // xform from the network
                EVERY 
   };
   virtual ~XFORMobs() {}

   // called when a GEOM was transformed:
   virtual void notify_xform(CGEOMptr&, STATE state) = 0;

   // send message that a GEOM was transformed
   static  void notify_xform_obs      (CGEOMptr&, STATE start);
   static  void notify_xform_every_obs(CGEOMptr&);

   // add/remove this observer to list for the given GEL:
   void   xform_obs(CGELptr& g)  { xform_obs_list(g).insert(this); }
   void unobs_xform(CGEOMptr &g) { xform_obs_list(g).erase(this); }

   // add/remove this observer to list for all GEOMs:
   void   xform_obs() { _all_xf.insert(this); }
   void unobs_xform() { _all_xf.erase(this); }

   // add/remove this observer to list for every transforms:
   // XXX - todo: figure out what this is for:
   void   every_xform_obs() { _every_xf.insert(this); }
   void unobs_every_xform() { _every_xf.erase(this); }

 protected :
   static map<CGELptr,XFORMobs_list*> _hash_xf;
   static XFORMobs_list _all_xf;
   static XFORMobs_list _every_xf;

   static XFORMobs_list &xform_obs_list(CGELptr &g)  {
      map<CGELptr,XFORMobs_list*>::iterator it = _hash_xf.find(g);
      XFORMobs_list *list;
      if (it == _hash_xf.end()) {
         list = new XFORMobs_list;
         _hash_xf[g] = list;
      } else
         list = it->second;
      return *list;
   }
};

#endif // GEOM_OBS_H_HAS_BEEN_INCLUDED
