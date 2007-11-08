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
 * meme.C:
 **********************************************************************/
#include "mesh/mi.H"
#include "std/config.H"
#include "meme.H"
#include "bbase.H"
#include "primitive.H"

using namespace mlib;

/*******************************************************
 * Meme
 *******************************************************/
Meme::Meme(Bbase* base, Bsimplex* s, bool force_boss) :
   SimplexData(0, 0),
   _owner(base) 
{
   if (s) {
      Meme* cur_boss = Bbase::find_boss_meme(s);

      // kick out the old boss if needed
      if (force_boss && cur_boss) {
         cur_boss->get_demoted();
         cur_boss = 0;
      }
      SimplexData::set(cur_boss ? (uint)_owner : Bbase::key(), s);
   }
}

int 
Meme::bnode_num() const
{
   return _owner->bnode_num(); 
}

bool
Meme::take_charge()
{
   // Become the boss meme of the simplex.
   // May need to kick out the current boss.

   // Must be true:
   assert(_simplex);
   assert(_owner);

   // Find the current boss
   Meme* cur_boss = Bbase::find_boss_meme(_simplex);
   if (cur_boss == this) {
      // (Meet the new boss, same as the old boss.)
      return 0;
   } else if (cur_boss) {
      cur_boss->get_demoted();
   }

   // Now there should be no boss for sure
   assert(!Bbase::find_boss_meme(_simplex));

   // Set our lookup key to the boss ID:
   _id = Bbase::key();

   bbase()->invalidate();

   return true;
}

bool
Meme::get_demoted()
{
   // Must be true:
   assert(_simplex);
   assert(_owner);

   static bool debug = Config::get_var_bool("DEBUG_MEME_DEMOTION",false);
   err_adv(debug, "%s::get_demoted", **class_name());

   if (_id == (uint)_owner) {
      // Already been demoted
      return false;
   }

   // We must have the boss ID now:
   assert(_id == Bbase::key());

   // The lookup key we'll switch to cannot already be in use:
   assert(!_simplex->find_data((uint)_owner));

   // make the switch
   _id = (uint)_owner;

   // Notify Bbase
   bbase()->invalidate();

   return true;
}

bool 
Meme::is_boss() const
{
   // Each Meme is stored on its Bsimplex using either the Bbase
   // class name as the lookup key, or by the address of its
   // Bbase owner. The first method is used when the Meme is the
   // "boss" of the Bsimplex, which means it alone (among Memes)
   // can modify the Bsimplex. The other Memes are there just for
   // informational purposes (i.e. to store info).

   return (_id == Bbase::key());
}

void 
Meme::notify_split(Bsimplex* new_simp)
{
   // An edge can split or a face can split. In either case new
   // simplices are generated. For each new simplex one
   // "notify_split" call is made to the splitting simplex, with
   // the newly created simplex passed as an argument. Here we
   // let the Bbase generate new memes on the new simplex..

   bbase()->notify_simplex_split(this, new_simp);
}

/*******************************************************
 * VertMeme
 *******************************************************/
VertMeme::VertMeme(Bbase* b, Lvert* v, bool force_boss) :
   Meme(b, v, force_boss),
   _is_sterile(false),
   _is_pinned(false),
   _cold_count(max_cold_count()),
   _debug(false),
   _suspend_change_notification(false)
{
   // Vert memes add themselves to the Bbase vert meme list:
   if (bbase())
      bbase()->add_vert_meme(this);
}

bool 
VertMeme::move_to(CWpt&) 
{
   cerr << class_name() << "::move_to: not implemented" << endl;
   return false; 
}

void
VertMeme::mark_parent_dirty() const
{
   // Ensure the parent Lvert or Ledge gets scheduled to recompute
   // subdiv loc for our vertex

   Bsimplex* p = get_parent_simplex(vert());
   if (is_vert(p)) {
      Lvert* lv = ((Lvert*)p);
      lv->mark_dirty(Lvert::SUBDIV_LOC_VALID_BIT);
      for (int i=0; i<lv->degree(); i++)
         lv->le(i)->clear_bit(Ledge::SUBDIV_LOC_VALID_BIT);

      // XXX - doesn't work. misses weak edges of adjacent
      // quads half the time

   } else if (is_edge(p)) {
      Ledge* le = ((Ledge*)p);
      le->lv(1)->mark_dirty(Lvert::SUBDIV_LOC_VALID_BIT);
      le->lv(2)->mark_dirty(Lvert::SUBDIV_LOC_VALID_BIT);
      le->clear_bit(Ledge::SUBDIV_LOC_VALID_BIT);
   }
}

