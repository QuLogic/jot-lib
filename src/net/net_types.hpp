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
/* Copyright 1995, Brown Computer Graphics Group.  All Rights Reserved. */

/* -------------------------------------------------------------------------
 *
 *                <     File description here    >
 *
 * ------------------------------------------------------------------------- */

#ifndef NET_TYPES_HAS_BEEN_INCLUDED
#define NET_TYPES_HAS_BEEN_INCLUDED

#include "std/support.H"
#include "mlib/points.H"
#include "mlib/point2i.H"
#include "mlib/point3i.H"
#include "stream.H"
#include "net.H"

template <class V>
inline STDdstream &operator<<(STDdstream &ds, const mlib::Vec2<V> &v) 
       { ds << "{" << v[0] << v[1] << "}";
         return ds; }

template <class V>
inline STDdstream &operator>>(STDdstream &ds, mlib::Vec2<V> &v) 
       { char brace; ds >> brace >> v[0] >> v[1] >> brace;
         return ds; }

template <class V>
inline STDdstream &operator<<(STDdstream &ds, const mlib::Vec3<V> &v)
       { ds <<"{"<< v[0] << v[1] << v[2]<<"}";
         return ds; }

template <class V>
inline STDdstream &operator>>(STDdstream &ds, mlib::Vec3<V> &v)
       { char brace;
         ds >> brace >> v[0] >> v[1] >> v[2] >> brace;
         return ds;}

template <class P, class V>
inline STDdstream &operator<<(STDdstream &ds, const mlib::Point3<P,V> &v) 
       { ds << "{" << v[0]<< v[1]<< v[2]<<"}";
         return ds; }

template <class P, class V>
inline STDdstream &operator>>(STDdstream &ds, mlib::Point3<P,V> &v) 
       { char brace;
         ds >> brace >> v[0] >> v[1] >> v[2] >> brace;
         return ds;}

inline STDdstream &operator<<(STDdstream &ds, const mlib::Point3i  &v) 
       { ds << "{" << v[0]<< v[1]<< v[2]<<"}";
         return ds; }

inline STDdstream &operator>>(STDdstream &ds, mlib::Point3i &v) 
       { char brace;
         ds >> brace >> v[0] >> v[1] >> v[2] >> brace;
         return ds;}


inline STDdstream &operator<<(STDdstream &ds, const mlib::Point2i  &v) 
       { ds << "{" << v[0]<< v[1]<<"}";
         return ds; }

inline STDdstream &operator>>(STDdstream &ds, mlib::Point2i &v) 
       { char brace;
         ds >> brace >> v[0] >> v[1] >> brace;
         return ds;}

inline STDdstream &operator<<(STDdstream &ds, const bool &b) 
      { int val = (b)?(1):(0);
         ds << "{" << val << "}";
         return ds; }

inline STDdstream &operator>>(STDdstream &ds, bool &b) 
       { int val;
         char brace;
         ds >> brace >> val >> brace;
         b = (val==1)?(true):(false);
         return ds;}


template <class P, class V>
inline STDdstream &operator<<(STDdstream &ds, const mlib::Point2<P,V> &v) 
       { ds << "{" << v[0] << v[1] << "}";
         return ds; }

template <class P, class V>
inline STDdstream &operator>>(STDdstream &ds, mlib::Point2<P,V> &v) 
       { char brace; ds >> brace >>v[0] >> v[1] >> brace;
         return ds; }




template <class L, class P, class V>
inline STDdstream &operator>>(STDdstream &ds, mlib::Line<L,P,V> &l)
       { return ds >> l.point() >> l.direction(); }
 
template <class L, class P, class V>
inline STDdstream &operator<<(STDdstream &ds, const mlib::Line<L,P,V> &l)
       { return ds << l.point() << l.direction(); }

template <class M, class P, class V, class L, class Q>
inline STDdstream &operator>>(STDdstream &ds,  mlib::Mat4<M,P,V,L,Q> &mat)
       { char brace;
         ds >> brace;
         for (int i=0; i < 4; i++) {
              ds >> brace;
              for (int j=0; j < 4; j++)
                 ds >> mat(i,j);
              ds >> brace;
         }
         ds >> brace;
         return ds; }

template <class M, class P, class V, class L, class Q>
inline STDdstream &operator<<(STDdstream &ds, const mlib::Mat4<M,P,V,L,Q> &mat)
     { ds << "{";
       for (int i=0; i < 4; i++) {
           ds << "{";
           for (int j=0; j < 4; j++)
               ds << mat(i,j);
           ds << "}";
       }
       ds << "}";
       return ds; }
        
template <class Q, class M, class P, class V, class L>
inline STDdstream  &operator<<(STDdstream &ds, const mlib::Quat<Q,M,P,V,L> &quat)
                                  { return ds << quat.v() << quat.w(); }

template <class Q, class M, class P, class V, class L>
inline STDdstream &operator>>(STDdstream &ds, mlib::Quat<Q,M,P,V,L> &quat)
                                  { return ds >> quat.v() >> quat.w(); }

template <class PLANE, class P, class V, class L>
inline STDdstream  &operator<<(STDdstream &ds, const mlib::Plane<PLANE,P,V,L> &plane)
     { return ds << "{" << plane.normal() << plane.d()<< "}"; }

template <class PLANE, class P, class V, class L>
inline STDdstream &operator>>(STDdstream &ds, mlib::Plane<PLANE,P,V,L> &plane)
     { char brace;
       return ds >> brace >> plane.normal() >> plane.d() >> brace; }

template <class T>
inline STDdstream  &operator<<(STDdstream &ds, const ARRAY<T> &list) {
   ds << "{";
   for (int i=0; i<list.num();i++) {
      ds << list[i];
   }
   ds << "}";
   return ds; 
}

template <class T>
inline STDdstream &operator>>(STDdstream &ds, ARRAY<T> &list) {
   char brace; ds >> brace;
   while (ds.check_end_delim()) {
      // Declare 'var' inside this loop (not outside) in case it is
      // an ARRAY type. Since the list is not being cleared before
      // we read from the stream, we want it to start out empty
      // each time thru this loop.
      T var;
      ds >> var;
      list.add(var);
   } 
   return ds >> brace;
}

template <class T>
inline STDdstream  &operator<<(STDdstream &ds, const vector<T> &list) {
   ds << "{";
   for (auto & elem : list) {
      ds << elem;
   }
   ds << "}";
   return ds;
}

template <class T>
inline STDdstream &operator>>(STDdstream &ds, vector<T> &list) {
   char brace; ds >> brace;
   while (ds.check_end_delim()) {
      // Declare 'var' inside this loop (not outside) in case it is
      // an vector type. Since the list is not being cleared before
      // we read from the stream, we want it to start out empty
      // each time thru this loop.
      T var;
      ds >> var;
      list.push_back(var);
   }
   return ds >> brace;
}

// XXX - should move to mlib Point2.H and Vec2.H
template <class P, class V>
inline istream&
operator>>(istream &is, mlib::Point2<P,V> &v) {
   char foo;
   return is >> foo >> v[0] >> v[1] >> foo;
}

template <class V>
inline istream &
operator>>(istream &is, mlib::Vec2<V> &v) 
{ 
   char dummy;
   return is >> dummy >> v[0] >> dummy >> v[1] >> dummy;
}


#endif  /* NETWORK_HAS_BEEN_INCLUDED */
