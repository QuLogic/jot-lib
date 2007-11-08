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
 * bsurface.C:
 **********************************************************************/
#include "disp/colors.H"
#include "geom/gl_view.H"       // GL_VIEW::init_line_smooth()
#include "gtex/basic_texture.H"
#include "gtex/ref_image.H"     // for VisRefImage::lookup()
#include "std/config.H"
#include "geom/world.H" 

#include "bpoint.H"
#include "bcurve.H"
#include "bsurface.H"

using namespace mlib;

bool draw_skin_only = false;

/**********************************************************************
 * Bsurface
 **********************************************************************/
bool Bsurface::_show_crease = false;
Bsurface::Bsurface(CLMESHptr& mesh) :  
   _patch(0),
   _retri_count(0),
   _target_len(0),
   _update_stamp(0)
{   
   // We don't call the Bbase constructor with the given mesh,
   // because we want the virtual function set_mesh() to be
   // over-ridden by Bsurface::set_mesh(). But that won't happen
   // if it's called in the Bbase constructor.       
   set_mesh(mesh);

   _bpoints.set_unique();
   _bcurves.set_unique();
}

Bsurface::~Bsurface()
{
   // XXX - Currently has no effect:
   unselect();

   destructor();

   // Putting the patch back into the mesh's drawables list might be
   // something to consider. Currently it seems control surfaces don't
   // get deleted, so it hasn't been an issue.
   _patch = 0;
   _bpoints.clear();
   _bcurves.clear();

   // Bbase destructor removes vert memes.
   // We have to remove our edge and face memes.
   static bool debug = Config::get_var_bool("DEBUG_BNODE_DESTRUCTOR",false);
   err_adv(debug, "Bsurface::~Bsurface: deleting %d edge, %d face memes",
           _ememes.num(), _fmemes.num());
   _fmemes.delete_all();
   _ememes.delete_all();

   // Get out of the mesh if needed:
   set_parent(0);
   set_mesh(0);
}


CBface_list& 
Bsurface::bfaces() const
{
   if (_patch != NULL)
      return _patch->faces();
   static Bface_list ret;
   return ret;
}

void 
Bsurface::set_parent(Bbase* p)
{
   // Set the Bbase parent and ensure patches are in agreement

   // Set our curves/points from the parent first,
   // because in Bbase::set_parent() our child will
   // try to set curves/points from us:
   Bsurface* s = upcast(p);
   if (s) {
      _bcurves = s->_bcurves.children();
      _bpoints = s->_bpoints.child_points();
   } else assert(p == 0);

   Bbase::set_parent(p);

   if (s) {
      patch()->set_parent(s->patch());
   }
}

void
Bsurface::delete_elements()
{
   // Delete all mesh elements controlled by this, and all memes
   // (boss or not).

   // Deleting elements at this level causes finer level mesh
   // elements to be deleted too. But it won't catch the non-boss
   // child memes, so let's get them now:
   if (_child)
      _child->delete_elements();

   // The following works if the Bbase puts vert memes on all the
   // vertices it controls. Deleting a mesh vertex automatically
   // deletes all adjacent edges and faces.

   if (_vmemes.empty())
      return;

   // find verts controlled by this bbase:
   //Bvert_list mine = find_boss_vmemes(_vmemes.verts()).verts();
   Bvert_list mine;
   for (int i = 0; i < _vmemes.num(); i++)
      if (_vmemes[i]->is_boss())
         mine += _vmemes[i]->vert();

   // delete vert memes
   _vmemes.delete_all();
   // delete the verts controlled by this:
   assert(_mesh);
   _mesh->remove_verts(mine);

   // remove remaining edges and faces
   Bedge_list mine_e;
   for (int i = 0; i < _ememes.num(); i++)
      if (_ememes[i]->is_boss())
         mine_e += _ememes[i]->edge();
   _ememes.delete_all();
   _mesh->remove_edges(mine_e);

   _fmemes.delete_all();
   _mesh->remove_faces(bfaces());

   //// the Bsurface is now totally empty
   //// so it should ditch the lists: _bcurves and _bpoints.
   //// this means we are changing the input set.
   //// handled like so:
   //if (!(_bcurves.empty() && _bpoints.empty())) {
   //   unhook();         // unhook from old "inputs"
   //   _bcurves.clear(); // change the definition
   //   _bpoints.clear(); //    of inputs
   //   hookup();         // hook up to new inputs
   //}            
}

