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
#ifndef _BASE_STROKE_HEADER
#define _BASE_STROKE_HEADER

#include "disp/color.H"
#include "disp/view.H"
#include "geom/texture.H"
#include "net/data_item.H"

/*****************************************************************
 * BaseStrokeOffset
 *****************************************************************/

#define CBaseStrokeOffset const BaseStrokeOffset

class BaseStrokeOffset : public DATA_ITEM {

   /******** STATIC MEMBER VARIABLES ********/   
 private:

   static TAGlist* _bso_tags;

 public:

   /******** MEMBER TYPES ********/
   enum type_t {                // Offset types that facilitate breaks
      OFFSET_TYPE_BEGIN = 0,    // First offset in a segment
      OFFSET_TYPE_MIDDLE,       // All offsets in the middle of a segment
      OFFSET_TYPE_END           // Last offset in a segment
   };

   /******** MEMBER VARIABLES ********/
   double       _pos;           // Position of offset
   double       _len;           // Length of offset in pixels
   double       _press;         // Pressure
   int          _type;

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   BaseStrokeOffset() : _pos(0), _len(0), _press(0), _type(OFFSET_TYPE_MIDDLE) {}

   BaseStrokeOffset(double p, double l, double pr, type_t t)
      : _pos(p), _len(l), _press(pr), _type(t) {}

   virtual ~BaseStrokeOffset() {}
   
   /******** MEMBER METHODS ********/
   int operator==(const BaseStrokeOffset &) {
      cerr << "BaseStrokeOffset::operator== - Dummy called!\n";
      return 0;
   }

   CBaseStrokeOffset& operator=(const BaseStrokeOffset &o) {
      _pos      = o._pos;
      _len      = o._len;
      _press    = o._press;
      _type     = o._type;
      return *this;
   }
   
   /******** DATA_ITEM METHODS ********/

   DEFINE_RTTI_METHODS_BASE("BaseStrokeOffset", CDATA_ITEM *);
   virtual DATA_ITEM*   dup()   const  { return new BaseStrokeOffset; }
   virtual CTAGlist&    tags()  const;

 protected:
   /******** I/O Access Methods ********/
   double&      pos_()          { return _pos; }
   double&      len_()          { return _len; }
   double&      press_()        { return _press; }
   int&         type_()         { return _type; }

};

STDdstream  &operator>>(STDdstream &ds, BaseStrokeOffset &o); 

/*****************************************************************
 * BaseStrokeOffsetLIST
 *****************************************************************/

// An array of offsets.  Offsets should be provided in the
// order in which they are connected in a drawn stroke.
// There can be breaks within offsets; the first offset in
// a segment should be type OFFSET_TYPE_BEGIN, all interior
// offsets should be OFFSET_TYPE_MIDDLE and the last offset
// in the segment should be OFFSET_TYPE_END.

// XXX - I didn't feel like capitalizing the whole classname,
// but this IS a ref counted item

#define CBaseStrokeOffsetLIST    const BaseStrokeOffsetLIST
#define CBaseStrokeOffsetLISTptr const BaseStrokeOffsetLISTptr

MAKE_PTR_BASEC(BaseStrokeOffsetLIST);

