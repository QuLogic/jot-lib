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
 * ref_frame_drawer.H
 *
 **********************************************************************/
#ifndef _REF_FRAME_DRAWER_H_
#define _REF_FRAME_DRAWER_H_

#include <vector>
#include "disp/gel.H"

/*****************************************************************
 * RefFrameDrawer
 * Draws stored collection of Strokes(gestures) as well as cell 
 * that the stokes are in
 *****************************************************************/
MAKE_PTR_SUBC(RefFrameDrawer, GEL);
typedef const RefFrameDrawer CRefFrameDrawer;
typedef const RefFrameDrawerptr CRefFrameDrawerptr;

class RefFrameDrawer : public GEL {
public:
  RefFrameDrawer(int ref_frame) : _ref_frame(ref_frame){}
  virtual ~RefFrameDrawer() {}
  virtual int draw(CVIEWptr& view);
  virtual int  draw_final(CVIEWptr & v);
  virtual DATA_ITEM *dup() const { return 0; }
  void set_ref_frame(int ref_frame) { _ref_frame = ref_frame; }
  
  
private:
  void draw_start();
  void draw_end();

private:
  int _ref_frame;
};

#endif  // _REF_FRAME_DRAWER_H_
