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
 * lmesh.C
 **********************************************************************/
#include "disp/ray.H"
#include "disp/colors.H"
#include "geom/world.H"   // XXX - for debugging
#include "mlib/statistics.H"
#include "std/config.H"
#include "lpatch.H"

using namespace mlib;

#include <string>
using namespace std;

// add LMESH to decoder hash table:
/*
// XXX - Moved this to bmesh.C
// to work around a static initialization
// order problem with BMESHobs not having
// its static members available before
// the LMESH constructor is called.
static class DECODERS {
public:
DECODERS() {
DECODER_ADD(LMESH);
}
} DECODERS_static;
*/

LMESH::LMESH(int num_v, int num_e, int num_f) :
   BMESH(num_v, num_e, num_f),
   _parent_mesh(0),
   _cur_mesh(0),     // XXX - see comment below
   _subdiv_mesh(0),
   _subdiv_level(0),
   _loc_calc(new HybridLoc),    // use hybrid rules by default
   _color_calc(new LoopColor)   // for colors we're not so choosy
{
   // Some compilers complain if 'this' is used in
   // member variable initialization section above
   _cur_mesh = this;
   _lmesh_tags = NULL;            // non-static tags
}

LMESH::~LMESH()
{
   delete_elements();
   delete _loc_calc;
   delete _color_calc;
}

LMESH* 
LMESH::get_lmesh(const string& exact_name)
{
   // Return an LMESH with the given name.  If one already exists,
   // return that one.  Otherwise create a new LMESH with the desired
   // name.  Fails (returns null) if the name is invalid (null string)
   // or the name is already taken by a BMESH that is not an LMESH.

   bool debug = NameLookup<BMESH>::_debug;

   // Make sure they're not asking for the null string:
   if (exact_name == "") {
      if (debug)
         cerr << "LMESH::get_lmesh: error: name is empty" << endl;
      return 0;
   }

   // Does a mesh with the requested name exist?
   BMESH* mesh = NameLookup<BMESH>::lookup(exact_name);

   // If no mesh of that name exists, create a new one:
   if (!mesh) {
      LMESH* ret = new LMESH;
      assert(ret && !ret->has_name());
      ret->set_name(exact_name);
      return ret;
   }

   // A BMESH with the requested name already exists;
   // if it is not also an LMESH, this is an error:
   if (debug && !LMESH::isa(mesh)) {
      cerr << "LMESH::get_lmesh: error: found requested name: "
           << exact_name
           << ", but it is not an LMESH"
           << endl;
   }
   return upcast(mesh);
}

void
LMESH::set_subdiv_loc_calc(SubdivLocCalc* c)
{
   // Delete the old one:
   delete _loc_calc;

   // Store the new one:
   _loc_calc = c;

   // Mark all vertices "dirty" to force recomputation
   // next time the mesh is used for anything:
   mark_all_dirty();

   // If there is a subdivision mesh already allocated,
   // change its subdiv_loc too:
   if (_subdiv_mesh)
      _subdiv_mesh->set_subdiv_loc_calc(c->dup());

   // For control meshes only -- make sure display lists are
   // invalidated so we can see the results of the change!
   if (is_control_mesh())
      changed(RENDERING_CHANGED);
}

void
LMESH::set_subdiv_color_calc(SubdivColorCalc* c)
{
   // Delete the old one:
   delete _color_calc;

   // Store the new one:
   _color_calc = c;

   // Mark all vertices "dirty" to force recomputation
   // next time the mesh is used for anything:
   mark_all_dirty();

   // If there is a subdivision mesh already allocated,
   // change its subdiv_loc too:
   if (_subdiv_mesh)
      _subdiv_mesh->set_subdiv_color_calc(c->dup());

   // For control meshes only -- make sure display lists are
   // invalidated so we can see the results of the change!
   if (is_control_mesh())
      changed(RENDERING_CHANGED);

   // XXX - this copied code (see set_subdiv_loc_calc())
   // is sub-optimal
}

void
LMESH::set_geom(GEOM *geom)
{
   BMESH::set_geom(geom);
   if (_subdiv_mesh)
      _subdiv_mesh->set_geom(geom);
}

int
LMESH::draw(CVIEWptr &v)
{
   // Only called on control mesh...
   assert(is_control_mesh());

   // Update subdivision if needed (down to current level):
   update();

   // BMESH functionality handles it...
   return BMESH::draw(v);
}

