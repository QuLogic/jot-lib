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
 * skin.C:
 **********************************************************************/
#include "disp/colors.H"
#include "geom/world.H"
#include "gtex/util.H"
#include "std/config.H"

#include "tess/mesh_op.H"
#include "tess/skin.H"
#include "tess/skin_meme.H"
#include "tess/subdiv_updater.H"
#include "tess/mesh_op.H"

using namespace mlib;

static bool debug = Config::get_var_bool("DEBUG_SKIN",false);

static int skin_num = 0; // for naming

Skin* Skin::_debug_instance = 0;

/*****************************************************************
 * InflateCreaseFilter:
 *****************************************************************/
bool
InflateCreaseFilter::accept(CBsimplex* s) const 
{
   // Reject non-edges:
   if (!is_edge(s))
      return false;
   Bedge* e = (Bedge*)s;

   if (e->nfaces() < 2)
      return false;

   // Accept it if it's labelled a crease, is owned by a
   // Bcurve, or the adjacent faces make a sharp angle:
   return (
      e->is_crease()                 ||
      Bcurve::find_controller(e)     ||
      rad2deg(norm_angle(e)) > 50
      );
}

/*****************************************************************
 * Utilities
 *****************************************************************/

inline VertMapper
subdiv_mapper(CVertMapper& pmap)
{
   Bedge_list a_edges = pmap.a_edges();
   Bedge_list b_edges = pmap.a_to_b(a_edges);
   assert(a_edges.num() == b_edges.num());

   if (0 && debug) {
      err_msg("parents: verts: %d --> %d, edges: %d --> %d",
              pmap.A().num(), pmap.B().num(), a_edges.num(), b_edges.num());
      err_msg("children: verts: %d --> %d",
              (child_verts<Bvert_list,Lvert>(pmap.A()) +
               child_verts<Bedge_list,Ledge>(a_edges)).num(),
              (child_verts<Bvert_list,Lvert>(pmap.B()) +
               child_verts<Bedge_list,Ledge>(b_edges)).num());

      Bvert_list c = (child_verts<Bvert_list,Lvert>(pmap.A()) +
                      child_verts<Bedge_list,Ledge>(a_edges));
      if (c.has_duplicates()) {
         err_msg("*** child verts have duplicates ***");
         if (pmap.A().has_duplicates()) {
            err_msg("  A verts have duplicates");
         }
         if (pmap.a_edges().has_duplicates()) {
            err_msg("  A edges have duplicates");
         }
         if (child_verts<Bvert_list,Lvert>(pmap.A()).has_duplicates()) {
            err_msg("  vert children have duplicates");
         }
         if (child_verts<Bedge_list,Ledge>(pmap.a_edges()).has_duplicates()) {
            err_msg("  edge children have duplicates");
         }
         WORLD::show_pts(c.pts());
      }
   }

   return VertMapper(
      child_verts<Bvert_list,Lvert>(pmap.A()) +
      child_verts<Bedge_list,Ledge>(a_edges),
      child_verts<Bvert_list,Lvert>(pmap.B()) +
      child_verts<Bedge_list,Ledge>(b_edges)
      );
}

static int
max_subdiv_edit_level(Bface_list all, Bface_list core=Bface_list())
{
   // what is the deepest subdiv level that is either:
   //   - controlled by a Bbase
   //   - has holes cut
   //   - has new surface regions attached

   err_adv(debug, "max_subdiv_edit_level:");
   if (all.empty()) {
      assert(core.empty());
      err_adv(debug, "  face list is empty");
      return 0;
   }
   assert(all.mesh() && all.contains_all(core));

   int R = 0;   // level at which deepest edits happened
   int k = 0;   // current level being examined
   while (!all.empty()) {
      k++;
      all = get_subdiv_faces(all,1);
      if (all.empty())
         break;

      // check for controllers
      if (!Bbase::find_owners(all).empty()) {
         err_adv(debug, "  controllers at level %d", k);
         R = k;
      }

      // check for holes at this level:
      if (all.has_any_secondary()) {
         all = all.primary_faces();
         err_adv(debug, "  hole at level %d", k);
         R = k; // holes exist at this level
      }

      // check for new regions
      if (core.empty())
         continue;
      core = get_subdiv_faces(core,1);
      Bface_list new_faces = core.two_ring_faces().primary_faces();
      // see if any of these new ones are actually new:
      new_faces.set_flags(1);
      all.clear_flags();
      new_faces = new_faces.filter(SimplexFlagFilter(1));
      if (!new_faces.empty()) {
         all.append(new_faces);
         err_adv(debug, "  new faces at level %d", k);
         R = k;  // new stuff exists at this level
      }
   }
   return R;
}

inline bool
is_bad(Bface* f)
{
   return (f && f->is_quad() && !f->quad_partner());
}

inline bool
check(CBface_list& faces)
{
   Bface_list bad;
   for (int i=0; i<faces.num(); i++)
      if (is_bad(faces[i]))
         bad += faces[i];
   if (bad.empty())
      return true;

   GtexUtil::show_tris(bad);
   return false;
}

/************************************************************
 * Skin:
 ************************************************************/
Skin*
Skin::create_cover_skin(
   Bface_list region,
   MULTI_CMDptr cmd)
{
   debug = false;
   err_adv(debug, "Skin::create_cover_skin");

   assert(cmd != 0);

   BMESH* b_mesh = region.mesh();
   VertMapper b_map(true);

   if (!b_map.is_valid()) {
      err_msg("Skin::create_cover_skin: invalid skel map");
      return 0;
   }

   // need an LMESH that contains all faces
   LMESH* mesh = LMESH::upcast(b_mesh);

   Skin* cur = new Skin(mesh, region, b_map, 0, false, "cover", cmd);
   cur->set_all_sticky();
   cur->freeze(cur->skin_faces().get_boundary().edges().get_verts());

   err_adv(debug, "  joining to skel...");
   if (cur->join_to_skel(Bface_list(), cmd)) {
      err_adv(debug, "  ...succeeded");
   } else {
      err_adv(debug, "  ...failed");
   }

   debug = false;
   _debug_instance = cur;
   Skin* ret = upcast(cur->control());

   for (int i = 0; i < Bsurface::get_surfaces(region).num(); i++) {
      if (Skin::isa(Bsurface::get_surfaces(region)[i]))
         ((Skin*)Bsurface::get_surfaces(region)[i])->_covers += ret;
   }
   // XXX - only work for faces(region) on the same skin
   if (region.num()>0 && Skin::find_controller(region[0])) {
      Skin::find_controller(region[0])->_covers += ret;
   }

   return ret;
}

