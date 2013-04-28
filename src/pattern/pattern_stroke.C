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
    pattern_stroke.H
    
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

#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
#pragma warning(disable: 4786)
#endif

#include "pattern_stroke.H"

PatternStroke::PatternStroke(CNDCZvec_list& g) 
{   
    _pts = g;
}
void 
PatternStroke::draw(CNDCZpt& c)
{
   //cerr << "Stroke had " << _pts.num() << endl;
   glBegin(GL_LINE_STRIP);   
	      for(int i=0; i < _pts.num(); ++i)
         {
            CNDCZpt p = c + _pts[i]; 
            glVertex2dv(p.data());
         }
	glEnd();
}

