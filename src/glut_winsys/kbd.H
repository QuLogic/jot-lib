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

#ifndef GLUT_KBD_H
#define GLUT_KBD_H

#include "std/support.H"
#include "dev/dev.H"
class GLUT_WINSYS;
class GLUT_KBD {
 protected:
   GLUT_WINSYS *_winsys;
   static ARRAY<GLUT_KBD *> _kbds;
   bool _shift;
   bool _ctrl;
 public:
   GLUT_KBD(GLUT_WINSYS *winsys);
   static GLUT_KBD *kbd();
   GLUT_WINSYS *winsys() { return _winsys; }

   // Send any modifier (shift/ctrl) change events
   void update_mods(mlib::CXYpt &cur);
};

#endif 

/* end of file kbd.H */