class BaseStrokeOffsetLIST : public REFcounter,
                             protected ARRAY<BaseStrokeOffset>, 
                             public DATA_ITEM {

   /******** STATIC MEMBER VARIABLES ********/   
 private:
   static TAGlist*     _bsol_tags;

 protected:
   /******** MEMBER VARIABLES ********/
   //Dupped and serialized
   int    _hangover;       // Whether to allow the offset to spill off base
                           // stroke's ends. (set true for creases)
   double _pix_len;        // Desired pixel length of the base path
   int    _replicate;      // If true, use rubberstamping (use for non-creases)
   int    _manual_t;       // Sets policy when replicate is true...
                           // If false:
                           //   Use old method where client uses gen_t on stroke
                           //   and client sets stretch, phase, len on offsetlist
                           // If true:
                           //   Client manually sets t's (no gen_t) on stroke
                           //   and client sets lower/upper_t here (and on stroke)

   //Not dupped or serialized
   double _fetch_len;      // Pixel length of dude getting this offset
   double _fetch_phase;    // Phase 0-1 to adjust offsets by
   double _fetch_stretch;  // Stretching factor

   double _fetch_lower_t;  // t limit when doing _manual_t replication
   double _fetch_upper_t;  //

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   BaseStrokeOffsetLIST(int num=0)  : 
      ARRAY<BaseStrokeOffset>(num),
      _hangover(true),
      _pix_len(0), 
      _replicate(false),
      _manual_t(false),
      _fetch_len(0), 
      _fetch_phase(0), 
      _fetch_stretch(1.0),
      _fetch_lower_t(0.0),
      _fetch_upper_t(1.0) {}

   virtual ~BaseStrokeOffsetLIST() {}

   /******** MEMBER METHODS ********/

   inline double        get_pix_len() const        { return _pix_len;      }
   void                 set_pix_len(double l)      { _pix_len = l;         }

   inline int           get_replicate() const      { return _replicate;    }
   void                 set_replicate(int r)       { _replicate = r;       }

   inline int           get_hangover() const       { return _hangover;     }
   void                 set_hangover(int h)        { _hangover = h;        }

   inline int           get_manual_t() const       { return _manual_t;     }
   void                 set_manual_t(int m)        { _manual_t = m;        }

   inline double        get_fetch_len() const      { return _fetch_len;    }
   void                 set_fetch_len(double l)    { _fetch_len = l;       }

   inline double        get_fetch_phase() const    { return _fetch_phase;  }
   void                 set_fetch_phase(double p)  { _fetch_phase = p;     }

   inline double        get_fetch_stretch() const  { return _fetch_stretch;}
   void                 set_fetch_stretch(double s){ _fetch_stretch = s;   }

   inline double        get_fetch_lower_t() const  { return _fetch_lower_t;}
   void                 set_fetch_lower_t(double t){ _fetch_lower_t = t;   }

   inline double        get_fetch_upper_t() const  { return _fetch_upper_t;}
   void                 set_fetch_upper_t(double t){ _fetch_upper_t = t;   }

   void                 copy(CBaseStrokeOffsetLIST& o);


   //Gets an interpolated offset at the
   //given t.  This hack method should not
   //be used by unauthorized personel...
   void                 get_at_t(double t, BaseStrokeOffset& o) const;

   /******** EXPOSED ARRAY<> METHODS ********/

   // 'Real' array accessors
   ARRAY<BaseStrokeOffset>::num;
   ARRAY<BaseStrokeOffset>::empty;
   ARRAY<BaseStrokeOffset>::clear;
   ARRAY<BaseStrokeOffset>::add;
   ARRAY<BaseStrokeOffset>::operator[];
   ARRAY<BaseStrokeOffset>::operator+=;
   ARRAY<BaseStrokeOffset>::last;
   ARRAY<BaseStrokeOffset>::first;
   ARRAY<BaseStrokeOffset>::pop;

   // 'Fake' array accessors 
   void                 fetch(int i,BaseStrokeOffset& o);
   int                  fetch_num();

   /******** DATA_ITEM METHODS ********/

   DEFINE_RTTI_METHODS_BASE("BaseStrokeOffsetLIST", CDATA_ITEM *);
   virtual DATA_ITEM*   dup()  const { return new BaseStrokeOffsetLIST; }
   virtual CTAGlist&    tags() const;

 protected:
   /******** I/O Methods ********/
   void                 get_all_offsets (TAGformat &d);
   void                 put_all_offsets (TAGformat &d) const;

   void                 get_offset (TAGformat &d);
   void                 put_offsets (TAGformat &d) const;

   /******** I/O Access Methods ********/
   double&              pix_len_()                 { return _pix_len; }
   int&                 replicate_()               { return _replicate; }
   int&                 hangover_()                { return _hangover; }
   int&                 manual_t_()                { return _manual_t; }
};

/*****************************************************************
 * BaseStrokeVertexData
 *****************************************************************/

#define CBaseStrokeVertexData const BaseStrokeVertexData

class BaseStrokeVertexData {
 public:

   BaseStrokeVertexData() {}
   virtual ~BaseStrokeVertexData() {}

   /******** MEMBER METHODS ********/

