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
 * rsurf.C:
 **********************************************************************/
#include "disp/colors.H"
#include "geom/world.H"
#include "gtex/util.H"
#include "std/config.H"

#include "tess/tess_debug.H"
#include "tess/ti.H"
#include "tess/rsurf.H"

using namespace mlib;

static bool debug = Config::get_var_bool("DEBUG_RSURF",false);

Rsurface_list Rsurface::_rsurfs;

/*****************************************************************
 * RmemeVertPair:
 *
 *  Specialized VertPair to send notifications to an Rmeme
 *****************************************************************/
class RmemeVertPair : public VertPair {
 public:
   //******** MANAGERS ********

   // using key, record data on v, associating to partner p
   RmemeVertPair(uint key, Bvert* v, Rmeme* r) :
      VertPair(key, v, r->vert()), _r(r) { assert(r != NULL); }

   //******** RUN TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("RmemeVertPair", RmemeVertPair*,
                        VertPair, CSimplexData*);

   //******** LOOKUPS ********

   // get the RmemeVertPair on v
   static RmemeVertPair* find_pair(uint key, CBvert* v) {
      return upcast(VertPair::find_pair(key, v));
   }

   //******** SimplexData VIRTUAL METHODS ********

   // Notification that the vertex moved
   virtual void notify_simplex_changed()  {
      if (_r)
         _r->ref_vert_changed();
   }

   // Notification that the vertex normal changed
   virtual void notify_normal_changed()   {
      if (_r)
         _r->ref_vert_changed();
   }

   // Notification that the ref vert has been deleted:
   virtual void notify_simplex_deleted()  {
      if (_r)
         _r->ref_vert_deleted();
      VertPair::notify_simplex_deleted();       // deletes this
   }

 protected:
   Rmeme* _r; // the rmeme that gets notified
};

/*****************************************************************
 * RFacePair:
 *
 *  Specialized FacePair to send notifications to an Rsurface
 *****************************************************************/
class RFacePair : public FacePair {
 public:
   //******** MANAGERS ********

   // using key, record data on v, associating to partner p
   RFacePair(Bface* f, Bface* p, Rsurface* rsurf) :
      FacePair(rsurf->pair_lookup_key(), f, p), _rsurf(rsurf) {
      assert(rsurf != NULL);
   }

   //******** RUN TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("RFacePair", RFacePair*, FacePair, CSimplexData*);

   //******** SimplexData VIRTUAL METHODS ********

   // Notification that the ref vert has been deleted:
   virtual void notify_simplex_deleted()  {
      if (_rsurf)
         _rsurf->ref_face_deleted(face());
      SimplexData::notify_simplex_deleted();       // deletes this
   }

 protected:
   Rsurface* _rsurf; // the Rsurface that gets notified
};

/*****************************************************************
 * HybridDisp
 *****************************************************************/
class HybridDisp : public HybridCalc<double> {
 public:
   HybridDisp(Bbase* bb) : _bb(bb) {}
   
   //******** VALUE ACCESSORS ********
   virtual double get_val(CBvert* v)   const;

   // need to teach how to clear a double:
   virtual void clear(double& x) const { x = 0; }

   //******** DUPLICATING ********
   virtual SubdivCalc<double> *dup() const { return new HybridDisp(_bb); }

 protected:
   Bbase* _bb; // bbase to use to find Rmemes
};

double
HybridDisp::get_val(CBvert* v) const
{
   // find the Rmeme on vertex v
   // (has to belong to our _bbase)

   if (!_bb) {
      err_adv(debug, "HybridDisp::get_val: no bbase");
      return 0;
   }
   Rmeme* r = Rmeme::upcast(_bb->find_vert_meme(v));
   if (!r) {
      err_adv(debug, "HybridDisp::get_val: lookup Rmeme failed");
      return 0;
   }
   return r->h();
}

/*****************************************************************
 * Rmeme:
 *****************************************************************/

// constructor for Rmeme of the control surface:
Rmeme::Rmeme(
   Bbase* b,             // Rsurface
   Lvert* v,             // vertex of thememe
   uint pair_lookup_key, // key for accessing vert pairs
   Lvert* ref,           // reference vert on side-A
   double h,             // offset amount
   double s,             // additional scale factor (used at creases)
   bool scale_h ) :      // Local or global length value
   VertMeme(b, v),
   _ref(ref),
   _h(0),       // set below
   _s(s), 
   _pair_key(pair_lookup_key)
{
   assert(_ref);

   gen_pair();

   _h = h;//*avg_strong_edge_len(_ref->get_adj());
   if( scale_h )
      _h *= avg_strong_edge_len(_ref->get_adj());
}

// constructor for Rmeme at subdiv level > 0:
Rmeme::Rmeme(Bbase* b, Lvert* v, uint pair_lookup_key) :
   VertMeme(b, v),
   _ref(0),
   _h(0),
   _s(0),
   _pair_key(pair_lookup_key)
{
   // XXX - no longer used

   err_adv(debug,
           "Rmeme::Rmeme: warning: version for subdiv level > 0 called");
}

