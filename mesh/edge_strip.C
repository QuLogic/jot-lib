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
 * edge_strip.C:
 **********************************************************************/
#include "bfilters.H"
#include "stripcb.H"
#include "mi.H"

/**********************************************************************
 * EdgeStrip:
 **********************************************************************/
void
EdgeStrip::draw(StripCB* cb)
{
   // Draw the strip -- e.g. by making calls to OpenGL or some
   // other graphics API, or by writing a description of the
   // strip to a text file. Which of these is done depends on the
   // implementation of the StripCB.

   // Stop now if nothing is going on
   if (empty())
      return;

   // Iterate over the strip. Keep in mind _verts[i] is the
   // leading vertex (with respect to this strip) of _edges[i].
   // Whenever _edges[i]->other_vertex(_verts[i]) is different
   // from _verts[i+1], the line strip is broken at that point.
   // So a single EdgeStrip encodes 1 or more GL_LINE_STRIPs.
   // The following loop checks for these breaks.

   bool started = 0;
   for (int i=0; i<_verts.num(); i++) {

      // start new line strip if needed:
      if (!started) {
         cb->begin_edges(this);
         cb->edgeCB(_verts[i], _edges[i]);
         started = 1;
      }

      // Get next vert:
      Bvert* next = next_vert(i);  // _edges[i]->other_vertex(_verts[i])

      // Continue line strip ("finish" this edge):
      cb->edgeCB(next, _edges[i]);

      // If we got to the end of the array or if _verts[i+1]
      // isn't part of current edge, then break the strip here.
      if ((i == _verts.num()-1) || (_verts[i+1] != next)) {
         cb->end_edges(this);
         started = 0;
      }
   }
}

Bedge*
EdgeStrip::next_edge(
   Bvert*               v,
   Bvert_list&          stack,
   CSimplexFilter&      filter
   )
{
   // Used in EdgeStrip::build() below.  Return an edge of the
   // given 'type'; if not all edges have been checked, then push
   // the vertex on the stack so remaining edges can be checked
   // next time.

   // The filter should accept edges of the desired type only the
   // *first* time it checks an edge. Examples are NewSilEdgeFilter,
   // UnreachedEdgeFilter() + CreaseEdgeFilter(), etc.

   // Check all adjacent edges:
   for (int i=v->degree()-1; i>=0; i--) {
      if (filter.accept(v->e(i))) { // if we find a good edge
         if (i > 0)                 //  and not all edges have been checked
            stack += v;             //  remember this vertex to come back to it
         return v->e(i);            // return the good edge
      }
   }
   return 0;
}

void
EdgeStrip::build(CBedge_list& edges, CSimplexFilter& filter)
{
   // Given a list of edges to search, build the strip from edges
   // that satisfy a given property.

   reset();

   for (int k=0; k<edges.num(); k++)
      build(0, edges[k], filter);
}

void
EdgeStrip::build_line_strip(
   Bvert*          v,           // leading vertex
   Bedge*          e,           // contains v, satisfied filter
   CSimplexFilter& filter,      // selects desired edges
   Bvert_list&     stack        // vertices to check later
   )
{
   // Run a strip from v thru e and continuing on to other edges
   // accepted by the filter. Incompletely explored vertices are
   // pushed on the stack so they can be revisited another time.
   //
   // Note: e has already satisfied the filter, and e contains v.

   assert(e && e->contains(v));

   while (e) {
      add(v,e);
      e = next_edge(v = e->other_vertex(v), stack, filter);
   }
}

void
EdgeStrip::build(Bvert* v, Bedge* e, CSimplexFilter& filter)
{
   // continue building the edge strip, starting with the
   // given edge e. if v is not null, e must contain v.
   // in that case v will be the leading vertex of the strip.

   // must have an edge to proceed,
   // and the edge must be accepted by the filter.
   if (!(e && filter.accept(e))) // someone has to punch its ticket
      return;

   assert(!v || e->contains(v));

   static Bvert_list stack(64);
   stack.clear();

   // first loop:
   build_line_strip(v ? v : e->v1(), e, filter, stack);

   // get the rest of them
   while (!stack.empty()) {
      if ((v = stack.pop()) && (e = next_edge(v, stack, filter))) 
         build_line_strip(v, e, filter, stack);
   }
}

void
EdgeStrip::build_with_tips(CBedge_list& edges, CSimplexFilter& filter)
{
   // Build the strip from the given pool of edges, with the
   // given filter.  Try to start the edge strip at the "tips" of
   // chains of edges of the desired type. The given filter
   // should just screen for edges of the desired kind;
   // internally this method also screens for edges that have not
   // yet been reached (added to the strip).

   // Clear edge flags to screen for unreached edges:
   set_adjacent_edges(edges.get_verts(), 1);
   edges.clear_flags();

   // Pull out the edge tips:
   Bedge_list tips = edges.filter(ChainTipEdgeFilter(filter));

   // Construct the filter that screens out previously reached
   // edges:
   UnreachedSimplexFilter unreached;
   AndFilter       wanted = unreached + filter;

   int k;

   // Start from all the tips first:
   for (k=0; k<tips.num(); k++) {
      Bedge* e = tips[k];
      Bvert* v = (e->v2()->degree(filter) != 2) ? e->v2() : e->v1();
      build(v, e, wanted);
   }

   // Now check the rest:
   for (k=0; k<edges.num(); k++)
      build(0, edges[k], wanted);
}