   virtual BaseStrokeVertexData* alloc(int n) 
      { return (n>0)?(new BaseStrokeVertexData[n]):0; }

   virtual void dealloc(BaseStrokeVertexData *d) 
      { if (d) delete [] d; }

   virtual BaseStrokeVertexData* elem(int n, BaseStrokeVertexData *d) 
      { return &(((BaseStrokeVertexData *)d)[n]); }

   int operator==(const BaseStrokeVertexData &) {
      cerr << "BaseStrokeVertexData::operator== - Dummy called!\n";
      return 0;
   }

   CBaseStrokeVertexData& operator=(const BaseStrokeVertexData &) {
      cerr << "BaseStrokeVertexData::operator= - Dummy called!\n";
      return *this;
   }

   virtual void copy(CBaseStrokeVertexData *)
      {
         // Nothing to copy 
      }

};

/*****************************************************************
 * BaseStrokeVertex
 *****************************************************************/

#define CBaseStrokeVertex const BaseStrokeVertex

class BaseStrokeVertex {
 public:

   /******** MEMBER TYPES ********/
   enum vis_state_t {
      VIS_STATE_VISIBLE = 0,     // Visible
      VIS_STATE_TRANSITION,      // Vertex marks a transition
      VIS_STATE_CLIPPED          // Not visible
   };

   enum vis_reason_t {
      VIS_REASON_NONE = 0,       // No reason given (unused/initial state)
      VIS_REASON_BAD_VERT,       // Vert's _good flag is false
      VIS_REASON_OFF_SCREEN,     // Vertex off screen
      VIS_REASON_BACK_FACING,    // Normal fails front facing test
      VIS_REASON_SINGULAR,       // Vert is neighboured by clipped verts
      VIS_REASON_ENDPOINT,       // Not really a clipping state.  Used at start/end transition.
      VIS_REASON_OFFSET,         // A break in offset profile looks like a clipping edge
      VIS_REASON_SUBCLASS        // Clipped - Subclass-based vis. failed
   };

   enum trans_type_t {
      TRANS_NONE = 0,            // No type (unused/initial state)
      TRANS_VISIBLE,             // Transition - Vert marks a clipped to vis trans.
      TRANS_CLIPPED              // Transition - Vert marks a vis to clipped trans.
   };

   /******** MEMBER VARIABLES ********/
   double   _lmax;
   double   _l;                               
   double   _t;                               

   mlib::NDCZpt   _loc;           
   mlib::NDCZpt   _base_loc;      

   bool     _good;
   mlib::NDCZvec  _dir;   
   mlib::Wvec     _norm;

   float    _width;         
   float    _alpha;

   vis_state_t    _vis_state;
   vis_reason_t   _vis_reason;          
   trans_type_t   _trans_type;

   BaseStrokeVertexData*   _data;     

   BaseStrokeVertex*       _refined_vertex;

   BaseStrokeVertex*       _next_vert;
   BaseStrokeVertex*       _prev_vert;

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   BaseStrokeVertex() 
      : _width(1.0), 
      _alpha(1.0),
      _vis_state(VIS_STATE_VISIBLE), 
      _vis_reason(VIS_REASON_NONE), 
      _trans_type(TRANS_NONE),
      _data(0),
      _refined_vertex(0),
      _next_vert(0),
      _prev_vert(0)
   {};
   virtual ~BaseStrokeVertex() {}; 
   
   /******** MEMBER METHODS ********/
   int operator==(const BaseStrokeVertex &) {
      cerr << "BaseStrokeVertex::operator== - Dummy called!\n";
      return 0;
   }
   CBaseStrokeVertex& operator=(const BaseStrokeVertex &v) {        
      _lmax       = v._lmax;
      _l          = v._l;
      _t          = v._t;
      _loc        = v._loc;
      _base_loc   = v._base_loc;
      _good       = v._good;
      _dir        = v._dir;
      _norm       = v._norm;
      _width      = v._width;
      _alpha      = v._alpha;
      _vis_state  = v._vis_state;
      _vis_reason = v._vis_reason;
      _trans_type = v._trans_type;
       
      // XXX - Asserts are temporary
      if (v._data) { 
         assert(_data);
         _data->copy(v._data);
      } else
         assert(!_data);
      return *this;

      // We don't copy this, because it's never
      // desired of copies (copying is only used
      // when arrays resize, and this won't happen
      // after refinement)
      //_refined_vertex = v._refined_vertex;

      // The prev,next pointers are also not
      // copied, as they too only get generated
      // in the refined base stroke after
      // all array resizing is done
   }

};