void
Rmeme::gen_pair()
{
   // Create an RmemeVertPair on the reference vert
   // and have it point to this meme.

   if (!_ref)
      return;
   if (_ref->find_data(_pair_key)) {
      err_adv(debug, "Rmeme::gen_pair: error: key already in use");
      return;
   }
   // XXX - temporary debugging
   if (!vert()) {
      err_adv(debug, "Rmeme::gen_pair: error: vert() is null");
      return;
   }
   if (vert()->loc() == Wpt(1,2,3))
      cerr << "coincidence" << endl;

   new RmemeVertPair(_pair_key, _ref, this);
}

void
Rmeme::delete_pair()
{
   // lookup the RmemeVertPair on the ref vert, and delete it.

   // XXX - Maybe should verify the pair key looks up our
   //       own RmemeVertPair? (Should never fail though).
   //       Yes we should, its breaking... :(

   delete RmemeVertPair::find_pair(_pair_key, _ref);
}

Rmeme::~Rmeme()
{
   delete_pair();
}

void 
Rmeme::ref_vert_changed() 
{
  if (bbase())
    bbase()->invalidate();
}

void 
Rmeme::ref_vert_deleted() 
{
   err_adv(debug, "Rmeme::ref_vert_deleted");

   _ref = 0;  // the reference vert was deleted, so we forget it
}

void 
Rmeme::set_ref(Lvert* r)
{
   // Don't freak if it's already done
   if (_ref == r)
      return;

   if (_ref) {
      err_adv(debug, "Rmeme::set_ref: warning: changing ref");
      delete_pair(); // get rid of old RmemeVertPair
   }

   _ref = r;
   gen_pair();
}

/*******************************************************
 * RmemeSFilter:
 *
 *  Accepts a vertex of a given Rsurface whose Rmeme
 *  has a non-zero scale factor s
 *
 *******************************************************/
class RmemeSFilter: public SimplexFilter {
 public:
   RmemeSFilter(Bbase* bb) : _bb(bb) { assert(_bb != 0); }

   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const {
      if (!is_vert(s))
         return false;

      Rmeme* rm = Rmeme::upcast(_bb->find_meme(s));
      
      return (rm && rm->s() != 0);
   }

 protected:
   Bbase* _bb;  // Bbase for finding Rmemes
};

void 
Rmeme::copy_attribs_v(VertMeme* v)
{
   // this meme comes from a vertex in the parent mesh.
   // we now update our reference vertex (if necessary)
   // and displacement magnitude from our parent.

   Rmeme* p = upcast(v); // parent

   err_adv(debug, "Rmeme::copy_attribs_v: should not be called");

   if (!(p && p->ref())) {
      err_adv(debug, "Rmeme::copy_attribs_v: can't get parent ref vert");
      return;
   }

   return;

   // make sure our ref vert will be there
   p->ref()->allocate_subdiv_vert();
   Lvert* r = p->ref()->subdiv_vertex();
   if (!r) {
      err_adv(debug, "Rmeme::copy_attribs_v: can't get subdiv ref vert");
      return;
   }

   set_ref(r);

   // compute _h from parent values, using smooth subdivision
   //
   Bbase* bb = p->bbase();
   HybridDisp calc(bb); // compute smooth displacements

   // If we're on the boundary of our Bbase act like a crease:
   BbaseBoundaryFilter bdry_filt(bb);
   switch (p->vert()->degree(bdry_filt)) {
    case 0:
      _h = calc.subdiv_val(p->vert());
    brcase 2: {
       Bedge_list bdry_edges = p->vert()->get_adj().filter(bdry_filt);
       assert(bdry_edges.num() == 2);
       double d0 = calc.get_val(bdry_edges[0]->other_vertex(p->vert())); 
       double d1 = calc.get_val(bdry_edges[1]->other_vertex(p->vert()));
       // use the mask [1 6 1]:
       _h = interp(interp(d0, d1, 0.5), p->h(), 0.75);
       }
    brcase 1:
       err_msg("Rmeme::copy_attribs_v: 1 boundary edge: not possible?!");
    default:
      // treat it like a "corner"
      _h = p->h();
   }

   // deal with scale factor too
   // (s == 0 means it's not used)
   if (p->s() > 0) {
      _s = offset_scale(_ref, InflateCreaseFilter());
   }
}

void 
Rmeme::copy_attribs_e(VertMeme* v1, VertMeme* v2)
{
   // this meme comes from a strong edge in the parent mesh.
   // we now update our reference vertex (if necessary)
   // and displacement magnitude from the parent elements.

   err_adv(debug, "Rmeme::copy_attribs_e: should not be called");

   Rmeme* p1 = upcast(v1);
   Rmeme* p2 = upcast(v2);

   if (!(p1 && p1->ref() && p2 && p2->ref())) {
      err_adv(debug, "Rmeme::copy_attribs_e: can't get parent ref verts");
      return;
   }
   Ledge* e = (Ledge*)(p1->ref()->lookup_edge(p2->ref()));
   if (!e) {
      err_adv(debug, "Rmeme::copy_attribs_e: can't get parent ref edge");
      return;
   }
   e->allocate_subdiv_elements();
   Lvert* r = e->subdiv_vertex();
   if (!r) {
      err_adv(debug, "Rmeme::copy_attribs_e: can't get subdiv ref vert");
      return;
   }

   set_ref(r);

   Bbase* bb = p1->bbase();
   Ledge* le = (Ledge*)p1->vert()->lookup_edge(p2->vert());
   assert(le);
   if (BbaseBoundaryFilter(bb).accept(le)) {
      // if boundary, use the crease rule
      _h = interp(p1->h(),p2->h(),0.5);
   } else {
      // else use smooth subdivision
      HybridDisp calc(bb);
      _h = calc.subdiv_val(le);
   }

   // deal with scale factor
   // (s == 0 means it's not used)
   if ((p1->s() > 0) && (p2->s() > 0)) {
      _s = offset_scale(_ref, InflateCreaseFilter());
   }
}

