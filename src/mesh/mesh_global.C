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
#include "lmesh.H"
#include "patch.H"
#include "mesh_global.H"
#include "std/config.H"

// static data
Bface_list MeshGlobal::_selected_faces;
Bedge_list MeshGlobal::_selected_edges;
Bvert_list MeshGlobal::_selected_verts;

//*****************************************************************

void
MeshGlobal::select(Bface*f) 
{ 
   // adds the face (and its quad partner, if it has one) to the
   // selected list, setting the selected flags as needed

   if (!f || f->is_selected())
      return;

   f->set_bit(Bsimplex::SELECTED_BIT);
   _selected_faces += f;

   if (f->is_quad())
      select(f->quad_partner());

   BMESH::set_focus(f->mesh(), get_patch(f));
}

void
MeshGlobal::deselect(Bface* f) 
{ 
   // removes the face (and its quad partner, if it has one) from the
   // selected list, clearing the selected flags as needed
   if (!(f && f->is_selected()))
      return;

   f->clear_bit(Bsimplex::SELECTED_BIT);
   _selected_faces -= f; // O(n) operation

   if (f->is_quad())
      deselect(f->quad_partner());
}

void
MeshGlobal::toggle_select(Bface* f) 
{ 
   // deselects the face if it is currently selected
   // and selects it otherwise

   if (!f)
      return;

   if (f->is_selected()) {
      deselect(f);
   } else {
      select(f);
   }
}

void
MeshGlobal::select(CBface_list& faces) 
{ 
   // selects the faces

   for (int i=0; i<faces.num(); i++) {
      select(faces[i]);
   }
}

void
MeshGlobal::deselect(CBface_list& faces) 
{ 
   // deselects the faces

   for (int i=0; i<faces.num(); i++) {
      deselect(faces[i]);
   }
}

void
MeshGlobal::deselect_all_faces() 
{ 
   // removes all faces from the selected list, clearing the selected
   // flags as needed

   _selected_faces.clear_bits(Bsimplex::SELECTED_BIT);
   _selected_faces.clear();
}

Bface_list 
MeshGlobal::selected_faces(BMESH* mesh) 
{ 
   // returns a list of all currently selected faces from the given
   // mesh

   return _selected_faces.filter(MeshSimplexFilter(mesh));
}

Bface_list
MeshGlobal::selected_faces_all_levels(BMESH* m)
{
   // returns a list of all currently selected faces from the given
   // mesh. if the given mesh is an LMESH*, also returns selected
   // faces from any other meshes in its subdivision hierarchy

   Bface_list ret; 

   if (!m) 
      return ret;

  // get the control mesh (this will just be m if
  // m in not an LMESH)
   BMESH* cm = get_ctrl_mesh(m);

   // return the faces selected from this mesh
   ret += selected_faces(cm);

   // If this is an lmesh, also find the selected
   // faces for any sub mesh in the hierarchy
   for (LMESH* lm = LMESH::upcast(cm); lm != 0; lm = lm->subdiv_mesh()) {
      ret += selected_faces(lm);
   }

   return ret;
}

//*****************************************************************

void
MeshGlobal::select(Bedge* e) 
{ 
   // adds the edge to the selected list, setting the selected flag
   // as needed

   if (!e || e->is_selected())
      return;

   e->set_bit(Bsimplex::SELECTED_BIT);
   _selected_edges += e;

   BMESH::set_focus(e->mesh(), get_patch(e));
}

void
MeshGlobal::deselect(Bedge* e) 
{ 
   // removes the edge from the selected list, clearing the selected
   // flag as needed

   if (!(e && e->is_selected()))
      return;

   e->clear_bit(Bsimplex::SELECTED_BIT);
   _selected_edges -= e;
}

void
MeshGlobal::toggle_select(Bedge* e) 
{ 
   // deselects the edge if it is currently selected
   // and selects it otherwise

   if (!e)
      return;

   if (e->is_selected()) {
      deselect(e);
   } else {
      select(e);
   }
}

void
MeshGlobal::deselect_all_edges() 
{ 
   // removes all edges from the selected list, clearing the
   // selected flag as needed

   _selected_edges.clear_bits(Bsimplex::SELECTED_BIT);
   _selected_edges.clear();
}

void
MeshGlobal::select(CBedge_list& edges) 
{ 
   // selects the edges

   for (int i=0; i<edges.num(); i++) {
      select(edges[i]);
   }
}

void
MeshGlobal::deselect(CBedge_list& edges) 
{ 
   // deselects the edges

   for (int i=0; i<edges.num(); i++) {
      deselect(edges[i]);
   }
}

