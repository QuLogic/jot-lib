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
#ifndef EDGE_STROKE_HEADER
#define EDGE_STROKE_HEADER

#include "std/config.H"
#include "stroke/outline_stroke.H"
#include "mesh/edge_strip.H"

class Patch;

#define CEdgeStrokeVertexData const EdgeStrokeVertexData

class EdgeStrokeVertexData : public BaseStrokeVertexData {
 public:

   const Bsimplex* sim;

   EdgeStrokeVertexData() : sim(0) {  }
   virtual ~EdgeStrokeVertexData() {}

   /******** MEMBER METHODS ********/

   virtual BaseStrokeVertexData* alloc(int n) 
      { return (n>0)?(new EdgeStrokeVertexData[n]):0; }

   virtual void dealloc(BaseStrokeVertexData *d) {
      if (d) {
         EdgeStrokeVertexData* hvd = (EdgeStrokeVertexData*)d;
         delete [] hvd;
      }
   }

   virtual BaseStrokeVertexData* elem(int n, BaseStrokeVertexData *d) 
      { return &(((EdgeStrokeVertexData *)d)[n]); }

   int operator==(const EdgeStrokeVertexData &) {
      cerr << "EdgeStrokeVertexData::operator==" 
           << " - Dummy called!\n";
      return 0; 
   }

   int operator==(const BaseStrokeVertexData & v) {
      return BaseStrokeVertexData::operator==(v);
   }

   virtual void copy(CBaseStrokeVertexData *d)
      {
         // XXX - We really should check that *d is 
         // of EdgeStrokeVertexData type...

         sim = ((CEdgeStrokeVertexData*)d)->sim;
      } 
};



#define CEdgeStroke const EdgeStroke

class EdgeStroke : public OutlineStroke {

 public:

   //****************************************
   // EdgeData
   //
   //   Data stored on a Bface by a EdgeStroke
   //   Can be used to look up the EdgeStroke
   //   given the Bfaces
   //****************************************
   class FaceData : public SimplexData {
    public:
      //******** MANAGERS ******** 

      // Use the EdgeStroke class name to
      // register EdgeStroke data on faces.
      // This assumes that only one
      // EdgeStroke can own a face.
      FaceData(EdgeStroke* s, Bface* f) :
         SimplexData(s->static_name(), f),
         _stroke(s) {}

      //******** ACCESSORS ********
      EdgeStroke* stroke() const { return _stroke; }
      Bface*      face()   const { return (Bface*) _simplex; }

      //******** SimplexData METHODS ********
      // XXX -- TO DO:  handle subdivision?
  
      /* read and write methods */
      DEFINE_RTTI_METHODS2("EdgeStroke::FaceData",
                           SimplexData, CSimplexData *);

    protected:
      EdgeStroke*   _stroke;        // owning stroke (1 per face)
   };
 
   //****************************************
   // EdgeStroke
   //****************************************


 public:
   EdgeStroke();

   virtual ~EdgeStroke() { }  

   virtual int         draw(CVIEWptr& v);

   void                set_edge_strip(EdgeStrip* strip);
                                       

   void   set_vis_step_pix_size(double s)      { _vis_step_pix_size = s; }

   static BMESH*       mesh()                  { return _mesh;       }
   static void         set_mesh(BMESH *mesh)   { _mesh = mesh;       }

   void  apply_offset(BaseStrokeVertex* v, BaseStrokeOffset* o);


   DEFINE_RTTI_METHODS2("EdgeStroke", OutlineStroke, CDATA_ITEM *);
   virtual DATA_ITEM *dup() const     { return copy(); }

   virtual             BaseStroke* copy() const;

   virtual void copy(CEdgeStroke& s);
   virtual void copy(COutlineStroke& o) { OutlineStroke::copy(o); }
   virtual void copy(CBaseStroke& s)    { BaseStroke::copy(s); }

   bool                check_vert_visibility(CBaseStrokeVertex &v);

   virtual void        interpolate_refinement_vert(BaseStrokeVertex *v, 
                                                   BaseStrokeVertex **vl, 
                                                   double u);

   void         clear_simplex_data();

 protected:

   EdgeStrip*  _strip;
   double _vis_step_pix_size;  // refinement step size (for visibility) in pixels

   // XXX hack for serialization
   static BMESH *_mesh;

 protected:

   // Utility function for refinement: 
   // add verts, distance 'step_size' apart, to the stroke 
   // along the given line segment.  
   void refine(mlib::CNDCZpt base_pt, 
               mlib::CNDCZvec vec, 
               double len, 
               double step_size,
               Bedge* e) {

      static int REFINE_LIMIT_HACK = Config::get_var_int("REFINE_LIMIT_HACK",50,true);

      double dist_so_far = step_size;

      // XXX - This goes sour behind the camera?!
      int cnt = 0;
      if ((base_pt[2]>0) && (base_pt[2]<1) && (fabs(base_pt[0])<=_max_x) && (fabs(base_pt[1])<=_max_y))
      {
         mlib::NDCZpt refine_pt = base_pt + (dist_so_far * vec);

         while((cnt < REFINE_LIMIT_HACK) && (dist_so_far < len) )
         {

            add(refine_pt);

            // Get the vertex we just added, so we can set its data.
            BaseStrokeVertex& refine_v = _verts[_verts.num()-1];
            assert(refine_v._data);

            ((EdgeStrokeVertexData *)refine_v._data)->sim = e;

            cnt++;
            dist_so_far += step_size;
            refine_pt = base_pt + (dist_so_far * vec);
         }
      }
   }

};


#endif // FEATURE_STROKE_HEADER

/* end of file feature_stroke.H */
