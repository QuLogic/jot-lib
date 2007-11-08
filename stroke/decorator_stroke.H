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
 * decorator_stroke.H:
 **********************************************************************/
#ifndef DECORATOR_STROKE_HEADER
#define DECORATOR_STROKE_HEADER

#include "view_stroke.H"
class Bedge;

class DecoratorStroke : public ViewStroke {
 protected:

   // The next 3 arrays are parallel:  an entry at 
   // index i of each array stores info for a vertex i
   // of the stroke. 

   ARRAY<CBedge*> _edges;  // edges on which vertices are located

   ARRAY<double>  _t_vals; // each entry gives distance along 
   // the corressponding edge at which  vertex 
   // is located (in object space)

   ARRAY<mlib::Wpt>     _surface_pts; 
   // object space locations of vertices (this is convenient
   // but redundant info, as it's already given by _edges
   // and _t_vals above)

 public:
   DecoratorStroke(Patch* p=0, double width=2);

   virtual ~DecoratorStroke() { 
   }  

   virtual int draw(CVIEWptr& v);

   // duplicates the stroke. The new stroke will have all of its
   // attributes copied, but will contain no vertices.
   virtual ViewStroke* dup_stroke() const;
   virtual void clone_attribs(ViewStroke& v) const;

   virtual void  clear();

   // Specify an object-space vertex location at distance
   // t along an edge.
   void  add_vert_loc(CBedge* edge, double t);
   int   num_verts() {return _surface_pts.num();}
   mlib::NDCZpt  getNDCZpt(int);
   ARRAY<mlib::Wpt> surface_points() {return _surface_pts;}

   /* read and write methods */
   DEFINE_RTTI_METHODS2("DecoratorStroke", ViewStroke, DATA_ITEM *);
   virtual DATA_ITEM *dup() const     { return dup_stroke(); }
};


#endif // DECORATOR_STROKE_HEADER

/* end of file decorator_stroke.H */