Bedge_list 
MeshGlobal::selected_edges(BMESH* mesh) 
{ 
   // returns a list of all currently selected edges from the given
   // mesh

   return _selected_edges.filter(MeshSimplexFilter(mesh));
}

Bedge_list
MeshGlobal::selected_edges_all_levels(BMESH* m)
{
   // returns a list of all currently selected edges from the given
   // mesh. if the given mesh is an LMESH*, also returns selected
   // edges from any other meshes in the subdivision hierarchy

   Bedge_list ret; 

   if (!m) 
      return ret;

   // get the control mesh (this will just be m if
   // m in not an LMESH)
   BMESH* cm = get_ctrl_mesh(m);

   // return the edges selected from this mesh
   ret += selected_edges(cm);

   // if this is an lmesh, also find the selected
   // edges for any sub mesh in the hierarchy

   if (LMESH::isa(cm)) {
      LMESH* lm = (LMESH*)cm;
      while(lm->subdiv_mesh()) {
         lm = lm->subdiv_mesh();
         ret += selected_edges(lm);
      }
   }

   return ret;
}

//*****************************************************************

void
MeshGlobal::select(Bvert* v) 
{ 
   // adds the vert to the selected list, setting the selected flag
   // as needed

   if (!v || v->is_selected())
      return;

   v->set_bit(Bsimplex::SELECTED_BIT);
   _selected_verts += v;

   BMESH::set_focus(v->mesh(), get_patch(v));
}

void
MeshGlobal::deselect(Bvert* v) 
{ 
   // removes the vert from the selected list, clearing the selected
   // flag as needed

   if (!(v && v->is_selected()))
      return;

   v->clear_bit(Bsimplex::SELECTED_BIT);
   _selected_verts -= v;
}

void
MeshGlobal::toggle_select(Bvert* v) 
{ 
   // deselects the vert if it is currently selected
   // and selects it otherwise

   if (!v)
      return;

   if (v->is_selected()) {
      deselect(v);
   } else {
      select(v);
   }
}

void
MeshGlobal::deselect_all_verts() 
{ 
   // removes all verts from the selected list, clearing the
   // selected flag as needed

   _selected_verts.clear_bits(Bsimplex::SELECTED_BIT);
   _selected_verts.clear();
}

void
MeshGlobal::select(CBvert_list& verts) 
{ 
   // selects the verts

   for (int i=0; i<verts.num(); i++) {
      select(verts[i]);
   }
}

void
MeshGlobal::deselect(CBvert_list& verts) 
{ 
   // deselects the verts

   for (int i=0; i<verts.num(); i++) {
      deselect(verts[i]);
   }
}

Bvert_list 
MeshGlobal::selected_verts(BMESH* mesh) 
{ 
   // returns a list of all currently selected verts from the given
   // mesh

   return _selected_verts.filter(MeshSimplexFilter(mesh));
}

Bvert_list
MeshGlobal::selected_verts_all_levels(BMESH* m)
{
   // returns a list of all currently selected verts from the given
   // mesh. if the given mesh is an LMESH*, also returns selected
   // verts from any other meshes in the subdivision hierarchy

   Bvert_list ret; 

   if (!m) 
      return ret;

   // get the control mesh (this will just be m if
   // m in not an LMESH)
   BMESH* cm = get_ctrl_mesh(m);

   // return the verts selected from this mesh
   ret += selected_verts(cm);

   // if this is an lmesh, also find the selected
   // verts for any sub mesh in the hierarchy

   if (LMESH::isa(cm)) {
      LMESH* lm = (LMESH*)cm;
      while(lm->subdiv_mesh()) {
         lm = lm->subdiv_mesh();
         ret += selected_verts(lm);
      }
   }

   return ret;
}

//*****************************************************************

inline
bool all_selected(CBface_list& faces) 
{
   return faces.all_satisfy(BitSetSimplexFilter(Bsimplex::SELECTED_BIT));
}

inline
bool all_selected(CBedge_list& edges) 
{
   return edges.all_satisfy(BitSetSimplexFilter(Bsimplex::SELECTED_BIT));
}

void
debug_sel_faces_per_level(LMESH* m)
{
   cerr << "sel faces per level" << endl;
   m = (LMESH*)get_ctrl_mesh(m);

   while(m) {
      cerr << "num sel faces level: " << m->subdiv_level() << ": "
           << MeshGlobal::selected_faces(m).num() << endl;
      m = m->subdiv_mesh();
   }
   cerr << "=====================" << endl;
}