void
Bsurface::set_mesh(CLMESHptr& mesh)
{
   // Record the new mesh, and also:
   //   For old mesh: get out of drawables and stop observing.
   //   For new mesh: get into drawables and start observing.
   Bbase::set_mesh(mesh);

   // If we have a patch it has to belong to the new mesh:
   if (_patch) {
      assert(_mesh == _patch->mesh());
      Patch::set_focus(_patch);
      return;
   }

   if (!_mesh)
      return;

   // Reach here if we have a mesh but no assigned patch.
   // Need to get a patch in that case.
   if (is_control()) {
      _patch = (Lpatch*)_mesh->new_patch();
      _mesh->drawables() -= _patch;     // Bsurface takes over drawing
   } else {
      // use the child patch of the parent surface
      Bsurface* s = parent_surface();
      Patch* p = s ? s->patch() : 0;
      // policy here may need work; for now try being a hard-ass
      assert(p != NULL);
      _patch = (Lpatch*)p->get_child();
   }
   assert(_patch != NULL);
   Patch::set_focus(_patch);
}
  
void 
Bsurface::gen_subdiv_memes() const 
{
   // In case subdiv elements have been generated at lower
   // levels of the mesh already, now is the time to shower
   // down memes upon them (as needed):

   if (_res_level > 0 && _mesh->subdiv_mesh() != NULL) {

      // Ensure the child is created
      ((Bsurface*)this)->produce_child();

      // Check exhaustively for any elements that need to
      // generate memes:
      _vmemes.notify_subdiv_gen();
      _ememes.notify_subdiv_gen();
      _fmemes.notify_subdiv_gen();

      // Pass it on:
      if (child_surface())
         child_surface()->gen_subdiv_memes();
   }
}

/*****************************************************************
 * Adding elements
 *****************************************************************/
EdgeMeme* 
Bsurface::add_edge_meme(EdgeMeme* e)
{
   if (e && e->bbase() == this) {
      _ememes += e;
      return e;
   }
   return 0;
}

EdgeMeme* 
Bsurface::add_edge_meme(Ledge* e) 
{
   // Sometimes they're not serious
   if (!e)
      return 0;

   // Don't create a duplicate meme:
   if (find_edge_meme(e))
      return find_edge_meme(e);

   // No more excuses...
   return new EdgeMeme(this, e);  // it adds itself to the _ememe list
}

FaceMeme*
Bsurface::add_face_meme(FaceMeme* f) 
{
   if (f && f->bbase() == this) {
      _fmemes += f;
      return f;
   }
   return 0;
}

FaceMeme* 
Bsurface::add_face_meme(Lface* f)
{
   // Register our face meme on the given face.

   // Screen out the wackos
   if (!f)
      return 0;

   // Don't create a duplicate meme:
   if (find_face_meme(f))
      return find_face_meme(f);

   // Put edge memes on all the edges:
   // (it's a no-op if the meme already exists)
   add_edge_meme(f->le(1));
   add_edge_meme(f->le(2));
   add_edge_meme(f->le(3));

   // Create the face meme:
   return new FaceMeme(this, f);  // it adds itself to the _fmeme list
}

void 
Bsurface::add_face_memes(CBface_list& faces) 
{
   if (!(_mesh && LMESH::upcast(faces.mesh()) == _mesh)) {
      err_msg("Bsurface::add_face_memes: error: bad mesh");
      return;
   }
   for (int i=0; i<faces.num(); i++)
      add_face_meme((Lface*)faces[i]);
}

FaceMeme* 
Bsurface::add_face(Bvert* u, Bvert* v, Bvert* w) 
{
   Lface* f = (Lface*)_mesh->add_face(u,v,w,_patch);

   if (!f) {
      err_msg("Bsurface::add_face: Error creating face");
      return 0;
   }

   // Put a face meme on it
   return add_face_meme(f);
}

FaceMeme* 
Bsurface::add_face(
   Bvert* u, Bvert* v, Bvert* w,
   CUVpt& a, CUVpt& b, CUVpt& c) 
{
   // Do like before but also set uv coordinates

   FaceMeme* ret = add_face(u, v, w);
   if (ret)
      UVdata::set(ret->face(),a,b,c);

   return ret;
}