void 
Rmeme::copy_attribs_q(VertMeme* v1, VertMeme* v2, VertMeme* v3, VertMeme* v4)
{
   // this meme comes from a weak edge (quad diagonal) in the
   // parent mesh.  we now update its uv coords, taking an
   // average of the uv coords from the 4 memes at the parent
   // quad corners.

   err_adv(debug, "Rmeme::copy_attribs_q: should not be called");

   Rmeme* p1 = upcast(v1);
   Rmeme* p2 = upcast(v2);
   Rmeme* p3 = upcast(v3);
   Rmeme* p4 = upcast(v4);

   if (!(p1 && p1->ref() &&
         p2 && p2->ref() &&
         p3 && p3->ref() &&
         p4 && p4->ref())) {
      err_adv(debug, "Rmeme::copy_attribs_q: can't get parent ref verts");
      return;
   }

   // the quad diagonal goes from v1 to v3 (standard quad order)
   Ledge* e = (Ledge*) get_quad_weak_edge(
      p1->ref(), p2->ref(), p3->ref(), p4->ref()
      );
   if (!e) {
      err_adv(debug, "Rmeme::copy_attribs_q: can't get parent ref edge");
      return;
   }
   e->allocate_subdiv_elements();
   Lvert* r = e->subdiv_vertex();
   if (!r) {
      err_adv(debug, "Rmeme::copy_attribs_q: can't get subdiv ref vert");
      return;
   }

   set_ref(r);

   _h = (p1->_h + p2->_h + p3->_h + p4->_h)/4;

   // no offset scale for quad interiors
}

inline Bface_list
get_faces(CBvert* v)
{
   Bface_list ret;
   if (v)
      v->get_faces(ret);
   return ret;
}

CWpt& 
Rmeme::compute_update()
{
   // if there is no ref vert, can't do nothing
   if (!_ref) {
      err_adv(debug, "Rmeme::compute_update: no ref vert");
      return VertMeme::compute_update();
   }

   // if displacement is zero, stick to ref vert
   if (_h == 0)
      return (_update = _ref->loc());

   // otherwise, offset along normal -- if any!
   Wvec n = _ref->norm(FacePairFilter(_pair_key));
   if (n.is_null()) {
      err_adv(debug, "Rmeme::compute_update: can't compute normal");
      show_vert(_ref,   10, Color::red);
      show_vert(vert(), 10, Color::blue);

      Bface_list faces;
      _ref->get_all_faces(faces);
      cerr << "  subdiv level: "
           << vert()->mesh()->subdiv_level()
           << ", ref subdiv level: " <<_ref->mesh()->subdiv_level() << endl;
      
      cerr << "  total faces around ref vert: "
           << faces.num() << endl;
      cerr << "  number accepted by FacePairFilter: "
           << faces.filter(FacePairFilter(_pair_key)).num() << endl;
      cerr << "  number that are secondary: "
           << faces.secondary_faces().num() << endl;

      GtexUtil::show_tris(faces.primary_faces(),   Color::yellow);
      GtexUtil::show_tris(faces.secondary_faces(), Color::orange);

      return VertMeme::compute_update();
   }

   // compute actual displacement t
   double t = _h;
   if (_s > 0) {
      t *= _s;
   }
   // compute the displaced location:
   return (_update = _ref->loc() + (n * t));
}

VertMeme* 
Rmeme::_gen_child(Lvert* lv) const 
{
   // XXX - should never be called
   err_adv(debug, "Rmeme::_gen_child (1): shouldn't be called");

   Bbase* c = bbase()->child();
   assert(c && !c->find_vert_meme(lv));

   Rmeme* ret = new Rmeme(c, lv, _pair_key);
   ret->copy_attribs_v((VertMeme*)this);
   return ret;
}

VertMeme* 
Rmeme::_gen_child(Lvert* lv, VertMeme* nbr) const 
{
   // XXX - should never be called
   err_adv(debug, "Rmeme::_gen_child (2): shouldn't be called");

   Bbase* c = bbase()->child();
   assert(c && !c->find_vert_meme(lv));

   Rmeme* ret = new Rmeme(c, lv, _pair_key);
   ret->copy_attribs_e((VertMeme*)this, nbr);
   return ret;
}

VertMeme* 
Rmeme::_gen_child(Lvert* lv, VertMeme* v2, VertMeme* v3, VertMeme* v4) const 
{
   // XXX - should never be called
   err_adv(debug, "Rmeme::_gen_child (4): shouldn't be called");

   Bbase* c = bbase()->child();
   assert(c && !c->find_vert_meme(lv));

   Rmeme* ret = new Rmeme(c, lv, _pair_key);
   ret->copy_attribs_q((VertMeme*)this, v2, v3, v4);
   return ret;
}

/*****************************************************************
 * Rsurface:
 *****************************************************************/
