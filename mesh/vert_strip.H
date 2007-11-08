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
 * vert_strip.H:
 **********************************************************************/
#ifndef VERT_STRIP_H_IS_INCLUDED
#define VERT_STRIP_H_IS_INCLUDED

#include "bvert.H"

class StripCB;
/**********************************************************************
 * VertStrip:
 *
 *      Builds and stores vert strips -- sequences of mesh vertices
 *      that can be drawn, e.g. as OpenGL point strips. 
 *
 *      Like TriStrips and EdgeStrips, uses a StripCB for the actual
 *      graphics calls.
 **********************************************************************/
#define CVertStrip const VertStrip
class VertStrip {
 public:
   //******** MANAGERS ********
   VertStrip() : _patch(0), _index(-1) {}
   virtual ~VertStrip();

   //******** ACCESSORS ********
   CARRAY<Bvert*> &verts() const { return _verts;}
   Patch*          patch() const { return _patch; }

   Bvert* vert(int i)      const { return _verts[i]; }

   bool   empty()          const { return _verts.empty(); }
   int    num()            const { return _verts.num(); }

   //******** THE MAIN JOB ********
   virtual void draw(StripCB* cb);

   //******** BUILDING ********
   virtual void reset() { _verts.clear(); }
   void add(Bvert* v)   { _verts += v; }

 protected:
   friend class Patch;

   //******** DATA ********
   ARRAY<Bvert*>  _verts;       // the sequence of verts
   Patch*         _patch;       // patch this is assigned to
   int            _index;       // index in patch's list

   //******** PROTECTED METHODS ********
   // setting the Patch -- nobody's bizness but the Patch's:
   // (to set it call Patch::add(VertStrip*))
   void   set_patch(Patch* p)           { _patch = p; }
   void   set_patch_index(int k)        { _index = k; }
   int    patch_index() const           { return _index; }
};

#endif // VERT_STRIP_H_IS_INCLUDED

/* end of file vert_strip.H */
