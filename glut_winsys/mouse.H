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
#ifndef GLUT_MOUSE_H
#define GLUT_MOUSE_H

#include "std/support.H"
#include "dev/dev.H"

class GLUT_WINSYS;
class GLUT_MOUSE : public Mouse {
   protected:
      GLUT_WINSYS               *_winsys;
      static ARRAY<GLUT_MOUSE *> _mice;

   public:
      GLUT_MOUSE(GLUT_WINSYS *);
      ~GLUT_MOUSE();
      virtual void     set_size(int, int) {
         cerr << "WARNING:  dummy GLUT_MOUSE::set_size() called" << endl; 
      }
      GLUT_WINSYS  *winsys() { return _winsys; }
      static GLUT_MOUSE *mouse();
};


class GLUT_CURSpush : public DEVhandler {
  protected :
   GLUT_WINSYS * _win;

  public :
 
    GLUT_CURSpush(GLUT_WINSYS *win) : _win(win) { }

    virtual void handle_event( CEvd &e );
    virtual void push        ( mlib::CXYpt &);
};


#endif