Rsurface::Rsurface(CLMESHptr& mesh) :
   Bsurface(mesh)
{
   // Create an empty Rsurface

   _rsurfs += this;
}

Rsurface::Rsurface(
   CLMESHptr& mesh,
   CBface_list& faces,
   double h,
   int level,
   bool flip_norms,
   bool scale_h) :
   Bsurface(mesh),
   _ref_faces(faces)
{
   // Create a replica of the given face set.
   // 'mesh' is expected to be the mesh of the face set
   // (the faces are expected to come from a single mesh.)
   // offset distance 'h' is relative to local edge length.
   // 'level' tells how many levels down to fit the face set
   // (i.e. it is the "res level")

   assert(level >= 0);

   // XXX - argh!!!
   // we want to refer to the side-A part of the mesh
   // to build the side-B part. first make sure the side-A
   // part is up-to-date all the way down to the level
   // we'll work at.
   //
   // update subdivision down to the desired level
   // handle the case that our mesh is not the control mesh
   ctrl_mesh()->update_subdivision(level + subdiv_level());

   // create verts & faces
   gen_verts(h,scale_h);
   gen_faces(flip_norms);

   // record the inputs
   // (anyone controlling verts in the face list)

   // XXX - uncomment the following line to get memory
   //       corruption and seg faults. haven't diagnosed 
   //       the exact reason yet.
//   _inputs = Bbase::find_owners(faces).bnodes();
   hookup();

   set_res_level(level);

   // Make sure we'll be recomputed
   invalidate();

   // XXX - need to build child rsurfaces to spec,
   // matching each cut and attach operation that has been
   // applied to the corresponding side A faces.
   //
   // for now:
   // update subdivision down to the desired level
   // handle the case that our mesh is not the control mesh
   ctrl_mesh()->update_subdivision(level + mesh->subdiv_level());

   _rsurfs += this;
}

Rsurface::Rsurface(Rsurface* parent)
{
   // Create a child Rsurface of the given parent:

   err_adv(debug,
           "Rsurface::Rsurface: constructor for child should not be called");

   assert(parent);

   _ref_faces = get_subdiv_faces(parent->_ref_faces, 1);

   // Record the parent and produce children if needed:
   set_parent(parent);

   // record the inputs now
   // (anyone controlling verts in the face list)
   _inputs = Bbase::find_owners(_ref_faces).bnodes();
   hookup();

   // Make sure we'll be recomputed
   invalidate();

   err_adv(debug, "child Rsurface created at level %d", bbase_level());

   _rsurfs += this;
}

Rsurface::~Rsurface()
{
   destructor();

   _rsurfs -= this;
}

void 
Rsurface::ref_face_deleted(Bface* a)
{
   // A reference face was deleted.
   // Remove it from the list before someone tries to
   // dereference the pointer.

   err_adv(debug, "Rsurface::ref_face_deleted");

   // XXX - slow method, may need to fix.
   _ref_faces -= a;
}

Bsimplex*
Rsurface::parent_simplex_A(Rmeme* child_vert_B) const
{
   // Returns the side A parent simplex
   // of the given "side B" vertex.
   //
   //
   //  side A                     side B               
   //  ------                     ------
   //
   //  parent ----------------->  parent                
   //     ^                                              
   //     |                                             
   //     |                                             
   //     |                                             
   //    ref  <----------------- child_vert_B          
   //                                                  
   //                                                  
   assert(child_vert_B && child_vert_B->ref());
   return child_vert_B->ref()->parent();
}

Lvert*
Rsurface::parent_vert(Rmeme* child_vert_B) const
{
   Bsimplex* s = parent_simplex_A(child_vert_B);
   return is_vert(s) ? (Lvert*)map_to_b((Bvert*)s) : 0;
}

Ledge*
Rsurface::parent_edge(Rmeme* child_vert_B) const
{
   Bsimplex* s = parent_simplex_A(child_vert_B);
   return is_edge(s) ? (Ledge*)map_to_b((Bedge*)s) : 0;
}

inline void
inhibit_new_subdivision(CBface_list& faces)
{
   if (!LMESH::isa(faces.mesh())) {
      err_adv(debug, "inhibit_new_subdivision: non-LMESH");
      return;
   }
   faces.get_verts().set_bits(Lvert::SUBDIV_ALLOCATED_BIT);
   faces.get_edges().set_bits(Ledge::SUBDIV_ALLOCATED_BIT);
   faces.            set_bits(Lface::SUBDIV_ALLOCATED_BIT);
}

// Following 3 methods are for debugging:
inline void
select_childless(CBvert_list& verts)
{
   assert(LMESH::isa(verts.mesh()));
   for (int i=0; i<verts.num(); i++) {
      Lvert* v = (Lvert*)verts[i];
      if (!v->subdiv_vertex())
         err_msg("select_childless: vertex"); // can't select verts yet
   }
}

inline void
select_childless(CBedge_list& edges)
{
   assert(LMESH::isa(edges.mesh()));
   for (int i=0; i<edges.num(); i++) {
      Ledge* e = (Ledge*)edges[i];
      if (!(e->subdiv_edge1() &&
            e->subdiv_edge2() &&
            e->subdiv_vertex()))
         MeshGlobal::select(e);
   }
}

