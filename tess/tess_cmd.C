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
#include "map3d/param_list.H"
#include "mesh/patch.H"
#include "tess_cmd.H"
#include "ti.H"

/*****************************************************************
 * MOVE_CMD
 *****************************************************************/
void
MOVE_CMD::add_delt(CWvec& d)
{
   // increase translation by d

   // if _vec was already applied,
   // apply extra translation by d:
   if (is_done()) {
      apply_translation(d);
   }
   _vec += d;
}

bool
MOVE_CMD::doit()
{
   if (is_done())
      return true;

   apply_translation(_vec);
   return COMMAND::doit();
}

bool
MOVE_CMD::undoit()
{
   if (!is_done())
      return true;

   apply_translation(-_vec);
   return COMMAND::undoit();
}

void
MOVE_CMD::apply_translation(CWvec& delt)
{
   static bool debug = Config::get_var_bool("DEBUG_MOVE_CMD",false);

   // apply translation to selected geometry:
   if (_bbases.empty()) {
      err_adv(debug, "MOVE_CMD::apply_translation: applying to GEOM");
      // apply the translation to the whole mesh (via its GEOM):
      assert(_geom);
      _geom->mult_by(Wtransf::translation(delt));
      // XXX - notification also sent in GEOM::mult_by()
      XFORMobs::notify_xform_obs(_geom, XFORMobs::MIDDLE);
   } else {
      err_adv(debug, "MOVE_CMD::apply_translation: applying to bbases");
      MOD::tick();
      _bbases.translate(delt, MOD());
   }
}

/*****************************************************************
 * XFORM_CMD
 *****************************************************************/
bool
XFORM_CMD::doit()
{
   if (is_done())
      return true;

   apply_xf(_xf);
   return COMMAND::doit();
}

bool
XFORM_CMD::undoit()
{
   if (!is_done())
      return true;

   apply_xf(_xf.inverse());
   return COMMAND::undoit();
}

void
XFORM_CMD::concatenate_xf(CWtransf& xf)
{
   // replace our transform _xf with xf*_xf

   // if _xf was already applied,
   // apply additional transform by xf:
   if (is_done()) {
      apply_xf(xf);
   }
   _xf = xf * _xf;
}

void
XFORM_CMD::apply_xf(CWtransf& xf)
{
   static bool debug = Config::get_var_bool("DEBUG_XFORM_CMD",false);
   err_adv(debug, "XFORM_CMD::apply_xf: applying to geoms");

   _geoms.mult_by(xf);
}

void 
XFORM_CMD::add(GEOMptr g)
{
   if (!g || _geoms.contains(g))
      return;
   if (is_done()) {
      g->mult_by(_xf);
   }
   _geoms += g;
}

/*****************************************************************
 * PUSH_FACES_CMD:
 *****************************************************************/
bool 
PUSH_FACES_CMD::doit() 
{
   // This pushes the region (makes it secondary):

   bool debug = Config::get_var_bool("DEBUG_PUSH_FACES",false);

   if (is_done())
      return true;

   if (!_region.can_push_layer()) {
      err_msg("PUSH_FACES_CMD::doit: can't push the region");
      if (debug) {
         MeshGlobal::select(_region);
      }
      COMMAND::doit();
      return false;
   }
   _region.push_layer(_do_boundary);
   return COMMAND::doit();      // update state in COMMAND
}

bool 
PUSH_FACES_CMD::undoit() 
{
   // This restores the region to primary status:

   bool debug = Config::get_var_bool("DEBUG_PUSH_FACES",false);

   if (!is_done())
      return true;

   if (!_region.can_unpush_layer()) {
      err_msg("PUSH_FACES_CMD::undoit: can't unpush the region");
      if (debug) {
         MeshGlobal::select(_region);
      }
      COMMAND::undoit();
      return false;
   }
   _region.unpush_layer(_do_boundary);
   return COMMAND::undoit();    // update state in COMMAND
}

/*****************************************************************
 * WPT_LIST_RESHAPE_CMD
 *****************************************************************/
