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
#include "disp/colors.H"
#include "geom/world.H"
#include "std/config.H"
#include "mesh/mi.H"

#include "tess/bbase.H"
#include "tess/mesh_op.H"

using namespace mlib;

static bool debug = Config::get_var_bool("DEBUG_MESH_OPS",false);

inline void
show(CBedge* e)
{
   if (debug) {
      WORLD::show(e->v1()->loc(), e->v2()->loc(), 3, Color::blue);
   }
}

/*****************************************************************
 * REPARENT_CMD
 *
 *   Given a vertex o that has been identified to vertex c,
 *   find the parent of o and tell it that its child is now c.
 *   Used when joining seams together. (The "open" part gets
 *   merged into the "closed" part, hence 'o' and 'c'.)
 *****************************************************************/
MAKE_PTR_SUBC(REPARENT_CMD,COMMAND);
class REPARENT_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   REPARENT_CMD(Bvert* o, Bvert* c);

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("REPARENT_CMD", REPARENT_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Lvert*       _o;
   Lvert*       _c;
};

REPARENT_CMD::REPARENT_CMD(Bvert* o, Bvert* c) : _o(0), _c(0) 
{
   assert(o && c && o->mesh() == c->mesh());
   if (!LMESH::isa(o->mesh()))
      return;
   _o = (Lvert*)o;
   _c = (Lvert*)c;
}

bool
REPARENT_CMD::doit()
{
   if (is_done())
      return true;

   if (!(_o && _c)) {
      return true;
   }
   if (_o == _c)
      return true;
   if (is_vert(_o->parent())) {
      Lvert* p = (Lvert*)_o->parent();
      assert(p->subdiv_vertex() == _o);
      p->assign_subdiv_vert(_c);
   } else if (is_edge(_o->parent())) {
      Ledge* p = (Ledge*)_o->parent();
      assert(p->subdiv_vertex() == _o);
      p->assign_subdiv_vert(_c);
   } else {
      // no parent, no problem
   }

   return COMMAND::doit();
}

bool
REPARENT_CMD::undoit()
{
   if (!is_done())
      return true;

   if (!(_o && _c)) {
      return true;
   }
   if (_o == _c)
      return true;
   if (is_vert(_o->parent())) {
      Lvert* p = (Lvert*)_o->parent();
      assert(p->subdiv_vertex() == _c);
      p->assign_subdiv_vert(_o);
   } else if (is_edge(_o->parent())) {
      Ledge* p = (Ledge*)_o->parent();
      assert(p->subdiv_vertex() == _c);
      p->assign_subdiv_vert(_o);
   } else {
      // no parent, no problem
   }

   return COMMAND::undoit();
}

/*****************************************************************
 * REDEF_FACE_V_CMD
 *
 *   Perform a lightweight face redefinition, replacing one vertex
 *   with another in a given Bface, but not changing the edges.
 *****************************************************************/
MAKE_PTR_SUBC(REDEF_FACE_V_CMD,COMMAND);
class REDEF_FACE_V_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   REDEF_FACE_V_CMD(Bface* f, Bvert* o, Bvert* n) : _f(f), _old(o), _new(n) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("REDEF_FACE_V_CMD", REDEF_FACE_V_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Bface*       _f;
   Bvert*       _old;
   Bvert*       _new;
};

bool
REDEF_FACE_V_CMD::doit()
{
   if (is_done())
      return true;

   if (!_f) {
      err_adv(debug, "REDEF_FACE_V_CMD::doit: error: null face");
      return false;
   }
   if (!_f->contains(_old)) {
      err_adv(debug, "REDEF_FACE_V_CMD::doit: error: old vert not in face");
      return false;
   }
   if (!_f->redef2(_old, _new)) {
      err_adv(debug, "REDEF_FACE_V_CMD::doit: error: Bface::redef2 failed");
      return false;
   }

   return COMMAND::doit();
}

bool
REDEF_FACE_V_CMD::undoit()
{
   if (!is_done())
      return true;

   if (!_f) {
      err_adv(debug, "REDEF_FACE_V_CMD::undoit: error: null face");
      return false;
   }
   if (!_f->contains(_new)) {
      err_adv(debug, "REDEF_FACE_V_CMD::undoit: error: new vert not in face");
      return false;
   }
   if (!_f->redef2(_new, _old)) {
      err_adv(debug, "REDEF_FACE_V_CMD::undoit: error: Bface::redef2 failed");
      return false;
   }

   return COMMAND::undoit();
}