bool
VertMeme::get_demoted()
{
   if (Meme::get_demoted()) {
      // ensure this vert will get subdivision loc recomputed:
      mark_parent_dirty();

      // Any existing child needs to hear the bad news:
      VertMeme* c = child();
      if (c)
         c->get_demoted();

      return 1;
   }
   return 0;
}

void
VertMeme::pin()
{
   if (!_is_pinned) {
      _is_pinned = true;
      VertMeme* c = child();
      if (c)
         c->pin();
   }
}

void
VertMeme::unpin()
{
   if (_is_pinned) {
      _is_pinned = false;
      VertMeme* c = child();
      if (c)
         c->unpin();
   }
}

bool
VertMeme::take_charge()
{
   if (Meme::take_charge()) {
      mark_parent_dirty();

      return 1;
   }
   return 0;
}

void
VertMeme::sterilize()
{
   // if sterile, this meme can be a "boss," (change the vert loc)
   // but it can't generate boss children.

   if (_is_sterile)
      return;

   // record new state:
   _is_sterile = true;

   // Any existing child needs to hear the bad news:
   VertMeme* c = child();
   if (c)
      c->get_demoted();
}

void
VertMeme::unsterilize()
{
   if (!_is_sterile)
      return;

   // record new state:
   _is_sterile = false;

   VertMeme* c = child();
   if (c && bbase()->res_level() > 0)
      c->take_charge();
}

bool
VertMeme::tracks_boss() 
{
   // Return true if:
   //
   //   * this meme is not the boss, but
   //   * it agrees with the current boss
   //     about where to put the vertex.

   if (is_boss())
      return false;

   if (!vert())
      return false;     // Or assert(0)?

   // get a scale for what is "close"
   static const double THRESH_SCALE =
      Config::get_var_dbl("VERT_MEME_THRESH_SCALE", 1e-1,true);
   double thresh = vert()->avg_edge_len()*THRESH_SCALE;

   VertMeme* boss = Bbase::find_boss_vmeme(vert());

   // the point we have to match:
   Wpt p = boss ? boss->compute_update() : loc();

   return (compute_update().dist(p) < thresh);
}

bool
VertMeme::is_sterile() const
{
   return _is_sterile || bbase()->res_level() < 1;
}

bool 
VertMeme::is_potent() const
{
   // Is this meme allowed to replicate boss subdiv memes?

   return (!is_sterile() && is_boss());
}

bool
VertMeme::apply_update(double thresh) 
{
   // Apply the update if it will change anything, and if so
   // return true.

   if (!is_boss())
      return false;

   // convert thresh from fraction of edge length
   // to length in 3D
   thresh = max(thresh, 0.0) * vert()->avg_edge_len();
   if (_update.dist(loc()) > thresh) {
      _suspend_change_notification = true;
      vert()->set_loc(_update);
      _suspend_change_notification = false;
      set_hot(); // stay scheduled for iterative relaxation
      return true;
   }
   return false;
}

void
VertMeme::set_hot()
{
   _cold_count = 0; 
   bbase()->Bnode::activate();
}

void 
VertMeme::vert_changed_externally()
{
   // Called when the vertex location changed but this VertMeme
   // didn't cause it:
   if (do_debug())
      cerr << class_name() << "::vert_changed_externally" << endl;
}

void 
VertMeme::notify_simplex_changed()
{
   if (!_suspend_change_notification) {

      // The vertex was repositioned by some agent other than
      // this meme. The following lets subclasses update internal
      // data to match the new vertex location.

      if (do_debug()) {
         cerr << class_name() << "::notify_simplex_changed" << endl;
      }
      vert_changed_externally();
      set_hot();
   }
}

void 
VertMeme::notify_normal_changed()
{
   set_hot();
}

VertMeme* 
VertMeme::nbr(int k) const 
{
   assert(bbase());
   return bbase()->find_vert_meme(vert()->nbr(k));
}

void 
VertMeme::get_nbrs(ARRAY<VertMeme*>& ret) const 
{
   assert(vert());
   ret.clear();
   for (int i=0; i<vert()->degree(); i++) {
      VertMeme* n = nbr(i);
      if (n)
         ret += n;
   }
}

