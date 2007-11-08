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
 * subdiv_updater.H
 *****************************************************************/
#ifndef SUBDIV_UPDATER_H_IS_INCLUDED
#define SUBDIV_UPDATER_H_IS_INCLUDED

#include "mesh/lmesh.H"

#include "bbase.H"

/*****************************************************************
 * SubdivUpdater:
 *
 *    A Bnode that updates a given mesh region via subdivision
 *    from the parent level, plus pre-existing memes when
 *    found. Any Bnode that references a given mesh region can
 *    use a SubdivUpdater as its input. Internally, the
 *    SubdivUpdater sets up its own hierarchy of parent
 *    SubdivUpdaters if needed.
 *
 *    TODO: SubdivUpdater should record itself on vertices
 *    it controls. When a 2nd client wants a SubdivUpdater
 *    for the same region (or a subset), we can re-use the
 *    same SubdivUpdater by looking it up from the vertices.
 *
 *****************************************************************/
#define CSubdivUpdater const SubdivUpdater
class SubdivUpdater : public Bnode, public BMESHobs {
 public:

   //******** MANAGERS ********

   // Given an input region to keep updated,
   // create a SubdivUpdater to manage it: 
   static SubdivUpdater* create(CBface_list& faces);
   static SubdivUpdater* create(CBedge_list& edges);
   static SubdivUpdater* create(CBvert_list& verts);

 protected:
   // Protected constructor
   SubdivUpdater(LMESHptr m, CBvert_list& verts);
 public:

   virtual ~SubdivUpdater();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SubdivUpdater", SubdivUpdater*, Bnode, CBnode*);

   //******** MESH ACCESSORS ********

   // The mesh operated on by this SubdivUpdater:
   CLMESHptr& mesh() { return _mesh; }

   // The control mesh in charge of the subdivision hierarchy::
   LMESH* ctrl_mesh() const { return _mesh ? _mesh->control_mesh() : 0; }

   // The currently displayed mesh in the subdivision hierarchy:
   LMESH* cur_mesh() const { return _mesh ? _mesh->cur_mesh() : 0; }

   // The level of the currently displayed subdivision mesh:
   int cur_level() const { return _mesh ? _mesh->cur_level() : -1; }

   // Same as above, but relative to the level of this mesh:
   int rel_cur_level() const { return _mesh ? _mesh->rel_cur_level() : 0; }

   // The level of this mesh in the LMESH hierarchy:
   int subdiv_level() const { return _mesh ? _mesh->subdiv_level() : -1; }

   // Vertices of region being updated:
   CBvert_list& verts() const { return _verts; }

   // Faces/edges of region being updated (1 or both may be empty):
   CBface_list& faces() const { return _faces; }
   CBedge_list& edges() const { return _edges; }

   //******** Bnode METHODS ********

   virtual Bnode_list inputs() const { return _inputs; }

   virtual void recompute();

   // Identifier used in diagnostic output. SubdivUpdater types print their
   // class name and level in the subdivision hierarchy.
   virtual str_ptr identifier() const {
      return Bnode::identifier() + "_level_" + str_ptr(subdiv_level());
   }

   //******** BMESHobs METHODS ********

   // Report Bnode identifier as BMESHobs name:
   virtual str_ptr name() const { return identifier(); }
   
   //*****************************************************************
 protected: 
   LMESHptr             _mesh;    // mesh the SubdivUpdater operates on
   Bface_list           _faces;   // (optional) surface region we keep updated
   Bedge_list           _edges;   // (optional) mesh edges we keep updated
   Bvert_list           _verts;   // (optional) vertices we keep updated
   Bnode_list           _inputs;  // Bbases of active memes of our verts
   SubdivUpdater*       _parent;  // parent (coarser-level SubdivUpdater)

   // Helper method for updating a vert thru subdivision:
   void update_vert(Lvert* v);
};

#endif // SUBDIV_UPDATER_H_IS_INCLUDED

/* end of file subdiv_updater.H */