FaceMeme* 
Bsurface::add_quad(Bvert* u, Bvert* v, Bvert* w, Bvert* x) 
{
   // Create the quad defined as follows:
   //                                                       
   //      x --------  w                                    
   //      |         / |                                    
   //      |  f2   /   |                                    
   //      |     /     |                                    
   //      |   /   f1  |                                    
   //      | /         |                                    
   //      u --------  v                                     
   //                                                       

   // Creates both faces with the "weak edge" from u to w.
   // Returns the face that is the "quad rep," which could
   // be either f1 or f2:
   Lface* f = (Lface*)_mesh->add_quad(u,v,w,x,_patch);

   if (!(f && f->is_quad())) {
      err_msg("Bsurface::add_quad: Error creating quad");
      return 0;
   }

   // Put edge memes on all the edges:
   // (it's a no-op if the meme already exists)
   add_edge_meme((Ledge*)u->lookup_edge(v));
   add_edge_meme((Ledge*)w->lookup_edge(v));
   add_edge_meme((Ledge*)x->lookup_edge(w));
   add_edge_meme((Ledge*)u->lookup_edge(x));
   add_edge_meme((Ledge*)u->lookup_edge(w));

   // Put a face meme on each; return the face meme of the
   // quad rep:
   add_face_meme((Lface*)f->quad_partner());
   return add_face_meme(f);
}

FaceMeme*
Bsurface::add_quad(
   Bvert* u, Bvert* v, Bvert* w, Bvert* x,
   CUVpt& a, CUVpt& b, CUVpt& c, CUVpt& d
   ) 
{
   // Create the quad defined as follows:
   //                                                       
   //      x --------  w                                    
   //      |         / |                                    
   //      |  f2   /   |                                    
   //      |     /     |                                    
   //      |   /   f1  |                                    
   //      | /         |                                    
   //      u --------  v                                     
   //                                                       

   // Puts face memes on both faces. Returns the face meme
   // of the "quad rep," which could be either f1 or f2:
   FaceMeme* ret = add_quad(u,v,w,x);

   if (!ret)
      return 0;
      
   // Assign uv-coords as requested:
   if (!UVdata::set(u,v,w,x,a,b,c,d))
      err_msg("Bsurface::add_quad: Error setting uv-coords");

   return ret;
}

/*****************************************************************
 * Removing memes
 *****************************************************************/
void 
Bsurface::rem_edge_meme(EdgeMeme* e) 
{
   if (!e)
      err_msg("Bsurface::rem_edge_meme: Error: meme is nil");
   else if (e->bbase() != this)
      err_msg("Bsurface::rem_edge_meme: Error: meme owner not this");
   else {
      _ememes -= e;
      delete e;
      err_adv(Config::get_var_bool("DEBUG_MEME_DESTRUCTOR",false),
         "%s::rem_edge_meme: %d left at level %d",
         **class_name(),
         _ememes.num(),
         bbase_level()
         );
   }
}

void 
Bsurface::rem_face_meme(FaceMeme* f) 
{
   if (!f)
      err_msg("Bsurface::rem_face_meme: Error: meme is nil");
   else if (f->bbase() != this)
      err_msg("Bsurface::rem_face_meme: Error: meme owner not this");
   else {
      _fmemes -= f;
      delete f;
      err_adv(Config::get_var_bool("DEBUG_MEME_DESTRUCTOR",false),
         "%s::rem_face_meme: %d left at level %d",
         **class_name(),
         _fmemes.num(),
         bbase_level()
         );
   }
}

/*****************************************************************
 * drawing methods
 *****************************************************************/
void
Bsurface::draw_creases()
{
   GL_VIEW::init_line_smooth(2.0f); // glPushAttrib(GL_ENABLE_BIT | ... );
      
   glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
   glColor3dv(Color::orange.data());    // GL_CURRENT_BIT
      
   // draw with a simple strip callback object:
   GLStripCB cb;
   _patch->draw_crease_strips(&cb);
      
   GL_VIEW::end_line_smooth();          // glPopAttrib();
}

int 
Bsurface::draw(CVIEWptr& v) 
{
   // Draw to the screen, w/ current selection color.

   // Not supposed to fail (of course):
   assert(is_control());

   if (draw_skin_only && !is_of_type("Skin"))
      return 0;

   if (!_is_shown)
      return 0;
   
   int ret = 0;    
   if (_patch) {
      if (_show_crease)
         draw_creases();

      // Changing the patch's color no longer invalidates display
      // lists, so this is lightweight. (Specifically, it's okay
      // to keep setting the same damn color over and over):
      _patch->set_color(draw_color());
   
      // for visualization purposes, when secondary faces
      // are shown, draw all surfaces transparently

      static const double BSURF_ALPHA = Config::get_var_dbl("BSURF_ALPHA",0.5);
      assert(_mesh);
      bool do_transp = _mesh->show_secondary_faces();
      double alpha = (do_transp ? BSURF_ALPHA : 1.0);
      _patch->set_transp(alpha);

      ret = _patch->draw(v);
   }   

   draw_debug();

   return ret;
}