double
LMESH::area() const
{
   // if current subdivision mesh is not this
   // (i.e., this is the control mesh),
   // make sure subdivision is up-to-date
   // then forward call to current mesh:

   ((LMESH*)this)->update();

   return _cur_mesh->BMESH::area();
}

double
LMESH::volume() const
{
   // if current subdivision mesh is not this
   // (i.e., this is the control mesh),
   // make sure subdivision is up-to-date
   // then forward call to current mesh:

   ((LMESH*)this)->update();

   return _cur_mesh->BMESH::volume();
}

void
LMESH::clear_creases()
{
   // handle corners
   int count = 0;
   for (int i=0; i<nverts(); i++) {
      if (lv(i)->corner_value()) {
         lv(i)->set_corner(0);
         count++;
      }
   }
   if (count)
      changed(RENDERING_CHANGED);

   err_msg("LMESH::clear_creases: cleared %d corners", count);

   // take care of creases...
   BMESH::clear_creases();
}


Patch*
LMESH::new_patch()
{
   Patch* ret = new Lpatch(this);

   _patches += ret;
   if (is_control_mesh())
      _drawables += ret;
   return ret;
}


Bedge*
LMESH::new_edge(Bvert* u, Bvert* v) const
{
   return new Ledge((Lvert*)u, (Lvert*)v);
}

Bvert*
LMESH::new_vert(CWpt& p) const
{
   return new Lvert(p);
}

Bface*
LMESH::new_face(
   Bvert* u,
   Bvert* v,
   Bvert* w,
   Bedge* e,
   Bedge* f,
   Bedge* g) const
{
   return new Lface((Lvert*)u, (Lvert*)v, (Lvert*)w,
                    (Ledge*)e, (Ledge*)f, (Ledge*)g);
}

int
LMESH::remove_vertex(Bvert* bv)
{
   assert(bv && bv->mesh() == this);
   Lvert* v = (Lvert*)bv;

   // Get it out of the dirty list
   rem_dirty_vert(v);

   // And make damn sure it stays out
   v->set_bit(Lvert::DEAD_BIT);

   return BMESH::remove_vertex(v);
}

int
LMESH::remove_edge(Bedge* be)
{
   // Need to delete edge's subdiv elements
   // first, before the edge is detached
   // and loses its connectivity info

   assert(be && be->mesh() == this);
   Ledge* e = (Ledge*)be;

   e->delete_subdiv_elements();
   return BMESH::remove_edge(e);
}

int
LMESH::remove_face(Bface* bf)
{
   // Need to delete face's subdiv elements
   // first, before the face is detached
   // and loses its connectivity info

   assert(bf && bf->mesh() == this);
   Lface* f = (Lface*)bf;

   f->delete_subdiv_elements();
   return BMESH::remove_face(f);
}

Bvert*
LMESH::add_vertex(Bvert* v)
{
   if (BMESH::add_vertex(v))
      add_dirty_vert((Lvert*)v);
   return v;
}

Bvert*
LMESH::add_vertex(CWpt& p)
{
   // following line triggers LMESH::add_vertex(Bvert*)
   // which adds the dirty vert:
   return BMESH::add_vertex(p);
}

BMESH&
LMESH::operator =(CBMESH& m)
{
   return BMESH::operator=(m);
}

BMESH&
LMESH::operator =(CLMESH& m)
{
   // Replace this mesh with identical copy of m:
   BMESH::operator=((CBMESH&)m);

   // Also copy corner information
   for (int k=0; k<nverts(); k++) {
      if (m.lv(k)->corner_value() > 0)
         lv(k)->set_corner(m.lv(k)->corner_value());
   }

   return *this;
}

BMESH&
LMESH::operator =(BODY& b)
{
   BMESH::operator=(b); return *((BMESH*)this);
}

int
LMESH::write_patches(ostream& os) const
{
   return control_mesh()->BMESH::write_patches(os);
}