bool
WPT_LIST_RESHAPE_CMD::doit()
{
   if (is_done())
      return true;

   _map->set_pts(_new_shape);
   return COMMAND::doit();
}

bool
WPT_LIST_RESHAPE_CMD::undoit()
{
   if (!is_done())
      return true;

   _map->set_pts(_old_shape);
   return COMMAND::undoit();
}

/*****************************************************************
 * TUBE_MAP_RESHAPE_CMD
 *****************************************************************/
bool
TUBE_MAP_RESHAPE_CMD::doit()
{
   if (is_done())
      return true;

   _map->set_pts(_new_shape);
   return COMMAND::doit();
}

bool
TUBE_MAP_RESHAPE_CMD::undoit()
{
   if (!is_done())
      return true;

   _map->set_pts(_old_shape);
   return COMMAND::undoit();
}

/*****************************************************************
 * REVERSE_FACES_CMD:
 *****************************************************************/
bool 
REVERSE_FACES_CMD::doit() 
{
   // Flip the face normals throughout the region:

   if (is_done())
      return true;
   
   _region.reverse_faces();
   return COMMAND::doit();      // update state in COMMAND
}

bool 
REVERSE_FACES_CMD::undoit() 
{
   // Flip 'em again (back to original orientation):

   if (!is_done())
      return true;

   _region.reverse_faces();
   return COMMAND::undoit();    // update state in COMMAND
}

/*****************************************************************
 * SHOW_BBASE_CMD:
 *****************************************************************/
bool 
SHOW_BBASE_CMD::doit() 
{
   if (is_done())
      return true;

   if (!_bbase->can_show()) {
      cerr << "SHOW_BBASE_CMD::doit: can't show the "
           << _bbase->class_name() << endl;
      return false;
   }

   _bbase->show();
   return COMMAND::doit();      // update state in COMMAND
}

bool 
SHOW_BBASE_CMD::undoit() 
{
   if (!is_done())
      return true;

   if (!_bbase->can_hide()) {
      cerr << "SHOW_BBASE_CMD::doit: can't hide the "
           << _bbase->class_name() << endl;
      return false;
   }

   _bbase->hide();
   return COMMAND::undoit();    // update state in COMMAND
}

/*****************************************************************
 * CREATE_RIBBONS_CMD:
 *****************************************************************/
bool CREATE_RIBBONS_CMD::_debug =
Config::get_var_bool("DEBUG_CREATE_RIBBONS",false);

inline bool
chains_match(CEdgeStrip& a, CEdgeStrip& b)
{
   // Return true if the chains have the same number of elements,
   // have breaks in the same locations, and do not have any
   // elements in common.

   if (a.num() != b.num())
      return false;

   // Ensure breaks match:
   for (int i=0; i<a.num(); i++)
      if (a.has_break(i) != b.has_break(i))
         return false;

   // Verify no elements in common:
   Bvert_list a_chain, b_chain;
   for (int j=0, k=0; a.get_chain(j, a_chain) && b.get_chain(k, b_chain); ) {
      assert(j == k);
      if (a_chain.contains_any(b_chain))
         return false;
   }

   return true;
}

inline double
avg_dist(CBvert_list& a, CBvert_list& b)
{
   // Given two sets of vertices with the same number in each,
   // return the average distance between matching vertices.

   assert(a.num() == b.num());

   if (a.empty())
      return 0;

   double ret = 0;
   for (int i=0; i<a.num(); i++)
      ret += a[i]->dist(b[i]);
   return ret / a.num();
}