void
Bsurface::draw_debug()
{
   if (!_is_shown)
      return;

   const double BASE_SIZE = 8.0;
   double size = BASE_SIZE * pow(0.75, cur_level());
   show_memes(Color::orange_pencil_d, Color::orange_pencil_l,(float) size);
   
   _bcurves.draw_debug();
}

void 
Bsurface::request_ref_imgs() 
{
   // Not supposed to fail (of course):
   assert(is_control());

   // Should not get this call unless it has been set up with a patch
   if ((_patch && _is_shown))
      _patch->request_ref_imgs();
}

int 
Bsurface::draw_vis_ref() 
{
   // Not supposed to fail (of course):
   assert(is_control());

   if (!_is_shown)
      return 0;

   // Draw to the visibility reference image (for picking)

   int ret = _patch ? _patch->draw_vis_ref() : 0;

   // draw "controls"
   if (is_selected()) {
      _bcurves.draw_vis_ref();
      _bpoints.draw_vis_ref();
   }

   return ret;
}

void 
Bsurface::hide()
{
   Bbase::hide();
   bfaces().push_layer(); 

   // XXX - hack, until dependencies work reliably:
//   _mesh->mark_all_dirty();
}

void 
Bsurface::show()
{
   Bbase::show();
   bfaces().unpush_layer(); 

   // XXX - hack, until dependencies work reliably:
//   _mesh->mark_all_dirty();
}

void 
Bsurface::set_selected() 
{
   Bbase::set_selected();
   Patch::set_focus(_patch);
}

/*****************************************************************
 * Bnode methods
 *****************************************************************/
Bsurface_list 
Bsurface::selected_surfaces()
{
   return Bsurface_list(_selection_list);
}

Bsurface* 
Bsurface::selected_surface()
{
   Bsurface_list surfaces = selected_surfaces();
   return (surfaces.num() == 1) ? surfaces[0] : 0;
}

CCOLOR& 
Bsurface::selection_color() const 
{
   static COLOR sel_color(0.8,0.9,1.0);//(0.8,0.9,1.0);
   return sel_color;
}

CCOLOR& 
Bsurface::regular_color() const 
{
   static COLOR reg_color(0.9,0.9,0.9);//(0.9,0.9,0.9);
   return reg_color;
}

/**********************************************************************
 * GLU tessellation routines
 **********************************************************************/
#include <GL/glu.h>

// declarations:

#ifdef WIN32 
typedef GLvoid (CALLBACK *glu_tess_func)(); 
#else 
#ifdef macosx
typedef GLvoid (*glu_tess_func)(void); 
#else
#ifdef __GNUC__
#if (defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)))
typedef GLvoid (*glu_tess_func)(void); 
#else 
typedef GLvoid (*glu_tess_func)(...); 
#endif 
#else
typedef GLvoid (*glu_tess_func)(void); 
#endif 
#endif
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

/********************************************
 * TessellatorData:
 *
 *  Carries around data used in GLU tessellator
 *  callbacks.
 ********************************************/
class TessellatorData : public Bvert_list {
 public:
   TessellatorData();

   GLUtesselator * tess()  { return _tess; }
   
   Bsurface*    surface()                       { return _surf; }
   void         set_surface(Bsurface* s)        { _surf = s; }

 protected:
   GLUtesselator* _tess;
   Bsurface*      _surf;
};

/********************************************
 * callbacks
 ********************************************/
static TessellatorData tess_data;

void CALLBACK
beginCB(GLenum type) 
{
//    assert(type == GL_TRIANGLES);
//    assert(tess_data.empty());
   //glBegin(type);
}

void CALLBACK
endCB()
{
//    assert(tess_data.empty());
   //glEnd();
}

void CALLBACK
edgeflagCB(GLboolean)
{
   // Don't do anything, the only reason it's here
   // is to hack GLU to do GL_TRIANGLES...
}

void CALLBACK
errorCB(GLenum err)
{
//    err_msg("GLU error: %s", gluErrorString(err));
}