void
LMESH::changed(change_t change)
{
   BMESH::changed(change);
//    if (_subdiv_mesh)
//       _subdiv_mesh->changed(change);

   // notify subdiv meshes
   for (LMESH* m = _subdiv_mesh; m; m = m->_subdiv_mesh)
      m->BMESH::changed(change);

   // ensure control mesh knows to invalidate display lists etc.
   // (if this *is* the control mesh, the first line took care of it)
   if (!is_control_mesh())
      control_mesh()->BMESH::changed(RENDERING_CHANGED);
/*
   return;

   switch(change) {
    case RENDERING_CHANGED:
      if (!is_control_mesh())
         control_mesh()->changed(change);
    case NO_CHANGE:
      break;
    case TOPOLOGY_CHANGED:
    case PATCHES_CHANGED:
    case TRIANGULATION_CHANGED:
    case VERT_POSITIONS_CHANGED:
    case VERT_COLORS_CHANGED:
    case CREASES_CHANGED:
      if (_subdiv_mesh)                              // if subdiv mesh, 
         _subdiv_mesh->changed(change);              //   pass it down.
      else if (!is_control_mesh())                   // else, invalidate display 
         control_mesh()->changed(RENDERING_CHANGED); //   lists in ctrl mesh
   }
*/
}

int
LMESH::size() const
{
   // Return the approximate memory used for this mesh.

   // The extra 4 bytes per vertex, edge, etc. is for the
   // pointer in the corresponding list (_verts, _edges, etc.).
   // For each Bvert we also count the expected number of Bedge*
   // pointers in the adjacency list.

   return (
      (nverts()   * (sizeof(Lvert)  + 6*sizeof(Bedge*) + 4)) +
      (nedges()   * (sizeof(Ledge)  + 4)) +
      (nfaces()   * (sizeof(Lface)  + 8)) +     // each face is in two lists
      (npatches() * (sizeof(Lpatch) + 4)) +
      sizeof(LMESH)
      );
}
void
LMESH::print() const
{
   BMESH::print();
   err_msg("\tsubdiv level: %d", _subdiv_level);
}

void 
LMESH::update_patch_blend_weights() 
{
   if (Config::get_var_bool("DEBUG_PATCH_BLEND_WEIGHTS",false)) {
      cerr << "LMESH::update_patch_blend_weights: level: " << cur_level() 
           << ", passes: " << patch_blend_smooth_passes() << endl
           << "subdiv edge scale: " << subdiv_edge_scale() << endl
           << "cur level: " << cur_level() << endl;
   }
   cur_mesh()->BMESH::update_patch_blend_weights();
}

int  
LMESH::patch_blend_smooth_passes() const 
{
   // number of passes at control level:
   int p = control_mesh()->BMESH::patch_blend_smooth_passes();

   // number at this level:
   return p; //(p + 1)*subdiv_edge_scale() - 1;
}

int
LMESH::intersect(
   RAYhit   &r,
   CWtransf &m,
   Wpt      &nearpt,
   Wvec     &n,
   double   &d,
   double   &d2d,
   XYpt     &uvc
   ) const
{
   // if current subdivision mesh is not this
   // (i.e., this is the control mesh),
   // make sure subdivision is up-to-date
   // then forward call to current mesh:

   ((LMESH*)this)->update();

   return _cur_mesh->BMESH::intersect(r,m,nearpt,n,d,d2d,uvc);
}

void
LMESH::transform(CWtransf &xform, CMOD& mod)
{
   // Apply the transform to this mesh:
   BMESH::transform(xform, mod);

   // It used to be that we could just apply the transform at the
   // control mesh level, knowing that the effect was propagated to
   // finer subdivision levels thru the normal subdivision
   // process. But now subdivision meshes have been generalized to
   // include portions that do not come from the next coarser level
   // in the hierarchy. So to be on the safe side we apply the
   // transformation at all levels, recursively:
   if (_subdiv_mesh)
      _subdiv_mesh->transform(xform, mod);
}

void
LMESH::_merge(BMESH* bm)
{
   // merge the given mesh into this one.

   // error checking was done before this protected method was called.
   // so this convenient upcast is safe:
   LMESH* m = (LMESH*)bm;

   // merge subdivision meshes (recursively) first. but if this one
   // has fewer levels of subdivision, truncate the other one to the
   // same number of levels.
   if (_subdiv_mesh && m->_subdiv_mesh)
      _subdiv_mesh->_merge(m->_subdiv_mesh);
   else
      m->delete_subdiv_mesh(); // ensure it has no finer level meshes

   // Get the dirty vertices from m and put them into this
   // mesh's dirty list:
   m->_dirty_verts.clear_bits(Lvert::DIRTY_VERT_LIST_BIT);
   while (!m->_dirty_verts.empty())
      add_dirty_vert((Lvert*)m->_dirty_verts.pop());

   // this concludes the LMESH-specific aspect of the merge
   // method. now just continue with the normal merge...
   BMESH::_merge(bm);
}