Skin* 
Skin::create_multi_sleeve(
   Bface_list interior,         // interior skeleton surface
   VertMapper skel_map,         // tells how some skel verts are identified
   MULTI_CMDptr cmd)
{
   err_adv(debug, "Skin::create_multi_sleeve");

   //assert(!has_secondary_any_level(interior));
   assert(cmd != 0);

   if (!skel_map.is_valid()) {
      err_msg("Skin::create_multi_sleeve: invalid skel map");
      return 0;
   }

   Bface_list exterior       = interior.exterior_faces();
   Bface_list all_skel_faces = interior + exterior;

   // need an LMESH that contains all faces
   LMESH* mesh = LMESH::upcast(all_skel_faces.mesh());
   if (!mesh) {
      err_adv(debug, "  bad skel mesh");
      return 0;
   }

   // Get relative refinement level
   int R = max_subdiv_edit_level(all_skel_faces, interior);

   err_adv(debug, "  skel region res level: %d", R);

   LMESH* ctrl = mesh->control_mesh();
   int old_lev = ctrl->cur_level();
   int ref_lev = mesh->subdiv_level() + R;
   ctrl->update_subdivision(ref_lev);
   if (ref_lev < old_lev)
      ctrl->update_subdivision(old_lev);

   Skin* cur = new Skin(mesh, exterior, skel_map, R, false, "sleeve", cmd);
   cur->set_all_sticky();
   cur->freeze(cur->skin_faces().get_boundary().edges().get_verts());
   for (int k=1; k<=R; k++) {
      
      err_adv(debug, "  top of loop: k = %d", k);

      // XXX - temporary
      if (!check(interior.mesh()->faces()))
         return cur;

      // Update the skeleton to the next level
      LMESH::update_subdivision(interior + exterior, 1);

      // Map skel faces to next level. Avoid new holes.
      // Take account of new regions that don't exist at
      // previous level.

      err_adv(debug, "  getting interior subdiv faces");
      int n = interior.num();
      interior = get_subdiv_faces(interior, 1); // no holes can form here
      if (4*n != interior.num()) {
         err_adv(debug, "  *** ERROR *** got %d interior subdiv faces from %d",
                 interior.num(), n);
      }

      err_adv(debug, "  getting skel subdiv faces");
      n = cur->skel_faces().num();
      exterior = get_subdiv_faces(cur->skel_faces(), 1);
      if (4*n != exterior.num()) {
         err_adv(debug, "  *** ERROR *** got %d exterior subdiv faces from %d",
                 exterior.num(), n);
      }

      // check for holes:
      if (exterior.has_any_secondary()) {
         err_adv(debug, "  removing secondary skel faces at level %d", k);
         exterior = exterior.primary_faces(); // remove holes
      }

      // check for new regions:
      err_adv(debug, "  checking for new faces");
      Bface_list new_faces = interior.two_ring_faces().minus(interior + exterior);
      assert(new_faces.is_all_primary()); // must be true (we think)
      if (!new_faces.empty()) {
         err_adv(debug, "  adding %d new faces at level %d",
                 new_faces.num(), k);
         exterior.append(new_faces);
      }

      // get version of skel_map at new level:
      cur = new Skin(cur, exterior, cmd);

      // make the ones on or near the boundary "sticky"
      cur->set_all_sticky(false);
      cur->set_sticky(
         cur->skin_faces().
         get_boundary().
         edges().
         get_verts().
         one_ring_faces().
         get_verts()
         );
      cur->freeze(cur->skin_faces().get_boundary().edges().get_verts());

      // put memes in place, not just use position they inherited
      // from subdivision
      cur->do_update();
   }

   // Join skin to skel at level R
   err_adv(debug, "  joining to skel...");
   if (cur->join_to_skel(interior, cmd)) {
      err_adv(debug, "  ...succeeded");
   } else {
      err_adv(debug, "  ...failed");
   }

   _debug_instance = cur;
   return upcast(cur->control());
}

inline void
report(CVertMapper& mapper, Cstr_ptr& msg)
{
   err_msg("  %s: %d verts to %d verts, %d edges to %d edges",
           **msg,
           mapper.A().num(),
           mapper.B().num(),
           mapper.a_edges().num(),
           mapper.a_to_b(mapper.a_edges()).num());
}

Skin::Skin(
   LMESHptr mesh,
   CBface_list& skel_faces,
   CVertMapper& skel_map,
   int R,
   bool reverse,
   Cstr_ptr& name,
   MULTI_CMDptr cmd
   ) :
   _skel_faces(skel_faces),
   _updater(0),
   _partner(0),
   _reverse(reverse)
{
   _inflate = false;
   err_adv(debug, "Skin::Skin: (control)");

   if (0 && debug) {
      report(skel_map, "skel map");
   }

   assert(mesh != 0);
   assert(R >= 0);

   set_mesh(mesh);
   set_name(name + str_ptr(++skin_num));

   if (!gen_faces(skel_map)) {
      err_adv(debug, "  can't generate skin over skel faces");
      return;
   }

   // not trusing Bbase::set_res_level...
   _res_level = R;

   // finish up: create child or join to skeleton:
   finish_ctor(cmd);
}