void
VertMeme::get_nbrs(ARRAY<EdgeMeme*>& ret) const
{
   // Return adjacent EdgeMemes that have the same owner as this.

   ret.clear();
   if (vert()) {
      EdgeMeme* m;
      for (int i=0; i<vert()->degree(); i++)
         if ((m = _owner->find_edge_meme(vert()->e(i))))
            ret += m;
   }
}

void
VertMeme::get_nbrs(ARRAY<FaceMeme*>& ret) const
{
   // Return adjacent FaceMemes that have the same owner as this.

   ret.clear();
   if (vert()) {
      static ARRAY<Bface*> faces;
      vert()->get_faces(faces);
      FaceMeme* m;
      for (int i=0; i<faces.num(); i++)
         if ((m = _owner->find_face_meme(faces[i])))
            ret += m;
   }
}

// spring_delt():
//
//   Used below in VertMeme::target_delt(). Computes the delta
//   used in relaxation to move the "center" toward the
//   "neighbor" with a strength weighted by the ratio of the
//   current length to the rest length.
inline Wvec
spring_delt(
   CWpt& c,     // center
   CWpt& n,     // neighbor
   double L     // rest length
   )
{
   return (n - c)*(n.dist(c)/L);
}

inline Wvec
spring_delt(CBvert* v, CBedge* e, double L)
{
   assert(v && e);
   Bvert* nbr = e->other_vertex(v);
   assert(nbr);
   return spring_delt(v->loc(), nbr->loc(), L);
}

uchar 
VertMeme::max_cold_count()
{
   static uchar ret = (uchar) Config::get_var_int("MAX_COLD_ITERS", 8);
   return ret;
}

inline Bvert_list
get_adjs(const Bvert* v, CBedge_list& edges)
{
   // The edges must contain vertex v;
   // Returns corresponding opposite verts.

   assert(v);
   Bvert_list ret(edges.num());
   for (int i=0; i<edges.num(); i++) {
      assert(v->nbr(edges[i]));
      ret += v->nbr(edges[i]);
   }
   return ret;
}

Wpt
VertMeme::smooth_target() const
{
   // Return the delta to the target location for being more
   // "relaxed"

   Bvert* v = vert();
   assert(v);

   Bedge_list all_edges = v->get_adj();
   Bedge_list star;
   for (int k = 0; k < all_edges.num(); k++) {
      Bsurface* p = Bsurface::find_controller(all_edges[k]);
      if (p && (p->name()=="roof_panel") && (Bbase::find_controller(v)!=p))
         continue;
      if (Primitive::isa(p) && ((Primitive*)p)->is_roof())
         continue;
      star += all_edges[k];
   }
   bool special = (star.num()!=all_edges.num());
   if (!special)
      star = v->get_manifold_edges(); // work with primary layer edges

   static bool do_simple = true;
//      !Config::get_var_bool("DO_SPRINGY_SMOOTHING",false);

   if (do_simple) {
      // forget the hooey below, using a simple approach here:
      if (special)
         return get_adjs(v, star).center();
      return v->qr_centroid();
   }

   //              q -------- r -------- q              
   //              |          |          |            
   //              |          |          |            
   //              |          |          |            
   //              |          |          |            
   //              |          |          |            
   //              r -------- v -------- r              
   //              |         / \         |               
   //              |       /     \       |              
   //              |     /         \     |               
   //              |   /             \   |               
   //              | /                 \ |               
   //             r -------------------- r  
   // 
   //    There are 2 kinds of "neighboring" vertices:
   //    regular (r) vertices, joined to this vertex by a
   //    strong edge, and quad (q) vertices, which lie at
   //    the opposite corner of a quad from this vertex. 
   //    q vertices are not necessarily connected to this
   //    vertex by an edge. r vertices are weighted by 1,
   //    q vertices are weighted by 1/2.

   Wvec ret;                    // weighted vector to centroid
   CWpt& c = v->loc();          // location of this vertex
   double w = 0;                // net weight of centroid components

   for (int i=0; i<star.num(); i++) {
      Bedge* e = star[i];
      if (e->is_strong()) {
         // add contribution from r neighbor
	 ret += spring_delt(v, e, lookup_rest_length(e));
         w += 1.0;
         Bface* f = ccw_face(e, v);
         if (f && f->is_quad()) {
            // add contribution from q neighbor
            Bvert* q = f->quad_opposite_vert(v);
            ret += 0.5*spring_delt(c, q->loc(),lookup_rest_length(f->weak_edge()));
            w += 0.5;
         }
      }
   }
   if (w > 0)
      ret /= w;

   return c + ret;
}