inline void
select_childless(CBface_list& faces)
{
   assert(LMESH::isa(faces.mesh()));
   for (int i=0; i<faces.num(); i++) {
      Lface* f = (Lface*)faces[i];
      if (!(f->subdiv_face1() &&
            f->subdiv_face2() &&
            f->subdiv_face3() &&
            f->subdiv_face_center()))
         MeshGlobal::select(f);
   }
   select_childless(faces.get_edges());
   select_childless(faces.get_verts());
}

void 
Rsurface::adopt(Rsurface* child)
{
   assert(child && !_child);
   assert(_res_level == 0);
   _res_level = child->res_level()+1;
   child->set_parent(this);

   // Set vert parents.
   // Have to do parent verts first...
   int n = child->vmemes().num();
   for (int i=0; i<n; i++) {
      Rmeme* r = child->rmeme(i);
      Lvert* p = parent_vert(r);
      if (p)
         p->set_subdiv_vert(r->vert());
   }
   // Do parent edges in a 2nd pass.
   for (int j=0; j<n; j++) {
      Rmeme* r = child->rmeme(j);
      Ledge* p = parent_edge(r);
      if (p)
         p->set_subdiv_elements(r->vert());
   }

   // Do faces last.
   CBface_list& faces = bfaces();
   for (int k=0; k<faces.num(); k++)
      ((Lface*)faces[k])->set_subdiv_elements();

   // Having set up our mesh elements as parents of
   // the child's mesh elements, we now flag any remaining
   // elements that don't have children so that they won't
   // try to create any.
   inhibit_new_subdivision(faces);

   // Trying to see where they aren't hooking up:
   if (debug)
      select_childless(faces);
}

void
Rsurface::recompute()
{
   // set ref faces for computing normals
   // XXX - should be more automatic, also better
   if (_patch) {
      bfaces().clear_flags();
   } else {
      err_adv(debug, "Rsurface::recompute: no patch");
   }

   _ref_faces.set_flags(1);
   Bbase::recompute();
   _ref_faces.clear_flags();
}

void
Rsurface::produce_child()
{
   if (_child)
      return;
   if (!_mesh->subdiv_mesh()) {
      err_msg("Rsurface::produce_child: Error: no subdiv mesh");
      return;
   }

   // It hooks itself up and takes care of everything...
   // even setting the child pointer is not necessary here.
   _child = new Rsurface(this);

   err_adv(debug, "Rsurface::produce_child: level %d", bbase_level());
}

#include "geom/gl_util.H"
void 
Rsurface::draw_debug()
{
   Bsurface::draw_debug();

   if (_show_memes && cur_subdiv_bbase() != 0) {
      const VertMemeList& vmemes = cur_subdiv_bbase()->vmemes();

      // set attributes for this mode
      glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
      glDisable(GL_LIGHTING);           // GL_ENABLE_BIT
      GL_COL(Color::orange, 1.0);       // GL_CURRENT_BIT
      glBegin(GL_LINES);
      
      for (int i=0; i<vmemes.num(); i++) {
         Rmeme* rm = Rmeme::upcast(vmemes[i]);
         if (rm && rm->vert() && rm->ref()) {
            glVertex3dv(rm->vert()->loc().data());
            glVertex3dv(rm->ref() ->loc().data());
         }
      }

      glEnd();
      glPopAttrib();
   }
}

void
Rsurface::gen_verts(double h, bool scale_h )
{
   // XXX
   scale_h = false;

   Bvert_list verts = _ref_faces.get_verts();

   InflateCreaseFilter filter;
   for ( int i=0; i<verts.num(); i++ ) {
      Lvert* lv = (Lvert*)_mesh->add_vertex(verts[i]->loc());
      double s = offset_scale(verts[i], filter);
      new Rmeme(this, lv, pair_lookup_key(), (Lvert*)verts[i], h, s, scale_h);
   }
}

void
Rsurface::gen_faces(bool flip_norms)
{
   for ( int i=0; i<_ref_faces.num(); i++ ) {
      Bface* f = _ref_faces[i];
      assert(f);

      if (f->is_quad()) {
         // only do it once per quad -- choose quad rep
         if (f->quad_rep() != f)
            continue;

         // get the four reference verts
         Bvert *a=0, *b=0, *c=0, *d=0;
         f->get_quad_verts(a,b,c,d);

         // get corresponding uv coords:
         UVpt uva, uvb, uvc, uvd;
         bool do_uv = UVdata::get_quad_uvs(a,b,c,d,uva,uvb,uvc,uvd);
            
         // get the two side-A faces (one is f):
         Bface* f1 = lookup_face(a,b,c);
         Bface* f2 = lookup_face(a,c,d);
         assert(f1 && f2);
            
         // get the corresponding replicated verts
         a = VertPair::map_vert(pair_lookup_key(), a); // these
         b = VertPair::map_vert(pair_lookup_key(), b); //  are
         c = VertPair::map_vert(pair_lookup_key(), c); //   now
         d = VertPair::map_vert(pair_lookup_key(), d); //    side-B
        
         if ( flip_norms ) {
            swap(b, d);
            swap(uvb, uvd);
         }

         if ( a && b && c && d ) {
            if (do_uv)
               add_quad(a, b, c, d, uva, uvb, uvc, uvd);
            else
               add_quad(a, b, c, d);
         } else 
            cerr << "Rsurface::gen_faces(): find partner failed" << endl;
      } else {

         // get the corresponding replicated verts
         Bvert* a = VertPair::map_vert(pair_lookup_key(), f->v1());
         Bvert* b = VertPair::map_vert(pair_lookup_key(), f->v2());
         Bvert* c = VertPair::map_vert(pair_lookup_key(), f->v3());

         // get corresponding uv coords:
         UVpt uva, uvb, uvc;
         bool do_uv = (UVdata::get_uv(a, f, uva) &&
                       UVdata::get_uv(b, f, uvb) &&
                       UVdata::get_uv(c, f, uvc));

         if ( flip_norms ) {
            swap(b, c);
            swap(uvb, uvc);
         }

         if ( a && b && c ) {
            if (do_uv)
               add_face(a, b, c, uva, uvb, uvc);
            else
               add_face(a, b, c);
         } else 
            cerr << "Rsurface::gen_faces(): find partner failed" << endl;
      }
   }

   // Now that side-B faces are all created, create face
   // pairs linking each side-A face to its side-B partner:
   for (int j=0; j<_ref_faces.num(); j++)
      set_face_pair(_ref_faces[j]);

   err_adv(debug, "Rsurface: level %d, faces side A: %d, side B: %d",
           subdiv_level(), _ref_faces.num(), bfaces().num());
}

