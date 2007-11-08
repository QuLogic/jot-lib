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
 * simplex_filter.H
 *****************************************************************/
#ifndef SIMPLEX_FILTER_H_IS_INCLUDED
#define SIMPLEX_FILTER_H_IS_INCLUDED

#include "bsimplex.H"

// Base class SimplexFilter is defined in Bsimplex.H

/*****************************************************************
 * BitSetSimplexFilter:
 *
 *   Accept a simplex if a given bit is set (in its flag).
 *****************************************************************/
class BitSetSimplexFilter : public SimplexFilter {
 public:
   BitSetSimplexFilter(uint b) : _b(b) {}

   virtual bool accept(CBsimplex* s) const {
      return (s && s->is_set(_b));
   }

 protected:
   uint _b;
};
typedef const BitSetSimplexFilter CBitSetSimplexFilter;

/*****************************************************************
 * BitClearSimplexFilter:
 *
 *   Accept a simplex if a given bit is clear (in its flag).
 *****************************************************************/
class BitClearSimplexFilter : public SimplexFilter {
 public:
   BitClearSimplexFilter(uint b) : _b(b) {}

   virtual bool accept(CBsimplex* s) const {
      return (s && s->is_clear(_b));
   }

 protected:
   uint _b;
};
typedef const BitClearSimplexFilter CBitClearSimplexFilter;

/*****************************************************************
 * MeshSimplexFilter:
 *
 *   Accept a simplex from the given mesh
 *****************************************************************/
class MeshSimplexFilter : public SimplexFilter {
 public:
   MeshSimplexFilter(BMESH* m) : _mesh(m) {}

   virtual bool accept(CBsimplex* s) const {
     return (s && s->mesh()==_mesh);
   }

 protected:
   BMESH* _mesh;
};
typedef const MeshSimplexFilter CMeshSimplexFilter;

/*****************************************************************
 * BvertFilter:
 *
 *      Accepts Bverts.
 *****************************************************************/
class BvertFilter : public SimplexFilter {
 public:
   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const { return is_vert(s); }
};
typedef const BvertFilter CBvertFilter;

/*****************************************************************
 * BedgeFilter:
 *
 *      Accepts Bedges.
 *****************************************************************/
class BedgeFilter : public SimplexFilter {
 public:
   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const { return is_edge(s); }
};
typedef const BedgeFilter CBedgeFilter;

/*****************************************************************
 * BfaceFilter:
 *
 *      Accepts Bfaces.
 *****************************************************************/
class BfaceFilter : public SimplexFilter {
 public:
   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const { return is_face(s); }
};
typedef const BfaceFilter CBfaceFilter;

/*****************************************************************
 * SimplexFlagFilter:
 *
 *      Accepts a simplex if its flag has a given value.
 *****************************************************************/
class SimplexFlagFilter : public SimplexFilter {
 public:
   SimplexFlagFilter(uchar f=1) : _flag(f) {}

   uchar flag()                 const   { return _flag; }
   void  set_flag(uchar f)              { _flag = f; }

   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const {
      return s && (s->flag() == _flag);
   }

 protected:
   uchar  _flag;        // required value
};

/*****************************************************************
 * UnreachedSimplexFilter:
 *
 *      Accepts a simplex if its flag is not set (i.e. is zero),
 *      AND SETS THE FLAG, so next time it won't be accepted.
 *****************************************************************/
class UnreachedSimplexFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      if (!s || s->flag())
          return 0;
      ((Bsimplex*)s)->set_flag();
      return 1;
   }
};

/*****************************************************************
 * AndFilter:
 *
 *      Accepts the AND of two filters
 *****************************************************************/
class AndFilter : public SimplexFilter {
 public:
   AndFilter(CSimplexFilter& f1, CSimplexFilter& f2) :
      _f1(f1), _f2(f2) {}

   virtual bool accept(CBsimplex* s) const {
      return _f1.accept(s) && _f2.accept(s);
   }

 protected:
   CSimplexFilter& _f1;
   CSimplexFilter& _f2;
};

inline AndFilter
operator+(CSimplexFilter& f1, CSimplexFilter& f2)
{
   return AndFilter(f1,f2);
}

/*****************************************************************
 * OrFilter:
 *
 *      Accepts the OR of two filters
 *****************************************************************/
class OrFilter : public SimplexFilter {
 public:
   OrFilter(CSimplexFilter& f1, CSimplexFilter& f2) :
      _f1(f1), _f2(f2) {}

   virtual bool accept(CBsimplex* s) const {
      return _f1.accept(s) || _f2.accept(s);
   }

 protected:
   CSimplexFilter& _f1;
   CSimplexFilter& _f2;
};

inline OrFilter
operator||(CSimplexFilter& f1, CSimplexFilter& f2)
{
   return OrFilter(f1,f2);
}

/*****************************************************************
 * NotFilter:
 *
 *      Accepts the NOT of a filter
 *****************************************************************/
class NotFilter : public SimplexFilter {
 public:
   NotFilter(CSimplexFilter& f) : _filter(f) {}

   virtual bool accept(CBsimplex* s) const {
      return !_filter.accept(s);
   }

 protected:
   CSimplexFilter& _filter;
};

inline NotFilter
operator!(CSimplexFilter& f)
{
   return NotFilter(f);
}

/*****************************************************************
 * SelectedSimplexFilter:
 *
 *      Accepts selected simplices.
 *****************************************************************/
class SelectedSimplexFilter : public SimplexFilter {
 public:
   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const {
      return s && s->is_selected();
   }
};
typedef const SelectedSimplexFilter CSelectedSimplexFilter;

#endif // SIMPLEX_FILTER_H_IS_INCLUDED

// end of file simplex_filter.H