Skin::Skin(
   Skin* parent,
   CBface_list& skel_faces,
   MULTI_CMDptr cmd) :
   _skel_faces(skel_faces),
   _updater(0),
   _partner(0),
   _reverse(false)
{
   _inflate = parent->_inflate;
   err_adv(debug, "Skin::Skin: (child)");

   assert(parent && parent->_res_level > 0);

   set_name(parent->name());

   _reverse = parent->_reverse;

//   assert(!_skel_faces.has_any_secondary());

   // Update parent mesh elements to our level.
   // This also ensures the child patch is created and filled.
   LMESH::update_subdivision(parent->skin_faces(), 1);
   err_adv(debug, "  updated subdivision at parent");

   // Bsurface method: record parent/child relationship mutually.
   //   set _parent   = parent and parent->_child = this.
   //   set Patch     = child of parent's Patch.
   //   set mesh      = child of parent's mesh.
   //   set res level = parent res level - 1.
   set_parent(parent);
   err_adv(debug, "  set parent");

   // Set up our vert mapper, based on the parent:
   _mapper = subdiv_mapper(parent->_mapper);
   if (_mapper.is_valid())
      err_adv(debug, "  set mapper");
   else
      err_adv(debug, "  failed to get subdiv mapper");

   // Generate verts, edges, faces of the new skin.
   // But if any already exist, it does not duplicate them.
   // Also add memes as needed.
   gen_faces(); 
   err_adv(debug, "  generated elements");

   Bface_list extras = skin_faces().minus(_mapper.a_to_b(_skel_faces));
   if (debug && !extras.empty()) {
      cerr << "found "
           << extras.num()
           << " unexpected faces" << endl;
      push(extras, cmd);
   }
   // finish up: create child or join to skeleton:
   finish_ctor(cmd);
}

void
Skin::finish_ctor(MULTI_CMDptr cmd)
{
   if (_skel_faces.has_any_secondary()) {
      if (debug) {
         cerr << "Skin::finish_ctor: pushing "
              << _mapper.a_to_b(_skel_faces.secondary_faces()).num()
              << " skin faces from "
              << _skel_faces.secondary_faces().num()
              << " skel faces"
              << endl;
      }
      push(_mapper.a_to_b(_skel_faces.secondary_faces()), cmd);
   }

   // Ensure we get recomputed
   invalidate();

   // XXX - move?
   create_subdiv_updater();

//   cmd->add(new SHOW_BBASE_CMD(this));
   show_surface(this,cmd); 

   if (debug)
      print_all_inputs();
}

Skin::~Skin()
{
   destructor();

   delete _updater;
   _updater=0;
}

Bnode_list 
Skin::inputs() const
{       
   return Bnode_list(_updater);
}

int 
Skin::draw(CVIEWptr& v)
{
   // we almost put something here once...

   return Bsurface::draw(v);
}

/*
void
Skin::debug_draw_memes(
   CCOLOR& sticky_color,
   CCOLOR& loose_color,
   CCOLOR&,
   double  sticky_size,
   double  loose_size,
   double
   )
{
   // Debug stuff --
   //    show where the memes are and what state they're in

   if (!_show_memes)
      return;
   Bbase* cur = cur_subdiv_bbase();
   if (!cur)
      return;

   const VertMemeList& vam = cur->vmemes();
   // Enable point smoothing and push gl state:
   GL_VIEW::init_point_smooth((float)sticky_size, GL_CURRENT_BIT);
   glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
   glBegin(GL_POINTS);
   for (int i=0; i<vam.num(); i++) {
      COLOR c = loose_color;
      SkinMeme* sm = SkinMeme::upcast(vam[i]);
      assert(sm);
      if (sm->is_sticky())
         c = sticky_color;
      if (sm->is_multi_tracking())
         c = interp(c, COLOR::black, 0.4);
      else if (!sm->is_tracking())
         c = interp(c, COLOR::white, 0.4);
      GL_COL(c, 1);                     // GL_CURRENT_BIT
      glVertex3dv(vam[i]->loc().data());
   }
   glEnd();
   GL_VIEW::end_point_smooth();         // pop state

   return;
}
*/

void 
Skin::create_subdiv_updater()
{
   // XXX - needs work to be efficient in hierarchy

   err_adv(0 && debug, "Skin::create_subdiv_updater at level %d", subdiv_level());

   SubdivUpdater* su = SubdivUpdater::create(skel_faces());
   if (su->inputs().empty()) {
      // This will happen if the disk is at the control level and is
      // not procedurally controlled at all. Then it should never
      // change and we have no input. We are assuming changes to the
      // mesh only happen via Bnodes (Bsurfaces etc.).
      err_adv(debug, "%s: not using SubdivUpdater", **identifier());
      delete su;
      su = 0;
   } else {
      _updater = su;
      su->set_name(str_ptr("su-") + name());
      hookup();
   }
}

Lface*
Skin::gen_face(Bface* f)
{
   if (!f) {
      err_adv(debug, "Skin::gen_face: null face");
      return 0;
   }
   Bvert* v1 = _mapper.a_to_b(f->v1());
   Bvert* v2 = _mapper.a_to_b(f->v2());
   Bvert* v3 = _mapper.a_to_b(f->v3());

   FaceMeme* fm = 0;
   UVpt a, b, c;

   // if the new face exists, the following does not re-create it:
   if (UVdata::get_uvs(f, a, b, c)) {
      if (_reverse)
         fm = add_face(v1, v3, v2, a, c, b);
      else
         fm = add_face(v1, v2, v3, a, b, c);
   } else {
      if (_reverse)
         fm = add_face(v1, v3, v2);
      else
         fm = add_face(v1, v2, v3);
   }
   return fm ? fm->face() : 0;
}

bool
Skin::gen_faces(const VertMapper& skel_mapper)
{
   // replicate skel faces to create new skin faces.
   // skel_mapper tells how some skel vertices are identified
   // (to each other).

   Bvert_list skel_verts = _skel_faces.get_verts();
   if (!gen_verts(skel_verts, skel_mapper)) {
      err_adv(debug, "Skin::gen_faces: error: gen verts failed");
      return false;
   }
   if (!_mapper.is_valid()) {
      err_adv(debug, "Skin::gen_faces: error: invalid mapper");
      return false;
   }

   for (int i=0; i<_skel_faces.num(); i++)
      gen_face(_skel_faces[i]);

   copy_edges(_skel_faces.get_edges());

   mesh()->changed();

   return true;
}

