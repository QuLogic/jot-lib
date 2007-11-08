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
/*****************************************************************
 * edge_frame.H
 *
 *                         n                                   
 *                         |                                   
 *                         |                                   
 *                         |                                   
 *                         o - - - b                          
 *                        /                                    
 *                       /                                     
 *                      t                                      
 *                                                             
 *      EdgeFrame: a coordinate frame associated with a Bedge.
 *      Origin is on the edge. Direction t is along the edge.
 *      Direction n is computed to be orthogonal to t (both are
 *      unit length). Direction b is determined by them:
 *              b = n x t.
 *
 *****************************************************************/
#ifndef EDGE_FRAME_H_IS_INCLUDED
#define EDGE_FRAME_H_IS_INCLUDED

#include "simplex_frame.H"

/*****************************************************************
 * EdgeFrame:
 *
 *      see diagram above.
 *****************************************************************/
class EdgeFrame : public SimplexFrame {
 public:

   //******** MANAGERS ********

   EdgeFrame(uint key, Bedge* e, CWvec& n, double s=0.5, bool flip=false) :
      SimplexFrame(key, e, (flip?(-e->vec()):(e->vec())), n),
      _orig_n(n),
      _flip(flip),
      _s(s) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("EdgeFrame", EdgeFrame*, SimplexFrame, CSimplexData*);

   //******** ACCESSORS ********

   Wvec         orig_n()        const   { return _orig_n; }
   double       s()             const   { return _s; }
   Bedge*       edge()          const   { return (Bedge*) _simplex; }

   //******** CoordFrame VIRTUAL METHODS ******** 

   virtual Wpt o() { return edge()->interp(_s); }

   //******** SimplexData NOTIFICATION METHODS ********

   virtual void notify_simplex_changed() {
      // a verttex of the edge changed position
      _dirty = 1;       // mark dirty to be recomputed later
      changed();        // notify observers
   }

 //*******************************************************
 // PROTECTED
 //*******************************************************
 protected:
   Wvec         _orig_n;        // original "normal"
   bool         _flip;          // whether _t is flipped
   double       _s;             // position along edge where "origin" is

   //******** SimplexFrame VIRTUAL METHODS ********
   virtual void recompute() {
      // align t with the edge vector
      set_((_flip?(-(edge()->vec())):(edge()->vec())), _orig_n);
      SimplexFrame::recompute();
   }
};
typedef const EdgeFrame CEdgeFrame;

#endif // EDGE_FRAME_H_IS_INCLUDED

// end of file edge_frame.H
