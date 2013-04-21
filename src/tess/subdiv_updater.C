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
#include "subdiv_updater.H"

static bool debug = Config::get_var_bool("DEBUG_SUBDIV_UPDATER",false);

SubdivUpdater::SubdivUpdater(LMESHptr m, CBvert_list& verts) :
   _parent(0)
{
   // Screened in SubdivUpdater::create():
   assert(m && m == verts.mesh());

   _mesh = m;
   _verts = verts;

   // Get bbases, add to inputs
   _inputs = Bbase::find_owners(verts).bnodes();

   if (debug) {
      err_msg("SubdivUpdater::SubdivUpdater:");
      cerr << "  "; print_dependencies();
      err_msg("  level %d, %d verts, %d boss memes",
              m->subdiv_level(), verts.num(),
              Bbase::find_boss_vmemes(verts).num());
   }

   // If not covered, create parent
   if (!Bbase::is_covered(verts)) {
   
      // Get parent verts list
      Bvert_list parents = LMESH::get_subdiv_inputs(verts);

      // If non-empty create parent SubdivUpdater
      if (!parents.empty()) {

         // Create parent subdiv updater on parent verts
         // and add to _inputs
         _parent = create(parents);
         _inputs += _parent;
      }

   }

   hookup();
}

SubdivUpdater::~SubdivUpdater()
{
   destructor();
   delete _parent;      // we created it, we delete it
}

SubdivUpdater* 
SubdivUpdater::create(CBvert_list& verts)
{
   // Given an input region to keep updated,
   // create a SubdivUpdater to manage it: 

   LMESHptr m = LMESH::upcast(verts.mesh());
   if (m) {
      return new SubdivUpdater(m, verts);
   } else
      err_adv(debug, "SubdivUpdater::create: can't get LMESH from vert list");

   return 0;
}

SubdivUpdater* 
SubdivUpdater::create(CBedge_list& edges)
{
   SubdivUpdater* ret = create(edges.get_verts());

   if (ret)
      ret->_edges = edges;
   return ret;
}

SubdivUpdater* 
SubdivUpdater::create(CBface_list& faces)
{
   SubdivUpdater* ret = create(faces.get_verts());

   if (ret)
      ret->_faces = faces;
   return ret;
}

void
SubdivUpdater::update_vert(Lvert* v) 
{
   // Update the given vertex if needed.

   // If it has a boss meme, skip it (already updated).
   if (!v || Bbase::has_boss(v))
      return;

   // Find its subdivision parent
   Bsimplex* p = v->parent();
   if (!p)
      return;

   // Have the parent compute the subdiv loc of our vert:
   if (is_edge(p))
      ((Ledge*)p)->update_subdivision();
   else if (is_vert(p))
      ((Lvert*)p)->update_subdivision();
}

void 
SubdivUpdater::recompute() 
{
   for (int i=0; i<_verts.num(); i++)
      update_vert((Lvert*)_verts[i]);
}

/* end of file subdiv_updater.C */