Lvert*
Skin::gen_vert(Bvert* skel_vert)
{
   // Generate a skin vertex corresponding to 
   // the given skeleton vertex v. but don't do it
   // if it has been done already.

   Lvert* skin_vert = (Lvert*)_mapper.a_to_b(skel_vert);
   if (!skin_vert) {
      skin_vert = (Lvert*)_mesh->add_vertex(skel_vert->loc());
      _mapper.add(skel_vert, skin_vert);
   }
   SkinMeme* m = SkinMeme::upcast(find_meme(skin_vert));
   if (m) {
      m->add_track_simplex(skel_vert);
   } else {
      new SkinMeme(this, skin_vert, skel_vert);
   }
   return skin_vert;
}

bool
Skin::gen_verts(CBvert_list& skel_verts)
{
   // replicate skel verts to create new skin verts.
   // ignores verts that have already been done.
   bool ret = true;
   for (int i=0; i<skel_verts.num(); i++)
      if (!gen_vert(skel_verts[i]))
         ret = false;
   return ret;
}

bool
Skin::gen_verts(CBvert_list& skel_verts,
                CVertMapper& skel_mapper)
{
   // like plain gen_verts(), but take into account skel verts that
   // are "identified" together. i.e. if s1 and s2 are skel verts that
   // are identified together, we create just a single skin vert
   // corresponding to both s1 and s2.

   // first generate skin verts for the skel verts that are
   // mapped *to* by other skel verts
   if (!gen_verts(skel_mapper.B())) {
      err_adv(debug, "Skin::gen_verts: can't replicate skel verts (pass 1)");
      return false;
   }

   // now create a skel-to-skin association of each mapped-from
   // skel vert thru its skel counterpart to the skin vert
   if (!_mapper.is_valid())
      err_msg("  invalid mapper before gen_verts");
   _mapper.add(skel_mapper.A(), _mapper.a_to_b(skel_mapper.B()));
   if (!_mapper.is_valid())
      err_msg("  invalid mapper after gen_verts");

   // now do the rest to make sure all are covered
   if (!gen_verts(skel_verts)) {
      err_adv(debug, "Skin::gen_verts: can't replicate skel verts (pass 2)");
      return false;
   }
   return true;
}

bool 
Skin::copy_edge(Bedge* a) const
{
   // copy edge attributes, e.g. from skel to skin

   Bedge* b = _mapper.a_to_b(a);
   if (!(a && b))
      return false;

   if (a->is_weak())
      b->set_bit(Bedge::WEAK_BIT);

   // more?

   return true;
}

bool 
Skin::copy_edges(CBedge_list& edges) const
{
   bool ret = true;
   for (int i=0; i<edges.num(); i++)
      if (!copy_edge(edges[i]))
         ret = false;
   return ret;
}

inline void
show_polys(BMESH* m)
{
   // for debugging: show polyline edges of a mesh

   if (!m) return;
   // Construct filter that accepts unreached polyline edges 
   UnreachedSimplexFilter    unreached;
   PolylineEdgeFilter        poly;

   EdgeStrip strip(m->edges(), unreached + poly);
   ARRAY<Bvert_list> chains;
   strip.get_chains(chains);
   for (int i=0; i<chains.num(); i++)
      WORLD::show_polyline(chains[i].pts(), 3, Color::blue_pencil_d, 0.5);
}

inline bool
join(CBvert_list& o, CBvert_list& c, MULTI_CMDptr& cmd, Cstr_ptr& msg)
{
   // Used in Skin::join_to_skel() to join seams of a mesh together.

   JOIN_SEAM_CMDptr join = new JOIN_SEAM_CMD(o, c);
   if (join->doit()) {
      err_adv(debug, "  joined %s (%d verts to %d verts)",
              **msg, o.num(), c.num());
      cmd->add(join);
      return true;
   } else {
      err_adv(debug, "  error: can't join %s", **msg);
      return false;
   }
}

bool
Skin::join_to_skel(CBface_list& interior, MULTI_CMDptr cmd)
{
   // Join external seams to connect skin to the base surfaces

   assert(cmd != 0);

   err_adv(debug, "Skin::join_to_skel");

   assert(interior.empty() || _skel_faces.mesh() == interior.mesh());
   Bface_list skel_region = _skel_faces + interior;

   // Ensure vert mapper is ready:
   if (!_mapper.is_valid()) {
      err_adv(debug, "  error: invalid vert mapper");
      return false;
   }
   // Do correction at boundary to allow joining
   // to underlying skeleton surfaces:
   if (!do_boundary_correction()) {
      err_adv(debug, "  error: boundary correction failed");
      return false;
   }

   // Find external boundaries in separate chains
   // (each is its own connected component):
   ARRAY<Bvert_list> skel_chains;
   skel_region.get_boundary().get_chains(skel_chains);
   if (skel_chains.empty()) {
      err_adv(debug, "  error: can't find seams to join");
      return false;
   }

   push(skel_region, cmd);

   for (int i=0; i<skel_chains.num(); i++) {
      // chains repeat 1st vertex at end, so remove it
      skel_chains[i].pop();
      // get matching skin chain
      Bvert_list skin_chain = _mapper.a_to_b(skel_chains[i]);
      // put skin chain first; it is the "open" chain, meaning
      // its vertices will be kicked out, replaced by skel verts
      if (!join(skin_chain, skel_chains[i], cmd, "skin to skel")) {
         return false;
      }
   }

   // "show" the surface. does nothing the first time through,
   // but need this for proper undo. i.e., undoing everything in 
   // reverse order, first we remove the skin, then restore the 
   // underlying skeleton surfaces.
   show_surface(this, cmd);

   mesh()->changed();

   return true;
}

inline uint
count_flag(CBsimplex* s)
{
   return (s && s->flag()) ? 1 : 0;
}

inline uint
num_edge_flags_set(CBface* f)
{
   if (!f) return 0;
   return count_flag(f->e1()) + count_flag(f->e2()) + count_flag(f->e3());
}

inline Bedge*
boundary_connector(CBface* f)
{
   if (num_edge_flags_set(f) != 2)
      return 0;
   if (!f->e1()->flag()) return f->e1();
   if (!f->e2()->flag()) return f->e2();
   if (!f->e3()->flag()) return f->e3();
   return 0;
}