/*****************************************************************
 * REDEF_FACE_E_CMD
 *
 *   Update a Bface to replace one edge with another.
 *****************************************************************/
MAKE_PTR_SUBC(REDEF_FACE_E_CMD,COMMAND);
class REDEF_FACE_E_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   REDEF_FACE_E_CMD(Bface* f, Bedge* o, Bedge* n) : _f(f), _old(o), _new(n) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("REDEF_FACE_E_CMD", REDEF_FACE_E_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Bface*       _f;
   Bedge*       _old;
   Bedge*       _new;
};

bool
REDEF_FACE_E_CMD::doit()
{
   if (is_done())
      return true;

   if (!_f) {
      err_adv(debug, "REDEF_FACE_E_CMD::doit: error: null face");
      return false;
   }
   if (!_f->contains(_old)) {
      err_adv(debug, "REDEF_FACE_E_CMD::doit: error: old edge not in face");
      return false;
   }
   if (!_f->redef2(_old, _new)) {
      err_adv(debug, "REDEF_FACE_E_CMD::doit: error: Bface::redef2 failed");
      return false;
   }

   return COMMAND::doit();
}

bool
REDEF_FACE_E_CMD::undoit()
{
   if (!is_done())
      return true;

   if (!_f) {
      err_adv(debug, "REDEF_FACE_E_CMD::undoit: error: null face");
      return false;
   }
   if (!_f->contains(_new)) {
      err_adv(debug, "REDEF_FACE_E_CMD::undoit: error: new edge not in face");
      return false;
   }
   if (!_f->redef2(_new, _old)) {
      err_adv(debug, "REDEF_FACE_E_CMD::undoit: error: Bface::redef2 failed");
      return false;
   }

   return COMMAND::undoit();
}

/*****************************************************************
 * REDEF_EDGE_CMD
 *
 *   Redefine an edge by replacing one vertex with another.
 *****************************************************************/
MAKE_PTR_SUBC(REDEF_EDGE_CMD,COMMAND);
class REDEF_EDGE_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   REDEF_EDGE_CMD(Bedge* e, Bvert* o, Bvert* n) : _e(e), _old(o), _new(n) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("REDEF_EDGE_CMD", REDEF_EDGE_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Bedge*       _e;
   Bvert*       _old;
   Bvert*       _new;
};

bool
REDEF_EDGE_CMD::doit()
{
   if (is_done())
      return true;

   if (!_e) {
      err_adv(debug, "REDEF_EDGE_CMD::doit: error: null edge");
      return false;
   }
   if (!_e->contains(_old)) {
      err_adv(debug, "REDEF_EDGE_CMD::doit: error: old vert not in edge");
      return false;
   }
   if (!_e->redef2(_old, _new)) {
      err_adv(debug, "REDEF_EDGE_CMD::doit: error: Bedge::redef2 failed");
      show(_e);
      return false;
   }

   return COMMAND::doit();
}

bool
REDEF_EDGE_CMD::undoit()
{
   if (!is_done())
      return true;

   if (!_e) {
      err_adv(debug, "REDEF_EDGE_CMD::undoit: error: null edge");
      return false;
   }
   if (!_e->contains(_new)) {
      err_adv(debug, "REDEF_EDGE_CMD::undoit: error: new vert not in edge");
      return false;
   }
   if (!_e->redef2(_new, _old)) {
      err_adv(debug, "REDEF_EDGE_CMD::undoit: error: Bedge::redef2 failed");
      return false;
   }

   return COMMAND::undoit();
}

/*****************************************************************
 * JOIN_SEAM_CMD
 *****************************************************************/
JOIN_SEAM_CMD::JOIN_SEAM_CMD(CBvert_list& o, CBvert_list& c) :
   _cmds(new MULTI_CMD())
{
   LMESH* m = LMESH::upcast(c.mesh());
   if (!(m && m == o.mesh() && c.num() == o.num())) {
      err_adv(debug, "JOIN_SEAM_CMD::JOIN_SEAM_CMD: bad chains");
      return;
   }
   if (!(c.forms_closed_chain() && o.forms_closed_chain())) {
      if (c.forms_closed_chain() || o.forms_closed_chain()) {
         err_adv(debug, "JOIN_SEAM_CMD::JOIN_SEAM_CMD: mixed chain types");
         return;
      }
      // non-closed chains have to share vertices at beginning and end:
      if (!(c.first() == o.first() && c.last() == o.last())) {
         err_adv(debug, "JOIN_SEAM_CMD::JOIN_SEAM_CMD: bad chain topology");
         return;
      }
   }
   _mesh = m;
   _o = o;
   _c = c;
}

