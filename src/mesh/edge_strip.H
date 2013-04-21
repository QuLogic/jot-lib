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
 * edge_strip.H:
 **********************************************************************/
#ifndef EDGE_STRIP_H_IS_INCLUDED
#define EDGE_STRIP_H_IS_INCLUDED

#include "bvert.H"

class StripCB;
/**********************************************************************
 * EdgeStrip:
 *
 *      Builds and stores edge strips -- sequences of mesh edges
 *      that can be drawn, e.g. as OpenGL line strips. A single
 *      EdgeStrip can encode several disconnected line strips.
 *
 *      Each vertex represents the starting vertex of the
 *      corresponding edge. Like TriStrips, uses a StripCB
 *      for the actual graphics calls.
 **********************************************************************/
#define CEdgeStrip const EdgeStrip
class EdgeStrip {
 public:
   //******** MANAGERS ********

   // Create an empty strip:
   EdgeStrip() : _patch(0), _index(-1) {}

   // Given a list of edges to search, build a strip of
   // edges that satisfy a given property.
   EdgeStrip(CBedge_list& list, CSimplexFilter& filter) :
      _patch(0),
      _index(-1) {
      build(list, filter);
   }
   virtual ~EdgeStrip() {}

   // Assignment operator:
   virtual EdgeStrip& operator=(CEdgeStrip& strip) {
      reset();  // In LedgeStrip: deletes subdiv strips
      _verts = strip._verts;
      _edges = strip._edges;
      return *this;
   }

   //******** BUILDING ********

   // Given a list of edges to search, build the strip from edges
   // that satisfy a given property. The given filter should
   // screen for edges of the desired kind that have ALSO not yet
   // been "reached" in the search (e.g. their flag is not set).
   void build(CBedge_list&, CSimplexFilter&);

   // Build the strip from the given pool of edges, with the
   // given filter.  Try to start the edge strip at the "tips" of
   // chains of edges of the desired type. The given filter
   // should just screen for edges of the desired kind;
   // internally this method further screens for edges that have
   // not yet been reached, by setting edge flags.
   void build_with_tips(CBedge_list&, CSimplexFilter&);

   // Similar to above, but builds the edge strip along the boundary
   // between faces accepted by the filter and those not accepted.
   // The different pieces each run CCW around the faces of interest.
   void build_ccw_boundaries(CBedge_list& edges, CSimplexFilter& face_filter);

   // Continue building the strip from a given edge type,
   // starting at the given edge and one of its vertices.
   // If the vertex is nil a vertex is chosen from the edge
   // arbitrarily.
   void build(Bvert* v, Bedge* e, CSimplexFilter&);

   // Add another segment to the strip:
   void add(Bvert* v, Bedge* e) {
      if (v && e) {
         _verts += v; _edges += e;
      }
   }

   // Clear the strip:
   virtual void reset() { _verts.clear(); _edges.clear(); }

   //******** ACCESSORS ********

   // Patch and mesh that own the strip:
   Patch* patch()          const { return _patch; }
   BMESH* mesh()           const { return _verts.mesh(); }

   // Returns edge and vert lists of this strip:
   CBedge_list& edges()    const { return _edges; }
   CBvert_list& verts()    const { return _verts; }

   // Convenience accessors for edge or vert number i:
   Bedge* edge(int i)      const { return _edges[i]; }
   Bvert* vert(int i)      const { return _verts[i]; }
   Bvert* next_vert(int i) const { return _edges[i]->other_vertex(_verts[i]); }

   Bvert* first()          const { return _verts[0]; }
   Bvert* last()           const { return next_vert(num()-1); }

   bool   empty()          const { return _verts.empty(); }
   int    num()            const { return _verts.num(); }

   // Returns true if the line strip is broken at vert i
   // (or if it's the first or last vert):
   bool has_break(int i)   const;

   // Returns number of distinct (disconnected) line strips:
   int num_line_strips()   const;

   // Return a list of distinct chains of edges, in the form
   // of a set of Bvert_lists. 
   void get_chains(ARRAY<Bvert_list>& chains) const;

   // Return the chain starting at index k,
   // and advance k to the start of the next chain.
   bool get_chain(int& k, Bvert_list& chain) const;

   // Diagnostic, for the paranoid
   bool same_mesh() const {
      return (_verts.same_mesh() &&
              _edges.same_mesh() &&
              _verts.mesh() == _edges.mesh()
              );
   }

   // Generate the reversed edge strip:
   EdgeStrip get_reverse() const;

   // Reverse the direction of this one:
   void reverse() { *this = get_reverse(); }

   // Return a strip of edges from this strip that are accepted
   // by the given filter:
   EdgeStrip get_filtered(CSimplexFilter& filter) const;
   
   // In case the edge strip has breaks, return an equivalent strip
   // (same edges) eliminating breaks where possible:
   EdgeStrip  get_unified() const;

   //******** VIRTUAL METHODS ********

   // draw the strip
   virtual void draw(StripCB* cb);

   // Subdivision strips:
   //   For an EdgeStrip, cur_strip() returns the strip itself.
   //   For an LedgeStrip, returns the child strip at the current
   //   subdivision level:
   virtual CEdgeStrip* cur_strip()      const   { return this; }

   // Returns the child strip at the given subdivision level.
   // The specified level is relative to this strip, so level 0
   // refers to this strip, level 1 is its child strip, etc.:
   virtual CEdgeStrip* sub_strip(int k) const  { return k==0 ? this : 0; }

   // Returns edge and vert lists of the "current" strip:
   // (see comment above about the virtual cur_strip() method):
   CBvert_list& cur_verts()     const   { return cur_strip()->verts(); }
   CBedge_list& cur_edges()     const   { return cur_strip()->edges(); }

   CBvert_list& verts(int k)    const   { return sub_strip(k)->verts(); }
   CBedge_list& edges(int k)    const   { return sub_strip(k)->edges(); }

 protected:
   friend class Patch;

   //******** DATA ********
   Bvert_list   _verts;       // the sequence of verts
   Bedge_list   _edges;       // each edge follows the corresponding vert
   Patch*       _patch;       // patch this is assigned to
   int          _index;       // index in patch's list

   //******** PROTECTED METHODS ********

   // Used internally when generating strips of edges of a given type.
   // The filter should accept edges of the correct type that have not
   // already been checked -- details in EdgeStrip::build():
   static Bedge* next_edge(Bvert*, Bvert_list&, CSimplexFilter&);

   // Run a strip from v thru e and continuing on to other edges
   // accepted by the filter. Incompletely explored vertices are
   // saved in the list so they can be revisited another time.
   void build_line_strip(Bvert* v, Bedge* e, CSimplexFilter&, Bvert_list&);

   // setting the Patch -- nobody's bizness but the Patch's:
   // (to set it call Patch::add(EdgeStrip*))
   void   set_patch(Patch* p)           { _patch = p; }
   void   set_patch_index(int k)        { _index = k; }
   int    patch_index() const           { return _index; }
};

#endif // EDGE_STRIP_H_IS_INCLUDED

/* end of file edge_strip.H */