Wvec
VertMeme::target_delt() const
{
   // Return the delta to the target location for being more
   // "relaxed"

   static double d = Config::get_var_dbl("SMOOTHING_DAMP", 0.5, true);
   return (smooth_target() - loc())*d;
}

void
VertMeme::notify_simplex_deleted()
{
   assert(bbase() != NULL);
   err_adv(Config::get_var_bool("DEBUG_MEME_DESTRUCTOR", false),
           "%s::~%s: level %d",
           **class_name(), **class_name(), bbase()->bbase_level());
   bbase()->rem_vert_meme(this);
}

VertMeme*
VertMeme::parent() const
{
   // Return the "parent" vert meme if it exists.

   Lvert* v = parent_vert();
   if (!v) return 0;
   Bbase* c = bbase()->parent();
   return c ? c->find_vert_meme(v) : 0;
}

VertMeme*
VertMeme::child() const
{
   // Return the "child" meme if it exists.
   // (I.e., the meme created by this meme, on the subdiv vert)

   Bbase* c = bbase()->child();
   return c ? c->find_vert_meme(subdiv_vert()) : 0;
}

VertMeme*
VertMeme::get_child() const 
{
   // Return the meme of the subdiv vert;
   // create it if necessary

   // need both of these to do anything:
   Bbase* cbase = bbase()->child();
   Lvert* svert = subdiv_vert();
   if (!(cbase && svert))
      return 0;

   // Return the child meme if it's there:
   VertMeme* ret = child();
   if (!ret)
      ret = _gen_child(svert);
   if (ret && !this->is_potent() && ret->is_boss())
      ret->get_demoted();
   return ret;
}

VertMeme*
VertMeme::update_child() const
{
   // Update the child meme -- creating it if needed.
   // (E.g., called when attributes of this meme have changed):

   // (The child inherits attributes (e.g. uv-coords) from
   //  this meme.  When attributes of this meme change, the
   //  child needs to get updated.)

   // Base class method - subclasses probably over-ride it
   // to do the following, plus extra stuff.

   // Get hold of the child:
   VertMeme* ret = get_child();

   if (ret)
      ret->update_from_parent();

   // Return the child:
   return ret;
}

void 
VertMeme::update_from_parent()
{
   Lvert* lv = vert();
   if (!lv)
      return;
   Bsimplex* ps = lv->parent();         // parent simplex
   Bbase*    pb = bbase()->parent();    // parent Bbase
   if (!(ps && pb))
      return;
   if (is_vert(ps)) {
      copy_attribs_v(pb->find_vert_meme((Bvert*)ps));
      return;
   }
   assert(is_edge(ps));
   Bedge* e = (Bedge*)ps;
   if (e->is_strong())
      copy_attribs_e(
         pb->find_vert_meme(e->v1()),
         pb->find_vert_meme(e->v2())
         );
   else {
      Bface* q = e->f1();
      assert(q && q->is_quad());
      Bvert *a, *b, *c, *d;
      q->get_quad_verts(a,b,c,d);
      copy_attribs_q(
         pb->find_vert_meme(a),
         pb->find_vert_meme(b),
         pb->find_vert_meme(c),
         pb->find_vert_meme(d)
         );
   }
}

bool
VertMeme::handle_subdiv_calc()
{
   // If our Bbase has a child that keeps a boss vert meme
   // on the subdivision vertex, that meme will be
   // responsible for computing the location of the
   // subdivision vertex. So we say "We'll handle this"
   // (it's an Lvert asking the question), though in fact
   // the child meme will really do the work:

   // Get the child meme (create it if needed)
   // and update its values:
   VertMeme* vm = get_child();

   // Return true if the child is controlling the vertex.
   // I.e. the child is taking on the job of computing the
   // subdiv vertex location:
   return (vm && vm->is_boss());
}

void 
VertMeme::notify_subdiv_gen() 
{
   // SimplexData virtual method:
   //
   // The vertex just generated its subdivision vertex.
   // This meme now has a chance to generate a child meme
   // on the subdivision vertex.

   gen_subdiv_memes();
}

void
VertMeme::gen_subdiv_memes() const
{
   // Meme virtual method

   // Generate a child meme on the subdiv vert if needed
   get_child();
}

