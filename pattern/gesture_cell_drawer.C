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
 * gesture_rect_drawer.C
 *
 **********************************************************************/

#include "geom/gl_view.H"
#include "std/config.H"
#include "pattern_pen_ui.H"
#include "gesture_cell_drawer.H"

using namespace mlib;


GestureCellDrawer::~GestureCellDrawer()
{
	//XXX need to clear out stuff
}

int
GestureCellDrawer::draw(CVIEWptr& v) { 
  
  int n=0;
  for(uint i=0; i < _strokes.size(); ++i)
  {
      n+= _strokes[i].draw(v);
  }
  return n;
}

int
GestureCellDrawer::draw_final(CVIEWptr& v) { 
 
  int n=0;
  for(uint i=0; i < _strokes.size(); ++i)
  {
     n += _strokes[i].draw_final(v);
     if(PatternPenUI::show_boundery_box)
       _cells[i]->draw_cell_bound();
  }
  return n;
}

void 
GestureCellDrawer::add(GELset strokes, GestureCell* cell)
{	
	_strokes.push_back(strokes);    
	_cells.push_back(cell);
}

void 
GestureCellDrawer::pop()
{   
	cerr << "GestureCellDrawer::pop()" << endl;
    //XXX - Delete the strokes and the cells	
    _strokes.pop_back();
	if(!_cells.empty()){
		GestureCell* last = _cells.back();
		if(last)
			delete last;
		_cells.pop_back();
	}
    GELset::pop();
    
}    

void 
GestureCellDrawer::clear()
{
	cerr << "GestureCellDrawer::clear()" << endl;
	_strokes.clear();	
	_cells.clear();
	GELset::clear();
}