bool
Skin::correct_face(Bface* f, bool& changed)
{
   //                                 
   //  BBBBBBBBBBBBBBB                
   //  B            /|                
   //  B          /  |                
   //  B   f    /    |   Change this, where quad face f        
   //  B      /      |   is adjacent to 2 boundary edgees...
   //  B    /        |                
   //  B  /          |                
   //  B/- - - - - - o                
   //                                 
   //  BBBBBBBBBBBBBBB                
   //  B\            |                
   //  B  \          |                
   //  B    \        |  ... to this, where neither face of
   //  B      \      |  the quad is adjacent to more than 1              
   //  B        \    |  boundary edge.              
   //  B          \  |                
   //  B - - - - - - o                
   //                                 

   assert(f);

   // boundary edges have flag == 1, others have flag == 0
   uint n = num_edge_flags_set(f);
   if (n < 2) return true;      // not a problem
   if (n > 2) {
      // unfixable problem
      err_adv(debug, "  can't fix face with %d boundary edges", n);
      return false;
   }

   // 2 boundary edges; get the non-boundary one:
   Bedge* e = boundary_connector(f);
   assert(e);

   // we want to swap it; only possible if it has 2 faces:
   if (e->nfaces() != 2) {
      err_adv(debug, "  can't fix edge with %d faces", e->nfaces());
      return false;
   }

   // swapping won't do any good if its other face has boundary edges too:
   if (!(num_edge_flags_set(e->f1()) == 0 || num_edge_flags_set(e->f2()) == 0)) {
      err_adv(debug, "  unfixable edge found, giving up");
      return false;
   }

   // try to swap it:
   if (e->do_swap()) {
      err_adv(debug, "  swapped edge");
      return changed = true;
   }
   err_adv(debug, "  edge swap failed");
   return false;
}

bool 
Skin::do_boundary_correction()
{
   // Find faces that have 3 boundary vertices and try to
   // swap an edge to avoid that. Otherwise joining to the
   // underlying skeleton surface will fail, since the skin
   // face will duplicate an existing skeleton face.

   CBface_list& skin_faces = bfaces();
   if (skin_faces.empty()) {
      err_adv(debug, "Skin::correct_faces: skin face list is empty");
      return true;
   }

   if (debug) {
      cerr << "Skin::do_boundary_correction: "
           << skin_faces.get_boundary().num_line_strips()
           << " pieces in boundary"
           << endl;
   }

   // clear flags of skin edges, but set boundary edge flags
   Bedge_list boundary = skin_faces.boundary_edges();
   skin_faces.get_edges().clear_flags();
   boundary.set_flags(1);

   bool     ret = true;
   bool changed = false;
   for (int i=0; i<skin_faces.num(); i++)
      if (!correct_face(skin_faces[i], changed))
         ret = false;

   if (changed)
      _mesh->changed(BMESH::TRIANGULATION_CHANGED);

   return ret;
}

inline int
set_sticky(SkinMeme* m, bool s)
{
   if (!m)
      return 0;
   m->set_sticky(s);
   return 1;
}

int
Skin::set_sticky(CBvert_list& verts, bool sticky) const
{
   int ret = 0;
   for (int i=0; i<verts.num(); i++) {
      ret += ::set_sticky(SkinMeme::upcast(find_meme(verts[i])), sticky);
   }
   return ret;
}

int 
Skin::set_all_sticky(bool sticky) const 
{
   Bvert_list verts = skin_verts();
   int n = set_sticky(verts, sticky);
   return n;
}

inline int
set_offset(SkinMeme* m, double h)
{
   if (!m)
      return 0;
   m->set_offset(h);
   return 1;
}

int
Skin::set_offsets(CBvert_list& verts, double h) const
{
   int ret = 0;
   for (int i=0; i<verts.num(); i++) {
      ret += ::set_offset(SkinMeme::upcast(find_meme(verts[i])), h);
   }
   return ret;
}

int 
Skin::set_all_offsets(double h) const 
{
   Bvert_list verts = skin_verts();
   int n = set_offsets(verts, h);
   return n;
}

inline void
freeze(SkinMeme* m)
{
   if (m)
      m->freeze();
}

void
Skin::freeze(CBvert_list& verts) const
{
   for (int i=0; i<verts.num(); i++)
      ::freeze(SkinMeme::upcast(find_meme(verts[i])));
}

void
Skin::restrict(CBvert_list& verts, SimplexFilter* f) const
{
   assert(f);
   for (int i=0; i<verts.num(); i++) {
      SkinMeme* m = SkinMeme::upcast(find_meme(verts[i]));
      if (m) {
         //assert(m->track_simplex() == 0 || f->accept(m->track_simplex()));
         m->set_track_filter(f);
      }
   }
}

void
Skin::set_non_penetrate(CBvert_list& verts, bool b) const
{
   for (int i=0; i<verts.num(); i++) {
      SkinMeme* m = SkinMeme::upcast(find_meme(verts[i]));
      if (m) {
         m->set_non_penetrate(b);
      }
   }
}

void
Skin::set_stay_outside(CBvert_list& verts, bool b) const
{
   for (int i=0; i<verts.num(); i++) {
      SkinMeme* m = SkinMeme::upcast(find_meme(verts[i]));
      if (m) {
         m->set_stay_outside(b);
      }
   }
}

inline void
scale_offset(SkinMeme* m, double s)
{
   if (m)
      m->scale_offset(s);
}

inline ARRAY<double>
get_offset_scales(CBvert_list& skels, CSimplexFilter& filter)
{
   ARRAY<double> ret(skels.num());
   for (int i=0; i<skels.num(); i++)
      ret += offset_scale(skels[i], filter);
   return ret;
}

void 
Skin::adjust_crease_offsets() const
{
   Bvert_list crease_skel_verts =
      skel_edges().filter(ProblemEdgeFilter()).get_verts();
   ARRAY<double> scales = get_offset_scales(
      crease_skel_verts, ProblemEdgeFilter()
      );
   Bvert_list crease_skin_verts = _mapper.a_to_b(crease_skel_verts);
   for (int i=0; i<crease_skin_verts.num(); i++)
      scale_offset(SkinMeme::upcast(find_meme(crease_skin_verts[i])), scales[i]);
}