void
EdgeStrip::build_ccw_boundaries(
   CBedge_list& edges,
   CSimplexFilter& face_filter
   )
{
   // Similar to previous...
   //
   // XXX - needs comments

   // Clear edge flags to screen for unreached edges:
   // set edge flags to 1 in 1-ring of verts,
   // then clear edge flags of internal edges
   set_adjacent_edges(edges.get_verts(), 1);
   edges.clear_flags();

   // get an edge filter that accepts "boundary" edges WRT the
   // given face filter
   BoundaryEdgeFilter boundary(face_filter);

   // Pull out the edge tips:
   Bedge_list tips = edges.filter(ChainTipEdgeFilter(boundary));

   // Construct the filter that screens out previously reached
   // edges:
   UnreachedSimplexFilter unreached;
   AndFilter       wanted = unreached + boundary;

   int k;

   // Start from all the tips first:
   for (k=0; k<tips.num(); k++) {
      Bedge* e = tips[k];
      Bvert* v = (e->v2()->degree(boundary) != 2) ? e->v2() : e->v1();
      Bface* f = e->screen_face(face_filter);
      assert(f); // e must have 1 face satisfying the filter

      // If this will start out running ccw, take it.
      // otherwise skip:
      if (f->next_vert_ccw(v) == e->other_vertex(v))
         build(v, e, wanted);
   }

   // Now check the rest:
   for (k=0; k<edges.num(); k++) {
      Bedge* e = edges[k];
      Bface* f = e->screen_face(face_filter);
      assert(f); // e must have 1 face satisfying the filter

      // Go CCW around faces
      build(f->leading_vert_ccw(e), e, wanted);
   }
}

bool
EdgeStrip::has_break(int i) const
{
   // Returns true if the line strip is broken at vert i
   // (or if it's the first or last vert):

   // For bogus input say it's not broken:
   if (empty() || (i < 0) || (i > _verts.num()))
      return false;

   // For the first or last vertex say it is broken:
   // Note: last vert is the one *after* _verts.last()
   if ((i == 0) || (i == _verts.num()))
      return true;

   // Just check the vertex:
   return (_verts[i] != next_vert(i-1));
}

int 
EdgeStrip::num_line_strips() const
{
   // Returns number of distinct (disconnected) line strips:

   if (empty())
      return 0;

   int ret = 1; // last edge has a break

   // check all edges but the last:
   for (int i=0; i<_edges.num()-1; i++)
      if (next_vert(i) != _verts[i+1])
         ret++;
   return ret;
}

bool
EdgeStrip::get_chain(int& k, Bvert_list& chain) const
{
   // Return the chain starting at index k,
   // and advance k to the start of the next chain.

   chain.clear();               // get set...

   if (k < 0 || k >= num())     // if out of range, reject
      return false;

   if (!has_break(k))           // if k is not a chain endpoint, reject
      return false;             

   chain += vert(k);            // add leading vertex
   do {
      chain.add(next_vert(k));  // add subsequent vertex
   } while (!has_break(++k));   // advance k, break at chain end

   return true;
}

void
EdgeStrip::get_chains(ARRAY<Bvert_list>& chains) const
{
   // Return a list of distinct chains of edges, in the form
   // of a set of Bvert_lists. 

   chains.clear();

   Bvert_list chain;
   for (int k=0; get_chain(k, chain); )
      chains += chain;
}

EdgeStrip 
EdgeStrip::get_reverse() const
{
   // Generate the reversed edge strip:

   EdgeStrip ret = *this;
   if (!empty()) {
      ret._edges.reverse();
      ret._verts += last();
      ret._verts.reverse();
      ret._verts.pop();
   }
   return ret;
}

EdgeStrip 
EdgeStrip::get_filtered(CSimplexFilter& filter) const
{
   EdgeStrip ret;
   for (int i=0; i<num(); i++)
      if (filter.accept(edge(i)))
         ret.add(vert(i), edge(i));
   return ret;
}

inline Bvert*
get_leading_vert(CEdgeStrip& source)
{
   CBvert_list& verts = source.verts();
   CBedge_list& edges = source.edges();
   for (int i=0; i<verts.num(); i++)
      if (verts[i]->degree(SimplexFlagFilter(1)) == 1 && edges[i]->flag())
         return verts[i];
   for (int j=0; j<verts.num(); j++)
      if (edges[j]->flag())
         return verts[j];
   return 0;
}

inline void
add_to_strip(Bvert* v, CEdgeStrip& source, EdgeStrip& ret)
{
   assert(v);
   CBvert_list& verts = source.verts();
   CBedge_list& edges = source.edges();
   int k = verts.get_index(v);
   assert(verts.valid_index(k) && edges[k]->flag());
   while (verts.valid_index(k) && edges[k]->flag()) {
      edges[k]->clear_flag();
      ret.add(verts[k], edges[k]);
      if (source.has_break(k+1)) {
         k = verts.get_index(source.next_vert(k));
      } else {
         k++;
      }
   }
}

EdgeStrip 
EdgeStrip::get_unified() const
{
   _edges.get_verts().clear_flag02();
   _edges.set_flags(1);
   EdgeStrip ret;
   Bvert* v = 0;
   while ((v = get_leading_vert(*this))) {
      add_to_strip(v, *this, ret);
   }
   return ret;
}

// end of file edge_strip.C