const BBOX&
LMESH::get_bb()
{
   // if current subdivision mesh is not this
   // (i.e., this is the control mesh),
   // make sure subdivision is up-to-date
   // then forward call to current mesh:

   update();

   return _cur_mesh->BMESH::get_bb();
}

// Helper function used below in allocate_adjacent_child_faces():
inline void
allocate_child_faces(Lface* f)
{
   if (f && f->subdiv_dirty())
      f->allocate_subdiv_elements();
}

// Helper function used below in update_sub(Ledge* e):
inline void
allocate_adjacent_child_faces(Ledge* e)
{
   if (!e) return;

   allocate_child_faces(e->lf(1));
   allocate_child_faces(e->lf(2));
   if (e->is_multi()) {
      CBface_list& faces = *e->adj();
      for (int i=0; i<faces.num(); i++) {
         allocate_child_faces((Lface*)faces[i]);
      }
   }
}

// Helper function used below in update_sub(Lvert* v)
inline void
update_sub(Ledge* e, bool do_faces=true)
{
   if (!e) return;

   // Allocate subdiv elements for the edge and try to
   // assign the subdiv vert location:
   e->update_subdivision();

   // Ensure adjacent subdiv faces are allocated:
   if (do_faces) {
      allocate_adjacent_child_faces(e);
   }
}

// Helper function used below in LMESH::allocate_subdiv_mesh():
inline void
update_sub(Lvert* v, bool do_edges=true)
{
   if (!v) return;

   
   // Allocate subdiv vert and assign the subdiv location:
   v->update_subdivision();

   // Visit all adjacent edges and faces...
   if (do_edges) {
      for (int j=0; j<v->degree(); j++)
         update_sub(v->le(j));
   }
}

bool
LMESH::allocate_subdiv_mesh()
{
   // Allocate the next-level subdivision mesh itself.
   // (NOT any vertices, edges or faces).

   if (_subdiv_mesh)
      return 1; // It was easy

   // Actually have to do it:
   _subdiv_mesh = new LMESH(
      nverts()     + nedges(),       // number of vertices
      2*nedges() + 3*nfaces(),       // number of edges
      4*nfaces()                     // number of faces
      );

   if (!_subdiv_mesh) {
      // System error - can't allocate memory.
      err_ret( "LMESH::allocate_subdiv_mesh: can't allocate subdiv mesh");
      return 0; // Failure. (Might also try exit(1) here.)
   }

   // Inherited stuff:
   _subdiv_mesh->_type = _type;
   _subdiv_mesh->_geom = _geom;
   _subdiv_mesh->set_parent(this);
   _subdiv_mesh->set_subdiv_loc_calc(_loc_calc->dup());
   _subdiv_mesh->set_subdiv_color_calc(_color_calc->dup());

   // Tell observers the subdivision mesh was allocated:
   BMESHobs::broadcast_subdiv_gen(this);

   return 1; // success
}

const char*
face_type(Bface* f)
{
   if (!f)                      return "NULL ";
   else if (f->is_primary())    return "primary ";
   else                         return "secondary ";
}

inline void
check(Ledge* e)
{
   if (!e) {
      cerr << "null edge" << endl;
      return;
   }
   if (e->subdiv_mask() == Ledge::REGULAR_SMOOTH_EDGE && e->nfaces() < 2) {
      cerr << "bad edge found." << endl;
      cerr << "  faces: "
           << face_type(e->f1())
           << face_type(e->f2());
      if (e->adj()) {
         CBface_list& adj = *e->adj();
         for (int i=0; i<adj.num(); i++)
            cerr << face_type(adj[i]);
      }
      cerr << endl;
      WORLD::show(e->v1()->loc(), e->v2()->loc());
      e->mask_changed();
      e->set_mask();
   }
}

inline void
debug_check_verts(Cstr_ptr& msg, CBvert_list& verts, CBvert_list& dirty_verts)
{
   Bvert_list A = verts.filter(BitSetSimplexFilter(Lvert::DIRTY_VERT_LIST_BIT));
   if (!dirty_verts.contains_all(A)) {
      Bvert_list bad = A.minus(dirty_verts);
      cerr << msg << ": found " << bad.num()
           << " vertices missing from dirty list" << endl;
      WORLD::show_pts(bad.pts(), 8, Color::red);
   }
   Bvert_list B = verts.filter(BitClearSimplexFilter(Lvert::DIRTY_VERT_LIST_BIT));
   if (dirty_verts.contains_any(B)) {
      Bvert_list bad = dirty_verts.minus(B);
      cerr << msg << ": found " << bad.num()
           << " unexpected vertices in dirty list" << endl;
      WORLD::show_pts(bad.pts(), 8, Color::blue);
   }
}