bool
JOIN_SEAM_CMD::doit()
{
   // if done, rest
   if (is_done())
      return true;

   // if it was done, then undone,
   // just replay the commands
   if (is_undone()) {
      _cmds->doit();
      return COMMAND::doit();
   }

   // have to build the commands

   // pull out edges along seam now, while mesh is undisturbed
   Bedge_list o_edges;
   Bedge_list c_edges;
   if (_o.forms_closed_chain() && _c.forms_closed_chain()) {
      o_edges = _o.get_closed_chain();
      c_edges = _c.get_closed_chain();
   } else {
      o_edges = _o.get_chain();
      c_edges = _c.get_chain();
   }
   if (o_edges.empty() || c_edges.empty()) {
      err_adv(debug, "JOIN_SEAM_CMD::doit: failed to get edge chains");
      return false;
   }

   // mark o chain edges w/ flag 1, others w/ flag 0:
   _o.clear_flag02();
   o_edges.set_flags(1);

   // for each face adjacent to an o vertex, redefine it to
   // replace o with c, but don't update the edges yet:
   for (int i=0; i<_o.num(); i++)
      redefine_faces(_o[i], _c[i]);

   // for each non-chain edge adjacent to an o vertex,
   // redefine it to replace o with c. (o-chain edges
   // have flag = 1; others have flag = 0).
   for (int j=0; j<_o.num(); j++)
      redefine_edges(_o[j], _c[j]);

   // for each o-chain edge, update adjacent faces
   // to replace the edge w/ corresponding c-chain edge
   for (int k=0; k<o_edges.num(); k++)
      redefine_faces(o_edges[k], c_edges[k]);

   // switch parents of o to think their children are c
   reparent_verts();

   // the command list is now built, and every command has
   // been executed as we went along. now tell the multi-cmd
   // that it "is done":
   _cmds->COMMAND::doit();

   // and remember for ourselves that we are done:
   return COMMAND::doit();
}

void
JOIN_SEAM_CMD::reparent_verts()
{
   for (int i=0; i<_o.num(); i++) {
      REPARENT_CMDptr cmd = new REPARENT_CMD(_o[i], _c[i]);
      cmd->doit();
      _cmds->add(cmd);
   }
}

void
JOIN_SEAM_CMD::redefine_faces(Bvert* o, Bvert* c)
{
   assert(o && c);

   // it is legal to "redefine" using the same vert,
   // but nothing happens
   if (o == c)
      return;

   Bface_list faces;
   o->get_all_faces(faces);
   for (int i=0; i<faces.num(); i++) {
      REDEF_FACE_V_CMDptr cmd = new REDEF_FACE_V_CMD(faces[i], o, c);
      cmd->doit();
      _cmds->add(cmd);
   }
}

void
JOIN_SEAM_CMD::redefine_edges(Bvert* o, Bvert* c)
{
   assert(o && c);

   // it is legal to "redefine" using the same vert,
   // but nothing happens
   if (o == c)
      return;

   // get a copy of the adjacency list because it will change as we go:
   Bedge_list edges = o->get_adj();
   for (int i=0; i<edges.num(); i++) {
      // only do edges w/ flag == 0
      if (!edges[i]->flag()) {
         REDEF_EDGE_CMDptr cmd = new REDEF_EDGE_CMD(edges[i], o, c);
         cmd->doit();
         _cmds->add(cmd);
      }
   }
}

void
JOIN_SEAM_CMD::redefine_faces(Bedge* o, Bedge* c)
{
   assert(o && c);

   // it is legal to "redefine" using the same edge,
   // but nothing happens
   if (o == c)
      return;

   Bface_list faces = o->get_all_faces();
   for (int i=0; i<faces.num(); i++) {
      REDEF_FACE_E_CMDptr cmd = new REDEF_FACE_E_CMD(faces[i], o, c);
      cmd->doit();
      _cmds->add(cmd);
   }
}

bool
JOIN_SEAM_CMD::undoit()
{
   // if not done, rest
   if (!is_done())
      return true;
   if (!_cmds->undoit()) {
      err_adv(debug, "JOIN_SEAM_CMD::undoit: error: multi-command failed");
      return false;
   }
   return COMMAND::undoit();
}

