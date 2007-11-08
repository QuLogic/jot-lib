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
 * tri_strip.C:
 **********************************************************************/
#include "bmesh.H"
#include "mesh_global.H"        // for debugging (selecting bad faces)
#include "stripcb.H"

// Values used in finding triangle strips.
//
// A face is "claimed" when it has been added to a strip.
//
// A face is "marked" during TriStrip::backup_strip(), below,
// during a traversal that "backs up" over a sequence of faces
// before adding them to a new strip.
//
// (Initially all face flags should be cleared to 0.)
//
static const uchar CLEARED_FLAG = 0;
static const uchar MARKED_FLAG  = 1;
static const uchar CLAIMED_FLAG = 2;

inline bool
is_cleared(Bface* f)
{
   return f->flag() == CLEARED_FLAG;
}

inline bool
is_marked(Bface* f)
{
   return f->flag() == MARKED_FLAG;
}

inline bool
is_claimed(Bface* f)
{
   return f->flag() == CLAIMED_FLAG;
}

inline void
mark_face(Bface* f)
{
   f->set_flag(MARKED_FLAG);
}

inline void
claim_face(Bface* f)
{
   f->set_flag(CLAIMED_FLAG);
}

/**********************************************************************
 * TriStrip:
 **********************************************************************/
Bface*
TriStrip::backup_strip(Bface* f, Bvert*& a) 
{
   // we'd like to draw a triangle strip starting at the 
   // given triangle and proceeding "forward." but to get 
   // the most bang for the buck, we'll first "backup" over 
   // as many triangles as possible to find a starting place 
   // from which we can generate a longer strip.

   assert(!f->flag());

   mark_face(f);

   Bface* ret = f;
   Bvert* b   = f->next_vert_ccw(a);
   Bvert* c   = f->next_vert_ccw(b);
   Bedge* e;

   int i = 0;
   while((e = f->edge_from_vert((i%2) ? b : a)) &&
         e->consistent_orientation()            &&
         e->is_crossable()                      &&
         (f = e->other_face(f))                 &&
         is_cleared(f)) {
      mark_face(f);
      ret = f;
      Bvert* d = f->other_vertex(a,b);
      c = b;
      b = a;
      a = d;
      i++;
   }

   _orientation = ((i%2) != 0);

   return ret;
}

bool
TriStrip::build(
   Bface* start,        // build a new strip starting here.
   Bface_list& stack    // used to build nearby parallel strips
   )
{
   // a set flag means this face is already in a TriStrip
   assert(!start->flag());

   // start fresh
   reset();

   // repeat 1st vertex if needed
   if (!start->orient_strip())
      start->orient_strip(start->v1());

   // get the starting vert. i.e., the strip will
   // continue onto the next face across the edge
   // opposite this vertex.
   Bvert *a = start->orient_strip(), *b, *c;

   // squash and stretch
   start = backup_strip(start,a);

   if (!start) {
      // should never happen, but can happen
      // if there are inconsistently oriented faces
      err_msg("TriStrip::build: error: backup_strip() failed");
      err_msg("*** check mesh for inconsistently oriented faces ***");

      return 0;
   }

   // claim it
   claim_face(start);

   // record direction of strip on 1st face:
   start->orient_strip(a);

   // faces alternate CCW / CW
   if (_orientation) {
      c = start->next_vert_ccw(a);
      b = start->next_vert_ccw(c);
   } else {
      b = start->next_vert_ccw(a);
      c = start->next_vert_ccw(b);
   }

   add(a,start);
   add(b,start);
   add(c,start);

   Bface* opp;
   if ((opp = start->opposite_face(b)) && is_cleared(opp) &&
      opp->patch() == start->patch()) {
      opp->orient_strip(_orientation ? a : c);
      stack += opp;
   }

   int i=_orientation;
   Bface* cur = start;
   while ((cur = cur->next_strip_face()) && !is_claimed(cur)) {
      claim_face(cur);
      i++;
      a = b;
      b = c;
      c = cur->other_vertex(a,b);
      cur->orient_strip(a);

      if ((opp = cur->opposite_face(b)) && is_cleared(opp) &&
          opp->patch() == start->patch()) {
         opp->orient_strip(i % 2 ? a : c);
         stack += opp;
      }

      add(c,cur);
   }

   return 1;
}

void
TriStrip::get_strips(
   Bface* start,
   ARRAY<TriStrip*>& strips
   ) 
{
   // if starting face was visited already, stop
   if (!is_cleared(start))
      return;

   // stack is used to record faces adjacent to
   // the current strip, in order to build additional
   // strips that align with the current one
   static Bface_list stack(1024);
   stack.clear();
   stack += start;

   BMESH* mesh = start->mesh();

   while (!stack.empty()) {
      start = stack.pop();
      if (is_cleared(start)) {
         TriStrip* strip = mesh->new_tri_strip();
         strip->build(start, stack);
         strips += strip;
      }
   }
}

void
TriStrip::draw(StripCB* cb)
{
   // check for empty strip before doing work:
   if (empty())
      return;

   // XXX -
   //   Temporary check to see if it ever happens (10/2003):
   if (!BMESH::show_secondary_faces() && _faces.has_any_secondary()) {
      err_msg("TriStrip::draw: warning: %d/%d secondary faces",
              _faces.num_secondary(), _faces.num());
      MeshGlobal::select(_faces.secondary_faces());
   }

   cb->begin_faces(this);

   // repeat 1st vertex if strip is
   // negatively oriented:
   if (_orientation)
      cb->faceCB(_verts[0], _faces[0]); // good thing it's not empty

   for (int i=0; i<_verts.num(); i++)
      cb->faceCB(_verts[i], _faces[i]);

   cb->end_faces(this);
}

/* end of file tri_strip.C */