inline str_ptr
get_name(LMESH* m)
{
   return (m && m->geom()) ? m->geom()->name() :
      (m ? "null geom" : "null mesh");
}

bool
LMESH::update_subdivision(int level)
{
   // Allocate subdivision meshes and mesh elements, and compute
   // subdivision positions (and optionally colors) down to the
   // given number of levels.
   //
   // "Level" is relative to this mesh.
   // I.e., level 1 means 1 level down from this one.

   // Returns 'true' if anything happened.
   bool ret = !_dirty_verts.empty();

   // Before going any further, make sure this mesh is up to
   // date.  You see, this mesh may have just been generated by
   // its parent.  And it may contain vertices whose locations
   // have not yet been computed, because the vertex or edge in
   // the parent mesh that would have computed the subdivision
   // location deferred to some observer that said it was going
   // to "handle" it. But that observer (e.g. a Bsurface) may not
   // have actually handled it yet, preferring to wait for the
   // update request which is going out now:
   BMESHobs::broadcast_update_request(this);

   // If level <= 0 there is nothing to do.
   // If can't allocate subdiv mesh, give up:
   if (level <= 0 || !allocate_subdiv_mesh()) {
      if (cur_mesh() == this)
         return false;
      control_mesh()->set_cur_mesh(this);
      return true;
   }

   // Debugging:
   static bool debug = Config::get_var_bool("DEBUG_SUBDIVISION",false);
   static uint fnum = 0;
   if (debug && ret) {
      if (fnum != VIEW::stamp()) {
         fnum = VIEW::stamp();
         cerr << "******** frame " << fnum << " ********" << endl;
      }
      if (level > 0 && ret) {
         err_adv(debug, "%s: level %d updating subdiv mesh",
                 **::get_name(this), _subdiv_level);
      }
   }
   err_adv(ret && debug,
           "level %d to level %d: %d faces, updating %d of %d vertices using %s",
           subdiv_level(),
           level,
           _faces.num(),
           _dirty_verts.num(),
           _verts.num(),
           **_loc_calc->name());

   // Q: How do we know which mesh elements to visit to tell them
   //    to generate and update their subdivision elements?
   // A: Well, we could just visit them all.
   // Q: NO, that would be too slow!!! Instead we choose this
   //    convoluted method: Vertices that haven't updated their
   //    subdivision vertices are in the "dirty" list. Edges and
   //    faces that haven't updated their subdivision elements
   //    don't have a dirty list to get into, but they are always
   //    adjacent to at least one dirty vertex. So we visit all the
   //    dirty vertices and tell them and their adjacent edges and
   //    faces to allocate their subdivision elements.

   if (debug && ret) {
      err_adv(0, "level %d: %d faces, updating %d of %d vertices using %s",
              subdiv_level(),
              _faces.num(),
              _dirty_verts.num(),
              _verts.num(),
              **_loc_calc->name());
      debug_check_verts("  before", _verts, _dirty_verts);
   }
   while (!_dirty_verts.empty()) {
      Lvert* v = (Lvert*)_dirty_verts.pop();
      v->clear_bit(Lvert::DIRTY_VERT_LIST_BIT);
      // See inline helper function update_sub(Lvert* v) above
      update_sub(v);
   }
   if (debug && ret) {
      debug_check_verts("  after", _verts, _dirty_verts);
   }

   if (0 && ret && debug)
      _subdiv_mesh->print();

   ret = _subdiv_mesh->update_subdivision(level-1) || ret;

   if (ret && is_control_mesh())
      changed(RENDERING_CHANGED);
   return ret;
}

inline void
update_verts(CBvert_list& verts)
{
   // helper for LMESH::update_subdivision(CBface_list& faces)
   for (int i=0; i<verts.num(); i++)
      ((Lvert*)verts[i])->update_subdivision();
}

inline void
update_edges(CBedge_list& edges)
{
   // helper for LMESH::update_subdivision(CBface_list& faces)
   for (int i=0; i<edges.num(); i++)
      ((Ledge*)edges[i])->update_subdivision();
}

inline void
update_faces(CBface_list& faces, bool debug)
{
   // helper for LMESH::update_subdivision(CBface_list& faces)

   err_adv(debug, "updating verts");
   update_verts(faces.get_verts());

   err_adv(debug, "updating edges");
   update_edges(faces.get_edges());

   err_adv(debug, "updating faces");
   for (int i=0; i<faces.num(); i++)
      allocate_child_faces((Lface*)faces[i]);
}