/*****************************************************************
 * FIT_VERTS_CMD
 *****************************************************************/
FIT_VERTS_CMD::FIT_VERTS_CMD(CBvert_list& verts, CWpt_list& new_locs)
{
   if (verts.num() != new_locs.num()) {
      err_msg("FIT_VERTS_CMD::FIT_VERTS_CMD: error: lists not equal: %d vs. %d",
              verts.num(), new_locs.num());
      return;
   }
   if (!LMESH::isa(verts.mesh())) {
      err_msg("FIT_VERTS_CMD::FIT_VERTS_CMD: error: need LMESH");
      return;
   }
   _verts    = verts;
   _old_lists += verts.pts();
   _new_locs = new_locs;
}

bool
FIT_VERTS_CMD::is_good() const
{
   return (
      _verts.num() == _new_locs.num() &&
      _verts.num() == _old_lists.last().num() &&
      (is_empty() || LMESH::isa(_verts.mesh()))
      );
}

inline void
get_parents(
   CBvert_list& children,
   CWpt_list&   child_locs,
   Bvert_list&  parents,
   Wpt_list&    parent_locs
   )
{
   assert(children.num() == child_locs.num());
   parents.clear();
   parent_locs.clear();
   if (!LMESH::isa(children.mesh()))
      return;
   for (int i=0; i<children.num(); i++) {
      Lvert* p = ((Lvert*)children[i])->parent_vert(1);
      if (p) {
         parents += p;
         parent_locs += child_locs[i];
      }
   }
}

inline void
fit_vert(Lvert* v, CWpt& new_loc)
{
   assert(v);
   VertMeme* vm = Bbase::find_boss_vmeme(v);
   if (vm) {
      vm->move_to(new_loc);
   } else{
      v->fit_subdiv_offset(new_loc);
   }
}

void
FIT_VERTS_CMD::fit_verts(CBvert_list& verts, CWpt_list& new_locs, int lev)
{
   // Recursive procedure to fit subdiv verts to desired new
   // locations.  first fits parent level (recursively), then assigns
   // offsets at this level to approximate the desired new locations.

   static bool debug = Config::get_var_bool("DEBUG_OVERSKETCH",false);

   err_adv(debug, "FIT_VERTS_CMD::fit_verts: fitting %d verts", verts.num());

   assert(is_done() || verts.num() == new_locs.num());

   if (verts.empty())
      return; // no-op

   // fit parent level to new locations first
   Bvert_list parents;
   Wpt_list   parent_locs;
   get_parents(verts, new_locs, parents, parent_locs);
   if (!parents.empty()) {
      if (!is_done()) _old_lists.push(parents.pts());
      fit_verts(parents, parent_locs, lev-1);
   }

   // now compute offsets at this level
   // needed to approximate new locs:
   for (int i=0; i<verts.num(); i++) {
      if (!is_done()) 
         fit_vert((Lvert*)verts[i], new_locs[i]);
      else
         fit_vert((Lvert*)verts[i], _old_lists[lev][i]);
   }

   // now actually recompute subdiv locations at this level to
   // complete the fitting process:
   if (!parents.empty()) {
      LMESH::update_subdivision(parents, 1);
   }

   BMESH* mesh = verts.mesh();
   if (mesh)
      mesh->changed(BMESH::VERT_POSITIONS_CHANGED);
   else
      err_adv(debug, "FIT_VERTS_CMD::fit_verts: warning: mesh is null");
}

bool
FIT_VERTS_CMD::doit()
{
   if (is_done())
      return true;
   if (is_empty())
      return true;

   assert(is_good());

   fit_verts(_verts, _new_locs, -1);

   return COMMAND::doit();
}

bool
FIT_VERTS_CMD::undoit()
{
   if (!is_done())
      return true;
   if (is_empty())
      return true;

   assert(is_good());

   fit_verts(_verts, _old_lists.last(), _old_lists.num()-1);

   return COMMAND::undoit();
}

/*****************************************************************
 * MOVE_VERT_CMD
 *****************************************************************/
MAKE_PTR_SUBC(MOVE_VERT_CMD,COMMAND);
class MOVE_VERT_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   MOVE_VERT_CMD(Bvert* v, CWpt& new_loc);

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("MOVE_VERT_CMD", MOVE_VERT_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   virtual bool doit();
   virtual bool undoit();

 protected:
   Bvert*       _vert;
   Wpt          _old_loc;
   Wpt          _new_loc;
};