void CALLBACK
vertexCB(void* vdata)
{
   Bvert* v = (Bvert*)vdata;

   if (tess_data.num() == 2) {
      if (tess_data[0] && tess_data[1] && v) {
         // we add memes just to the newly created edges
         // (not existing boundary edges, in particular),
         // and also to the new face
         Bsurface* surf = tess_data.surface();
         if (!tess_data[0]->lookup_edge(tess_data[1]))
            surf->add_edge(tess_data[0], tess_data[1]);
         if (!tess_data[1]->lookup_edge(v))
            surf->add_edge(tess_data[1], v);
         if (!tess_data[0]->lookup_edge(v))
            surf->add_edge(tess_data[0], v);
         surf->add_face(tess_data[0], tess_data[1], v);
      }
      tess_data.clear();
   } else {
      tess_data += v;
   }
}

TessellatorData::TessellatorData() :
   _tess(gluNewTess()),
   _surf(0)
{
   // We don't handle "combine" callbacks since we
   // don't want the operation to happen. If GLU wants
   // to generate a combine callback and we haven't
   // supplied the callback, it will generate an error
   // in errorCB and do nothing else. That's okay. It
   // means we can't tessellate self-intersecting
   // contours like figure-8's.
   
   // XXX - looks like the docs lied. it goes ahead and
   // tessellates even with no combine callback. contrary to
   // info here:
   //
   // http://www.eecs.tulane.edu/www/graphics/doc/OpenGL-Man-Pages/gluTessCallback.html
   
   gluTessCallback(_tess, GLU_TESS_BEGIN,  
                   (glu_tess_func) &beginCB);
   gluTessCallback(_tess, GLU_TESS_VERTEX, 
                   (glu_tess_func) &vertexCB);
   gluTessCallback(_tess, GLU_TESS_EDGE_FLAG, 
                   (glu_tess_func) &edgeflagCB);
   gluTessCallback(_tess, GLU_TESS_END,  
                   (glu_tess_func) &endCB);
   gluTessCallback(_tess, GLU_TESS_ERROR, 
                   (glu_tess_func) &errorCB);
   gluTessCallback(_tess, GLU_TESS_COMBINE, 0); // we reject this
}

void
Bsurface::tessellate(CARRAY<Bvert_list>& contours) 
{
   tess_data.clear();
   tess_data.set_surface(this);

   int i, j;

   //----------------------------------------------------------------
   // XXX - hack to tell GLU the plane so it won't crash like a ninny
   Wplane   P;
   Wpt_list pts;
   for (i=0; i<contours.num(); i++)
      for (j=0; j<contours[i].num(); j++)
         pts += contours[i][j]->loc();
   // calculate the plane
   pts.update_length();
   pts.get_plane(P);
   if (P.is_valid()) {
      Wvec n = P.normal();
      gluTessNormal(tess_data.tess(), n[0], n[1], n[2]);
   }
   //----------------------------------------------------------------
   
   gluTessBeginPolygon(tess_data.tess(), this);
   
   for (i=0; i<contours.num(); i++) {
      gluTessBeginContour(tess_data.tess());
      for (j=0; j<contours[i].num(); j++) {
         Bvert* bv = contours[i][j];
         gluTessVertex(tess_data.tess(), 
                       (GLdouble*)(bv->loc().data()), bv);
      }
      
      gluTessEndContour(tess_data.tess());
   }
   
   gluTessEndPolygon(tess_data.tess());

   _mesh->changed(BMESH::TOPOLOGY_CHANGED);
}

/**********************************************************************
 * retessellation
 **********************************************************************/
bool
Bsurface::do_swaps()
{
   cerr << "Bsurface::do_swaps: not implemented" << endl;
   return 0;
}

bool
Bsurface::do_splits()
{
   cerr << "Bsurface::do_splits: not implemented" << endl;
   return 0;
}

bool
Bsurface::do_collapses()
{
   cerr << "Bsurface::do_collapses: not implemented" << endl;
   return 0;
}

bool
Bsurface::do_retri()
{
   // invoke retriangulation ops.

   // alternate split and collapse operations some of the time.
   // mop up with swaps.

   static bool debug = Config::get_var_bool("PRINT_RETRI",false);
   bool ret = 0;
   switch (++_retri_count % 6) {
    case    0:

      if (debug) err_msg("Bsurface::do_retri: Splitting edges...");
      ret = do_splits();
      if (debug) err_msg("Bsurface::do_retri: Swapping edges...");
      ret = do_swaps() || ret;

    brcase  3:
      if (debug) err_msg("Bsurface::do_retri: Collapsing edges...");
      ret = do_collapses();
      if (debug) err_msg("Bsurface::do_retri: Swapping edges...");
      ret = do_swaps() || ret;
     brdefault:
      ; // resting
   }

   // return true if anything happened.
   return ret;
}

