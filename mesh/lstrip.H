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
 * lstrip.H:
 **********************************************************************/
#ifndef LSTRIP_H_IS_INCLUDED
#define LSTRIP_H_IS_INCLUDED

#include "tri_strip.H"
#include "lface.H"

/**********************************************************************
 * Lstrip:
 *
 *   A triangle strip for LMESH (subdivision mesh) that optimizes 
 *   triangle strip creation by using the triangle strips already 
 *   created on the control mesh to efficiently generate triangle 
 *   strips for each subdivision mesh.
 **********************************************************************/
#define CLstrip const Lstrip
class Lstrip : public TriStrip {
 public:
   //******** MANAGERS ********
   Lstrip(int s=0) : TriStrip(s), _right_substrip(0), _left_substrip(0) {}
   virtual ~Lstrip() { delete_substrips(); }

   void delete_substrips();

   //******** BUILDING ********
   virtual void reset();
           void add(Bvert* v, Bface* f) { TriStrip::add(v,f); }

   //******** DRAWING ********
   virtual void draw(StripCB* cb)           { draw(cur_level(), cb); }

 protected:
   //******** MEMBER DATA ********
   Lstrip*      _right_substrip;        // the 2 
   Lstrip*      _left_substrip;         //       child strips

   //******** INTERNAL METHODS ********
   // convenience lookups.
   // given vertex or edge (respectively) in parent strip,
   // find corresponding vertex in subdiv strip:
   Bvert* subvert(int j) const {
      return ((Lvert*)_verts[j])->subdiv_vertex();
   }
   Bvert* subvert(int j, int k) const {
      return ((Ledge*)_verts[j]->lookup_edge(_verts[k]))->subdiv_vertex();
   }

   //******** BUILDING METHODS ********
   void build_substrip1(Lstrip* substrip);
   void build_substrip2(Lstrip* substrip);
   void add(Bvert*);

   //******** MANAGING SUBSTRIPS ********
   void generate_substrips();
   int  cur_level() const;

   //******** DRAWING ********
   void draw(int level, StripCB* cb);
};

#endif // LSTRIP_H_IS_INCLUDED

/* end of file lstrip.H */