inline Bvert_list
gen_interp_row(CBvert_list& r, CBvert_list& s, double v)
{
   // Given matching vertex lists r and s from the same mesh,
   // generate a list of new vertices (created here) whose
   // positions are interpolated from r and s. E.g., if the
   // interpolation amount v is 0.3, the resulting vertices will
   // be located 0.3 of the way from r to s.

   Bvert_list ret(r.num());
   assert(r.num() == s.num());
   if (r.empty())
      return ret;
   BMESH* mesh = r.mesh();
   assert(mesh != 0 && s.mesh() == mesh);
   assert(v > 0 && v < 1);

   // XXX - Usually this is called for chains of vertices that
   // wrap around, forming closed loops. In that case we need to
   // handle the last point differently to avoid creating
   // duplicate internal vertices. More generally, each vertex
   // chain might intersect itself repeatedly, in which case we
   // should also avoid generating duplicate internal vertices.
   // (The "ribbon" will be non-manifold in that case!)  we're
   // not handling that case "for now."
   int n = r.num();
   bool does_wrap = r.first() == r.last();
   if (does_wrap) {
      assert(s.first() == s.last()); // The other one should also wrap
      n--;
   }
   for (int i=0; i<n; i++)
      ret += mesh->add_vertex(interp(r[i]->loc(), s[i]->loc(), v));
   if (does_wrap)
      ret += ret.first(); // like r and s, we re-use the first one
   return ret;
}

void
CREATE_RIBBONS_CMD::gen_ribbon(CBvert_list& a, CBvert_list& b, Patch* p)
{
   // Create a single "ribbon" (row of quads) between two matching
   // connected components of the boundaries.

   //    a0--------a1--------a2--------a3--------a4---...       
   //     |         |         |         |         |               
   //     |         |         |         |         |               
   //     .         .         .         .         .               
   //     .         .         .         .         .               
   //     .         .         .         .         .               
   //     .         .         .         .         .               
   //     |         |         |         |         |               
   //     |         |         |         |         |               
   //    b0--------b1--------b2--------b3--------b4---...       

   assert(a.forms_chain() && b.forms_chain());
   assert(a.num() > 1 && a.num() == b.num());

   BMESH* mesh = a.mesh();
   assert(mesh != 0 && b.mesh() == mesh && p != 0 && p->mesh() == mesh);
   
   // Average vertical separation (WRT diagram above):
//   double h = avg_dist(a,b);

   // Average edge length of the two chains:
   double l = interp(a.get_chain().avg_len(), b.get_chain().avg_len(), 0.5);
   assert(l > 0);

   // Compute number of vertical edges (WRT diagram above):
   // XXX - no need for this until RIBBON is created as a surface
   int n = 1; //max(1, (int)round(h/l));
   err_adv(_debug, "CREATE_RIBBONS_CMD::gen_ribbon: using %d vertical segments",
           n);

   ARRAY<double> uvals = make_params(a.num() - 1);
   UVpt_list  prev_uvs = make_uvpt_list(uvals, 0);
   Bvert_list prev_row = b;

   Bsurface* surf = new Bsurface((LMESH*)a.mesh());

   for (int k=1; k<=n; k++) {
      // Work row by row to generate internal vertices, except on
      // the last row when we use the vertices of 'a'. Then
      // create quads joining the current and previous rows of
      // vertices. Assign uv-coordinates as well.
      double v = (k<n) ? double(k)/n : 1.0;
      UVpt_list  cur_uvs = make_uvpt_list(uvals, v);

      // Create internal vertices (except for the final row):
      Bvert_list cur_row = (k<n) ? gen_interp_row(b, a, v) : a;
      assert(cur_row.num() == a.num());

      // Create a band of quads from the current and previous rows:
      for (int i=1; i<cur_row.num(); i++) {
         Bface* f = surf->add_quad(prev_row[i-1], prev_row[i  ],
                                   cur_row [i  ],  cur_row[i-1],
                                   prev_uvs[i-1], prev_uvs[i  ],
                                   cur_uvs [i  ],  cur_uvs[i-1])->face();//, p);
         assert(f && f->is_quad());
         _ribbon += f;
         _ribbon += f->quad_partner();
      }

      prev_uvs = cur_uvs;
      prev_row = cur_row;
   }
}

void
CREATE_RIBBONS_CMD::gen_ribbons(CEdgeStrip& a, CEdgeStrip& b, Patch* p)
{
   // Generate "ribbons" of quads joining the two boundaries.
   // Process each separate connected component of the boundaries
   // independently.

   BMESH* mesh = a.mesh();
   assert(mesh != 0 && b.mesh() == mesh && p != 0 && p->mesh() == mesh);
   assert(_ribbon.empty());

   // Each "chain" is a separate connected component of the boundary:
   Bvert_list a_chain, b_chain;
   for (int j=0, k=0; a.get_chain(j, a_chain) && b.get_chain(k, b_chain); ) {
      assert(j == k);
      gen_ribbon(a_chain, b_chain, p);
   }
   mesh->changed();
}

