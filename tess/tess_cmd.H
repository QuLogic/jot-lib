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
#ifndef INCLUDE_TESS_CMD_H
#define INCLUDE_TESS_CMD_H

#include "geom/command.H"
#include "map3d/map1d3d.H"
#include "map3d/map2d3d.H"
#include "mesh/bmesh.H"
#include "bbase.H"

/*****************************************************************
 * MOVE_CMD
 *
 *   COMMAND that can move a GEOM or a set of Bbases. 
 *****************************************************************/
MAKE_PTR_SUBC(MOVE_CMD,COMMAND);
class MOVE_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   MOVE_CMD() {}
   MOVE_CMD(CBbase_list& bb, CWvec& d=Wvec()) : _bbases(bb), _vec(d) {}
   MOVE_CMD(CGEOMptr&     g, CWvec& d=Wvec()) : _geom(g),    _vec(d) {}

   //******** ACCESSORS ********

   // increase the translation by the given delta
   void add_delt(CWvec& delta);

   // send notification to xform observers if using a GEOM
   void notify_xf_obs(XFORMobs::STATE s) const {
      if (_geom)
         XFORMobs::notify_xform_obs(_geom, s);
   }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("MOVE_CMD", MOVE_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Bbase_list           _bbases; //!< Bbases to be moved in current operation
   GEOMptr              _geom;   //!< GEOM to be moved (if no bbases)
   Wvec                 _vec;    //!< delta to move by

   //******** UTILITIES ********

   void apply_translation(CWvec& delt);
};

/*****************************************************************
 * XFORM_CMD
 *
 *   COMMAND that can transform a set of GEOMs. 
 *****************************************************************/
MAKE_PTR_SUBC(XFORM_CMD,COMMAND);
class XFORM_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   XFORM_CMD() {}
   XFORM_CMD(CGEOMptr& g, CWtransf& xf=Identity) :
      _xf(xf) {
      if (g) _geoms += g;
   }

   //******** ACCESSORS ********

   void add(GEOMptr g);

   CGEOMlist& geoms() const { return _geoms; }

   // replace our transform t with xf*t
   void concatenate_xf(CWtransf& xf);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("XFORM_CMD", XFORM_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   GEOMlist     _geoms;  //!< list of GEOMs to transform
   Wtransf      _xf;     //!< transform to apply

   //******** UTILITIES ********
   // apply the given transform to the objects
   void apply_xf(CWtransf& xf);
};

/*****************************************************************
 * PUSH_FACES_CMD
 *
 *      Show or hide a region of mesh. Can be used to "undo"
 *      the act of creating a new piece (or "region") of a
 *      mesh.  "doit" makes the region primary so you can see
 *      it; "undoit" makes the region secondary so you can't.
 *
 *****************************************************************/
MAKE_PTR_SUBC(PUSH_FACES_CMD,COMMAND);
class PUSH_FACES_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   PUSH_FACES_CMD(CBface_list& r, bool do_boundary=true) :
      _region(r),
      _do_boundary(do_boundary) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("PUSH_FACES_CMD", PUSH_FACES_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   // This pushes the region (makes it secondary):
   virtual bool doit();

   // This restores the region to primary status:
   virtual bool undoit();

 protected:
   Bface_list   _region;
   bool         _do_boundary;
};

/*****************************************************************
 * UNPUSH_FACES_CMD
 *
 *      Make a region of mesh visible (primary). I.e., it's
 *      the reverse of PUSH_FACES_CMD.
 *****************************************************************/
MAKE_PTR_SUBC(UNPUSH_FACES_CMD, UNDO_CMD);
class UNPUSH_FACES_CMD : public UNDO_CMD {
 public:

   //******** MANAGERS ********

   UNPUSH_FACES_CMD(CBface_list& r, bool do_boundary=true) :
      UNDO_CMD(new PUSH_FACES_CMD(r, do_boundary))  {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("UNPUSH_FACES_CMD",
                        UNPUSH_FACES_CMD*, UNDO_CMD, CCOMMAND*);
};

/*****************************************************************
 * REVERSE_FACES_CMD
 *
 *      Reverse the normals within a region of mesh.
 *****************************************************************/
MAKE_PTR_SUBC(REVERSE_FACES_CMD,COMMAND);
class REVERSE_FACES_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   REVERSE_FACES_CMD(CBface_list& r, bool done) :
      COMMAND(done), _region(r) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("REVERSE_FACES_CMD",
                        REVERSE_FACES_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   // Flip the face normals throughout the region:
   virtual bool doit();

   // Flip 'em again (back to original orientation):
   virtual bool undoit();

 protected:
   Bface_list   _region;
};

/*****************************************************************
 * SHOW_BBASE_CMD
 *
 *      Show or hide a Bbase. Used to "undo" or "redo" the
 *      act of creating a new Bbase.
 *
 *      XXX - when a curve is hidden, need to actually
 *            detach it at its endpoints so it has no
 *            influence on the rest of the mesh anymore.
 *****************************************************************/
MAKE_PTR_SUBC(SHOW_BBASE_CMD,COMMAND);
class SHOW_BBASE_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   SHOW_BBASE_CMD(Bbase* b, bool done=true) :
      COMMAND(done), _bbase(b) { assert(_bbase); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SHOW_BBASE_CMD", SHOW_BBASE_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   // Show it:
   virtual bool doit();

   // Hide it:
   virtual bool undoit();

   //******** DIAGNOSTIC ********

   virtual void print() const {
      cerr << class_name() << " (" << _bbase->class_name() << ") ";
   }

 protected:
   Bbase*    _bbase;
};

/*****************************************************************
 * HIDE_BBASE_CMD
 *
 *   Hide a Bbase. I.e., it's the reverse of SHOW_BBASE_CMD.
 *****************************************************************/
MAKE_PTR_SUBC(HIDE_BBASE_CMD, UNDO_CMD);
class HIDE_BBASE_CMD : public UNDO_CMD {
 public:

   //******** MANAGERS ********

   HIDE_BBASE_CMD(Bbase* b, bool done=true) :
      UNDO_CMD(new SHOW_BBASE_CMD(b, !done))  {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("HIDE_BBASE_CMD",
                        HIDE_BBASE_CMD*, UNDO_CMD, CCOMMAND*);
};

/*****************************************************************
 * CREATE_RIBBONS_CMD
 *
 *  Given matching EdgeStrips 'a' and 'b' of boundary edges, 
 *  connect corresponding edges together like so:
 *                                                              
 *     a0--------a1--------a2--------a3--------a4---...       
 *      |         |         |         |         |               
 *      |         |         |         |         |               
 *      |         |         |         |         |               
 *      |         |         |         |         |               
 *     b0--------b1--------b2--------b3--------b4---...       
 *
 *  Additional vertices may be created along the vertical lines
 *  if needed to preserve good aspect ratios.
 *
 *****************************************************************/
MAKE_PTR_SUBC(CREATE_RIBBONS_CMD,COMMAND);
class CREATE_RIBBONS_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   CREATE_RIBBONS_CMD(CEdgeStrip&a, CEdgeStrip& b, Patch* p=0) :
      COMMAND(false), _a(a), _b(b), _patch(p) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("CREATE_RIBBONS_CMD",
                        CREATE_RIBBONS_CMD*, COMMAND, CCOMMAND*);

   //******** ACCESSORS ********

   Patch* patch() const { return _patch; }

   //******** COMMAND VIRTUAL METHODS ********

   // Generate the internal faces:
   virtual bool doit();

   // Hide the internal faces:
   virtual bool undoit();

   // Delete the faces created in doit():
   virtual bool clear();

 protected:
   EdgeStrip  _a;       // given chain of edges
   EdgeStrip  _b;       // matching chain of edges
   Bface_list _ribbon;  // faces generated by this command
   Patch*     _patch;

   static bool _debug;  // if true, print debug info as needed

   //******** PROTECTED METHODS ********

   // Return true if the command is ready to be executed:
   bool is_ready() const;

   // Generate "ribbons" of quads joining the two boundaries:
   void gen_ribbons(CEdgeStrip& a, CEdgeStrip& b, Patch* p);

   // Create a single "ribbon" (row of quads) between two matching
   // connected components of the boundaries.
   void gen_ribbon(CBvert_list& a, CBvert_list& b, Patch* p);
};

/*****************************************************************
 * WPT_LIST_RESHAPE_CMD
 * 
 *    Reshape a Wpt_list. (Used in oversketching curves.)
 *
 *      -- Kiran Kulkarni
 *****************************************************************/
MAKE_PTR_SUBC(WPT_LIST_RESHAPE_CMD,COMMAND);
class WPT_LIST_RESHAPE_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   WPT_LIST_RESHAPE_CMD(Wpt_listMap* map, CWpt_list& new_pts) : _map(0) {
      assert(map);
      _map       = map;
      _old_shape = map->get_wpts();
      _new_shape = new_pts;
   }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("WPT_LIST_RESHAPE_CMD",
                        WPT_LIST_RESHAPE_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Wpt_listMap* _map;
   Wpt_list     _old_shape;
   Wpt_list     _new_shape;
};

/*****************************************************************
 * TUBE_MAP_RESHAPE_CMD
 * 
 *    Reshape a UV parameterized tube. (Used in editing surfs of revolution.)
 *
 *****************************************************************/
MAKE_PTR_SUBC(TUBE_MAP_RESHAPE_CMD,COMMAND);
class TUBE_MAP_RESHAPE_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   TUBE_MAP_RESHAPE_CMD(TubeMap* map, CWpt_list& new_pts) : _map(0) {
      assert(map);
      _map       = map;
      _old_shape = map->get_wpts();
      _new_shape = new_pts;
   }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("TUBE_MAP_RESHAPE_CMD",
                        TUBE_MAP_RESHAPE_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   TubeMap* _map;
   Wpt_list     _old_shape;
   Wpt_list     _new_shape;
};

#endif // INCLUDE_TESS_CMD_H