bool
LMESH::update_subdivision(CBface_list& faces, int k)
{
   static bool debug = Config::get_var_bool("DEBUG_BFACE_LIST_SUBDIVISION",false);

   if (debug) {
      cerr << "LMESH::update_subdivision: " << faces.num()
           << " faces to level "<< k << endl;
   }

   if (k  < 0)
      return false;

   if (faces.empty())
      return true;

   LMESH* m = LMESH::upcast(faces.mesh());
   if (!m) {
      err_adv(debug, "  error: non-LMESH");
      return false;
   }

   // Ensure mesh is up-to-date wrt controllers
   BMESHobs::broadcast_update_request(m);

   if (k == 0)
      return true;

   // generate lext-level mesh if needed
   if (!m->allocate_subdiv_mesh()) {
      err_adv(debug, "  error: can't allocate subdiv mesh");
      return false;
   }

   // update subdivision for the one-ring, 1-level down
   update_faces(faces.one_ring_faces(), debug);
   update_faces(faces.secondary_faces(), debug);

   assert(m->subdiv_mesh());
   m->subdiv_mesh()->changed(VERT_POSITIONS_CHANGED);

   // then recurse
   return update_subdivision(get_subdiv_faces(faces,1), k-1);
}

void
LMESH::subdivide_in_place()
{
   if (is_control_mesh()) {

      // produce the subdivision mesh
      refine();

      // copy it for safety
      // (blowing us away blows it away)
      LMESHptr sub = new LMESH;
      *sub = *_subdiv_mesh;

      // now blow us away and replace
      // with copy of subdivision mesh
      *this = *sub;
   } else
      err_msg("LMESH::subdivide_in_place: error: this not control mesh");
}

void
LMESH::delete_subdiv_mesh()
{
   // Notify observers that it's gonna happen
   BMESHobs::broadcast_sub_delete(this);

   if (is_control_mesh() && _subdiv_mesh) {
      int k;
      for (k = 0; k < nfaces(); k++)
         lf(k)->delete_subdiv_elements();
      for(k = 0; k < nedges(); k++)
         le(k)->delete_subdiv_elements();
      for(k = 0; k < nverts(); k++)
         lv(k)->delete_subdiv_vert();
      for(k = 0; k < _patches.num(); k++)
         ((Lpatch*)_patches[k])->clear_subdiv_strips(1);
   }
   if (is_control_mesh())
      set_cur_mesh(this);
   _subdiv_mesh = 0;    // deletes _subdiv_mesh thru ref-counting
}

void
LMESH::delete_elements()
{
   delete_subdiv_mesh();
   BMESH::delete_elements();
}

void 
LMESH::set_parent(LMESH* parent) 
{
   assert(parent);
   _parent_mesh = parent; 
   _subdiv_level = parent->subdiv_level() + 1;
   LMESH* c = control_mesh();
   assert(c != 0);
   if (c->has_name()) {
      // XXX - how to turn an int into a string without using str_ptr?
      set_name(c->get_name() + "-sub" + **str_ptr(_subdiv_level));
   }
}

void
LMESH::set_cur_mesh(LMESH* cur)
{
   if (is_control_mesh()) {
      if (_cur_mesh != cur) {
         bool coi = is_focus(_cur_mesh);
         _cur_mesh = cur;
         if (coi)
            set_focus(_cur_mesh);
         changed(RENDERING_CHANGED);
      }
   } else
      err_msg("LMESH::set_cur_mesh: error: called on non-control mesh");
}

void
LMESH::refine()
{
   // Called on control mesh.
   //
   // Effect is to assign:
   //   _cur_mesh = _cur_mesh->subdiv_mesh()

   if (!is_control_mesh())
      return;

   if (is_control_mesh()) {

      int new_level = _cur_mesh->subdiv_level() + 1;

      // do it:
      update_subdivision(new_level);
   }
}

void
LMESH::unrefine()
{
   if (is_control_mesh() && _cur_mesh != this) {
      set_cur_mesh(_cur_mesh->_parent_mesh);
      while (edit_level() > cur_level())
         dec_edit_level();
   }
}

