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
#ifndef CAM_H_IS_INCLUDED
#define CAM_H_IS_INCLUDED


#include "disp/base_collide.H"
#include "std/support.H"
#include "mlib/points.H"
#include "disp/gel.H"
#include "disp/bbox.H"
#include "std/stop_watch.H"

using namespace mlib;

#ifdef WIN32
#define uint UINT
#endif


#define CSCREENptr  const SCREENptr
#define CCAMptr     const CAMptr
#define CCAMdata    const CAMdata
#define CCAMdataptr const CAMdataptr

// would like to use typedef, but this same #define
// occurs in many .H files and so is a pain to fix:
class VIEWptr;
#define CVIEWptr const VIEWptr

//----------------------------------------------
//
//  CAMdata -
//     contains high-level camera parameters
//  has the ability to generate a matrix that
//  transforms points in world-space to normalized
//  device coordinates (suitable for passing to OGl)
//
//----------------------------------------------
class SCREEN; typedef REFptr<SCREEN> SCREENptr;
class CAM;
class CAMobs;
class REF_CLASS(CAMdata) : public REFcounter {
  public:
   enum eye { LEFT = 0, RIGHT, MIDDLE, NUM_EYE_POSITIONS };

  protected : 

   CAM*         _cam;
   mlib::Wpt    _from;            // camera location
   mlib::Wpt    _at;              // point we're looking at
   mlib::Wpt    _up;              // point in the "up" direction
   mlib::Wpt    _center;          // "center" of interest... different from "at"?
   double       _focal;           // distance from _from to the film plane (WC)
   double       _width;           // width of film plane rectangle (WC)
   double       _height;          // height o film plane rectangle (WC)
   bool         _perspective;     // true = perspective, false = orthographic
   double       _tilt_up;         // used in film_normal()
   double       _tilt_left;
   double       _iod;             // interocular distance in ROOM units (inches)
   double       _feature_scale;   // size of the data scale in world units

   mlib::Wvec   _film_normal;     // film plane normal used for head tracking 
   bool         _film_normal_flag;// indicates film plane normal was explicitly set

   // Indicates camera parameters were loaded from file:
   bool         _loaded_from_file; 

   // Auxiliary members
   ARRAY<CAMobs *> _cblist;

   bool            _changed, _cached;
   bool            _proj_mat_dirty;
   bool            _pos_mat_dirty   [NUM_EYE_POSITIONS];
   mlib::Wtransf   _proj_matrix;
   mlib::Wtransf   _pos_matrix      [NUM_EYE_POSITIONS];

   uint            _stamp;      // incremented whenever changed() is called   
   double          _dist;       // cached data...
   mlib::Wvec      _at_v, _up_v, _right_v, _pup_v;

   void callback() const;
   void cache()    const;
   bool film_vectors (mlib::CWvec&, mlib::CWvec&, mlib::Wvec&, mlib::Wvec&) const;
   bool film_normal  (mlib::CWvec&, mlib::CWvec&, mlib::Wvec&) const;
   bool setup_vectors(mlib::Wvec &,  mlib::Wvec&, mlib::Wpt&,
                        mlib::Wvec&, mlib::Wvec&, mlib::Wvec&) const;

  public : 
   CAMdata(CAM* = 0);
   void      add_cb(CAMobs *cb)         { _cblist.add(cb);}
   void      rem_cb(CAMobs *cb)         { _cblist.rem(cb);}
 
   mlib::CWpt&from()            const   { return _from; }
   mlib::CWpt&at()              const   { return _at  ; }
   mlib::CWpt&up()              const   { return _up  ; }
   mlib::CWpt&center()          const   { return _center; }
   double     focal()           const   { return _focal; }
   bool       persp()           const   { return _perspective; }
   double     width()           const   { return _width; }
   double     height()          const   { return _height; }
   double     iod()             const   { return _iod; }
   mlib::Wvec film_normal()     const   { return _film_normal; }
   uint       stamp()           const   { return _stamp; }
   double     feature_scale()   const   { return _feature_scale; }
   double     distance()        const   { cache(); return _dist; }
   mlib::Wvec at_v()            const   { cache(); return _at_v; }
   mlib::Wvec up_v()            const   { cache(); return _up_v; }
   mlib::Wvec pup_v()           const   { cache(); return _pup_v;}
   mlib::Wvec right_v()         const   { cache(); return _right_v;}
           