void
Rsurface::set_face_pair(Bface* a)
{
   // Given side-A face 'a', find corresponding side-B face 'b',
   // and create a FacePair attached to a, pointing to b.

   if (!a) {
      err_msg("set_face_pair: error: side-A face is null");
      return;
   }
   
   // look up the corresponding side-B face
   Bface* b = lookup_face(
      VertPair::map_vert(pair_lookup_key(), a->v1()),
      VertPair::map_vert(pair_lookup_key(), a->v2()),
      VertPair::map_vert(pair_lookup_key(), a->v3())
      );

   if (b)
      new RFacePair(a, b, this);
   else
      err_msg("set_face_pair: error: can't find side-B face");
}

inline bool 
is_full(CBedge* e)
{
   return (e && e->nfaces() == 2);
}

inline void
correct_edge(Bedge* e)
{
   // pusher's remorse.
   // we pushed faces on one side, then the other.
   // now we wish we had put all the faces together first
   // and pushed them as a whole, so we would not create
   // these internal edges with nfaces() == 0.
   // this hack is to fix that, based on marking the
   // pushed faces with flag 1, all others with flag 0.
   // some day we might figure out how to do this better.

   if (debug) cerr << "correct_edge: ";
   if (!e) {
      err_adv(debug, "null edge");
      return;
   }
   if (e->nfaces() != 0) {
      err_adv(debug, "can't fix edge: nfaces == %d", e->nfaces());
      return;
   }
   if (!e->adj()) {
      err_adv(debug, "no extra faces");
      return;
   }
   Bface_list faces = e->adj()->filter(SimplexFlagFilter(1));
   if (faces.num() != 2) {
      err_adv(debug, "unexpected number of faces: %d", faces.num());
      return;
   }
   assert(e->can_promote());
   e->promote(faces[0]);
   assert(e->can_promote());
   e->promote(faces[1]);
   err_adv(debug, "fixed one");
}

inline void
correct_edges(CBedge_list& edges)
{
   for (int i=0; i<edges.num(); i++) {
      correct_edge(edges[i]);
   }
}

void
Rsurface::correct_ribbon_boundary(
   CEdgeStrip& boundary,
   CBface_list& B,
   MULTI_CMDptr cmd
   )
{
   // in case the boundary runs adjacent to existing ribbons,
   // have to push corresponding parts of existing ribbons

   // clear edge and face flags
   // have to use edges().get_verts(), otherwise miss some
   boundary.edges().get_verts().clear_flag02();

   // looking for existing ribbon faces that must be pushed
   Bface_list ribbon_faces;
   Bedge_list marked_edges;

   for (int i=0; i<boundary.num(); i++) {
      
      //         --- CCW -->
      //    b0 ------------- b1                                 
      //     |               |                                 
      //     |     mark      |                                  
      //     |     this      |                                  
      //     |     quad      |                                  
      //     |               |                                 
      //    a0 ------------- a1
      //         --- CW -->

      Bvert* b0 = boundary.vert(i);
      Bvert* b1 = boundary.next_vert(i); 
      Bvert* a0 = side_a_vert(b0);
      Bvert* a1 = side_a_vert(b1);

      Bface* q1 = lookup_quad(b0,b1,a0,a1);
      if (!q1)
         continue;
      assert(q1->is_quad());
      Bface* q2 = q1->quad_partner();
      assert(q2);
      if (q1->flag()) {
         assert(q2->flag());
         continue;
      }
      assert(!q2->flag());
      q1->set_flag(1);
      q2->set_flag(1);
      ribbon_faces += q1;
      ribbon_faces += q2;
      marked_edges += boundary.edge(i);
      if (debug) {
         err_msg("  found ribbony edge");
         GtexUtil::show(boundary.edge(i));
      }
   }
   // this messes w/ the nearby flags
   push(ribbon_faces, cmd);

   // make sure all faces nearby have cleared flags
   boundary.edges().get_verts().clear_flag02();

   // mark the untouchable edges now:
   marked_edges.set_flags(1);

   // mark A, B, and ribbon faces
   Bface_list A;
   if (!map_to_a(B,A)) {
      err_adv(debug, "Rsurface::correct_ribbon_boundary: can't find side A");
      return;
   }
   (A + B + ribbon_faces).set_flags(1);

   // bring up those maligned faces; would be unnecessary if we had
   // pushed them as a set... but that's hard to do when working at
   // different subdivision levels... don't have time to figure it out
   correct_edges(marked_edges);
   correct_edges(map_to_a(marked_edges));
}