void
LMESH::inc_edit_level()
{
   int prev_edit_level = edit_level();

   control_mesh()->BMESH::inc_edit_level();
   if (is_control_mesh() && cur_level() < edit_level())
      update_subdivision(edit_level()); // calls changed()
   else
      control_mesh()->changed(RENDERING_CHANGED);

   MeshGlobal::edit_level_changed(control_mesh(),
                                  prev_edit_level,
                                  control_mesh()->edit_level());
}

void
LMESH::dec_edit_level()
{
   int prev_edit_level = edit_level();

   control_mesh()->BMESH::dec_edit_level();
   control_mesh()->changed(RENDERING_CHANGED);

   MeshGlobal::edit_level_changed(control_mesh(),
                                  prev_edit_level,
                                  control_mesh()->edit_level());
}

bool
LMESH::set_edit_level(int level)
{
   // XXX make sure it doesnt go
   // past current subdiv level ?
   // ... john
                
   while (level > edit_level())
      control_mesh()->inc_edit_level();
   while (level < edit_level())
      control_mesh()->dec_edit_level();
        
   return true;
}

void
LMESH::mark_all_dirty()
{
   int i;
   for (i=0; i<nverts(); i++)
      lv(i)->mark_dirty(Lvert::SUBDIV_LOC_VALID_BIT);
   for (i=0; i<nedges(); i++)
      le(i)->clear_bit(Ledge::SUBDIV_LOC_VALID_BIT);
   if (_subdiv_mesh)
      _subdiv_mesh->mark_all_dirty();
}

void
LMESH::fit(ARRAY<Lvert*>& verts, bool do_gauss_seidel)
{
   static bool debug = Config::get_var_bool("DEBUG_LMESH_FIT",false);
   static bool move_along_normal =
      Config::get_var_bool("FITTING_MOVE_ALONG_NORMAL",false);

   if (verts.empty())
      return;

   // calculate the bounding box of the vertices
   BBOX box;
   int i;
   for (i=0; i<verts.num(); i++)
      box.update(verts[i]->loc());

   double max_err = box.dim().length() * 1e-5;

   int n = verts.num();

   // get original control point locations
   ARRAY<Wpt> C(n);   // original control points
   ARRAY<Wpt> L(n);   // current limit points
   for (i=0; i<n; i++) {
      C += verts[i]->loc();
      L += Wpt::Origin();
   }

   if(debug) {
      cerr << "LMESH::fit- fitting " << n << " vertices"<<endl;
      cerr << "Max_err = " << max_err <<endl;
   }

   // do 50 iterations...
   double prev_err = 0;
   ARRAY<double> errors;
   for (int k=0; k<50; k++) {

      errors.clear();

      double err = 0;

      if (do_gauss_seidel) {

         // Gauss-Seidel iteration: use updated values from the
         // current iteration as they are computed...
         for (int j=0; j<n; j++) {
            // don't need that L[] array...
            Wpt limit;
            verts[j]->limit_loc(limit);
            Wvec delt = C[j] - limit;
            errors+=delt.length();
            err += delt.length();
            if(move_along_normal)
               delt = delt*verts[j]->norm()*verts[j]->norm();
            verts[j]->offset_loc(delt);
         }

      } else {
         // compute the new offsets from the offsets computed in the
         // previous iteration
         int j;
         for (j=0; j<n; j++)
            verts[j]->limit_loc(L[j]);

         for (j=0; j<n; j++) {
            Wvec delt = C[j] - L[j];

            err += delt.length();
            errors+=delt.length();
            if(move_along_normal)
               delt = delt*verts[j]->norm()*verts[j]->norm();
            verts[j]->offset_loc(delt);
         }
      }
      // compute the average error:
      err /= n;

      double avg,std_d,max,min;
      if (debug) {
         if (prev_err != 0) {
            err_msg("Iter %d: avg error: %f, reduction: %f",
                    k, err, err/prev_err);
            statistics(errors,true,&avg,&std_d,&max,&min);
         } else {
            err_msg("Iter %d: avg error: %f", k, err);
            statistics(errors,true,&avg,&std_d,&max,&min);
         }
      } else
         statistics(errors,false,&avg,&std_d,&max,&min);

      prev_err = err;

      if (max < max_err) {
         if(debug) cerr << "Terminating at " << k <<" th iterations"<<endl;
         return;
      }
   }
}

inline void
add_p(Lvert* v, Bvert_list& vp, Bedge_list& ep)
{
   // Helper method used below in get_parents();
   // Given an Lvert, add its parent simplex to the
   // appropriate list depending on whether the parent
   // is an Lvert or Ledge.

   assert(v);

   Bsimplex* p = v->parent();
   if (!p)
      return;
   if (is_vert(p))
      vp += (Bvert*)p;
   else if (is_edge(p))
      ep += (Bedge*)p;
   else
      assert(0);
}

