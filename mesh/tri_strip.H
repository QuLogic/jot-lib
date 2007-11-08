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
 * tri_strip.H:
 **********************************************************************/
#ifndef TRI_STRIP_H_IS_INCLUDED
#define TRI_STRIP_H_IS_INCLUDED

#include "bface.H"

class StripCB;
/**********************************************************************
 * TriStrip:
 *
 *    Builds and stores triangle strips
 **********************************************************************/
#define CTriStrip const TriStrip
class TriStrip {
 public:
   //******** MANAGERS ********
   TriStrip(int s=0) : _orientation(!!s), _verts(0), _faces(0) {} // start empty
   virtual ~TriStrip() {}

   //******** ACCESSORS ********
   CBvert_list& verts()         const { return _verts;}
   CBface_list& faces()         const { return _faces;}

   Bvert* vert(int i)           const { return _verts[i]; }
   Bface* face(int i)           const { return _faces[i]; }

   bool   orientation()         const { return _orientation;}
   bool   empty()               const { return _verts.empty(); }
   int    num()                 const { return _verts.num(); }

   //******** BUILDING ********
   virtual void reset() { _verts.clear(); _faces.clear(); _orientation = 0; }
   void add(Bvert* v, Bface* f) { _verts += v; _faces += f; }
   static  void get_strips(Bface*, ARRAY<TriStrip*>&);

   //******** DRAWING ********
   virtual void draw(StripCB* cb);

 protected:
   // True if strip is "negatively oriented",
   // i.e. need to start w/ repeated 1st vertex:
   bool           _orientation; // XXX - rename; 
   Bvert_list     _verts;       // sequence of vertices
   Bface_list     _faces;       // face i corresponds to verts i, i-1, i-2

   Bface*       backup_strip(Bface*, Bvert*&);
   bool         build(Bface*, Bface_list&);
};

#endif // TRI_STRIP_H_IS_INCLUDED

/* end of file tri_strip.H */