MOVE_VERT_CMD::MOVE_VERT_CMD(Bvert* v, CWpt& new_loc) : _vert(0)
{
   assert(v);
   _vert = v;
   _old_loc = v->loc();
   _new_loc = new_loc;
}

bool
MOVE_VERT_CMD::doit()
{
   if (is_done())
      return true;

   if (!_vert)
      return true;

   VertMeme* vm = Bbase::find_boss_vmeme(_vert);
   if (vm) {
      vm->move_to(_new_loc);
   } else{
      _vert->set_loc(_new_loc);
   }

   return COMMAND::doit();
}

bool
MOVE_VERT_CMD::undoit()
{
   if (!is_done())
      return true;

   if (!_vert)
      return true;

   VertMeme* vm = Bbase::find_boss_vmeme(_vert);
   if (vm) {
      vm->move_to(_old_loc);
   } else{
      _vert->set_loc(_old_loc);
   }

   return COMMAND::undoit();
}

/*****************************************************************
 * SUBDIV_OFFSET_CMD
 *****************************************************************/
inline ARRAY<double>
get_offsets(CBvert_list& verts)
{
   if (!LMESH::isa(verts.mesh()))
      return ARRAY<double>(0);

   ARRAY<double> ret(verts.num());
   for (int i=0; i<verts.num(); i++)
      ret += ((Lvert*)verts[i])->offset();
   return ret;
}


inline void
get_parents(
   CBvert_list&         children,
   CARRAY<double>&      child_offsets,
   Bvert_list&          parents,        // return val
   ARRAY<double>&       parent_offsets  // return val
   )
{
   assert(children.num() == child_offsets.num());
   parents.clear();
   parent_offsets.clear();
   if (!LMESH::isa(children.mesh()))
      return;
   for (int i=0; i<children.num(); i++) {
      Lvert* p = ((Lvert*)children[i])->parent_vert(1);
      if (p) {
         parents += p;
         parent_offsets += child_offsets[i];
      }
   }
}

SUBDIV_OFFSET_CMD::SUBDIV_OFFSET_CMD(
   CBvert_list&    verts,
   CARRAY<double>& offsets
   )
{
   if (verts.num() != offsets.num()) {
      err_msg("SUBDIV_OFFSET_CMD: error: lists not equal: %d vs. %d",
              verts.num(), offsets.num());
      return;
   }
   if (!LMESH::isa(verts.mesh())) {
      err_msg("SUBDIV_OFFSET_CMD::SUBDIV_OFFSET_CMD: error: need LMESH");
      return;
   }
   _verts   = verts;
   _offsets = offsets;

   Bvert_list    parents;
   ARRAY<double> parent_offsets;
   get_parents(verts, offsets, parents, parent_offsets);
   if (!parents.empty())
      _parent_cmd  = new SUBDIV_OFFSET_CMD(parents, parent_offsets);
   _ctrl_vert_cmds = new MULTI_CMD;
}

bool
SUBDIV_OFFSET_CMD::is_good() const
{
   return (
      _verts.num() == _offsets.num() &&
      (is_empty() || LMESH::isa(_verts.mesh()))
      );
}

inline Wpt_list
get_smooth_locs(CBvert_list& verts)
{
   if (!LMESH::isa(verts.mesh()))
      return Wpt_list();

   Wpt_list ret(verts.num());
   for (int i=0; i<verts.num(); i++)
      ret += ((Lvert*)verts[i])->smooth_loc_from_parent();
   return ret;
}

inline ARRAY<Wvec>
get_parent_norms(CBvert_list& verts)
{
   if (!LMESH::isa(verts.mesh()))
      return ARRAY<Wvec>();

   ARRAY<Wvec> ret(verts.num());
   for (int i=0; i<verts.num(); i++)
      ret += get_norm(((Lvert*)verts[i])->parent());
   return ret;
}

inline void
mesh_changed(BMESH* mesh, int line_num)
{
   if (mesh)
      mesh->changed(BMESH::TOPOLOGY_CHANGED);
   else
      err_adv(debug, "mesh_op.C:%d: warning: mesh is null", line_num);
}