/*****************************************************************
 * Simple Array Class (SARRAY)
 *****************************************************************/

// A simple array class that avoids excessive 
// reallocation and copying.  Like ARRAY, a 
// new element can be appended with add(e), which
// copying e over the new element. But, a new
// element can be requested via next() which
// avoids the need to copy over the new element
// as with add. This saves time, but returns an
// element of undefined state (useful in this app)

#define CSARRAY const SARRAY

template <class T> class SARRAY {

   /******** MEMBER VARIABLES ********/
 protected:
   T*       _array;
   T*       _new_array;
   int      _num;
   int      _max;

 public:

   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

   SARRAY(int m=0) : _array(0), _num(0), _max(0)  {
      if (m>0) realloc(m);
   }

   virtual ~SARRAY() {
      clear();
      if (_array)
         delete [] _array;
   }

   /******** MEMBER METHODS ********/

   int          num()                   const   { return _num; }
   inline T&    operator[](int j)       const   { return _array[j]; }
   void         clear()                         { _num=0; }
   inline void  add(const T& e)                 { next() = e; }
   inline void  del()                           { if (_num>0) _num--; }

   inline int   next_ind() {
      if (_num == _max)
         realloc();
      return _num++;
   }
   inline T&    next() {
      int i = next_ind();
      return _array[i];
   }

   void  realloc(int new_max=0) {
      if (new_max && new_max <= _max) return;
      alloc(new_max);
      copy();
      cleanup();
   }

 protected:
   virtual void alloc(int new_max) { 
      _max = (new_max==0) ? (_max?_max*2:2) : new_max;
      _new_array = new T [_max];
      assert(_new_array);
   }

   virtual void copy() { 
      for (int i=0; i<_num; i++)
         _new_array[i] = _array[i];
   }

   virtual void cleanup() { 
      if (_array)
         delete [] _array;
      _array = _new_array;
      _new_array = 0;
   }
};

/*****************************************************************
 * StrokeVertexArray
 *****************************************************************/

class StrokeVertexArray : public SARRAY<BaseStrokeVertex> {

   /******** MEMBER VARIABLES ********/
 protected:
   BaseStrokeVertexData*   _data;
   BaseStrokeVertexData*   _new_data;
   BaseStrokeVertexData*   _proto;

 public:

   /******** CONSTRUCTOR/DECONSTRUCTOR *******/


   StrokeVertexArray() :
      SARRAY<BaseStrokeVertex>(0), _data(0), _new_data(0), _proto(0) {}

   ~StrokeVertexArray() {
      if (_proto&&_data)
         _proto->dealloc(_data); delete _proto;
   }

   void set_proto(BaseStrokeVertexData *bsvd) {
      if (_proto)
         delete _proto;
      _proto = bsvd;
   }

 protected:
        
   virtual void alloc(int new_max) { 
      SARRAY<BaseStrokeVertex>::alloc(new_max);
      if (!_proto) return;

      _new_data = _proto->alloc(_max);
      assert(_new_data);

      for (int i=0; i<_max; i++) 
         _new_array[i]._data = _proto->elem(i,_new_data);
   }

   virtual void cleanup() { 
      SARRAY<BaseStrokeVertex>::cleanup();
      if (!_proto) return;
      if (_data) _proto->dealloc(_data);

      _data = _new_data;
      _new_data = 0;
   }
};

   class COL4 {
    private:
      double _data[4];
    public:
      COL4() {}
      COL4(CCOLOR &c,double a) { _data[0]=c[0]; _data[1]=c[1]; _data[2]=c[2]; _data[3]=a;  }
      void set(CCOLOR &c,double a) { _data[0]=c[0]; _data[1]=c[1]; _data[2]=c[2]; _data[3]=a; }
      void set(CCOLOR &c) { _data[0]=c[0]; _data[1]=c[1]; _data[2]=c[2]; }
      void set(double a) { _data[3]=a; }
      const double * data() { return _data; }
      int operator==(const COL4 &) { cerr << "COL4::operator== -  Dummy called!\n"; return 0; }

   };

