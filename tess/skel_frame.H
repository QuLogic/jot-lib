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
 * skel_frame.H
 *****************************************************************/
#ifndef SKEL_FRAME_H_IS_INCLUDED
#define SKEL_FRAME_H_IS_INCLUDED

#include "mesh/vert_frame.H"

/************************************************************
 * SkelFrame:
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
 *   Coordinate frame for use with a skeletal curve or point.
 *   Vector t aligns with given 'prev' and 'next' edges.
 *   Vector n is computed from given 'orig_n' direction
 *   (by subtracting component along t).
 ************************************************************/
class SkelFrame : public VertFrame {
 public:

   //******** MANAGERS ********

   SkelFrame(uint key,
             Bvert* v,
             CWvec& n,
             Bedge* prev=0,
             Bedge* next=0) :
      VertFrame(key,v,Wvec::X(),n),
      _orig_n(n),
      _prev(prev),
      _next(next) {
      assert((!_prev || _prev->contains(v)) &&
             (!_next || _next->contains(v)));
      _dirty = 1;
   }

   SkelFrame(uint key,
             Bvert* v,
             CWvec& t,
             CWvec& n,
             Bedge* prev=0,
             Bedge* next=0) :
      VertFrame(key,v,t,n),
      _orig_n(n),
      _prev(prev),
      _next(next) {
      assert((!_prev || _prev->contains(v)) &&
             (!_next || _next->contains(v)));
      _dirty = 1;
   }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SkelFrame", SkelFrame*, VertFrame, CSimplexData*);

   //******** ACCESSORS ********

   void set_next(Bedge* e)              { _next = e; recompute(); changed(); }
   void set_prev(Bedge* e)              { _prev = e; recompute(); changed(); }

   Wvec orig_n()                const   { return _orig_n; }
   
   //******** VertFrame VIRTUAL METHODS ********

   virtual void set(CWvec&, mlib::CWvec&) {
      // the frame is computed from the Bedges, so no external
      // caller should be trying to set the vectors directly.
      err_msg("SkelFrame::set: shouldn't be called");
   }
   virtual void apply_xform(CWtransf& xf) {
      // called when the whole mesh is transformed
      _orig_n = xf*_orig_n;
      VertFrame::apply_xform(xf);
   }

   // Prevents warnings:
   void set(uint id, Bsimplex* s)       { SimplexData::set(id, s); }
   void set(Cstr_ptr& str, Bsimplex* s) { SimplexData::set(str, s); }

   //******** SimplexData NOTIFICATION METHODS ********

   virtual void notify_simplex_changed() {
      // the vertex changed position
      _dirty = 1;       // mark dirty to be recomputed later
      changed();        // notify observers
   }
   virtual void notify_normal_changed() {
      // the vertex itself didn't change position
      // but a neighboring vertex did.
      _dirty = 1;       // mark dirty to be recomputed later
      changed();        // notify observers
   }

 //*******************************************************
 // PROTECTED
 //*******************************************************
 protected:
   Wvec         _orig_n;        // original "normal"
   Bedge*       _prev;          // previous edge
   Bedge*       _next;          // next edge

   //******** INTERNAL METHODS ********

   // convenience stuff used in updating
   CWpt& prev_loc() const { return _prev->other_vertex(vert())->loc(); }
   CWpt& next_loc() const { return _next->other_vertex(vert())->loc(); }
   Wvec  prev_vec()       { return (o() - prev_loc()).normalized(); }
   Wvec  next_vec()       { return (next_loc() - o()).normalized(); }

   // compute the tangent direction from the given edges
   Wvec tan() {
      return ((_prev && _next) ? ((prev_vec() + next_vec())/2.0) :
              _next            ? next_vec() :
              _prev            ? prev_vec() :
              _t);
   }

   //******** SimplexFrame VIRTUAL METHODS ********

   virtual void recompute() {
      // align t with the edge vectors
      Wvec t = tan();
      if (t.is_null()) {
         err_msg("SkelFrame::recompute: tangent is not defined");
         t = _t;
      }
      set_(t, _orig_n);           // cache _t and _n vectors
      SimplexFrame::recompute();  // cache _xf and _inv_xf matrices
   }
};

#endif // SKEL_FRAME_H_IS_INCLUDED

/* end of file skel_frame.H */
