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
/***************************************************************************
pattern_stroke.H

    PatternStroke
        -Stroke used to render Pattern3dStroke(pattern_group.c) in 2d
        -Derives from BaseStroke
        -Has check_vert_visibility Basestroke method to use visibility 
         check: if there is a face on the ref image(thus visible) that
         intersects with the location of the vertex, then draw it.
-------------------
    Simon Breslav
    Fall 2004
***************************************************************************/
#ifndef _PATTERN_STROKE_H_IS_INCLUDED_
#define _PATTERN_STROKE_H_IS_INCLUDED_

//#include "stroke/base_stroke.H"
//#include "pattern/quad_cell.H"
//#include <map>
#include "std/config.H"
#include "geom/gl_view.H"
// *****************************************************************
// * PatternStroke
// *****************************************************************
#define CPatternStroke PatternStroke const
class PatternStroke {
public:   
   PatternStroke(CNDCZvec_list& g);
   virtual ~PatternStroke() {}  
   void draw(CNDCZpt& c);
private:
   NDCZvec_list _pts;
};




/*
#define CPatternVertexData PatternVertexData const
class PatternVertexData : public BaseStrokeVertexData {
 public:
   CBface* _face; 
   
   PatternVertexData(): _face(0) { }
   virtual ~PatternVertexData(){}
   // ******** MEMBER METHODS ******** //
   virtual BaseStrokeVertexData* alloc(int n) 
      { return (n>0)?(new PatternVertexData[n]):0; }

   virtual void dealloc(BaseStrokeVertexData *d) {
      if (d) {
         PatternVertexData* hvd = (PatternVertexData*)d;
         delete [] hvd;
      }
   }

   virtual BaseStrokeVertexData* elem(int n, BaseStrokeVertexData *d) 
      { return &(((PatternVertexData *)d)[n]); }

   int operator==(const PatternVertexData &) 
      {       cerr << "HatchingVertexData::operator== - Dummy called!\n";     return 0; }

   int operator==(const BaseStrokeVertexData & v) {
      return BaseStrokeVertexData::operator==(v);
      return 0;
   }

   virtual void copy(CBaseStrokeVertexData *d){
      _face = ((CPatternVertexData*)d)->_face;
   } 
  
};

// *****************************************************************
// * PatternStroke
// *****************************************************************
#define CPatternStroke PatternStroke const
class PatternStroke : public BaseStroke {
public:
   
   PatternStroke();
   virtual ~PatternStroke() {}   
      
   // ******** BaseStroke METHODS ******** //    
   virtual BaseStroke*  copy() const;   
   virtual void         copy(CPatternStroke& v); 
   virtual void         copy(CBaseStroke& v) { BaseStroke::copy(v); }
   virtual bool         check_vert_visibility(CBaseStrokeVertex &v);
   
   virtual BaseStrokeVertexData *   data_proto() { return new PatternVertexData; }
  
   void add(mlib::CNDCZpt& pt,CBface* my_face, double width, double alpha, mlib::CWvec& norm,  bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      v._norm = norm; 
      v._width = width;
      v._alpha = alpha;
      ((PatternVertexData *)v._data)->_face = my_face;     
      set_dirty();
   }
   // ******** DATA_ITEM METHODS ******** //
   DEFINE_RTTI_METHODS2("PatternStroke", BaseStroke, CDATA_ITEM *);
   virtual DATA_ITEM*   dup() const             { return copy(); }
   static bool taggle_vis;
};

// *****************************************************************
// * Pattern3dStroke
// *****************************************************************
#define CPattern3dStroke const Pattern3dStroke
class Pattern3dStroke : public DATA_ITEM {

 protected:
   // ******** MEMBER VARIABLES ******** //
   QuadCell*            _cell;  
   PatternStroke*       _stroke;   
     // Basic stroke parameters
   //ARRAY<PatternVertex> _verts;
   ARRAY<CBface*>       _faces;
   mlib::Wpt_list             _pts;
   ARRAY<mlib::Wvec>          _norms;
   
   ARRAY<mlib::Wvec>          _bar;
   ARRAY<double>        _alpha;  // chached alpha
   ARRAY<double>        _width;  // chached width
 
   mlib::UVpt_list            _uv_p;   // uv point relative to the cell 
   
   mlib::UVpt_list            _offset_uv;   // uv realative to central point
   mlib::UVpt                 _avg_uv_pt;        // central point
   
   double               _pix_size;
   double               _cam_dist;
   //mlib::Wvec                 _light;
   //int                  _light_num;
      
   BaseStrokeOffsetLISTptr    _offsets;  
  
   bool                 _visible;
   
   // For simplisity just get it from the gesture when we draw it 
   double               _winding; 
   double               _straightness;
   double               _area;
   double               _start_width;  //cache the width
 public:
   // ******** CONSTRUCTOR/DESTRUCTOR ******** //
   // Creating a new cell
   Pattern3dStroke(QuadCell* cell,         
                CARRAY<CBface*>& faces,
                mlib::CWpt_list& pl,
                CARRAY<mlib::Wvec>& nl,
                CARRAY<mlib::Wvec>& bar,
                CARRAY<double>& alpha, 
                CARRAY<double>& width, 
                mlib::CUVpt_list& uv_p, 
                CBaseStrokeOffsetLISTptr& ol,
                BaseStroke * proto, 
                double winding_, 
                double straightness_);
   
   // Making a copy
   // XXX - copy cell ndc area to the new cell, this assumes that strokes
   // will be copied in the cells of the same dimentions, this is not hard
   // to fix and if two cells differ in size could use UV to translate the
   // area...however cannot just get the area since if the cell in the oblique
   // position then the area is messed up... 
   Pattern3dStroke(QuadCell* cell, 
                double pix_size,
                double start_width,
                //mlib::Wvec light,
                //int light_num,               
                CARRAY<CBface*> &faces,
                mlib::CWpt_list &pl,
                CARRAY<mlib::Wvec> &nl,
                CARRAY<mlib::Wvec>& bar,
                CARRAY<double>& alpha, 
                CARRAY<double>& width, 
                mlib::CUVpt_list& uv_p, 
                CBaseStrokeOffsetLISTptr &ol,
                BaseStroke * proto,
                double winding_,
                double straightness_);
               
   Pattern3dStroke(){ }
   ~Pattern3dStroke();

   // ******** MEMBER METHODS ******** //
   void         draw_setup();
   void         draw_start();
   int          draw(CVIEWptr &v);
   void         draw_end();
   
   double       get_light_ratio(mlib::Wvec normal);
  
   static mlib::UVpt  uv_2_offsetUV(mlib::UVpt avg_pt, mlib::UVpt uv)       
      { return mlib::UVpt(uv[0]-avg_pt[0], uv[1]-avg_pt[1]);}
   static mlib::UVpt  offsetUV_2_uv(mlib::UVpt avg_pt, mlib::UVpt offset_uv)
      { return mlib::UVpt(avg_pt[0]+offset_uv[0], avg_pt[1]+offset_uv[1]);}
      
   static mlib::UVpt_list  uv_2_offsetUV(mlib::UVpt avg_pt, mlib::UVpt_list uv);
   static mlib::UVpt_list  offsetUV_2_uv(mlib::UVpt avg_pt, mlib::UVpt_list offset_uv); 
   
   //static mlib::UVpt_list  scaleUV(mlib::UVpt my_avg_pt, mlib::UVpt_list my_offset_uv,
   //                         mlib::UVpt avg_pt, mlib::UVpt min, mlib::UVpt max);                                        )   
   
   // Stroke Proparties
   COLOR        color()        const { return HSVCOLOR(_stroke->get_color()); }
   double       length()       const { return _pts.length() / sqrt(_cell->area()); } 
   double       spread()       const { return _pts.spread(); }     
   double       winding()      const { return _winding; }      
   double       straightness() const { return _straightness; }
   double       area()         const { return _area; }
   
   bool         is_parallel_to(Pattern3dStroke *, double& avg_dist, bool dir);

   static bool stroke_distance_dir (Pattern3dStroke* a, Pattern3dStroke* b);
   static mlib::Wvec avarage_vector(Pattern3dStroke* a);   
   // Accessorts
   QuadCell*                    get_cell()         const{ return _cell;       }
   mlib::CWpt_list &                  get_pts()          const{ return _pts;        }
   CARRAY<mlib::Wvec>&                get_norms()        const{ return _norms;      }
   int                          get_verts_num()    const{ return _pts.num();  }    
   
   CARRAY<mlib::Wvec>&                get_bar()          const{ return _bar;        }  
   CARRAY<double>&              get_alpha()        const{ return _alpha;      }
   double                       get_alpha(int i)   const{ return _alpha[i];   }
   void                         set_alpha(int i, double val) { _alpha[i] = val; }
   CARRAY<double>&              get_width()        const{ return _width;      }
   double                       get_width(int i)   const{ return _width[i];   }
   
   mlib::CUVpt_list&                  get_uv_p()         const{ return _uv_p;       }
   mlib::CUVpt&                       get_uv_p(int i)    const{ return _uv_p[i];    }
   mlib::CUVpt_list&                  get_offset_uv()    const{ return _offset_uv;}
   mlib::CUVpt&                       get_avg_uv_pt()    const{ return _avg_uv_pt;     }

   double                       get_pix_size()     const{ return _pix_size;   }
   double                       get_start_width()  const{ return _start_width;   }
   //mlib::Wvec                         get_light()        const{ return _light;      }
   //int                          get_light_num()    const{ return _light_num;  }
   PatternStroke*               get_stroke()       const{ return _stroke;     }
   const BaseStrokeOffsetLISTptr& get_offsets()    const{ return _offsets;    }
   
   // Reverse all the arrays
   void                         reverse();
 protected:
   void         stroke_real_setup();
   void         stroke_pts_setup();
   // ******** DATA_ITEM METHODS ******** //
   static TAGlist       *_ps_tags;

 public:
   DEFINE_RTTI_METHODS_BASE("Pattern3dStroke", CDATA_ITEM*);
   virtual DATA_ITEM    *dup() const { return new Pattern3dStroke; }
   virtual CTAGlist     &tags() const;  
   
   // ******** I/O *********** //
   void    put_faces (TAGformat &d) const;
   void    get_faces (TAGformat &d);
   
   void    get_pts (TAGformat &d);
   void    put_pts (TAGformat &d) const;

   void    get_norms (TAGformat &d);
   void    put_norms (TAGformat &d) const;
   
   void    put_alpha(TAGformat &d) const;
   void    get_alpha (TAGformat &d);

   void    put_width(TAGformat &d) const;
   void    get_width(TAGformat &d);

   void    put_pix_size(TAGformat &d) const;
   void    get_pix_size(TAGformat &d);   

};

#define CStroke_List const Stroke_List
class Stroke_List : public ARRAY<Pattern3dStroke *>
{
  public:
     typedef std::map<double, int, std::less<double> > stroke_map_less;
    
    Stroke_List() : _color(CCOLOR(0,0,0)), 
                    _avg_length(0),
                    _avg_spread(0),
                    _avg_winding(0),
                    _avg_straightness(0),
                    _area(0),
                    _group_id(0){}
		
    void set_group_id(int i) { _group_id = i; }
    int  get_group_id() const{ return _group_id; }
    //Go through the list of strokes given and pull out the once
    // that qualify and remove them from the list given
    virtual void   add_strokes(ARRAY<Pattern3dStroke*>& stroke);
    void update() { update_avg_info(); 
	    //cerr <<  "Density is " << density()<< endl;
	 }
    //Go through the owned strokes and make sure they qualify
    //If they don't put them in the list in the parameter
    void   refine_list(ARRAY<Pattern3dStroke*>& wrong_strokes){
      strokes_not_good(wrong_strokes);
   }
    double difference_to_group(Stroke_List* list){
      return is_differnt_to_group(list);     
    }
    
    // Accessors
    mlib::Wvec     avarage_vector()   const;
    COLOR    color()            const { return _color; }
    double   avg_length()       const { return _avg_length; }
    double   avg_spread()       const { return _avg_spread; }
    double   avg_winding()      const { return _avg_winding; }
    double   avg_straightness() const { return _avg_straightness; }     
    double   density() const { return (empty()) ? 0 : _area / _array[0]->get_cell()->area(); }
    
    DEFINE_RTTI_METHODS_BASE("Stroke_List", CDATA_ITEM*);  
  protected:  
    virtual double is_differnt_to_group(Stroke_List*) { return 0; }
    
   // If it is the same color
   virtual bool stroke_can_be_added(Pattern3dStroke* stroke);
   
    // Make sure it's a big enouth group
   virtual void strokes_not_good(ARRAY<Pattern3dStroke*>&);   
   void   update_avg_info();
   

   Pattern3dStroke* add_closest_parallel_stroke(Pattern3dStroke* start_stroke,
                                             ARRAY<Pattern3dStroke*>& unsorted,
                                             bool dir);
   // Get a closest stroke from within the list of strokes
   //stroke_map_t::iterator sort_strokes(Pattern3dStroke* to_stroke, 
   //               stroke_map_t& sorted,
   //               ARRAY<Pattern3dStroke*>& unsorted);
  private:
    COLOR              _color;
    double             _avg_length;
    double             _avg_spread;
    double             _avg_winding;
    double             _avg_straightness;
    // Density For the whole list
    double             _area;
    int                _group_id;

};
typedef ARRAY<Stroke_List*> Group_List;
   
#define CStructured_Hatching const Structured_Hatching
class Structured_Hatching : public Stroke_List
{
 public:
   virtual void   add_strokes(ARRAY<Pattern3dStroke*>& stroke);
   DEFINE_RTTI_METHODS3("Structured_Hatching", Structured_Hatching*, Stroke_List, CStroke_List*);
 protected: 
   virtual bool   stroke_can_be_added(Pattern3dStroke*);   
   virtual double is_differnt_to_group(Stroke_List*);
   virtual void strokes_not_good(ARRAY<Pattern3dStroke*>&);   
};



#define CStippling const Stippling
class Stippling : public Stroke_List
{
 public:

 DEFINE_RTTI_METHODS3("Stippling", Stippling*, Stroke_List, CStroke_List*);
 protected:
   virtual bool stroke_can_be_added(Pattern3dStroke*); 
   virtual double is_differnt_to_group(Stroke_List*);
   virtual void strokes_not_good(ARRAY<Pattern3dStroke*>&);   
};

#define COtherStrokes const OtherStrokes
class OtherStrokes : public Stroke_List
{
 public:
   DEFINE_RTTI_METHODS3("OtherStrokes", OtherStrokes*, Stroke_List, CStroke_List*);
 protected:
   virtual double is_differnt_to_group(Stroke_List*);
   virtual void strokes_not_good(ARRAY<Pattern3dStroke*>&) {}
   virtual bool stroke_can_be_added(Pattern3dStroke*); 
};

#define CNullStrokes const NullStrokes
class NullStrokes : public Stroke_List
{
 public:
   DEFINE_RTTI_METHODS3("NullStrokes", NullStrokes*, Stroke_List, CStroke_List*);
 protected:
   virtual double is_differnt_to_group(Stroke_List*);   

};
*/

#endif