/*****************************************************************
 * BaseStroke
 *****************************************************************/

#define VIS_TYPE_NONE           0
#define VIS_TYPE_SCREEN         1
#define VIS_TYPE_NORMAL         2
#define VIS_TYPE_SUBCLASS       4

#define CBaseStroke const BaseStroke

class BaseStroke : public DATA_ITEM {

   friend class StrokeUI;        

   /******** STATIC MEMBER VARIABLES ********/   
 private:
   static TAGlist*      _bs_tags;
   static bool          _debug;
   static bool          _repair;

   //Since many strokes will use the paper
   //textures, we don't want to waste tonnes
   //of memory with redundant copies, or waste
   //time switching between different texture
   //objects that are identical.  Instead, we
   //cache all of the textures, and their
   //filenames, and serve up textures from the cache.
   static LIST<str_ptr>*      _stroke_texture_names;
   static LIST<TEXTUREptr>*   _stroke_texture_ptrs;
   static LIST<str_ptr>*      _stroke_texture_remap_orig_names;
   static LIST<str_ptr>*      _stroke_texture_remap_new_names;

 protected:
   static unsigned int  _stamp;
   static int           _strokes_drawn;
   static float         _scale; //PIX->NDC
   static float         _max_x;
   static float         _max_y;
   static mlib::Wpt           _cam;
   static mlib::Wvec          _cam_at_v;

   /******** MEMBER VARIABLES ********/

   bool                    _dirty;
   double                  _ndc_length;
   int                     _vis_verts;

   StrokeVertexArray       _verts;
   StrokeVertexArray       _draw_verts;
   StrokeVertexArray       _refine_verts;

   // May or may not be duped (see following flag)
   BaseStrokeOffsetLISTptr _offsets;

   // Not dupped or in IO!
   bool                    _propagate_offsets; 
   // Whether to also copy offsets when this stroke is copied.

   double                  _offset_lower_t;
   double                  _offset_upper_t;

   
   int                     _vis_type;
   bool                    _gen_t;        // Whether to generate t vals automatically
   double                  _min_t;        // Min and max t vals the user must respect
   double                  _max_t;        // or which the auto t's are generated for

   COLOR                   _highlight_color;
   bool                    _is_highlighted;

   bool                    _overdraw;
   BaseStroke*             _overdraw_stroke;

   /******** DUPABLE MEMBER VARIABLES ********/

   // Dupped and in the IO tags

   COLOR                   _color;
   float                   _width;
   float                   _alpha;
   float                   _fade;
   float                   _halo; 
   float                   _taper;
   float                   _flare;
   float                   _aflare;
   float                   _angle;
   float                   _contrast;
   float                   _brightness;
   float                   _offset_stretch;
   float                   _offset_phase;
   str_ptr                 _tex_file;
   str_ptr                 _paper_file;

   int                     _press_vary_width;
   int                     _press_vary_alpha;

   // Dupped and NOT in the IO tags

   TEXTUREptr              _tex;                   
   TEXTUREptr              _paper;                 
   float                   _pix_res;
   bool                    _use_depth;
   int                     _use_paper;

 public:
   /******** Static Member Methods *******/
   static void             set_debug(bool d) { _debug = d;     }
   static bool             get_debug()       { return _debug;  }

   static void             set_repair(bool r) { _repair = r;     }
   static bool             get_repair()       { return _repair;  }

   static int 		   get_strokes_drawn() { return _strokes_drawn; } 
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   BaseStroke();
   virtual ~BaseStroke();

   /******** Member Methods *******/

   // Subclasses will return their data type instead
   virtual BaseStrokeVertexData* data_proto() {
      return new BaseStrokeVertexData;
   }

   virtual BaseStroke*  copy() const;
   virtual void         copy(CBaseStroke& v);

   virtual void         clear();

