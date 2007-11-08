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

#ifndef DECAL_LINE_STROKE_HEADER
#define DECAL_LINE_STROKE_HEADER

#include "stroke/outline_stroke.H"
#include "mlib/points.H"

class Bsimplex;
class BMESH;
class Patch;

#define CDecalVertexData const DecalVertexData

class DecalVertexData : public BaseStrokeVertexData {

 public:

   const Bsimplex* sim;

   DecalVertexData() : sim(0) {  }
   virtual ~DecalVertexData() {}

   /******** MEMBER METHODS ********/

   virtual BaseStrokeVertexData* alloc(int n) 
      { return (n>0)?(new DecalVertexData[n]):0; }

   virtual void dealloc(BaseStrokeVertexData *d) {
      if (d) {
         DecalVertexData* hvd = (DecalVertexData*)d;
         delete [] hvd;
      }
   }

   virtual BaseStrokeVertexData* elem(int n, BaseStrokeVertexData *d) 
      { return &(((DecalVertexData *)d)[n]); }

   int operator==(const DecalVertexData &) 
      {       cerr << "DecalVertexData::operator== - Dummy called!\n";     return 0; }

   int operator==(const BaseStrokeVertexData & v) {
      return BaseStrokeVertexData::operator==(v);
      return 0;
   }

   virtual void copy(CBaseStrokeVertexData *d)
      {
         // XXX - We really should check that *d is 
         // of DecalVertexData type...

         sim = ((CDecalVertexData*)d)->sim;
      } 

};




#define CDecalLineStroke const DecalLineStroke

class DecalLineStroke : public OutlineStroke {

 private:
   static TAGlist*              _dls_tags;

 public:
   
   class vert_loc {
    public:
      vert_loc() {}  // need this to instantiate ARRAY<vert_loc>
      vert_loc(mlib::CWpt pt, const Bsimplex* s=0, double pr = 1.0): 
         loc(pt), sim(s), press(pr) {}
    public:
      mlib::Wpt loc;  
      const Bsimplex* sim;
      double press;
 
      bool operator==(const vert_loc&) {
         cerr << "WARNING: dummy vert loc operator== called" << endl;
         return true;
      }

   };

 protected:

   ARRAY<vert_loc>  _vert_locs;

   // XXX hack for serialization
   static BMESH *_mesh;

 protected:

   /******** I/O Methods ********/
   //Deprecated
   void                 get_vert_locs (TAGformat &d);
   void                 put_vert_locs (TAGformat &d) const;
   //Preferred
   void                 get_vertex_loc (TAGformat &d);
   void                 put_vertex_locs (TAGformat &d) const;


 public:
   DecalLineStroke(Patch* p=0);

   virtual ~DecalLineStroke() {}  

   virtual int   draw(CVIEWptr &);

   virtual       BaseStroke* copy() const;

   virtual void copy(CDecalLineStroke& v) { OutlineStroke::copy(v); }
   virtual void copy(COutlineStroke& o)   { OutlineStroke::copy(o); }
   virtual void copy(CBaseStroke& v)      { BaseStroke::copy(v); }

   virtual void  clear();

   int   add_vert_loc(mlib::CWpt loc, 
                      const Bsimplex* s=0, 
                      double press = 1.0);

   int   num_vert_locs() { return _vert_locs.num(); }

   CARRAY<vert_loc>&  get_vert_locs() {
      return _vert_locs;
   }

   BaseStrokeVertex*  refine_vert(int i, bool left);

   /* read and write methods */
   DEFINE_RTTI_METHODS2("DecalLineStroke", OutlineStroke, CDATA_ITEM *);
   virtual DATA_ITEM*  dup() const     { return copy(); }
   virtual CTAGlist&   tags() const;

   static BMESH *mesh()                { return _mesh;}
   static void   set_mesh(BMESH *mesh) { _mesh = mesh;}

   // prevent warnings:
   virtual void interpolate_vert(BaseStrokeVertex *v,
                                 BaseStrokeVertex *vleft, double u) {
      BaseStroke::interpolate_vert(v, vleft, u);
   }

   virtual void   interpolate_vert(BaseStrokeVertex *v,
                                   BaseStrokeVertex **vl, 
                                   double u);

   virtual void        interpolate_refinement_vert(BaseStrokeVertex *v, 
                                                   BaseStrokeVertex **vl, 
                                                   double u);

   bool check_vert_visibility(CBaseStrokeVertex &v);

   void                xform_locations(mlib::CWtransf& t);

};


#endif // DECAL_LINE_STROKE_HEADER

/* end of file decal_line_stroke.H */
