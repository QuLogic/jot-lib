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
 * stripcb.H:
 **********************************************************************/
#ifndef STRIPCB_H_IS_INCLUDED
#define STRIPCB_H_IS_INCLUDED

#include "mesh/bface.H"

/**********************************************************************
 * StripCB:
 *
 *      used to "draw" triangle strips, edge strips, and vertex
 *      strips. this could mean making OpenGL calls, or writing to
 *      stdout a description of the strip in some file format.
 **********************************************************************/
class TriStrip;   // BMESH triangle strip class
class EdgeStrip;  // BMESH line strip class
class VertStrip;  // BMESH point strip class

class StripCB {
 public:

   StripCB() : alpha(1.0) {}
   virtual ~StripCB() {}

   // for triangle strips:
   virtual void begin_faces(TriStrip*)          {}
   virtual void faceCB (CBvert*, CBface*)       {} 
   virtual void end_faces  (TriStrip*)          {}

   // for line strips:
   virtual void begin_edges(EdgeStrip*)         {}
   virtual void edgeCB (CBvert*, CBedge*)       {}
   virtual void end_edges  (EdgeStrip*)         {}

   // for point strips:
   virtual void begin_verts(VertStrip*)         {}
   virtual void vertCB (CBvert*)                {}
   virtual void end_verts  (VertStrip*)         {}

   // for triangles
   virtual void begin_triangles()               {}  
   virtual void end_triangles  ()               {}

   double alpha;
};

#endif // STRIPCB_H_IS_INCLUDED

/* end of file stripcb.H */
