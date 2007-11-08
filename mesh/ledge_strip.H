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
 * ledge_strip.H:
 **********************************************************************/
#ifndef LEDGE_STRIP_H_IS_INCLUDED
#define LEDGE_STRIP_H_IS_INCLUDED

#include "edge_strip.H"         // base class
#include "lface.H"              // subdivision faces, edges, verts

/**********************************************************************
 * LedgeStrip:
 *
 *      Edge strip class for LMESHs (subdivision meshes).
 *
 *      An edge strip in the control mesh corresponds to an
 *      edge strip with twice as many edges in the next-level
 *      subdivision mesh.
 **********************************************************************/
#define CLedgeStrip const LedgeStrip
class LedgeStrip : public EdgeStrip {
 public:
   //******** MANAGERS ********

   // Create an empty strip:
   LedgeStrip() : _substrip(0) {}

   // Given a list of edges to search, build a strip of
   // edges that satisfy a given property.
   LedgeStrip(CBedge_list& list, CBedgeFilter& filter) :
      EdgeStrip(list, filter),
      _substrip(0) {}

   virtual ~LedgeStrip() { delete_substrip(); }

   void clear_subdivision(int level);

   // Assignment operator:
   virtual EdgeStrip& operator=(CEdgeStrip& strip) {
      delete_substrip();
      return EdgeStrip::operator=(strip);
   }

   //******** ACCESSORS ********

   LMESH* lmesh()               const   { return (LMESH*)mesh(); }

   int    cur_level()           const;  // level of "current" mesh in hierarchy
   int    rel_cur_level()       const;  // cur level relative to this mesh

   //******** BedgeStrip VIRTUAL METHODS ********

   virtual void reset() { delete_substrip(); EdgeStrip::reset(); }

   virtual void draw(StripCB* cb) { draw(rel_cur_level(), cb); }

   // Returns the child strip at the current subdivision level:
   virtual CEdgeStrip* cur_strip() const { return get_strip(rel_cur_level()); }

   // Returns the child strip at the given subdivision level,
   // relative to this strip. E.g., returns _substrip when k == 1.
   virtual CEdgeStrip* sub_strip(int k) const { return get_strip(k); }

 protected:
   //******** MEMBER DATA ********
   LedgeStrip*  _substrip;  // corresponding finer-level edge strip

   //******** MANAGING SUBSTRIP ********

   // Returns the child strip at the given subdivision level,
   // relative to this strip. E.g., returns _substrip when k == 1.
   const LedgeStrip* get_strip (int level) const { 
      if (level<=0)
         return this; 
      else {
         ((LedgeStrip*)this)->generate_substrip();
         return _substrip->get_strip(level-1);
      }
   }
   void delete_substrip() { delete _substrip; _substrip = 0;}
   void generate_substrip();

   bool need_rebuild() const;

   //******** DRAWING (INTERNAL) ********
   void draw(int level, StripCB* cb);
};

#endif // LEDGE_STRIP_H_IS_INCLUDED

/* end of file ledge_strip.H */