void
Skin::freeze_problem_verts(bool mode) const
{
   Bvert_list crease_skel_verts =
      skel_edges().filter(ProblemEdgeFilter()).get_verts();
   Bvert_list corner_skel_verts = crease_skel_verts.filter(
      VertDegreeFilter(0, StrongNPrEdgeFilter())
      );
   crease_skel_verts = crease_skel_verts.minus(corner_skel_verts);
   Bvert_list crease_skin_verts = _mapper.a_to_b(crease_skel_verts);
   Bvert_list corner_skin_verts = _mapper.a_to_b(corner_skel_verts);

   static RestrictTrackerFilter rtf;
   restrict(crease_skin_verts, &rtf);
   freeze  (corner_skin_verts);
   if(mode) freeze(crease_skin_verts);
}

inline void
track_deeper(SkinMeme* m, int R)
{
   if (!m)
      return;
   if (R < 1)
      return;
   Bsimplex* s = m->track_simplex();
   if (!is_vert(s)) {
      err_msg("track_deeper: non vert");
      return;
   }
   Lvert* v = ((Lvert*)s)->subdiv_vert(R);
   if (!v) {
      err_msg("track_deeper: failed to get subdiv vert");
   }
   // set it anyway
   m->set_track_simplex(v);
}

void
Skin::track_deeper(CBvert_list& verts, int R) const
{
   for (int i=0; i<verts.num(); i++)
      ::track_deeper(SkinMeme::upcast(find_meme(verts[i])), R);
}

inline void
compute_smoothing(CVertMemeList& memes)
{
   for (int i=0; i<memes.num(); i++)
      memes[i]->compute_delt();
}

inline bool
apply_smoothing(CVertMemeList& memes)
{
   bool ret = false;
   for (int i=0; i<memes.num(); i++) {
      if (memes[i]->apply_delt())
         ret = true;
   }
   return ret;
}

inline void
do_smoothing(CVertMemeList& memes, int iters, BMESH* mesh)
{
   assert(mesh);
   for (int i=0; i<iters; i++) {
      compute_smoothing(memes);
      if (!apply_smoothing(memes)) {
         err_adv(debug, "do_smoothing: finished %d/%d iterations", i, iters);
         return;
      }
      mesh->changed(BMESH::VERT_POSITIONS_CHANGED);
   }
   err_adv(debug, "do_smoothing: finished %d iterations", iters);
}

void 
Skin::debug_smoothing(int iters)
{
   if (!_debug_instance) {
      err_msg("Skin::debug_smoothing: no instance");
      return;
   }

   // go coarse to fine:
   for (Skin* skin=_debug_instance->control(); skin; skin=skin->child()) {
      Bvert_list skin_verts = skin->skin_verts();
      if (skin->_partner)
         skin_verts.append(skin->_partner->skin_verts());
      ::do_smoothing(find_boss_vmemes(skin_verts), iters, skin->mesh());
   }
   return;

   while (_debug_instance->rel_cur_level() > 0 &&
          upcast(_debug_instance->child())) {
      _debug_instance = upcast(_debug_instance->child());
   }
   while (_debug_instance->rel_cur_level() < 0 &&
          upcast(_debug_instance->parent())) {
      _debug_instance = upcast(_debug_instance->parent());
   }
   if (_debug_instance->rel_cur_level() != 0) {
      err_msg("Skin::debug_smoothing: no skin at cur level %d",
              _debug_instance->cur_level());
      return;
   }
   err_msg("Skin::debug_smoothing: starting smoothing at level %d",
           _debug_instance->subdiv_level());

   Bvert_list skin_verts = _debug_instance->skin_verts();
   if (_debug_instance->_partner)
      skin_verts.append(_debug_instance->_partner->skin_verts());

   ::do_smoothing(find_boss_vmemes(skin_verts), iters,_debug_instance->mesh());

   _debug_instance->mesh()->changed(BMESH::VERT_POSITIONS_CHANGED);
}

static void
push_all_levels(CBface_list& faces, MULTI_CMDptr cmd, bool do_boundary=true)
{
   // XXX - should move somewhere standard

   // push all the parents and children of the given set of faces.
   // first do parent level; then if there is anything left to do
   // at this level, do that.
   if (faces.empty())
      return;
   push_all_levels(get_parent_faces(faces, 1), cmd, do_boundary);
   push(faces.primary_faces(), cmd, do_boundary);
   assert(faces.is_all_secondary());
}

/*****************************************************************
 * inflate
 *****************************************************************/
Skin* 
Skin::create_inflate(
   CBface_list& skel,
   double h,
   int R,
   bool reverse,
   bool mode,
   MULTI_CMDptr cmd
   )
{   
   err_adv(debug, "Skin::create_inflate: R = %d", R);

//   assert(!has_secondary_any_level(skel));
   assert(cmd != 0);

   // need an LMESH that contains all faces
   LMESH* mesh = LMESH::upcast(skel.mesh());
   if (!mesh) {
      err_adv(debug, "  bad skel mesh");
      return 0;
   }

   Skin* ret = 0;

   Bface_list parent_faces = get_parent_faces(skel, 1);
   if (!parent_faces.empty() && !mode) {
      Skin* parent = create_inflate(parent_faces, h, R+1, reverse, mode, cmd);
      if (!parent) {
         err_adv(debug, "  create parent failed");
         return 0;
      }
      ret = new Skin(parent, skel, cmd);
   } else {
      ret = new Skin(mesh, skel, VertMapper(), R, reverse, "inflate", cmd);
   }
   if (!ret) {
      err_adv(debug, "  can't allocate Skin");
      return 0;
   }

   if (debug) {
      Bedge_list bad_edges =
         ret->skin_faces().get_edges().filter(!ConsistentEdgeFilter());
      if (!bad_edges.empty()) {
         err_msg("  found inconsistent edges!!");
         GtexUtil::show(bad_edges, 4);
      }
   }

   ret->set_all_sticky(true);

   ret->set_non_penetrate(ret->skin_verts(), false);

   ret->set_all_offsets(h);
   ret->adjust_crease_offsets();

   MULTI_CMDptr push_cmd = new MULTI_CMD();
   if (mode) push(skel, push_cmd);
   // lock down crease vertices before smoothing
   ret->freeze_problem_verts(mode);
   if (mode) {
      push_cmd->undoit();
      skel.unpush_layer(true);
   }

   // track over the fine-level surface...
   // probably a bad idea; just trying
   // XXX - check this for errors when holes show up
   ret->track_deeper(ret->skin_verts(), R);

   int num_smooth_passes = Config::get_var_int("SKIN_SMOOTH_ITERS",20);

   if (num_smooth_passes > 0) {
      // do we want the vertices at this level to start out at the
      // smoothed positions inherited from the parent mesh?  the
      // following line does that. comment out to let the verts at
      // this level start from their assigned skel vert locations:
      ret->do_update();

      ::do_smoothing(
         find_boss_vmemes(ret->skin_verts()),
         num_smooth_passes,
         ret->mesh()
         );
   }
   _debug_instance = ret;

   err_adv(debug, "  created inflate Skin, R = %d", R);

   if (abs(h) > 1e-3) ret->_inflate = true;

   return ret;
}

