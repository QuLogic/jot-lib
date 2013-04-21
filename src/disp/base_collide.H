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
#ifndef BASE_COLLIDE_H_IS_INCLUDED
#define BASE_COLLIDE_H_IS_INCLUDED

#include "mlib/points.H"

/*****************************************************************
 * BaseCollide:
 *
 *      Checks for collisions with objects in space
 *****************************************************************/

class BaseCollide {
 public :

   virtual ~BaseCollide() {}
   BaseCollide() {}

   static BaseCollide *instance()        { return _instance;     }
   static mlib::CWvec get_move(mlib::CWpt& p, mlib::CWvec& v) {
      return _instance ? _instance->_get_move(p,v) : v;
   }

   static bool update_scene() {
      return _instance ? _instance->_update_scene() : false;
   }

 protected:
   //static BaseCollide;
   static BaseCollide*      _instance;

   // given a proposed move in 3D, returns a possibly shortened
   // version of the vector that won't go through any objects
   // in the world...
   virtual mlib::CWvec _get_move(mlib::CWpt& p, mlib::CWvec& v) {
	return v;
   }

   virtual bool _update_scene() = 0; 

};

/*****************************************************************
 * BaseGravity:
 *
 *      Adds artificial gravity to the scene
 *****************************************************************/

class BaseGravity {
 public :

   virtual ~BaseGravity() {}
   BaseGravity() {}

   static BaseGravity *instance()        { return _instance;     }
   static mlib::CWvec get_dir(mlib::CWpt& p) {
	  // cout << "GET_DIR" << endl;
	   return _instance ? _instance->_get_dir(p) : mlib::CWvec(0,0,0);
   }

 protected:
   //static BaseCollide;
   static BaseGravity*      _instance;

   virtual mlib::CWvec _get_dir(mlib::CWpt& p) {
	return mlib::CWvec(0,0,0);
   }
};

#endif // BASE_COLLIDE_H_IS_INCLUDED

/* end of file base_collide.H */
