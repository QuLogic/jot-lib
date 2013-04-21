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
#ifndef __SHADOWABLE_H__
#define __SHADOWABLE_H__
#include "mlib/points.H"
#include "geom/geom.H"
#ifndef SHADOW_PTR_DEFINED
#define SHADOW_PTR_DEFINED
MAKE_PTR_SUBC(Shadow, GEOM);
#endif

class Shadowable {
 public:
   //**** PUBLIC INTERFACE ****
   void init_shadow(const mlib::Wplane &pln);
   void show_shadow(); 
   void hide_shadow(); 
 protected:
   virtual ~Shadowable() {}

   //**** OVERRIDES ****
   virtual Shadowptr      create_shadow(const mlib::Wplane &pln) = 0;
   virtual void           cast_shadow() = 0;
   virtual GEOMptr        get_geom() = 0;
   
   //**** MEMBER VARIABLES ****
   REFptr<Shadow> _shadow;
};
#endif