bool
Rsurface::sew_ribbons(CBface_list& side_b, MULTI_CMDptr cmd)
{
   err_adv(debug, "Rsurface::sew_ribbons");
   LMESH* mesh = LMESH::upcast(side_b.mesh());
   if (!mesh) {
      err_adv(debug, "  can't get side B mesh");
      return 0;
   }
   // XXX - perhaps the ribbon should be a Bsurface...
   // in the meantime, create it as a separate Patch, and
   // ensure that it gets drawn by putting in in the drawables list
   // of the control mesh
   Patch* p = mesh->new_patch();
   if (!mesh->is_control_mesh())
      mesh->control_mesh()->drawables() += p;
   assert(p && p->mesh() == mesh);

   // Get chains of boundary edges in CCW order:
   EdgeStrip boundary = side_b.get_boundary();
   if (!boundary.edges().nfaces_is_equal(1)) {
      err_adv(debug, "  error: boundary is not all border");
      return false;
   } else if (side_b.is_all_secondary()) {
      // The region is a hole, need to reverse direction of the
      // boundary path so it goes CCW around the not-hole.
      boundary.reverse();
   } else if (!side_b.is_all_primary()) {
      err_adv(debug, "  error: mixed primary/secondary faces");
      return false;
   }

   // in case the boundary runs adjacent to existing ribbons,
   // have to push corresponding parts of existing ribbons
   correct_ribbon_boundary(boundary, side_b, cmd);

   // du is a step in UV-space corresponding to 1 edge length,
   // based on existing UV-coords in surrounding mesh region.
   double du = min_uv_delt(boundary.edges());

   double u = 0;
   for (int i=0; i<boundary.num(); i++) {
      
      // uv coords:
      //   u increments by 1.0 along each horizontal edge
      //   v varies from 0 to 1 vertically (up)
      //                                                       
      //         --- CCW -->
      //    b0 ------------- b1      du                         
      //     |               |       ^                         
      //     |     make      |       |                          
      //     |     this      |       | v                        
      //     |     quad      |       |                          
      //     |               |       |                         
      //    a0 ------------- a1      0                          
      //         --- CW -->
      //     u ------------> u+du

      if (boundary.has_break(i)) {
         u = 0; // restart u-coordinate
      }
      if (boundary.edge(i)->flag()) {
         u = 0;         // restart u-coordinate
         err_adv(debug, "  skipping edge");
         continue;      // don't do this one
      }
      Bvert* b0 = boundary.vert(i);
      Bvert* b1 = boundary.next_vert(i); 
      Bvert* a0 = side_a_vert(b0);
      Bvert* a1 = side_a_vert(b1);

      if (!(b0 && b1 && a0 && a1)) {
         cerr << "Rsurface::sew_ribbons: error: missing ";
         if (!b0) cerr << "b0 ";
         if (!b1) cerr << "b1 ";
         if (!a0) cerr << "a0 ";
         if (!a1) cerr << "a1 ";
         cerr << endl;
         if (debug)
            GtexUtil::show(boundary.edge(i), 4);
         continue;
      }
      assert(b0 && b1 && a0 && a1);

      if (lookup_edge(a0, a1)) {
         UVpt ua0(u   ,  0);
         UVpt ua1(u+du,  0);
         UVpt ub0(u   , du);
         UVpt ub1(u+du, du);

         // debug hell
         if (is_full(lookup_edge(a0, a1))) {
            show_vert(a0, 20, COLOR::blue);
            show_vert(a1, 20, COLOR::blue);
         } else if (is_full(lookup_edge(b0, b1))) {
            show_vert(b0, 20, COLOR::blue);
            show_vert(b1, 20, COLOR::blue);
         } else if (is_full(lookup_edge(a0, b0))) {
            show_vert(a0, 20, COLOR::blue);
            show_vert(b0, 20, COLOR::blue);
         } else if (is_full(lookup_edge(a1, b1))) {
            show_vert(a1, 20, COLOR::blue);
            show_vert(b1, 20, COLOR::blue);
         } else {
            mesh->add_quad(a0, a1, b1, b0, ua0, ua1, ub1, ub0, p);
         }
         u += du;
      } else {

         // not mentioned in the above diagram, there may be
         // 1 or 2 vertices between a0 and a1, like this:
         //
         //    a0 ------ t0 ------ t1 ---- a1
         //
         //    a0 ---------- t0 ---------- a1
         //                                   

         Bvert* t0 = a0->next_border_vert_cw();
         Bvert* t1 = t0->next_border_vert_cw();
         if (!(t0 && t1)) {
            err_adv(debug, "  can't get t0, t1 verts");
            continue;
         }
         if (!(t1 == a1 || t1->next_border_vert_cw() == a1)) {
            err_adv(debug, "  can't match a & b verts");
            continue;
         }
         mesh->add_face(a0, t0, b0, p);
         u = 0;
         UVpt ut0(u   ,  0);
         UVpt ut1(u+du,  0);
         UVpt ub0(u   , du);
         UVpt ub1(u+du, du);
         mesh->add_quad(t0, t1, b1, b0, ut0, ut1, ub1, ub0, p);
         u += du;
         if (t1 != a1) {
            assert(lookup_edge(t1, a1));
            mesh->add_face(t1, a1, b1, p);
            u = 0;
         }
      }
   }

   if (cmd)
      cmd->add(new UNPUSH_FACES_CMD(p->faces()));

   mesh->changed();

   return 1;
}