VertMeme*
VertMeme::gen_child(Lvert* lv, VertMeme* v1, VertMeme* v2)
{
   // Static method to screen for the legality of generating a
   // child from the given vert memes. This version is used to
   // generate the vert meme for the subdivision vertex of a
   // strong edge. (I.e. not a quad diagonal).

   if (!(lv && v1 && v2))
      return 0;

   // This is already screened in EdgeMeme::notify_subdiv_gen(),
   // but assert it anyway:
   assert(v1->bbase() == v2->bbase() && v1->bbase()->child() != NULL);

   // They have to be the same type.
   if (v1->class_name() != v2->class_name())
      return 0;

   Bbase* c = v1->bbase()->child();
   VertMeme* ret = c->find_vert_meme(lv);
   if (!ret)
      ret = v1->_gen_child(lv, v2);
   // skipping the following for now:
   if (0 && ret) {
      bool p1 = v1->is_epotent();
      bool p2 = v2->is_epotent();
      if (p1 && p2) {
         // leave the child alone
      } else if (!(p1 || p2)) {
         // parents are a weak bunch -- child can't be a boss
         ret->get_demoted();
      } else {
         // parents are mixed -- child can be a boss but can't
         // propagate its own children
         ret->sterilize();
      }
   }
   return ret;
}

VertMeme*
VertMeme::gen_child(
   Lvert* lv,
   VertMeme* v1,
   VertMeme* v2,
   VertMeme* v3,
   VertMeme* v4)
{
   static bool debug = Config::get_var_bool("DEBUG_MEME_SUBDIV_GEN",false);

   // Static method to screen for the legality of generating a
   // child vert meme from the given vert memes. This version
   // is used to generate the vert meme for the subdivision
   // vertex of an edge internal to a quad. The 4 vert memes
   // are from the 4 corners of the quad.

   if (!(lv && v1 && v2 && v3 && v4)) {
      err_adv(debug, "VertMeme::gen_child: rejecting null pointer(s)");
      return 0;
   }

   // This is already screened in EdgeMeme::notify_subdiv_gen(),
   // but assert it anyway:
   assert(v1->bbase() == v2->bbase() &&
          v1->bbase() == v3->bbase() &&
          v1->bbase() == v4->bbase() &&
          v1->bbase()->child() != NULL);

   // They have to be the same type.
   if (v1->class_name() != v2->class_name() ||
       v1->class_name() != v3->class_name() ||
       v1->class_name() != v4->class_name()) {
      err_adv(debug, "VertMeme::gen_child: rejecting mixed meme types");
      return 0;
   }

   Bbase* c = v1->bbase()->child();
   if (!c)
      return 0;
   VertMeme* ret = c->find_vert_meme(lv);
   if (!ret)
      ret = v1->_gen_child(lv, v2, v3, v4);
   if (ret) {
      bool p1 = v1->is_epotent();
      bool p2 = v2->is_epotent();
      bool p3 = v3->is_epotent();
      bool p4 = v4->is_epotent();
      if (p1 && p2 && p3 && p4) {
         // leave the child alone
      } else if (!(p1 || p2 || p3 || p4)) {
         // parents are a weak bunch -- child can't be a boss
         ret->get_demoted();
      } else {
         // parents are mixed -- child can be a boss but can't
         // propagate its own children
         ret->sterilize();
      }
   }
   return ret;
}

double 
VertMeme::lookup_rest_length(CBedge* e) const
{
   EdgeMeme* em = 0;
   if (bbase() && (em = bbase()->find_edge_meme(e)))
      return em->rest_length();
   return EdgeMeme::lookup_rest_length(e);
}

/*******************************************************
 * EdgeMeme
 *******************************************************/
EdgeMeme::EdgeMeme(Bbase* b, Ledge* e, bool force_boss) :
   Meme(b, e, force_boss),
   _rest_length(0)
{
   if (bbase())
      bbase()->add_edge_meme(this);
}

double 
EdgeMeme::lookup_rest_length(CBedge* e)
{
   EdgeMeme* em = Bbase::find_boss_ememe(e);

   return em ? em->rest_length() : local_length(e);
}

double 
EdgeMeme::local_length(CBedge* e)
{
   // compute a value for rest length from current
   // conditions near the edge:

   // safety first
   if (!e) return 0;

   // regular edge: take average of adjacent face edge lengths:
   if (e->is_strong())
      return avg_face_edge_len(e);

   // "weak" edge (quad diagonal):
   // get the quad
   Bface* f = e->f1();
   assert(f && f->is_quad());

   // take sqrt(2) * average length of quad sides:
   return M_SQRT2*f->quad_avg_dim();
}