bool
SUBDIV_OFFSET_CMD::doit()
{
   if (is_done())
      return true;

   if (is_empty())
      return true;

   assert(is_good());

   // If the command has not been executed before, apply the offsets
   // first at the parent level, then reduce the amount of offset to
   // be applied at this level to take into account the parent offsets.
   if (is_clear()) {
      Wpt_list old_base = get_smooth_locs(_verts);
      if (_parent_cmd) _parent_cmd->doit();
      Wpt_list    new_base  = get_smooth_locs(_verts);
      ARRAY<Wvec> new_norms = get_parent_norms(_verts);
      for (int i=0; i<_verts.num(); i++) {
         if (((Lvert*)_verts[i])->parent()) {
            double dh = (new_base[i] - old_base[i])*new_norms[i];
            _offsets[i] -= dh;
         } else {
            // control verts cannot be moved via their subdiv offset;
            // instead move them directly w/ a MOVE_VERT_CMD:
            _ctrl_vert_cmds->add(
               new MOVE_VERT_CMD(
                  _verts[i],
                  _verts[i]->loc() + (_verts[i]->norm() * _offsets[i])
                  )
               );
            _offsets[i] = 0;
         }
      }
   }

   // Now, whether it is the 1st time or later,
   // actually apply the corrected offsets
   if (_parent_cmd)
      _parent_cmd->doit();

   LMESH::update_subdivision(get_parent_faces(_verts.one_ring_faces(), 1), 1);

   for (int i=0; i<_verts.num(); i++) {
      if (((Lvert*)_verts[i])->parent())
         ((Lvert*)_verts[i])->add_offset(_offsets[i]);
   }
   _ctrl_vert_cmds->doit();

   mesh_changed(_verts.mesh(), __LINE__);

   return COMMAND::doit();
}

bool
SUBDIV_OFFSET_CMD::undoit()
{
   if (!is_done())
      return true;

   if (is_empty())
      return true;

   assert(is_good());

   _ctrl_vert_cmds->undoit();
   for (int i=0; i<_verts.num(); i++) {
      if (((Lvert*)_verts[i])->parent())
         ((Lvert*)_verts[i])->add_offset(-_offsets[i]);
   }

   if (_parent_cmd)
      _parent_cmd->undoit();

   LMESH::update_subdivision(get_parent_faces(_verts.one_ring_faces(), 1), 1);

   mesh_changed(_verts.mesh(), __LINE__);

   return COMMAND::undoit();
}

/*****************************************************************
 * INFLATE_SKIN_ADD_OFFSET_CMD
 *****************************************************************/
INFLATE_SKIN_ADD_OFFSET_CMD::INFLATE_SKIN_ADD_OFFSET_CMD(
   Skin* skin,
   double offset
   )
{
   if (!skin->is_inflate()) {
      err_msg("INFLATE_SKIN_ADD_OFFSET_CMD: error: input skin not an inflate");
      return;
   }
   _skin   = skin;
   _offset = offset;
}

void
add_offset(Skin* skin, double offset)
{
   if (!skin) return;

   VertMemeList memes = skin->vmemes();
   for (int i = 0; i < memes.num(); i++) {
      SkinMeme::upcast(memes[i])->add_offset(offset);
   }

   add_offset(skin->child(), offset);
}

bool
INFLATE_SKIN_ADD_OFFSET_CMD::doit()
{
   if (is_done())
      return true;

   add_offset(_skin, _offset);

   return COMMAND::doit();
}

bool
INFLATE_SKIN_ADD_OFFSET_CMD::undoit()
{
   if (!is_done())
      return true;

   add_offset(_skin, -_offset);

   return COMMAND::undoit();
}


/*****************************************************************
 * CREASE_INC_CMD
 *****************************************************************/
CREASE_INC_CMD::CREASE_INC_CMD(
   CBedge_list& edges,
   ushort v,
   bool is_inc
   )
{
   _edges   = edges;
   _v = v;
   _is_inc = is_inc;
}

bool
CREASE_INC_CMD::doit()
{
   if (is_done())
      return true;

   if (_is_inc) _edges.inc_crease_vals(_v);
   else _edges.dec_crease_vals(_v);

   return COMMAND::doit();
}

bool
CREASE_INC_CMD::undoit()
{
   if (!is_done())
      return true;

   if (_is_inc) _edges.dec_crease_vals(_v);
   else _edges.inc_crease_vals(_v);

   return COMMAND::undoit();
}

// end of file mesh_op.C