inline void
get_parents(CBvert_list& verts, Bvert_list& vp, Bedge_list& ep)
{
   // Helper method used below in get_parents();
   // from the given list of vertices, return the parent
   // simplices in two lists: one of Lverts, one of Ledges.

   if (verts.empty())
      return;

   assert(LMESH::isa(verts.mesh()));

   for (int i=0; i<verts.num(); i++)
      add_p((Lvert*)verts[i], vp, ep);
}

inline void
clear_face_flags(CBvert_list& verts)
{
   // Helper method used below in get_parents();
   // clear flags of faces adjacent to the given vertices,
   // including faces that are not stricly adjacent, but
   // that are part of a quad that contains the vertex.

   Bface_list star;
   for (int i=0; i<verts.num(); i++) {
      verts[i]->get_q_faces(star);
      star.clear_flags();
   }
}

inline void
try_append(Bface_list& A, Bface* f)
{
   // Helper method used below in try_append();

   if (f && !f->flag()) {
      f->set_flag();
      A += f;
   }
}

inline void
try_append(Bface_list& A, CBface_list& B)
{
   // Helper method used below in get_q_faces();
   // append faces from list B to list A, provided
   // the face flag is clear. Set the flag after
   // appending.

   for (int i=0; i<B.num(); i++)
      try_append(A, B[i]);
}

inline Bface_list
get_q_faces(CBvert_list& verts)
{
   // Helper method used below in get_parents();
   // return all faces adjacent to the given vertices,
   // including faces that are not stricly adjacent, but
   // that are part of a quad that contains the vertex.

   Bface_list ret, star;
   for (int i=0; i<verts.num(); i++) {
      verts[i]->get_q_faces(star);
      try_append(ret, star);
   }
   return ret;
}

Bvert_list
LMESH::get_subdiv_inputs(CBvert_list& verts)
{
   static bool debug = Config::get_var_bool("DEBUG_LMESH_SUBDIV_INPUTS",false);

   // Given a set of vertices from the same LMESH, return
   // the vertices of the parent LMESH that affect the
   // subdivision locations of the given vertices.

   // Require verts share common LMESH
   // XXX - could relax this, provided we test each Bvert
   //       to ensure it is really an Lvert.
   if (!isa(verts.mesh()))
      return Bvert_list();

   // Get direct parent vertices and edges
   Bvert_list vp;       // vertex parents
   Bedge_list ep;       // edge parents
   get_parents(verts, vp, ep);

   err_adv(debug, "%d verts: parents: %d verts, %d edges",
           verts.num(), vp.num(), ep.num());

   // Clear flags of all adjacent faces
   clear_face_flags(vp);
   ep.clear_flag02();

   // Put all adjacent faces into a list
   Bface_list faces = get_q_faces(vp);
   err_adv(debug, "parent faces from verts: %d", faces.num());
   try_append(faces, ep.get_primary_faces());
   err_adv(debug, "parent faces from edges too: %d", faces.num());

   // Pull out the vertices:
   return faces.get_verts();
}

void
LMESH::send_update_notification()
{
   // Send controllers a message to do their mesh modifications
   // now:
   BMESH::send_update_notification();

   // If that did anything, subdivision meshes may be out of
   // date. Update them if needed:
   update();
}

inline LMESHptr
bmesh_to_lmesh(BMESHptr mesh)
{
   // If the given mesh is an LMESH, return it.
   // Otherwise allocate a new LMESH, copy the data
   // from the given mesh, and return the LMESH.

   if (!mesh) return 0;
   LMESHptr ret = LMESH::upcast(&*mesh);
   if (!ret) {
      ret = new LMESH;
      *ret = *mesh;
   }
   return ret;
}

LMESHptr
LMESH::read_jot_stream(istream& in)
{
   // Convenience method to read a mesh from a given stream
   // and return it as an LMESH:

   return bmesh_to_lmesh(BMESH::read_jot_stream(in));
}

LMESHptr
LMESH::read_jot_file(char* filename)
{
   // Convenience method to read a mesh from a given stream
   // and return it as an LMESH:

   return bmesh_to_lmesh(BMESH::read_jot_file(filename));
}

CTAGlist &
LMESH::tags() const
{
   return BMESH::tags();
}

// end of file lmesh.C