bool
Rsurface::map_to_b(CBface_list& A, Bface_list& B) const
{
   // Map the entire A-side region to corresponding B-side faces,
   // but if any face can't be mapped, return the empty list:

   B.clear();
   if (A.empty())
      return true;      // "success"
   for (int i=0; i<A.num(); i++) {
      Bface* b = map_to_b(A[i]);
      if (b) {
         B += b;
      } else {
         B.clear();     // Ned: important to B.clear()
         return false;
      }
   }
   return true;
}

bool 
Rsurface::map_to_b(CEdgeStrip& A, EdgeStrip& B) const
{
   // Map an A-side edge strip to the corresponding strip on side B,
   // but if any element can't be mapped, return the empty strip:

   B.reset();
   if (A.empty())
      return true;      // "success"
   for (int i=0; i<A.num(); i++) {
      Bvert* v = map_to_b(A.vert(i));
      Bedge* e = map_to_b(A.edge(i));
      if (v && e) {
         B.add(v,e);
      } else {
         B.reset();
         return false;
      }
   }
   return true;
}

Rsurface* 
Rsurface::find_b(CBface_list& A, Bface_list& B)
{
   // (Static method)
   //
   // Given a region of mesh, find an Rsurface of the same
   // mesh whose side A faces includes the region. Return
   // both the Rsurface and the corresponding B-side faces.

   B.clear();

   LMESH* m = LMESH::upcast(A.mesh());
   if (!m)
      return 0;

   // Search over existing Rsurfaces.
   // This is the reason for keeping the global list of Rsurfaces.
   for (int i=0; i<_rsurfs.num(); i++) {
      Rsurface* r = _rsurfs[i];
      if (r->mesh() == m && r->map_to_b(A, B))
         return r;
   }
   return 0;
}

bool
Rsurface::map_to_a(CBface_list& B, Bface_list& A)
{
   // Map the entire B-side region to corresponding A-side faces,
   // but if any face can't be mapped, return the empty list:

   A.clear();
   if (B.empty())
      return false;
   for (int i=0; i<B.num(); i++) {
      Bface* a = side_a_face(B[i]);
      if (a) {
         A += a;
      } else {
         A.clear();
         return false;
      }
   }
   return true;
}

bool
Rsurface::map_to_a(CBedge_list& B, Bedge_list& A)
{
   // Map the entire B-side region to corresponding A-side edges,
   // but if any edge can't be mapped, return the empty list:

   A.clear();
   if (B.empty())
      return false;
   for (int i=0; i<B.num(); i++) {
      Bedge* a = side_a_edge(B[i]);
      if (a) {
         A += a;
      } else {
         A.clear();
         return false;
      }
   }
   return true;
}

bool 
Rsurface::handle_punch(CBface_list& region, MULTI_CMDptr cmd)
{
   // In case the given region is part of a side A or side B
   // inflate, then get the other side, punch it out, and sew up
   // ribbons connecting the two holes. Returns true on success.

   bool debug = ::debug || Config::get_var_bool("DEBUG_PUNCH",false);
   if (debug) cerr << "Rsurface::notify_punch: ";

   if (region.empty()) {
      err_adv(debug, "empty region");
      return false;
   }

   if (!region.is_all_primary()) {
      err_adv(debug, "error: non-primary region");
      return false;
   }

   if (!region.can_push_layer()) {
      err_adv(debug, "region says it can't push layer");
      return false;
   }
   // If this is going to work, the given region is either part
   // of side A or B of some Rsurface. Next, we figure out which
   // it is (if any).
   Bface_list B;
   Bface_list A;
   Rsurface* rsurf = upcast(Bbase::find_owner(region));
   if (rsurf) {
      B = region;
      if (!map_to_a(B,A)) {
         err_adv(debug, "can't find side A");
         return false;
      }
   } else {
      A = region;
      if (!(rsurf = find_b(region, B))) {
         err_adv(debug, "can't find side B");
         return false;
      }
   }
   assert(B.num() == region.num());
   int k = rsurf->res_level();
   assert(k >= 0);
   Bface_list Bk = remap_level(B, k);
   if (Bk.num() != pow(4.0,k)*B.num()) {
      err_adv(debug, "error mapping to level %d", k);
      return false;
   }

   // we were gonna try to push A, B, and the existing ribbons
   // connecting them all at once, but it's complicated if the
   // existing ribbons happen at a finer subdiv level...
   // need more time to figure it out, but have not time
   push(A + B, cmd);

   // Ran out of excuses...
   err_adv(debug, "sewing ribbons");
   return sew_ribbons(Bk, cmd);
}

// end of file rsurf.C