VertMeme*
EdgeMeme::v1() const
{
   return bbase()->find_vert_meme(edge()->v1()); 
}

VertMeme*
EdgeMeme::v2() const
{
   return bbase()->find_vert_meme(edge()->v2()); 
}

VertMeme*
EdgeMeme::vf1() const 
{
   return bbase()->find_vert_meme(edge()->opposite_vert1()); 
}

VertMeme*
EdgeMeme::vf2() const 
{
   return bbase()->find_vert_meme(edge()->opposite_vert2()); 
}

void
EdgeMeme::notify_simplex_deleted()
{
   assert(bbase() != NULL);
   err_adv(Config::get_var_bool("DEBUG_MEME_DESTRUCTOR", false),
           "%s::~%s: level %d",
           **class_name(), **class_name(), bbase()->bbase_level());
   bbase()->rem_edge_meme(this);
}

VertMeme*
EdgeMeme::child() const
{
   // Return the "child" vert meme if it exists.
   // (I.e., the vert meme on the subdiv vert of this edge)

   Bbase* c = bbase()->child();
   return c ? c->find_vert_meme(subdiv_vert()) : 0;
}

EdgeMeme*
EdgeMeme::child_e1() const
{
   // Return the first child edge meme if it exists.
   // (I.e., the edge meme on subdiv_e1() of this edge)

   Bbase* c = bbase()->child();
   return c ? c->find_edge_meme(subdiv_e1()) : 0;
}

EdgeMeme*
EdgeMeme::child_e2() const
{
   // Return the first child edge meme if it exists.
   // (I.e., the edge meme on subdiv_e2() of this edge)

   Bbase* c = bbase()->child();
   return c ? c->find_edge_meme(subdiv_e2()) : 0;
}

VertMeme*
EdgeMeme::get_child() const 
{
   // Return the meme of the subdiv vert;
   // create it if necessary

   // XXX - 12/2002
   //   having trouble getting this to work right...
   //   the problem is that the situation can change at
   //   a given level k (e.g. verts get demoted), then
   //   at levels > k some previously-generated memes
   //   should be affected (e.g. demoted too). here
   //   we're going with the inefficient solution that
   //   at least works:
   gen_subdiv_memes();

   // Return the child vert meme:
   return child();
}

VertMeme*
EdgeMeme::update_child() const
{
   // Update the child vert meme -- creating it if needed.

   // The child inherits attributes (e.g. uv-coords) from
   // the vert memes around this edge.  When attributes of
   // those memes change, the child needs to get updated.

   // Get hold of the child:
   VertMeme* ret = get_child();

   if (ret)
      ret->update_from_parent();

   // Return the child:
   return ret;
}

bool
EdgeMeme::handle_subdiv_calc()
{
   // If our Bbase has a child that keeps a boss vert meme
   // on the subdivision vertex, that meme will be
   // responsible for computing the location of the
   // subdivision vertex. So we say "We'll handle this"
   // (it's an Ledge asking the question), though in fact
   // the child meme will really do the work:

   // Get the child meme (create it if needed)
   // and update its values:
   VertMeme* vm = get_child();

   // Return true if the child is controlling the vertex.
   // I.e. the child is taking on the job of computing the
   // subdiv vertex location:
   return (vm && vm->is_boss());
}

void 
EdgeMeme::notify_subdiv_gen() 
{
   // Callback for when our edge has generated its
   // subdivision elements. Ensure memes are generated on
   // the two subdivision edges and the subdivision vertex.

   gen_subdiv_memes();
}

void
EdgeMeme::propagate_length(double len) const
{
   // Pass on the given rest length to both child edges

   EdgeMeme* e = 0;
   if ((e = child_e1()))
      e->set_rest_length(len);
   if ((e = child_e1()))
      e->set_rest_length(len);
}

void
EdgeMeme::set_rest_length(double len)
{
   if (_rest_length != len) {
      _rest_length = len;
      propagate_length(len/2);
   }
}