   // Setting methods
   void      changed();
   void      start_manip(); //Notifies all the CAMobs of start of cam manip
   void      end_manip();   //Notifies all the CAMobs of end of cam manip
           
   bool      get_changed()const        { return _changed; }
   void      clear_changed_flag()      { _changed = false;}

   void      set_from    (mlib::CWpt &p)     { _from   = p; changed();}
   void      set_at      (mlib::CWpt &p)     { _at     = p; changed();}
   void      set_up      (mlib::CWpt &p)     { _up     = p; changed();}
   void      set_center  (mlib::CWpt &p)     { _center = p; changed();}
   void      set_focal   (double f)    { _focal  = f; changed();}
   void      set_persp   (bool   p)    { _perspective = p;changed();}
   void      set_width   (double w)    { _width  = w; changed();}
   void      set_height  (double h)    { _height = h; changed();}
   void      set_iod     (double i)    { _iod    = i; changed();}
   void      set_film_normal(mlib::CWvec &v) { _film_normal_flag = 1;
      _film_normal = v;   changed();}
   void      unset_film_normal()       { _film_normal_flag=0;changed();}

   bool     loaded_from_file() const { return _loaded_from_file; }

   void      rotate   (mlib::CWline &, double);
   void      swivel   (double);
   void      translate(mlib::CWvec &);
   bool      operator ==(CCAMdata &d) const;
   CAMdata  &operator = (CCAMdata &d);

   // CAMdata::xform() returns the matrix that transforms world to eye
   // coordinates, transformed by the given screen and eye specs if
   // needed for stereo viewing.  The matrix returned should be loaded
   // into the MODELVIEW stack whenever drawing is about to start. This
   // is already done by gl_view.C.
   virtual mlib::CWtransf& xform(SCREENptr, eye e=MIDDLE) const;

   // CAMdata::projection_xform() returns the matrix that
   // transforms eye coordinates to the window coordinates.
   virtual mlib::CWtransf&
      projection_xform(SCREENptr &, mlib::CNDCpt &,double,double, eye e=MIDDLE) const;

   friend  STDdstream &operator<<(STDdstream &ds, CCAMdataptr &p);
   friend  STDdstream &operator>>(STDdstream &ds,  CAMdataptr &p);

   void      notify_callbacks();
};
inline ostream &operator<<(ostream &os, CAMdata *p) 
{ return os << "CAM(" << p->from()  << " " << p->at()    << " " <<
      p->up()    << " " << p->width() << " " <<
      p->height()<<")"; }

class CAMobs {
 public:
   virtual ~CAMobs() {}
   virtual void notify(CCAMdataptr &data) = 0;
   virtual void notify_manip_start(CCAMdataptr &data) { };
   virtual void notify_manip_end(CCAMdataptr &data) { };
};

//----------------------------------------------
//
//  CAM-
//     abstract camera class that contains
//  a high-level camera data object.   This class
//  also encapsulates the passing of camera data
//  to OGl.
//
//----------------------------------------------
class REF_CLASS(CAM): public SCHEDULER {
  protected: 
   str_ptr      _name;

   CAMdataptr   _data;

   mlib::NDCpt  _min;        // lower left corner of view in NDC coords
   double       _width;      // width of display window in NDC coords
   double       _height;     // height of display window in NDC coords
   double       _zoom;       // zoom factor on XY coordinate system

   bool          _ndc_proj_dirty;
   mlib::Wtransf _ndc_proj;
   mlib::Wtransf _ndc_proj_inv;

