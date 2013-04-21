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
 * stroke_pattern_drawer.H
 *
 **********************************************************************/
#ifndef _STROKE_PATTERN_DRAWER_H_
#define _STROKE_PATTERN_DRAWER_H_

#include <vector>
#include "disp/gel.H"
#include "stroke_pattern.H"

/*****************************************************************
 * StrokePatternDrawer
 * Draws stored collection of Strokes(gestures) as well as cell 
 * that the stokes are in
 *****************************************************************/
MAKE_PTR_SUBC(StrokePatternDrawer, GEL);
typedef const StrokePatternDrawer CStrokePatternDrawer;
typedef const StrokePatternDrawerptr CStrokePatternDrawerptr;

class StrokePatternDrawer : public GEL {
public:
  StrokePatternDrawer(StrokePattern* pattern) : _pattern(pattern){}
  virtual ~StrokePatternDrawer() {}
  virtual int draw(CVIEWptr& view);
  virtual int  draw_final(CVIEWptr & v);
  virtual DATA_ITEM *dup() const { return 0; }
  
  
private:
  void draw_start();
  void draw_end();

private:
  StrokePattern* _pattern;
};

#endif  // _STROKE_PATTERN_DRAWER_H_