   virtual void add(mlib::CNDCZpt& pt, bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      set_dirty();
   }
   virtual void add(mlib::CNDCZpt& pt, mlib::CWvec& norm,  bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      v._norm = norm; 
      set_dirty();
   }
   virtual void add(double t, mlib::CNDCZpt& pt, bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      v._t = t; assert(t >= _min_t); assert(t <= _max_t);
      set_dirty();
   }
   virtual void add(double t, mlib::CNDCZpt& pt, mlib::CWvec& norm,  bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      v._norm = norm; 
      v._t = t; assert(t >= _min_t); assert(t <= _max_t);
      set_dirty();
   }

   virtual void add(mlib::CNDCZpt& pt, float width, bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      v._width = width;
      set_dirty();
   }

   virtual void add(mlib::CNDCZpt& pt, float width, mlib::CNDCZvec &dir, bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      v._width = width;
      v._dir = dir;
      set_dirty();
   }

   /******** DRAWING MEMBER METHODS ********/
   inline void    set_dirty()             { _dirty = true; }

   virtual void   draw_start();
   virtual int    draw(CVIEWptr &);
   virtual int    draw_debug(CVIEWptr &); 
   virtual void   draw_end();


   /******** ACCESSOR MEMBER METHODS ********/

   virtual void   set_color(CCOLOR& c)    { _color = c; set_dirty();    }
   CCOLOR&        get_color() const       { return _color;              }

   virtual void   set_width(float w)      { _width = w; set_dirty();    }
   float          get_width() const       { return _width;              }

   virtual void   set_alpha(float a)      { _alpha = a; set_dirty();    }
   float          get_alpha() const       { return _alpha;              }

   virtual void   set_fade(float af)      { _fade = af; set_dirty();    }
   float          get_fade() const        { return _fade;               }

   virtual void   set_halo(float h)       { _halo = h; set_dirty();     } 
   float          get_halo()      const   { return _halo;               }

   virtual void   set_taper(float t)      { _taper = t; set_dirty();    }
   float          get_taper()     const   { return _taper;              }

   virtual void   set_flare(float f)      { _flare = f; set_dirty();    }
   float          get_flare()     const   { return _flare;              }

   virtual void   set_aflare(float f)     { _aflare = f; set_dirty();   }
   float          get_aflare() const      { return _aflare;             }

   virtual void   set_pix_res(float p)    { _pix_res = p; set_dirty();  }
   float          get_pix_res() const     { return _pix_res;            }

   virtual void   set_use_depth(bool d)   { _use_depth = d; set_dirty();}
   bool           get_use_depth() const   { return _use_depth;          }

   virtual void   set_angle(float a)      { _angle = a; set_dirty();    }
   float          get_angle() const       { return _angle;              }

   virtual void   set_contrast(float c)   { _contrast = c; set_dirty(); }
   float          get_contrast() const    { return _contrast;           }

   virtual void   set_brightness(float b) { _brightness = b;set_dirty();}
   float          get_brightness() const  { return _brightness;         }

   virtual void   set_use_paper(int t)       { _use_paper = t;       }
   int            get_use_paper() const      { return _use_paper;    }

   //XXX - Moved these into the realm of duppable settings
   //XXX - Actually, that's a mistake!! Just phase (for now),
   //      seems we can't hijack these cause we use stretch
   //      for important things...
   virtual void   set_offset_stretch(float s)   { _offset_stretch = s; set_dirty();    }
   float          get_offset_stretch() const    { return _offset_stretch;              }

   virtual void   set_offset_phase(float s)     { _offset_phase = s; set_dirty();      }
   float          get_offset_phase() const      { return _offset_phase;                }

   int                       num_verts()      const { return _verts.num(); }
   const StrokeVertexArray&  get_verts()      const { return _verts;       }
   const StrokeVertexArray&  get_draw_verts() const { return _draw_verts;  }

   double                    get_ndc_length()  { return _ndc_length;    }

   // User sets texture via it's filename (relative
   // to JOT_ROOT) or as a TEXTUREptr and the
   // associated filename. Setting the name
   // as NULL_STR will disable texturing, and 
   // enable antialiasing.
   virtual void   set_texture(str_ptr tn);
   virtual void   set_texture(TEXTUREptr tp, str_ptr tn);

   TEXTUREptr     get_texture()           { return _tex;                }
   Cstr_ptr       get_texture_file() const{ return _tex_file;           }

