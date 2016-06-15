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
/***************************************************************************
    PatternStroke
        -Stroke used to render Pattern3dStroke(pattern_group.c) in 2d
        -Derives from BaseStroke
        -Has check_vert_visibility Basestroke method to use visibility 
         check: if there is a face on the ref image(thus visible) that
         intersects with the location of the vertex, then draw it.
-------------------
    Simon Breslav
    Fall 2004
***************************************************************************/
#ifndef _PATTERN_STROKE_H_IS_INCLUDED_
#define _PATTERN_STROKE_H_IS_INCLUDED_

//#include "stroke/base_stroke.hpp"
//#include "pattern/quad_cell.hpp"
//#include <map>
#include "std/config.hpp"
#include "geom/gl_view.hpp"
// *****************************************************************
// * PatternStroke
// *****************************************************************
#define CPatternStroke PatternStroke const
class PatternStroke {
public:   
   PatternStroke(CNDCZvec_list& g);
   virtual ~PatternStroke() {}  
   void draw(CNDCZpt& c);
private:
   NDCZvec_list _pts;
};

#endif