  protected:

  public:
   CAM(Cstr_ptr &s):_name(s),//,_data(new CAMdata(this)),
      _min(-1,-1),_width(2),_height(2),
      _zoom(1), _ndc_proj_dirty(1) { _data = new CAMdata(this);}
      virtual           ~CAM()               { }
      virtual str_ptr    name()        const { return str_ptr(_name); }

      void     set_aspect(double a);

      CAMdataptr   data()              { return _data; }
      CCAMdataptr &data()        const { return _data; }
      CAMdata     *data_ptr()          { return _data; }
      const CAMdata     *data_ptr()    const { return _data; }
      double       aspect()      const { return _height/_width; }
      mlib::CNDCpt      &min   ()      const { return _min;  }
      double       width ()      const { return _zoom * _width;  }
      double       height()      const { return _zoom * _height; }
      double       zoom  ()      const { return _zoom; }
      mlib::CWtransf    &xform(SCREENptr s=SCREENptr(),
                               CAMdata::eye e=CAMdata::MIDDLE) const{
         return _data->xform(s,e);}
      mlib::CWtransf    &projection_xform(SCREENptr s=SCREENptr(),
                                          CAMdata::eye e=CAMdata::MIDDLE) const {
         return _data->projection_xform(s,min(),width(),height(),e);}

      void       set_min  (mlib::CNDCpt n) { _min  = n; }
      void       set_zoom (double z) { _zoom = z; }
      mlib::CWtransf&  ndc_projection() const;
      mlib::CWtransf&  ndc_projection_inv() const;
      void       data_changed()      { _ndc_proj_dirty = 1; }

      friend  ostream &operator<<(ostream &os, CAM *p) 
         { os << p->xform(); return os; }
      CAM   &operator = (const CAM &cam);
      int    operator == (const CAM &cam);

      mlib::Wvec  film_dir(mlib::CXYpt &p)           const;
      mlib::Wpt   xy_to_w (mlib::CXYpt &p, mlib::CWpt &w)  const;
      mlib::Wpt   xy_to_w (mlib::CXYpt &p, double d) const;
      mlib::Wpt   xy_to_w (mlib::CXYpt &p)           const;
      mlib::XYpt  w_to_xy (mlib::CWpt  &p)           const;
};

// ---------------------------------------------
// CAMhist -
//   history list for the camera.
// ---------------------------------------------
#define CCAMhist const CAMhist
class CAMhist : public LIST<CAMptr> {
 public :
   CAMhist(int num=100):LIST<CAMptr>(num) {}
};

// ---------------------------------------------
// 
// SCREEN -
//    virtual base class for a screen description.  examples
// of a screen are desktop monitors, responsive workbenches,
// or cave walls.
// 
// the SCREEN object is responsible for setting up a CAMera's
// parameters to render for a particular SCREEN.
// 
// it also attempts to synchronize mouse interaction
// with BOOTH parameters.
// 
// ---------------------------------------------
#define CSCREENptr const SCREENptr
class REF_CLASS(SCREEN) : public REFcounter
{
  protected:
   CAMptr  _saved_cam;

  public:
   virtual ~SCREEN() {}
   SCREEN() { _saved_cam = new CAM("saved cam"); }

   virtual void push_eye_offset (CCAMptr &,CAMdata::eye);
   virtual void pop_eye_offset  (const CAM *c);
   virtual void config_cam      (CCAMptr &) {}

   virtual mlib::Wvec iod_v() = 0;
};

class REF_CLASS(SCREENbasic) : public SCREEN
{
  protected:
   CAMptr   _cam;

  public:
   SCREENbasic( CCAMptr &cam ) : _cam(cam) {}

      virtual mlib::Wvec iod_v() {
         CCAMdataptr &data = ((CCAMptr &)_cam)->data();
         return data->right_v(); 
      }
};

#endif // CAM_H_IS_INCLUDED

/* end of file cam.H */