   // User sets paper texture via it's filename (relative
   // to JOT_ROOT) or as a TEXTUREptr and the
   // associated filename. Setting the name as NULL_STR 
   // will disable paper texturing.
   virtual void   set_paper(str_ptr tn);
   virtual void   set_paper(TEXTUREptr tp, str_ptr tn);

   TEXTUREptr     get_paper()             { return _paper;              }
   Cstr_ptr       get_paper_file() const  { return _paper_file;         }


   virtual void   set_vis(int v)          { _vis_type = v; set_dirty(); }
   int            get_vis()               { return _vis_type;           }

   void set_offsets(BaseStrokeOffsetLISTptr o) {
      _offsets = o;
      set_dirty();
   }
   BaseStrokeOffsetLISTptr get_offsets()        { return _offsets;            }
   CBaseStrokeOffsetLISTptr get_offsets() const { return _offsets;            }

   void set_propagate_offsets(bool b)     { _propagate_offsets = b;     }
   bool get_propagate_offsets()           { return _propagate_offsets;  }

   virtual void   set_min_t(double t)     { _min_t = t; set_dirty();    }
   double         get_min_t() const       { return _min_t;              }

   virtual void   set_max_t(double t)     { _max_t = t; set_dirty();    }
   double         get_max_t() const       { return _max_t;              }

   virtual void   set_gen_t(bool g)       { _gen_t = g; set_dirty();    }
   bool           get_gen_t() const       { return _gen_t;              }


   virtual void   set_offset_lower_t(double t)  { _offset_lower_t = t; set_dirty();    }
   double         get_offset_lower_t() const    { return _offset_lower_t;              }

   virtual void   set_offset_upper_t(double t)  { _offset_upper_t = t; set_dirty();    }
   double         get_offset_upper_t() const    { return _offset_upper_t;              }

   virtual void   set_press_vary_width(bool w)  { _press_vary_width = (w?1:0); set_dirty();  }
   bool           get_press_vary_width() const  { return _press_vary_width==1;               }

   virtual void   set_press_vary_alpha(bool a)  { _press_vary_alpha = (a?1:0); set_dirty();  }
   bool           get_press_vary_alpha() const  { return _press_vary_alpha==1;               }

   virtual void   set_overdraw(bool o)          { _overdraw = o; set_dirty();       }
   bool           get_overdraw() const          { return _overdraw;                 }

   void           highlight(bool b, CCOLOR & sel_col=COLOR::red) 
   {
      if(b) {
         _highlight_color = sel_col;
         _is_highlighted = true;
      }
      else{
         _is_highlighted = false;
      }
      set_dirty();
   }

   bool           get_is_highlighted()    { return _is_highlighted;    }

   /******** INTERNAL MEMBER METHODS ********/
  
 protected:

   virtual int    draw_dots();            
   virtual int    draw_circles();            
   virtual int    draw_body();            

   virtual void   update();
   virtual void   compute_length_l();
   virtual void   compute_visibility();      
   virtual bool   set_vert_visibility(BaseStrokeVertex &v);
   virtual bool   check_vert_visibility(CBaseStrokeVertex &) { return true; }
   virtual void   compute_refinements();

   virtual void   compute_t();
   
   virtual void   compute_connectivity();

   virtual BaseStrokeVertex* refine_vert(int i, bool left);
   virtual void   interpolate_vert(BaseStrokeVertex *v,
                                   BaseStrokeVertex *vleft, double u);
   virtual void   interpolate_vert(BaseStrokeVertex *v,
                                   BaseStrokeVertex **vl, double u);
   virtual void   interpolate_refinement_vert(BaseStrokeVertex *v,
                                              BaseStrokeVertex **vl, double u);
   virtual void   interpolate_offset(BaseStrokeOffset *o, BaseStrokeOffset **ol, double u);
   // Subclass this to change offset LOD, etc.
   virtual void   apply_offset(BaseStrokeVertex* v, BaseStrokeOffset* o);
   
   virtual void   setup_offsets();
   virtual void   compute_resampling();
   virtual void   compute_resampling_with_offsets();
   virtual void   compute_resampling_without_offsets();

