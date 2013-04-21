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
 * mesh_op.H
 *
 *   Operations for splitting and joining seams in a BMESH
 *
 *****************************************************************/
#ifndef MESH_OP_H_IS_INCLUDED
#define MESH_OP_H_IS_INCLUDED

#include "geom/command.H"
#include "mesh/lmesh.H"
#include "skin.H"
#include "skin_meme.H"

/*****************************************************************
 * JOIN_SEAM_CMD
 *
 *   mesh topology op: joing together two matching edge chains
 *   in a mesh.
 *****************************************************************/
MAKE_PTR_SUBC(JOIN_SEAM_CMD,COMMAND);
class JOIN_SEAM_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   // The two vert lists must form chains.
   // The vertices of o ("open") will be merged into
   // the vertices of c ("closed"):
   JOIN_SEAM_CMD(CBvert_list& o, CBvert_list& c);

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("JOIN_SEAM_CMD", JOIN_SEAM_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   // Join the seam:
   virtual bool doit();

   // Unjoin the seam:
   virtual bool undoit();

 protected:
   Bvert_list   _o;     // open chain
   Bvert_list   _c;     // closed chain

   LMESHptr     _mesh;
   MULTI_CMDptr _cmds;

   void redefine_edges(Bvert* o, Bvert* c);
   void redefine_faces(Bvert* o, Bvert* c);
   void redefine_faces(Bedge* o, Bedge* c);
   void reparent_verts();
};

/*****************************************************************
 * FIT_VERTS_CMD
 *
 *   Fit a set of verts to some new locations. Can involve either
 *   changing meme data, moving verts directly, or defining new
 *   subdiv offsets for them.
 *****************************************************************/
MAKE_PTR_SUBC(FIT_VERTS_CMD,COMMAND);
class FIT_VERTS_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   FIT_VERTS_CMD(CBvert_list& verts, CWpt_list& new_locs);

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("FIT_VERTS_CMD", FIT_VERTS_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Bvert_list   _verts;
   ARRAY<Wpt_list>     _old_lists;
   Wpt_list     _new_locs;

   //******** UTILITIES ********

   // Returns true if internal data is consistent w/ expectations:
   bool is_good() const;

   bool is_empty() const { return _verts.empty(); }

   // Recursive procedure to fit subdiv verts to desired new
   // locations.  first fits parent level (recursively), then assigns
   // offsets at this level to approximate the desired new locations.
   void fit_verts(CBvert_list& verts, CWpt_list& new_locs, int lev);
};

/*****************************************************************
 * SUBDIV_OFFSET_CMD
 *
 *   Fit a set of verts to some new locations. Can involve either
 *   changing meme data, moving verts directly, or defining new
 *   subdiv offsets for them.
 *****************************************************************/
MAKE_PTR_SUBC(SUBDIV_OFFSET_CMD,COMMAND);
class SUBDIV_OFFSET_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   SUBDIV_OFFSET_CMD(CBvert_list& verts, CARRAY<double>& offsets);

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SUBDIV_OFFSET_CMD", SUBDIV_OFFSET_CMD*,
                        COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Bvert_list           _verts;
   ARRAY<double>        _offsets;
   SUBDIV_OFFSET_CMDptr _parent_cmd;
   MULTI_CMDptr         _ctrl_vert_cmds;

   //******** UTILITIES ********

   // Returns true if internal data is consistent w/ expectations:
   bool is_good() const;

   bool is_empty() const { return _verts.empty(); }

   // Recursive procedure to assign new offsets to subdiv verts.
   // First assigns offsets to parent level (recursively), then
   // assigns offsets at this level, minus the effects of the new
   // offsets at the parent level.
   void fit_verts(CBvert_list& verts, CARRAY<double>& offsets);
};

/*****************************************************************
 * INFLATE_SKIN_ADD_OFFSET_CMD
 *
 *   increase/decrease the height of inflation
 *****************************************************************/
MAKE_PTR_SUBC(INFLATE_SKIN_ADD_OFFSET_CMD,COMMAND);
class INFLATE_SKIN_ADD_OFFSET_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   INFLATE_SKIN_ADD_OFFSET_CMD(Skin* skin, double offset);

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("INFLATE_SKIN_ADD_OFFSET_CMD", INFLATE_SKIN_ADD_OFFSET_CMD*,
                        COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Skin*           _skin;
   double          _offset;
};

/*****************************************************************
 * CREASE_INC_CMD
 *
 *   increase/decrease the sharpness of creases
 *****************************************************************/
MAKE_PTR_SUBC(CREASE_INC_CMD,COMMAND);
class CREASE_INC_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   CREASE_INC_CMD(CBedge_list& edges, ushort v, bool is_inc);

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("CREASE_INC_CMD", CREASE_INC_CMD*,
                        COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Bedge_list           _edges;
   ushort          _v;
   bool           _is_inc;
};

#endif // MESH_OP_H_IS_INCLUDED

// end of file mesh_op.H
