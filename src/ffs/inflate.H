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
 * inflate.H
 *****************************************************************/
#ifndef INFLATE_H_IS_INCLUDED
#define INFLATE_H_IS_INCLUDED

/*!
 *  \file inflate.H
 *  \brief Contains the declaration of the INFLATE widget and INFLATE_CMD.
 *
 *  \ingroup group_FFS
 *  \sa inflate.C
 *
 */

#include "geom/command.H"
#include "tess/bsurface.H"

#include "gest/draw_widget.H"

/*****************************************************************
 * INFLATE:
 *****************************************************************/
MAKE_PTR_SUBC(INFLATE,DrawWidget);
typedef const INFLATE    CINFLATE;
typedef const INFLATEptr CINFLATEptr;

//! \brief "Widget" that handles inflating a shape to create a
//! higher-dimensional shape from a lower-dimensional one.
class INFLATE : public DrawWidget {
 public:

   //******** MANAGERS ********

   // no public constructor
   static INFLATEptr get_instance();

   static INFLATEptr get_active_instance() { return upcast(_active); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("INFLATE", INFLATE*, DrawWidget, CDATA_ITEM*);

   //******** MODE NAME ********

   //! displayed in jot window
   virtual str_ptr      mode_name()     const { return "inflate"; }

   //******** ACTIVATION ********

   // Static methods to activate an INFLATE for a given region
   // of surface, a curve, or a single point, respectively.
   // Each checks for errors and returns true on success.
   
   //! Region reachable from bf
   static bool init(Bface *bf, double dist, double dur = default_timeout()) {
      return get_instance()->setup(bf, dist, dur);
   }

   //! Region of surface:
   static bool init(CBface_list& faces, double dist, double dur = default_timeout()) {
      return get_instance()->setup(faces, dist, dur);
   }

   //! Do the inflate operation on the set of faces reachable from
   //! the given Bface f, using offset distance h, which is relative
   //! to the local edge lengths around each vertex:
   static bool do_inflate(Bface* face, double h, Bsurface*& output,
                          Bface_list*& reversed_faces, MULTI_CMDptr cmd=0);   
   static bool do_inflate(Bface_list faces, double h, Bsurface*& output,
                          Bface_list*& reversed_faces, MULTI_CMDptr cmd=0); 

   //******** DRAW FSA METHODS ********

   //! They drew a straight line. Decides appropriate callback:
   virtual int  line_cb       (CGESTUREptr& gest, DrawState*&);

   //! Generic stroke. Could be a few things, e.g. non-uniform inflate:
   virtual int  stroke_cb     (CGESTUREptr& gest, DrawState*&);

   //! Tap callback (usually cancel):
   virtual int  tap_cb        (CGESTUREptr& gest, DrawState*&);

   //! Turn off the widget:
   virtual int  cancel_cb     (CGESTUREptr& gest, DrawState*&);

   //******** DrawWidget virtual methods ********

   virtual BMESHptr bmesh() const { return _mesh; }
   
   //******** GEL METHODS ********

   virtual int draw(CVIEWptr& v);

   //*****************************************************************

 protected:

   mlib::Wpt_list _lines;         //!< guidelines

   // The INFLATE operates on ONE of the following:
   //   - isolated vertex
   //   - chain of edges
   //   - set of faces:
   // XXX - edges not implemented yet

   // chain of edges:
   //   not implemented

   // set of faces:
   Bface*	_orig_face;			//!< Face from which to start an inflate
   Bface_list   _faces;         //!< faces "inside" the boundary
   LedgeStrip   _boundary;      //!< boundary of faces
   double       _d;             //!< length for lines (used for set of faces)
   double       _preview_dist;  //!< Distance used for preview lines

   LMESHptr     _mesh;          //!< mesh of vertex, edges, or faces
   bool         _mode;  //!< what kind of input? false for Bface, true for Bface_list

   //******** STATICS: ********
   static INFLATEptr      _instance;

   static void clean_on_exit();

   //******** MANAGERS ********
   INFLATE();
   virtual ~INFLATE() {}

   //! For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<INFLATE, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   //******** SETTING UP ********

   //! Protected methods to check conditions, cache info, and
   //! activate the INFLATE on a given region of surface, a
   //! curve, or a single point, respectively.  Each checks for
   //! errors and returns true on success:
   bool setup(CBface_list& faces, double dist, double dur);
   bool setup(Bface *bf, double dist, double dur);

   //! Clear cached info when deactivated:
   virtual void reset();

   //******** INTERACTION ********

   //! Build the preview lines for the set of faces:
   void build_lines();

   //******** INFLATING SURFACES ********

};

/*****************************************************************
 * INFLATE_CMD
 *****************************************************************/
MAKE_PTR_SUBC(INFLATE_CMD,COMMAND);

//! \brief Perform an INFLATE operation.
class INFLATE_CMD : public COMMAND {
 public:
   //******** MANAGERS ********
   INFLATE_CMD(Bface *orig_face, double dist) :
       _orig_face(orig_face),
       _mode(false),
       _dist(dist),
       _output(0),
       _reversed_faces(0) {
      _other_cmds = new MULTI_CMD;
   }

   INFLATE_CMD(Bface_list orig_faces, double dist) :
       _orig_faces(orig_faces),
       _mode(true),
       _dist(dist),
       _output(0),
       _reversed_faces(0) {
      _other_cmds = new MULTI_CMD;
   }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("INFLATE_CMD", COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********
   virtual bool doit();
   virtual bool undoit();
   virtual bool clear();

   //******** DIAGNOSTIC ********

   virtual void print() const {
      cerr << class_name();
   }

 protected:
   Bface*       _orig_face;
   Bface_list   _orig_faces;
   bool         _mode;
   double       _dist;
   Bsurface*    _output;
   Bface_list*  _reversed_faces; 
   MULTI_CMDptr _other_cmds;
};

#endif // INFLATE_H_IS_INCLUDED

// end of file inflate.H
