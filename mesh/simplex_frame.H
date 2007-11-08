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
 * simplex_frame.H
 *****************************************************************/
#ifndef SIMPLEX_FRAME_H_IS_INCLUDED
#define SIMPLEX_FRAME_H_IS_INCLUDED

#include "map3d/coord_frame.H"
#include "bvert.H"

/*****************************************************************
 * SimplexFrame:
 *
 *
 *   A CoordFrame associated with a Bsimplex. Origin is
 *   associated with the simplex (e.g. the position of a
 *   vertex, or midpoint of an edge), and vectors t and n
 *   are provided.
 *
 *****************************************************************/
class SimplexFrame : public SimplexData, public CoordFrame {
 public:

   //******** MANAGERS ********

   SimplexFrame(uint key, Bsimplex* s, CWvec& t, mlib::CWvec& n) :
      SimplexData(key, s),
      _dirty(1) { set_(t,n); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SimplexFrame", SimplexFrame*, SimplexData, CSimplexData*);

   //******** VIRTUAL METHODS ********

   // public method for setting the vectors:
   virtual void set(CWvec& t, mlib::CWvec& n) {
      set_(t,n);        // call internal set() method
      changed();        // then notify observers
   }
   virtual void apply_xform(CWtransf& xf) {
      // Transform t and n but not o, which is tied to the vertex.
      set_(xf*_t, xf*_n);
      changed(); // in any case observers can be told
   }

   //******** CoordFrame VIRTUAL METHODS ********

   // Subclasses must implement virtual Wpt o();

   virtual Wvec      t() { update(); return _t; }
   virtual Wvec      n() { update(); return _n; }
   virtual Wtransf  xf() { update(); return _xf; }
   virtual Wtransf inv() { update(); return _inverse; }

   virtual void changed() {
      _dirty = true;
      CoordFrame::changed();
   }

   //******** SimplexData NOTIFICATION METHODS ********

   virtual void notify_simplex_changed()                { changed(); }
   virtual void notify_simplex_xformed(CWtransf& /* xf */) {
      // XXX - what to do really?
      // apply_xform(xf);
   }
   virtual void notify_simplex_deleted() {
      _observers.notify_frame_deleted(this); 
      SimplexData::notify_simplex_deleted(); // deletes this
   }

   // Prevents warnings:
   void set(uint id, Bsimplex* s)       { SimplexData::set(id, s); }
   void set(Cstr_ptr& str, Bsimplex* s) { SimplexData::set(str, s); }

 //*******************************************************
 // PROTECTED
 //*******************************************************
 protected:
   Wvec                 _t;             // "tangent" direction
   Wvec                 _n;             // "normal" direction
   Wtransf              _xf;            // mapping from local to world
   Wtransf              _inverse;       // mapping from world to local
   bool                 _dirty;         // needs to be recomputed

   //******** INTERNAL METHODS ********

   // internal set() method not for public use:
   void set_(CWvec& t, mlib::CWvec& n) {
      // enforce policy that vectors must be orthonormal.
      // if caller passes in zero-length or parallel vectors
      // the frame will be degenerate.
      _t = t.normalized();       // preference to t
      _n = n.orthogonalized(_t).normalized();
   }

   virtual void recompute() {
      // subclasses may do more...
      // but whatever they do, they should
      // clear the _dirty flag
      _dirty   = 0; // need this before calling next line...
      _xf      = Wtransf(o(), t(), b(), n());
      _inverse = _xf.inverse();      // 'true' means it is rigid motion
   }

   void update() const {
      if (_dirty)
         ((SimplexFrame*)this)->recompute();
   }
};
typedef const SimplexFrame CSimplexFrame;

#endif // SIMPLEX_FRAME_H_IS_INCLUDED

// end of file simplex_frame.H