bool
Skin::create_inflate(
   CBface_list& skel,
   double h,
   MULTI_CMDptr cmd,
   bool mode
   )
{

   // do a hack to make things more functional "for now"
   // find the level at which Bbases are operating,
   // and do the inflate operation there. but sew the
   // ribbons at this level (skel level). this way, for a small
   // offset, leading to hi-res skeleton region, we can get
   // away with a coarse skin. might suck though if the two
   // skins do smoothing independently and the ribbons don't
   // match up...

   double inner_offset = min(h, 0.0);
   double outer_offset = max(h, 0.0);
   Skin*  inside = create_inflate(skel, inner_offset, 0,  true, mode, cmd);
   Skin* outside = create_inflate(skel, outer_offset, 0, false, mode, cmd);

   if (!(inside && outside)) {
      err_adv(debug, "Skin::create_inflate: could not create inside and outside");
      return false;
   }

   if (!mode) outside->set_partner(inside);

   if (!mode) push_all_levels(skel, cmd, false);
   else {
      push(skel, cmd, false);
      push(inside->skin_faces(), cmd, false);
   }

   // create inf mapper
   if (!mode) {
      VertMapper in = inside->_mapper;
      VertMapper out = outside->_mapper;
      Bvert_list in_list = in.a_to_b(out.A());
      inside->set_inf_mapper(VertMapper(in_list, out.B(), true));
      outside->set_inf_mapper(VertMapper(out.B(), in_list, true)); 
   }

   // create ribbons
   
   // get matching boundaries of the two skin regions.
   // the "outer" one runs CCW as usual, but the "inner" one runs CW.
   EdgeStrip skel_strip = skel.get_boundary();
   if (skel_strip.empty()) {
      err_adv(debug, "Skin::create_inflate: no boundary -- no ribbons");
      return true;
   }
   EdgeStrip outer_strip = outside->_mapper.a_to_b(skel_strip);
   EdgeStrip inner_strip =  inside->_mapper.a_to_b(skel_strip);
   if (outer_strip.empty() || inner_strip.empty()) {
      err_adv(debug, "Skin::create_inflate: error finding skin boundaries");
      return false;
   }
   CREATE_RIBBONS_CMDptr rib = new CREATE_RIBBONS_CMD(outer_strip, inner_strip);
   if (rib->doit()) {
      cmd->add(rib);
      if (!mode) {
         inside->mesh()->compute_creases();
         return true;
      }
      VertMapper skel_map(inner_strip.verts(), skel_strip.verts(), false); // false means 1-way map
      if (Skin::create_multi_sleeve(inside->skin_faces()+inside->skel_faces(), skel_map, cmd))
         return true;
   }

   err_adv(debug, "Skin::create_inflate: no boundary -- no ribbons");
   return false;
}

void
Skin::add_offsets(double dist)
{
   if (!_inflate)
      return;
   err_adv(debug, "Skin::add_offsets:: changing offsets");
   WORLD::add_command(new INFLATE_SKIN_ADD_OFFSET_CMD(this, dist));
}

void 
Skin::set_partner(Skin* partner)
{
   assert(partner && !(_partner || partner->_partner));
   _partner = partner;
   _partner->_partner = this;
   if (parent()) {
      parent()->set_partner(_partner->parent());
   }
}

Bvert_list
Skin::frozen_verts(CBvert_list& verts) const
{
   Bvert_list ret(verts.num());
   for (int i=0; i<verts.num(); i++) {
      SkinMeme* m = SkinMeme::upcast(find_meme(verts[i]));
      if (m && m->is_frozen()) {
         ret += m->vert();
      }
   }
   return ret;
}

Bvert_list
Skin::unfrozen_verts(CBvert_list& verts) const
{
   return verts.minus(frozen_verts(verts));
}

Bvert_list
Skin::sticky_verts(CBvert_list& verts) const
{
   Bvert_list ret(verts.num());
   for (int i=0; i<verts.num(); i++) {
      SkinMeme* m = SkinMeme::upcast(find_meme(verts[i]));
      if (m && m->is_sticky()) {
         ret += m->vert();
      }
   }
   return ret;
}

Wpt_list
Skin::track_points(CBvert_list& verts) const
{
   Wpt_list ret(verts.num());
   for (int i=0; i<verts.num(); i++) {
      SkinMeme* m = SkinMeme::upcast(find_meme(verts[i]));
      if (m && m->is_tracking()) {
         ret += m->track_pt();
      } else {
         ret += verts[i]->loc();
      }
   }
   return ret;
}

inline ARRAY<Wline>
bundle_lines(CWpt_list& a, CWpt_list& b)
{
   assert(a.num() == b.num());
   ARRAY<Wline> ret(a.num());
   for (int i=0; i<a.num(); i++)
      ret += Wline(a[i], b[i]);
   return ret;
}

inline Wpt_list
centroids(CBvert_list& verts)
{
   Wpt_list ret(verts.num());
   for (int i=0; i<verts.num(); i++) {
      ret += verts[i]->qr_centroid();
   }
   return ret;
}

