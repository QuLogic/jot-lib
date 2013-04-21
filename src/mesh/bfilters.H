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
 * bfilters.H
 **********************************************************************/
#ifndef BFILTERS_H_HAS_BEEN_INCLUDED
#define BFILTERS_H_HAS_BEEN_INCLUDED

#include "bmesh.H"
#include "mesh_global.H"

/*****************************************************************
 * FoldVertFilter:
 *
 *   Tell whether a Bvert is a "fold vertex" WRT a given edge
 *   filter... i.e., the vertex has degree 2 WRT that filter
 *   and both edges lie on the same side of the vertex normal
 *   when all are projected to screen space.
 *****************************************************************/
inline bool
is_fold(CWpt& a, mlib::CWpt& p, mlib::CWpt& b, mlib::CWvec& n)
{
   // Returns true if the sequence of points a, p, b
   // projected to image space lies all on one side of the
   // image space line defined by the middle point p and the
   // given vector n.

   VEXEL nv = mlib::VEXEL(p, n).perpend();
   return XOR((VEXEL(a, p-a) * nv) < 0, (mlib::VEXEL(b, b-p) * nv) < 0);
}

class FoldVertFilter: public BvertFilter {
 public:
   FoldVertFilter(CSimplexFilter& f) : _edge_filter(f) {}

   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const;

 protected:
   CSimplexFilter& _edge_filter;
};


/*****************************************************************
 * VertDegreeFilter:
 *
 *   Accept a vertex with a given degree WRT a given edge filter
 *****************************************************************/
class VertDegreeFilter: public BvertFilter {
 public:
   // by default checks the ordinary degree (counts edges):
   VertDegreeFilter(int n, CSimplexFilter& f = BedgeFilter()) :
      _n(n), _edge_filter(f) {}

   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const;

 protected:
   int                  _n;                     // required degree
   CSimplexFilter&      _edge_filter;
};

/*******************************************************
 * CornerTypeVertFilter:
 *
 *  Accepts a vertex whose "degree" WRT a given edge
 *  filter is 3 or more.
 *
 *******************************************************/
class CornerTypeVertFilter: public SimplexFilter {
 public:
   CornerTypeVertFilter(CSimplexFilter& f = BedgeFilter()) : _edge_filter(f) {}

   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const {
      if (!is_vert(s))
         return false;
      return ((Bvert*)s)->degree(_edge_filter) > 2;
   }

 protected:
   CSimplexFilter& _edge_filter;
};

/*****************************************************************
 * VertFaceDegreeFilter:
 *
 *   Accept a vertex with a given FACE degree WRT a given face filter
 *****************************************************************/
class VertFaceDegreeFilter: public SimplexFilter {
 public:
   VertFaceDegreeFilter(int n = 0, CSimplexFilter& f = BfaceFilter()) :
      _n(n), _face_filter(f) {}

   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const {
      if (!is_vert(s))
         return false;
      return (((Bvert*)s)->face_degree(_face_filter) == _n);
   }

 protected:
   int                  _n;             // required degree
   CSimplexFilter&      _face_filter;
};

#endif  // BFILTERS_H_HAS_BEEN_INCLUDED

/* end of file bfilters.H */