   virtual void   compute_stylization();
   virtual void   compute_directions();
   virtual void   compute_vertex_arrays();

   virtual void   init_overdraw();
   virtual void   compute_overdraw();

   virtual int    find_closest_i(mlib::CNDCZpt&);
   virtual bool   find_closest_t(mlib::CNDCZpt& , double , double &, int);
   virtual int    interpolate_vert_by_t(BaseStrokeVertex *, double );

   /******** VERTEX ARRAYS ********/
 protected:
   
   ARRAY<mlib::NDCZpt>        _array_verts;
   ARRAY<mlib::UVpt>          _array_tex_0;
   ARRAY<COL4>          _array_color;
   ARRAY<unsigned int>  _array_i0;
   ARRAY<unsigned int>  _array_i1;
   ARRAY<int>           _array_counts;
   int                  _array_counts_total;


//Public?   
 public:
   BaseStrokeOffsetLISTptr generate_offsets(mlib::CNDCZpt_list&, CARRAY<double>&);
   void                    closest(const mlib::NDCpt &, mlib::NDCpt &, double &);

   /******** DATA_ITEM METHODS ********/
 public:
   DEFINE_RTTI_METHODS_BASE("BaseStroke", CDATA_ITEM *);
   virtual DATA_ITEM*   dup() const  { return copy(); }
   virtual CTAGlist&    tags() const;

 protected:
   /******** I/O Methods ********/
   virtual void   get_texture_file (TAGformat &d);
   virtual void   put_texture_file (TAGformat &d) const;
   virtual void   get_paper_file (TAGformat &d);
   virtual void   put_paper_file (TAGformat &d) const;
   virtual void   get_offsets (TAGformat &d);
   virtual void   put_offsets (TAGformat &d) const;

   /******** I/O Access Methods ********/
   COLOR&         color_()          { return _color; }
   float&         width_()          { return _width; }
   float&         alpha_()          { return _alpha; }
   float&         fade_()           { return _fade; }
   float&         halo_()           { return _halo; }
   float&         taper_()          { return _taper; }
   float&         flare_()          { return _flare; }
   float&         aflare_()         { return _aflare; }
   float&         angle_()          { return _angle; }
   float&         contrast_()       { return _contrast; }
   float&         brightness_()     { return _brightness; }
   float&         offset_stretch_() { return _offset_stretch; }
   float&         offset_phase_()   { return _offset_phase; }

   int&           press_vary_width_()  { return _press_vary_width; }
   int&           press_vary_alpha_()  { return _press_vary_alpha; }
 
	
   int            use_paper()       { return _use_paper; }

 public:

   /******** OPERATORS ********/
   int operator==(const BaseStroke &) {
      cerr << "BaseStroke::operator== -  Dummy called!\n";
      return 0;
   }
   int operator=(const BaseStroke &) {
      cerr << "BaseStroke::operator=  -  Dummy called!\n";
      return 0;
   }
};

/*****************************************************************
 * BaseStrokeArray:
 *
 *      Convenience: ARRAY of BaseStrokes w/ helper methods.
 *
 *****************************************************************/
class BaseStrokeArray : public ARRAY<BaseStroke*> {
 public:

   //******** MANAGERS ********

   BaseStrokeArray(int n=0) : ARRAY<BaseStroke*>(n) {}

   //******** CONVENIENCE ********

   void delete_all() {
      while (!empty())
         delete pop();
   }
   void clear_strokes() const {
      for (int i=0; i<_num; i++)
         _array[i]->clear();
   }
   void copy(CBaseStroke& proto) const {
      for (int i=0; i<_num; i++)
         _array[i]->copy(proto);
   }
   void set_offsets(CBaseStrokeOffsetLISTptr& offsets) const {
      for (int i=0; i<_num; i++)
         _array[i]->set_offsets(offsets);
   }
   int draw(CVIEWptr& v) const {
      int ret = 0;
      if (!empty()) {
         _array[0]->draw_start();
         for (int i=0; i<_num; i++)
            ret += _array[i]->draw(v);
         _array[0]->draw_end();
      }
      return ret;
   }
};

#endif // _BASE_STROKE_HEADER

/* end of file base_stroke.H */