bool
CREATE_RIBBONS_CMD::is_ready() const
{
   // Return true if the command is ready to be executed:

   if (is_done()) {
      // It was already done, so it's not ready to be re-done:
      return false;
   }

   if (is_undone()) {
      // It was done, then undone; to redo all that's needed
      // is to make the ribbon primary again:
      return !_ribbon.empty() && _ribbon.is_all_secondary();
   }

   // Must not have been executed previously:
   assert(is_clear());

   // Both edge chains must belong to the same LMESH:
   if (!LMESH::upcast(_a.mesh()) || _a.mesh() != _b.mesh()) {
      err_adv(_debug, "CREATE_RIBBONS_CMD::is_ready: invalid mesh");
      return false;
   }

   // Both edge chains must match:
   if (!chains_match(_a, _b)) {
      err_adv(_debug, "CREATE_RIBBONS_CMD::is_ready: chains don't match");
      return false;
   }

   // Both must consist of border edges:
   if (!(_a.edges().all_satisfy(BorderEdgeFilter()) &&
         _b.edges().all_satisfy(BorderEdgeFilter()))) {
      err_adv(_debug, "CREATE_RIBBONS_CMD::is_ready: non-borders in boundary");
      return false;
   }

   // The internal faces must not have been generated already:
   if (!_ribbon.empty()) {
      err_adv(_debug, "CREATE_RIBBONS_CMD::is_ready: ribbon not empty");
      return false;
   }

   // It's okay:
   return true;
}

bool
CREATE_RIBBONS_CMD::doit()
{
   // Given matching EdgeStrips 'a' and 'b' of boundary edges, 
   // connect corresponding edges together like so:
   //                                                             
   //    a0--------a1--------a2--------a3--------a4---...       
   //     |         |         |         |         |               
   //     |         |         |         |         |               
   //     |         |         |         |         |               
   //     |         |         |         |         |               
   //    b0--------b1--------b2--------b3--------b4---...       
   //
   // Additional vertices may be created along the vertical lines
   // if needed to preserve good aspect ratios.

   if (is_done())
      return true;

   if (!is_ready()) {
      err_adv(_debug, "CREATE_RIBBONS_CMD::doit: preconditions not met");
      return false;
   }

   if (is_undone()) {
      // The "ribbon" was created, but was made secondary.
      // To redo the operation, just restore the ribbon to primary status.
      assert(!_ribbon.empty() && _ribbon.is_all_secondary());
      _ribbon.unpush_layer();
      return COMMAND::doit();      // update state in COMMAND
   }

   assert(is_clear());

   // If no patch was provided, create one and ensure that it gets
   // drawn by putting in in the drawables list of the control mesh
   if (!_patch) {
      LMESHptr mesh = LMESH::upcast(_a.mesh());
      assert(mesh != 0);
      _patch = mesh->new_patch();
      assert(_patch && _patch->mesh() == mesh);
      if (!mesh->is_control_mesh())
         mesh->control_mesh()->drawables() += _patch;
   }

   gen_ribbons(_a, _b, _patch);

   return COMMAND::doit();      // update state in COMMAND
}

bool
CREATE_RIBBONS_CMD::undoit()
{
   if (!is_done())
      return true;

   _ribbon.push_layer();
   return COMMAND::undoit();    // update state in COMMAND
}

bool
CREATE_RIBBONS_CMD::clear()
{
   if (is_clear())
      return true;
   if (is_done())
      return false;
   if (!COMMAND::clear())       // update state in COMMAND
      return false;

   // XXX - Not implemented
   //       Should delete mesh elements created in doit()...
   err_msg("CREATE_RIBBONS_CMD::clear: not implemented");

   return true;
}

// end of file tess_cmd.C
