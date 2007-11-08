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
 * lvert_strip.H:
 **********************************************************************/
#ifndef LVERT_STRIP_H_IS_INCLUDED
#define LVERT_STRIP_H_IS_INCLUDED

#include "vert_strip.H"         // base class
#include "lface.H"              // subdivision faces, edges, verts

/**********************************************************************
 * LvertStrip:
 *
 *      Vertex strip class for LMESHs (subdivision meshes).
 *
 *      A vertex strip in the control mesh corresponds to a vertex
 *      strip with the same number of vertices in the next-level
 *      subdivision mesh.
 **********************************************************************/
#define CLvertStrip const LvertStrip
class LvertStrip : public VertStrip {
 public:
   //******** MANAGERS ********
   LvertStrip() : _substrip(0) {}
   virtual ~LvertStrip() { delete_substrip(); }

   void clear_subdivision(int level);

   Lvert* lv(int i) const { return (Lvert*)_verts[i]; }

   //******** BUILDING ********
   virtual void reset() { delete_substrip(); VertStrip::reset(); }

   //******** DRAWING ********
   virtual void draw(StripCB* cb) { draw(cur_level(), cb); }

 protected:
   //******** MEMBER DATA ********
   LvertStrip*  _substrip;  // corresponding finer-level vert strip

   //******** MANAGING SUBSTRIP ********
   void delete_substrip() { delete _substrip; _substrip = 0;}
   void generate_substrip();
   int  cur_level() const;

   //******** DRAWING (INTERNAL) ********
   void draw(int level, StripCB* cb);
};

#endif // LVERT_STRIP_H_IS_INCLUDED

/* end of file lvert_strip.H */
