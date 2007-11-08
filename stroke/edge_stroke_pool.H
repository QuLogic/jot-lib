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

#ifndef EDGE_STROKE_POOL_HEADER
#define EDGE_STROKE_POOL_HEADER

#include "stroke/b_stroke_pool.H"
#include "mesh/edge_strip.H"
#include "net/data_item.H"

class EdgeStroke;

#define CEdgeStrokePool const EdgeStrokePool
class EdgeStrokePool : public BStrokePool {
 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist          *_esp_tags;

 protected:
   EdgeStrip     _strip;  // Sequence of edges defining the "baseline" 
                          // associated with the strokes in this pool.

   // XXX hack for serialization
   static BMESH *_mesh;

 public:
   // XXX hack for simplex data key
   // Used to name via static_name, but this
   // class no londer is a DATA_ITEM so use
   // address of this unique int instead
   static int foo;

 public:

   //****************************************
   // EdgeData
   //
   //   Data stored on a Bedge by an EdgeStrokePool
   //   Can be used to look up the EdgeStrokePool
   //   given the Bedge.
   //****************************************
   class EdgeData : public SimplexData {
    public:
      //******** MANAGERS ********
      // use the EdgeStrokePool class name to register EdgeStrokePool data on
      // edges. this assumes that only one EdgeStrokePool can own an
      // edge.
      EdgeData(EdgeStrokePool* p, Bedge* e) :
         //SimplexData(p->static_name(), e),
         SimplexData((unsigned int)&(p->foo), e),
         _pool(p) {}

      //******** ACCESSORS ********
      EdgeStrokePool* pool() const { return _pool; }
      Bedge*  edge()  const { return (Bedge*) _simplex; }

      //******** SimplexData METHODS ********
      // XXX -- TO DO:  handle subdivision?
  

      /* read and write methods */
      DEFINE_RTTI_METHODS2("EdgeStrokePool::EdgeDate",
                           SimplexData, CSimplexData *);

    protected:
      EdgeStrokePool*   _pool;        // owning pool (1 per edge)
   };

   //****************************************
   // EdgeStrokePool
   //****************************************


 public:
   EdgeStrokePool(EdgeStroke* proto = 0);

   virtual ~EdgeStrokePool();

   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("EdgeStrokePool", BStrokePool, CBStrokePool*);
   virtual DATA_ITEM       *dup() const { return new EdgeStrokePool; }
   virtual CTAGlist        &tags() const;
   virtual STDdstream      &decode(STDdstream &ds);

   /******** I/O functions ********/

   virtual void            get_edge_strip (TAGformat &d);
   virtual void            put_edge_strip (TAGformat &d) const;

   
   virtual void         draw_flat(CVIEWptr &);

   void set_edge_strip(CEdgeStrip&  strip);
   CEdgeStrip& get_edge_strip() { return _strip; }

   void add_to_strip(Bvert* v, Bedge* e);
   void set_stroke_strips();
   void verify_offsets();
   
   istream &read_stream(istream &is);
   ostream &write_stream(ostream &os) const;

   static BMESH *mesh()                { return _mesh;}
   static void   set_mesh(BMESH *mesh) { _mesh = mesh;}
};


#endif // EDGE_STROKE_POOL_HEADER

/* end of file edge_stroke_pool.H */