void    
Bsurface::absorb_mesh_copy(CLMESHptr& m)
{
   // Copy the elements of the given mesh, adding all the new
   // faces to the Patch belonging to this Bsurface:

   // Number of vertices we started with:
   // (used when adding edges and faces, below)
   int num_v = _mesh->nverts();
   int num_e = _mesh->nedges();
   int num_f = _mesh->nfaces();
   
   // Copy verts
   int k;
   for (k=0; k<num_v; k++) {
      Lvert* v = (Lvert*) _mesh->add_vertex(m->bv(k)->loc());
      if (m->lv(k)->corner_value())
         v->set_corner(m->lv(k)->corner_value());
   }

   // Copy edges
   for (k=0; k<num_e; k++) {
      Ledge* old_edge = (Ledge*)m->be(k);
      Ledge* new_edge = (Ledge*)_mesh->add_edge(
         old_edge->v1()->index() + num_v,
         old_edge->v2()->index() + num_v)
         ;
      if (old_edge->is_crease())
         new_edge->set_crease(old_edge->crease_val());
      if (old_edge->is_patch_boundary())
         new_edge->set_patch_boundary();
      if (old_edge->is_weak())
         new_edge->set_bit(Bedge::WEAK_BIT);
   }
   
   // Copy faces
   for (k=0; k<num_f; k++) {
      Bface* f = m->bf(k);
      if (UVdata::lookup(f))
         add_face(
            _mesh->bv(f->v1()->index() + num_v),
            _mesh->bv(f->v2()->index() + num_v),
            _mesh->bv(f->v3()->index() + num_v),
            UVdata::get_uv(f->v1(), f),
            UVdata::get_uv(f->v2(), f),
            UVdata::get_uv(f->v3(), f)
            );
      else
         add_face(
            _mesh->bv(f->v1()->index() + num_v),
            _mesh->bv(f->v2()->index() + num_v),
            _mesh->bv(f->v3()->index() + num_v)
            );
   }

   _mesh->changed(BMESH::TOPOLOGY_CHANGED);
}

void
Bsurface::recompute()
{
   // XXX - Debug checks:
   //    If we are re-updating on the same frame (shouldn't!)
   //    then tell about it.
   if (_update_stamp == VIEW::stamp())
      err_msg("Bsurface::recompute: WARNING: frame %d, re-updating %d verts",
              VIEW::stamp(), _vmemes.num());

   // update vert positions:
   Bbase::recompute();

   _update_stamp = VIEW::stamp();
}

void 
Bsurface::absorb(Bcurve* c)
{
   // Add it to the _bcurves list, and add its
   // endpoints (if any) to the _bpoints list.

   if (c) {
      _bcurves += c;    // adds uniquely
      absorb(c->b1());
      absorb(c->b2());
   }
}

void 
Bsurface::absorb(Bpoint* p)
{
   // Add it to the _bpoints list.

   if (p)
      _bpoints += p;    // adds uniquely
}

/*******************************************************
 * BsurfaceFilter:
 *
 *      Used in Bsurface::hit_surface() below
 *******************************************************/
class BsurfaceFilter : public SimplexFilter {
 public:
   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const {
      return (is_face(s) && Bsurface::find_controller((Bface*)s));
   }
};

Bsurface* 
Bsurface::hit_surface(CNDCpt& p, double rad, Wpt& hit, Bface** face)
{
   // get the visibility reference image:
   VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());
   if (!vis_ref) {
      err_msg("Bsurface::hit_surface: Error: can't get vis ref image");
      return 0;
   }
   vis_ref->update();

   // Find a face of a Bsurface in the search region, and get the Bsurface:
   Bface* f = (Bface*)vis_ref->find_near_simplex(p, rad, BsurfaceFilter());
   Bsurface* ret = find_controller(f);
   if (ret) {
      // Fill in the hit point itself
      f->near_point(p, hit);
      if (face)
         *face = f;
   }
   return ret;
}

Bnode_list 
Bsurface::inputs()  const 
{ 
  return Bbase::inputs();
}


/* end of file bsurface.C */
