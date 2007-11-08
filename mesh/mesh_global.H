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

/**********************************************************************
 * mesh_global.H
 **********************************************************************/
#ifndef MESH_GLOBAL_H_IS_INCLUDED
#define MESH_GLOBAL_H_IS_INCLUDED

#include "bedge.H"
#include "bface.H"

/*****************************************************************
 * MeshGlobal:
 *
 *      Contains static methods relating to the mesh library.
 *
 *****************************************************************/

class MeshGlobal {
 public:

   //************ FACE SELECTION *******

   // adds the face (and its quad partner, if it has one) to the
   // selected list, setting the selected flags as needed.
   static void              select(Bface* f);

   // removes the face (and its quad partner, if it has one) from the
   // selected list, clearing the selected flags as needed.
   static void              deselect(Bface* f);

   // deselects the face if it is currently selected
   // and selects it otherwise
   static void              toggle_select(Bface* f);

   // selects the faces 
   static void              select(CBface_list& faces);

   // deselects the faces
   static void              deselect(CBface_list& faces);

   // removes all faces from the selected list, clearing 
   // the selected flags as needed
   static void              deselect_all_faces();

   // returns a "read-only" list of all currently selected faces
   static CBface_list&      selected_faces() { return _selected_faces; }

   // returns a list of all currently selected faces from the given mesh
   static Bface_list        selected_faces(BMESH* mesh);

   // returns a list of all currently selected faces from the given
   // mesh. if the given mesh is an LMESH*, also returns selected
   // faces from any other meshes in its subdivision hierarchy
   static Bface_list        selected_faces_all_levels(BMESH* mesh);

   // deselect all selected elements of the given mesh (at all levels):
   static void deselect_all(BMESH* mesh) {
      deselect(selected_faces_all_levels(mesh));
      deselect(selected_edges_all_levels(mesh));
      deselect(selected_verts_all_levels(mesh));
   }

   static void deselect_all(CBMESH_list& meshes) {
      for (int i=0; i<meshes.num(); i++)
         deselect_all(meshes[i]);
   }

   //************ EDGE SELECTION *******

   // adds the edge to the selected list, setting the selected flags
   // as needed.
   static void              select(Bedge* e);

   // removes the edge from the selected list, clearing the selected
   // flags as needed.
   static void              deselect(Bedge* e);

   // deselects the edge if it is currently selected
   // and selects it otherwise
   static void              toggle_select(Bedge* e);

   // selects the edges
   static void              select(CBedge_list& edges);

   // deselects the edges
   static void              deselect(CBedge_list& edges);

   // removes all edges from the selected list, clearing the selected
   // flags as needed
   static void              deselect_all_edges();

   // returns a "read-only" list of all currently selected edges
   static CBedge_list&      selected_edges() { return _selected_edges; }

   // returns a list of all currently selected edges from the given mesh
   static Bedge_list        selected_edges(BMESH* mesh);
  
   // returns a list of all currently selected edges from the given
   // mesh. if the given mesh is an LMESH*, also returns selected
   // edges from any other meshes in the subdivision hierarchy
   static Bedge_list        selected_edges_all_levels(BMESH* mesh);

   //************ VERT SELECTION *******

   // adds the vert to the selected list, setting the selected flags
   // as needed.
   static void              select(Bvert* v);

   // removes the vert from the selected list, clearing the selected
   // flags as needed.
   static void              deselect(Bvert* v);

   // deselects the vert if it is currently selected
   // and selects it otherwise
   static void              toggle_select(Bvert* v);

   // selects the verts
   static void              select(CBvert_list& verts);

   // deselects the verts
   static void              deselect(CBvert_list& verts);

   // removes all verts from the selected list, clearing the selected
   // flags as needed
   static void              deselect_all_verts();

   // returns a "read-only" list of all currently selected verts
   static CBvert_list&      selected_verts() { return _selected_verts; }

   // returns a list of all currently selected verts from the given mesh
   static Bvert_list        selected_verts(BMESH* mesh);
  
   // returns a list of all currently selected verts from the given
   // mesh. if the given mesh is an LMESH*, also returns selected
   // verts from any other meshes in the subdivision hierarchy
   static Bvert_list        selected_verts_all_levels(BMESH* mesh);

   //************ SELECTION UPDATE *******

   // updates the list of currently selected components of the given
   // mesh to reflect a change in edit level. 'from' is the former
   // edit level, 'to' is the current edit level.
   static void              edit_level_changed(BMESH* mesh, int from, int to);

   //******** DESELECT ALL ********
   static void deselect_all() {
      deselect_all_verts();
      deselect_all_edges();
      deselect_all_faces();
   }

 protected:

   //******** STATIC DATA ********
   static Bface_list _selected_faces;
   static Bedge_list _selected_edges;
   static Bvert_list _selected_verts;
};


#endif  // MESH_GLOBAL_IS_INCLUDED

/* end of file mesh_global.H */
