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
 * face_pair.H
 *****************************************************************/
#ifndef FACE_PAIR_H_IS_INCLUDED
#define FACE_PAIR_H_IS_INCLUDED

#include "mesh/bface.H"

/*****************************************************************
 * FacePair: (copied from class VertPair)
 *
 *  Simplex data stored on a given face with an associated key,
 *  records a "partner" face.
 *
 *  Used by Rmemes (see Rsurf.H) to represent a surface that is
 *  "inflated" from a given surface region (as a Bface_list).
 *  FacePair is a SimplexData stored on the original faces via a
 *  known lookup key that is unique to those faces.  Given the
 *  key, you can check any face in the original faces and find
 *  the corresponding "inflated" face. Can also find the normal
 *  (WRT the original face set) at any face of the original
 *  faces.
 *   
 *****************************************************************/
class FacePair : public SimplexData {
 public:
   //******** MANAGERS ********

   // using key, record data on v, associating to partner p
   FacePair(uint key, Bface* f, Bface* p) : SimplexData(key, f), _p(p) {}

   //******** RUN TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("FacePair", FacePair*, SimplexData, CSimplexData*);

   //******** ACCESSORS ********

   Bface* face()    const { return (Bface*)simplex(); }
   Bface* partner() const { return _p; }

   // not sure if we want to be able to set the partner
   // outside the destructor

   //******** LOOKUPS ********

   // static methods useful when you know the key and want to
   // find the partner of a given face f

   // get the FacePair on f
   static FacePair* find_pair(uint key, CBface* f) {
      return f ? upcast(f->find_data(key)) : 0;
   }

   // find the partner of f
   static Bface* map_face(uint key, CBface* f) {
      FacePair* pair = find_pair(key, f);
      return pair ? pair->partner() : 0;
   }

   // find the partners of a whole set of faces
   // (returns all on success, or none)
   static bool map_faces(uint key, CBface_list& faces, Bface_list& ret) {
      ret.clear();
      for (int i=0; i<faces.num(); i++) {
         Bface* f = map_face(key, faces[i]);
         if (f)
            ret += f;
      }
      return (ret.num() == faces.num());
   }

   //******** SimplexData VIRTUAL METHODS ********

   virtual void notify_simplex_deleted()  {
      // Once things are worked out fully, this is never supposed
      // to happen. This is here temporarily for debugging.
      err_msg("FacePair::notify_simplex_deleted: not implemented");
      SimplexData::notify_simplex_deleted();
   }

 protected:
   Bface* _p; // associated partner face
};

/*****************************************************************
 * FacePairFilter:
 *
 *  Accepts Bfaces labelled w/ FacePair data using a given key.
 *****************************************************************/
class FacePairFilter : public SimplexFilter {
 public:
   FacePairFilter(uint key) : _key(key) {}

   uint key()           const   { return _key; }
   void set_key(uint k)         { _key = k; }

   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const {
      return is_face(s) ? FacePair::find_pair(_key, (Bface*)s) != NULL : false;
   }

 protected:
   uint _key;   // for looking up FaceData
};

typedef const FacePair CFacePair;

#endif // FACE_PAIR_H_IS_INCLUDED

/* end of file face_pair.H */