void 
Skin::draw_debug()
{
   if (!_show_memes)
      return;

   //if (!debug)
   //   return;

   // frozen: blue
   // sticky: orange
   // unglued: grey

   Skin* cur = upcast(cur_subdiv_bbase());
   if (!cur) return;
   Bvert_list    verts = cur->skin_verts();
   Bvert_list   frozen = cur->frozen_verts(verts);
   verts = verts.minus(frozen);
   Bvert_list   sticky = cur->sticky_verts(verts);
   Bvert_list  unglued = verts.minus(sticky);
   GL_VIEW::draw_pts(frozen.pts(),  Color::blue,   0.8, 8);
   GL_VIEW::draw_pts(sticky.pts(),  Color::orange, 0.8, 8);
   GL_VIEW::draw_pts(unglued.pts(), Color::grey6,  0.8, 8);

   if (debug) GL_VIEW::draw_lines(
      bundle_lines(verts.pts(), cur->track_points(verts)),
      Color::yellow,
      0.8,
      1,
      false
      );
   if (debug) GL_VIEW::draw_lines(
      bundle_lines(verts.pts(), centroids(verts)),
      Color::red,
      0.8,
      1,
      false
      );
}

/*****************************************************************
 * SkinCurveMap
 *****************************************************************/

inline double
get_param( CWpt_list& pts, int k )
{
   return pts.partial_length(k)/pts.length();
}

SkinCurveMap::SkinCurveMap(
   Bsimplex_list& simps,
   ARRAY<Wvec>& bcs,
   Skin* skin,
   Map0D3D* p0,
   Map0D3D* p1
   ) : Map1D3D(p0, p1),
       _simps(simps),
       _bcs(bcs),
       _skin(skin)
{
   assert(_simps.num() == _bcs.num());

   
   if (!p0 && !p1)
      assert(_simps.first() == _simps.last() && _bcs.first() == _bcs.last());

   hookup();
   
   if (Config::get_var_bool("DEBUG_SKIN_CURVE_MAP_CONSTRUCTOR",false,true)) {
      cerr << " -- in SkinCurveMapconstructor with simp/bc pairs: " << endl;
      int i;
      for (i = 0; i < _simps.num(); i++) {
         cerr << _simps[i] << "; " << _bcs[i] << endl;
      }
      cerr << " -- end simp/bc pairs list" << endl;
   }
}

void
SkinCurveMap::recompute()
{
}

Wpt
SkinCurveMap::map(double t) const
{
   // Point on curve at parameter value t.

   Wpt_list _pts = get_wpts();
   t = fix(t);
   int i;
   for (i = 0; i < _pts.num()-1; i++) {
      if (get_param(_pts, i+1) >= t)
         break;
   }
   if (get_param(_pts, i+1) - get_param(_pts, i) < 1e-12) 
      return _pts[i];
   t = (t - get_param(_pts, i)) / (get_param(_pts, i+1) - get_param(_pts, i));

   return (1-t) * _pts[i] + t * _pts[i+1];
}

Wpt_list 
SkinCurveMap::get_wpts() const 
{ 
  // Return a Wpt_list describing the current shape of the map
  Wpt_list ret(_simps.num()+2*!(!_p0));
  if (!(!_p0)) ret += _p0->map();
  for ( int i=0; i<_simps.num(); i++ ) {
    Wpt pt;
    _simps[i]->bc2pos(_bcs[i], pt);
    ret += pt;
  }
  if (!(!_p0)) ret += _p1->map();
  ret.update_length();
  return ret;
}

inline Wvec
simplex_normal(Bsimplex* s)
{
   if (is_vert(s))
      return ((Bvert*)s)->norm();
   if (is_edge(s))
      return ((Bedge*)s)->norm();
   if (is_face(s))
      return ((Bface*)s)->norm();
   return Wvec(1,0,0);
}

Wvec 
SkinCurveMap::norm(double t) const
{
   // The local unit-length normal:
   t = fix(t);
   Wpt_list pts = get_wpts();
   int i;
   for (i = 0; i < pts.num()-1; i++) {
      if (get_param(pts, i+1) >= t)
         break;
   }
   assert(get_param(pts, i+1) - get_param(pts, i) > 1e-5);
   t = (t - get_param(pts, i)) / (get_param(pts, i+1) - get_param(pts, i));

   Wvec norm1, norm2;
   if (_p0) {
      if (i == 0) {
         norm1 = _p0->norm();
         norm2 = simplex_normal(_simps[i]);
      } else if (i == pts.num()-2) {
         norm1 = simplex_normal(_simps[i-1]);
         norm2 = _p1->norm();
      } else {
         norm1 = simplex_normal(_simps[i-1]);
         norm2 = simplex_normal(_simps[i]);
      }
   } else {
      norm1 = simplex_normal(_simps[i]);
      norm2 = simplex_normal(_simps[i+1]);
   }
   return ((1-t) * norm1 + t * norm2).normalized();
}

double 
SkinCurveMap::length() const
{
   // Total length of curve:

   // XXX - if this is called often should cache the info

   return get_wpts().length();
}

double
SkinCurveMap::hlen() const
{
   // The "planck length" for this curve; i.e., the step size
   // below which you'd just be sampling the noise:

   // XXX - this isn't a good way to calculate this...
   //       but it's okay "for now."

   double h = ((_simps.num()  < 2) ?  1.0 :
               (_simps.num() == 2) ? 1e-9 :
               1.0/_simps.num());
   return max(h, 1e-9)/10.0;
}

void
SkinCurveMap::set_pts(CBsimplex_list& simps, ARRAY<Wvec>& bcs)
{
   assert(simps.num() == bcs.num());
   
   _simps = simps;
   _bcs = bcs;

   static bool debug = Config::get_var_bool("DEBUG_SKIN_CURVE_SETUV",false,true);

   if (debug) {
      cerr << "-- In SkinCurveMap::set_ptss() got hte following simp/bc pair list" << endl;
      int i;
      for (i = 0; i < _simps.num(); i++) {
         cerr << i << " " << _simps[i] << "; " << _bcs[i] << endl;
      }
      cerr << "-- End list" << endl;
   }

   // Notify dependents they're out of date and sign up to be
   // recomputed
   invalidate();
}

// end of file skin.C