void              
MeshGlobal::edit_level_changed(BMESH* mesh, int from, int to)
{
   // updates the list of selected components of the given
   // mesh to reflect a change in edit level. 'from' is the old
   // edit level, 'to' is the new edit level.

   // we want the components at the new level to reasonably correspond
   // to what's selected at the old level. the policy is that if the
   // edit level is refined (increased), then components at the new
   // level should be selected if they are descendents of selected
   // components at the old level.  likewise, if the edit level is
   // unrefined (decreased), then components at the new level should
   // be selected only if all of their descendents at the old level
   // were selected.

   static bool debug = Config::get_var_bool("DEBUG_EDIT_LEVEL_CHANGED",false);

   err_adv(debug, "MeshGlobal::edit_level_changed()");
   err_adv(debug, "from: %d", from);
   err_adv(debug, "to: %d", to);

   if (!LMESH::isa(mesh)) {
      err_adv(debug, "no valid lmesh");
      // don't have an lmesh, so do nothing
      return;
   }

   if (from==to) {
      err_adv(debug, "level unchanged");
      // edit level unchanged, do nothingy
      return;
   }

   // get the subdiv mesh corresponding to old edit level
   LMESH* old_edit_mesh = get_subdiv_mesh(get_ctrl_mesh(mesh), from);

   if (old_edit_mesh == 0) {
      err_adv(debug, "null mesh for old level");
      return;
   }

   // get the selected components for the old edit level
   Bface_list selected_faces = MeshGlobal::selected_faces(old_edit_mesh);
   Bedge_list selected_edges = MeshGlobal::selected_edges(old_edit_mesh);
   Bvert_list selected_verts = MeshGlobal::selected_verts(old_edit_mesh);

   Bface_list new_selected_faces;
   Bedge_list new_selected_edges;
   Bvert_list new_selected_verts;

   // loop indices
   int i=0;     // faces
   int j=0;     // edges
   int k=0;     // verts
   
   int diff = to-from;  // the change in edit level

   err_adv(debug, "change in level %d", diff);

   if (diff > 0) { // edit level refined

      // get the children of the old selected components at the new level

      for (i=0; i<selected_faces.num(); ++i) {
         ((Lface*)selected_faces[i])->append_subdiv_faces(diff, new_selected_faces);      }

      for (j=0; j<selected_edges.num(); ++j) {
         ((Ledge*)selected_edges[j])->append_subdiv_edges(diff, new_selected_edges);
      }
      for (k=0; k<selected_verts.num(); ++k) {
         Lvert* v = ((Lvert*)selected_verts[k])->subdiv_vert(diff);
         if (v)
            new_selected_verts += v;
      }

   } else { // edit level unrefined

      // get the parents of the old selected components at the new level

      Bface_list parent_faces;
      Bedge_list parent_edges;
      Bvert_list parent_verts;

      for (i=0; i<selected_faces.num(); ++i) {
         Lface* parent_f = ((Lface*)selected_faces[i])->parent(-diff);
         if (!parent_f) {
            err_adv(debug, "MeshGlobal::edit_level_changed: missing parent face");
            continue;
         }
         parent_faces.add_uniquely(parent_f);
      }

      for (j=0; j<selected_edges.num(); ++j) {
         Ledge* parent_e = ((Ledge*)selected_edges[j])->parent_edge(-diff);
         if (!parent_e)
            continue;
         parent_edges.add_uniquely(parent_e);
      }

      for (k=0; k<selected_verts.num(); ++k) {
         Lvert* parent_v = ((Lvert*)selected_verts[k])->parent_vert(-diff);
         if (!parent_v)
            continue;
         parent_verts.add_uniquely(parent_v);
      }

      // if all descendents of a parent component were selected at the old level, 
      // select the parent
      
      for (i=0; i<parent_faces.num(); ++i) {
         // get all the parent's children at the old edit level
         Bface_list child_faces; 
         ((Lface*)parent_faces[i])->append_subdiv_faces(-diff, child_faces);

         if (all_selected(child_faces)) {
            new_selected_faces += parent_faces[i];
         }
      }

      for (j=0; j<parent_edges.num(); ++j) {
         // get all the parent's children at the old edit level
         Bedge_list child_edges; 
         ((Ledge*)parent_edges[j])->append_subdiv_edges(-diff, child_edges);

         if (all_selected(child_edges)) {
            new_selected_edges += parent_edges[j];
         }
      }

      // this case is simpler: verts have 1 child vert
      new_selected_verts = parent_verts;
   }

   // deselect components for old level
   deselect(selected_faces);
   deselect(selected_edges);
   deselect(selected_verts);

   // select components for the new level
   select(new_selected_faces);
   select(new_selected_edges);
   select(new_selected_verts);
}

/* end of file mesh_global.C */