void
EdgeMeme::gen_subdiv_memes() const
{
   // Ensure edge memes are created at the two subdivision
   // edges, and generate a vert meme on the subdivision
   // vertex of our edge. That vert meme should inherit
   // properties of adjacent vert memes of our Bbase.

   static bool debug = Config::get_var_bool("DEBUG_MEME_SUBDIV_GEN",false);

   // If it has a child it's propagating:
   Bbase* c = bbase()->child();
   if (!c) {
      static bool warned = false;
      if (!warned) {
         err_adv(debug, "EdgeMeme::gen_subdiv_memes: no child bbase of %s",
                 **bbase()->class_name());
         err_adv(debug, "  (suppressing further warnings)");
         warned = 1;
      }
      return;
   }

   // Propagate two edge memes
   // (it's a no-op if they exist already):
   c->add_edge_meme(subdiv_e1());
   c->add_edge_meme(subdiv_e2());
   if (has_rest_length())
      propagate_length(_rest_length/2);

   // Propagate a vert meme:
   VertMeme* vm = 0;
   if (edge()->is_weak()) {
      // It's the interior edge of a quad.
      // Do 4-way averaging of the quad vert memes:
      vm = VertMeme::gen_child(subdiv_vert(), v1(), v2(), vf1(), vf2());
   } else {
      // Do 2-way averaging of vert memes:
      vm = VertMeme::gen_child(subdiv_vert(), v1(), v2());
   }

   // Don't let it get delusions of grandeur if this edge is
   // not a boss:
   if (!is_boss() && vm && vm->is_boss())
      vm->get_demoted();

   if (vm && debug) {
      bool p1 = v1()->is_epotent();
      bool p2 = v2()->is_epotent();
      err_adv(!(p1 && p2), "%s edge: child is: %s",
              XOR(p1, p2) ? "mixed" : "wimpy",
              vm->is_boss() ? (vm->is_sterile() ? "sterile" : "boss") :
              "non-boss");
   }
}

int  
EdgeMeme::length_ratio_decreasing(const void* a, const void* b)
{
   // Static method used to sort edges in decreasing order
   // by their "length ratio" (actual length divided by
   // desired length). Not used currently.

   EdgeMeme* m1 = *((EdgeMeme **)a);
   EdgeMeme* m2 = *((EdgeMeme **)b);

   return Sign2(m2->length_ratio() - m1->length_ratio());
}

/*******************************************************
 * FaceMeme
 *******************************************************/
FaceMeme::FaceMeme(Bbase* b, Lface* v) :
   Meme(b, v, true) 
{
   if (bbase())
      bbase()->add_face_meme(this);
}

VertMeme*
FaceMeme::v1() const
{
   return bbase()->find_vert_meme(face()->v1()); 
}

VertMeme*
FaceMeme::v2() const
{
   return bbase()->find_vert_meme(face()->v2()); 
}

VertMeme*
FaceMeme::v3() const
{
   return bbase()->find_vert_meme(face()->v3()); 
}

VertMeme*
FaceMeme::vq() const
{
   // Returns NULL if the face is not a quad:
   return bbase()->find_vert_meme(face()->quad_vert());
}

void
FaceMeme::notify_simplex_deleted()
{
   assert(bbase() != NULL);
   err_adv(Config::get_var_bool("DEBUG_MEME_DESTRUCTOR", false),
           "%s::~%s: level %d",
           **class_name(), **class_name(), bbase()->bbase_level());
   bbase()->rem_face_meme(this);
}

void
FaceMeme::notify_subdiv_gen()
{
   // The face has just generated its subdivision elements.
   // Now propagate edge and face memes in the subdivision mesh.

   // If the Bbase won't do it, we'll handle it pro-actively:
   gen_subdiv_memes();
}

void
FaceMeme::gen_subdiv_memes() const
{
   Bbase* c = bbase()->child();
   if (!c)
      return;

   // Propagate edge memes at 3 internal subdiv edges:
   c->add_edge_meme(subdiv_e1());
   c->add_edge_meme(subdiv_e2());
   c->add_edge_meme(subdiv_e3());

   // Propagate face memes at 4 subdiv faces:
   c->add_face_meme(subdiv_f1());
   c->add_face_meme(subdiv_f2());
   c->add_face_meme(subdiv_f3());
   c->add_face_meme(subdiv_fc());
}


int
VertMemeList::meme_count(Cstr_ptr& class_name)
{
   int ret = 0;
   for (int i=0; i<_num; i++)
      if (_array[i]->is_of_type(class_name))
         ret++;
   return ret;
}

/* end of file meme.C */
