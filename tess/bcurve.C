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
 * bcurve.C:
 **********************************************************************/
#include "disp/colors.H"                // Color::grey7 etc.
#include "geom/gl_view.H"               // for GL_VIEW::init_line_smooth()
#include "geom/world.H"
#include "gtex/color_id_texture.H"      // for ColorIDTexture::draw_edges()
#include "gtex/ref_image.H"             // for VisRefImage::lookup()
#include "mesh/mi.H"
#include "mlib/statistics.H"
#include "npr/ffstexture.H"
#include "npr/wpath_stroke.H"
#include "std/config.H"

#include "ti.H"
#include "action.H"
#include "tess_cmd.H"
#include "tex_body.H"
#include "bcurve.H"
#include "bsurface.H"
#include "uv_surface.H"
#include "skin.H"
#include "skin_meme.H"

using namespace mlib;
using namespace tess;

inline bool 
is_stylized() 
{
   return VIEW::peek()->rendering() == FFSTexture::static_name() ||
      VIEW::peek()->rendering() == "FFSTexture2"; 
}

static bool debug_res_level =
Config::get_var_bool("DEBUG_BCURVE_RES_LEVEL",false);

void 
CurveMeme::set_t(double t) 
{
   _t = t;
   do_update();
}

void 
CurveMeme::copy_attribs_v(VertMeme* v)
{
   CurveMeme* p = upcast(v); // parent curve meme

   if (p) {
      static bool debug = Config::get_var_bool("DEBUG_MEMES",false);
      err_adv(debug, "CurveMeme::copy_attribs_v");
      set_t(p->_t);
   }
}

//! this meme comes from an edge in the parent mesh.
//! take an average of _t values from the two memes
//! at the parent edge endpoints
void 
CurveMeme::copy_attribs_e(VertMeme* v1, VertMeme* v2)
{

   if (!map())
      return;

   CurveMeme* p1 = upcast(v1);
   CurveMeme* p2 = upcast(v2);
   if (p1 && p2) {
      static bool debug = Config::get_var_bool("DEBUG_MEMES",false);
      err_adv(debug, "CurveMeme::copy_attribs_e");
      set_t(map()->avg(p1->_t, p2->_t));
   }
}

//! Compute 3D vertex location using the curve map
CWpt& 
CurveMeme::compute_update() 
{
   
   assert(map());
   return (_update = map()->map(_t));
}

inline Wpt
qr_centroid_skincurve(SkinCurveMap* m, Bvert* v)
{
   Wpt ret;  // computed centroid to be returned
   double net_weight = 0; 
   Bedge_list nbrs = v->get_manifold_edges(); // work with primary layer edges
   Bedge_list star;
   for (int i = 0; i < nbrs.num(); i++) {
      Bbase* ctrl = Bbase::find_controller(nbrs[i]);
      if (Bcurve::isa(ctrl) || Skin::isa(ctrl))
         star += nbrs[i];
   }

   if (star.empty())
      return v->loc();

   double avg_len = star.avg_len();
   assert(avg_len > 0);
   for (int i=0; i<star.num(); i++) {
      Bedge* e = star[i];
      if (e->is_strong()) {
         // add contribution from r neighbor
         double w = e->length()/avg_len;
         Bbase* ctrl = Bbase::find_controller(e);
         if (Bcurve::isa(ctrl) && ((Bcurve*)ctrl)->map()==m)
            w = 0.5;
	      ret = ret + w*e->other_vertex(v)->loc();
         net_weight += w;
         Bface* f = ccw_face(e, v);
         if (f && Skin::find_controller(f) && f->is_quad() && f->quad_opposite_vert(v)) {
            // add contribution from q neighbor
            Bvert* q = f->quad_opposite_vert(v);
            w = 0.5*q->loc().dist(v->loc())/avg_len;
            ret = ret + w*q->loc();
            net_weight += w;
         }
      }
   }
   assert(net_weight > 0);
   return ret/net_weight;
}

Wpt  
CurveMeme::smooth_target() const
{
   // Target location for being more "relaxed":

   if (SkinCurveMap::isa(map())) {
      return qr_centroid_skincurve((SkinCurveMap*)map(), vert());
   }
   return VertMeme::smooth_target();

   // Ignore any vertices outside this curve, work toward
   // even spacing of vertices along the curve.

   // XXX - not necessarily the best policy.

   Bvert* v = vert();
   assert(v);
   static EdgeMemeList nbrs;
   get_nbrs(nbrs);
   if (nbrs.num() != 2)
      return loc();
   return (nbrs[0]->edge()->other_vertex(v)->loc() +
           nbrs[1]->edge()->other_vertex(v)->loc())/2;
}


//! Compute a delta in parameter t that moves us toward the
//! vert centroid
bool
CurveMeme::compute_delt()
{

   // reset delt to 0:
   _dt = 0;

   if (is_pinned())
      return false;

   // don't do relaxation when surfaces are absent:
   if (vert()->degree(!PolylineEdgeFilter()) == 0)
      return false;

   Map1D3D* m = map();
   if (!m)
      return false;     // should never happen

   // Constrain vertices at endpoints to stick:
   if (_t == m->t_min() || _t == m->t_max())
      return false;

   // compute _dt, return true on success:
   bool ret = m->solve(_t, target_delt(), _dt, vert()->avg_edge_len()/2);
   if (0 && do_debug()) {
       static ofstream tout;
      static ofstream kout;
      static bool did_init=false;
      if (!did_init) {
         did_init = true;
         tout.open("t.dat", ios::trunc);
         kout.open("k.dat", ios::trunc);
      }
      if (tout && kout) {
         tout << _t << endl;
         assert(map());
         int k= -1;
         map()->get_wpts().interpolate(_t, 0, &k, 0);
         kout << k << endl;
      } else {
         cerr << "CurveMeme::compute_delt: could not open output files"
              << endl;
      }
   }
   return ret;
}

//! Adjust the parameter t as computed previously in
//! compute_delt()
bool
CurveMeme::apply_delt()
{

   if (_dt == 0)
      return false;

   Map1D3D* m = map();
   if (!m)
      return false;      // should never happen

   double old_t = _t;
   _t = m->add(_t, _dt); // add the change, dealing w/ wrap-around
   _dt =  0;             // reset delta t

   // compute and set the corresponding Wpt location:
   if (do_update()) {
      _count++;
      set_hot();  // stay scheduled for iterative relaxation
      if (0 && do_debug()){
         cerr << "CurveMeme::apply_delt: succeeded" << endl;
      }
      return true;
   }
   _t = old_t;
   if (0 && do_debug())
      cerr << "CurveMeme::apply_delt: failed" << endl;
   return false;
}

bool
CurveMeme::apply_update(double thresh)
{
   if (_count >= 20)
      return false;
   return VertMeme::apply_update(thresh);
}

//! notify simplex changed
void
CurveMeme::notify_simplex_changed()
{
   VertMeme::notify_simplex_changed();
   if (!_suspend_change_notification)
      _count = 0;
}

//! Generate a child vert meme for this vert, passing on
//! the t-val:
VertMeme* 
CurveMeme::_gen_child(Lvert* subvert) const 
{

   if (do_debug())
      cerr << "CurveMeme::_gen_child" << endl;

   if (!subvert)
      return 0;
   Bcurve* c = curve()->child();
   assert(c && !c->find_vert_meme(subvert));

   VertMeme* ret = new CurveMeme(c, subvert, _t, is_boss());
   if (is_pinned())
      ret->pin();
   return ret;
}

//! Generate a child vert meme for an edge shared w/ the
//! given vert meme, assigning t-value between the two:
VertMeme* 
CurveMeme::_gen_child(Lvert* subvert, VertMeme* vm) const 
{

   if (!subvert)
      return 0;

   Bcurve* c = curve()->child();
   assert(c && !c->find_vert_meme(subvert));

   CurveMeme* cm = (CurveMeme*)vm; // upcast our comrade

   // Be careful about averaging t-vals for closed curves:
   return new CurveMeme(c, subvert, map()->avg(_t, cm->_t), true);
}

/*****************************************************************
 * Bcurve
 *****************************************************************/
Bcurve::Bcurve(
   CLMESHptr&   mesh,
   CWpt_list&   pts,
   CWvec&       n,
   int          num_edges,
   int          res_lev,
   Bpoint*      b1,
   Bpoint*      b2
   ) : _map(0),
       _skin(0),
       _stroke_3d(0),
       _got_style(0),
       _max_height(0),
       _shadow_visible(true),
       _move_by(0),
       _cur_reshape_mode(RESHAPE_NONE)
{
   set_mesh(mesh);

   Map0D3D* p0 = NULL;
   Map0D3D* p1 = NULL;
   if (b1 || b2) {
      assert(b1 && b2);
      assert(!pts.is_closed());
      p0 = b1->map();
      p1 = b2->map();
      assert(p0 && p1);
   } else {
      assert(pts.is_closed());
   }

   _map = new Wpt_listMap(pts,p0,p1,n);

   // XXX - need to deal with res_level
   if (res_lev < 0) {
      err_adv(debug_res_level,
              "Bcurve::Bcurve: Warning using default res level = 3");
      res_lev = 3;
   }
   set_res_level(res_lev);

   // Don't call make_params(num_edges+1), because it
   // already adds the +1 internally:
   resample(make_params(num_edges), b1, b2);

   if (is_closed())
      _vmemes.pin();

   // Make sure we'll be recomputed
   invalidate();

   hookup();  // set up dependencies on inputs

   if (Config::get_var_bool("DEBUG_BCURVE_CONSTRUCTOR",false))
      err_msg("Bcurve::Bcurve: %d uniform samples, res level %d, %s",
              num_edges, _res_level, is_closed() ? " closed" : "not closed");     
   _stroke_3d = new WpathStroke(mesh);
   _got_style = false;
}


Bcurve::Bcurve(
   CLMESHptr& mesh,
   CWpt_list&      pts,          
   CWvec&          n,          
   CARRAY<double>& t,          
   int             res_lev,
   Bpoint*         b1,      
   Bpoint*         b2 
   ) : _map(0),
       _skin(0),
       _stroke_3d(0),
       _got_style(0),
       _max_height(0),
       _shadow_visible(true),
       _move_by(0),
       _cur_reshape_mode(RESHAPE_NONE)
{
   set_mesh(mesh);

   Map0D3D* p0 = NULL;
   Map0D3D* p1 = NULL;
   if (b1 || b2) {
      assert(b1 && b2);
      assert(!pts.is_closed());
      p0 = b1->map();
      p1 = b2->map();
      assert(p0 && p1);
   } else {
      assert(pts.num() > 1);
      p0 = new WptMap(pts.first(),n);
      p1 = new WptMap(pts.last(), n);
      b1 = new Bpoint(mesh, p0);
      b2 = new Bpoint(mesh, p1);
   }

   _map = new Wpt_listMap(pts,p0,p1,n);

   // XXX - need to deal with res_level
   if (res_lev < 0) {
      err_adv(debug_res_level,
              "Bcurve::Bcurve: Warning using default res level = 3");
      res_lev = 3;
   }
   set_res_level(res_lev);

   resample(t, b1, b2);

   if (is_closed())
      _vmemes.pin();

   // Make sure we'll be recomputed
   invalidate();

   hookup();  // set up dependencies on inputs

   if (Config::get_var_bool("DEBUG_BCURVE_CONSTRUCTOR",false))
      err_msg("Bcurve::Bcurve: %d non-uniform samples, res level %d, %s",
              t.num()-1, _res_level, is_closed() ? " closed" : "not closed");  
   _stroke_3d = new WpathStroke(mesh);
   _got_style = false;
}

Bcurve::Bcurve(
   CLMESHptr&           mesh,
   Map1D3D*             map,
   CARRAY<double>&      t,          
   int                  res_lev,
   Bpoint*              b1,      
   Bpoint*              b2 
   ) : _map(0),
       _skin(0),
       _stroke_3d(0),
       _got_style(0),
       _max_height(0),
       _shadow_visible(true),
       _move_by(0),
       _cur_reshape_mode(RESHAPE_NONE)
{
   // XXX - Need to verify that this actually works correctly

   set_mesh(mesh);

   bool debug = Config::get_var_bool("DEBUG_BCURVE_MAP_CONSTRUCTOR",false);

   if (debug) {
      cerr << " -- in map constructor with t vals: " << endl;
      int i;
      for (i = 0; i < t.num(); i++) {
         cerr << t[i] << " ";
      }
      cerr << endl;
      cerr << "res_lev argument = " << res_lev << endl;
   }

   Map0D3D* p0 = NULL;
   Map0D3D* p1 = NULL;
   if (b1 || b2) {
      assert(b1 && b2);
      assert(!map->is_closed());
      p0 = b1->map();
      p1 = b2->map();

      // we should make sure the endpoints of the map1d3d passed in
      // match those that are stored in the bpoints.

      assert(map->p0() == p0);
      assert(map->p1() == p1);

      assert(p0 && p1);
   } else {
      assert( (p0 == NULL) && (p1 == NULL) );
      assert(map->is_closed());
   }

   _map = map;

   // XXX - need to deal with res_level
   if (res_lev < 0) {
      err_adv(debug_res_level,
              "Bcurve::Bcurve: Warning using default res level = 3");
      res_lev = 3;
   }
   set_res_level(res_lev);

   resample(t, b1, b2);

   if (is_closed())
      _vmemes.pin();

   // Make sure we'll be recomputed
   invalidate();

   hookup();  // set up dependencies on inputs

   if (Config::get_var_bool("DEBUG_BCURVE_CONSTRUCTOR",false))
      err_msg("Bcurve::Bcurve: %d non-uniform samples, res level %d, %s",
              t.num()-1, _res_level, is_closed() ? " closed" : "not closed");    
   _stroke_3d = new WpathStroke(mesh);
   _got_style = false;
}

     
//! This is the constructor for an embedded curve.
Bcurve::Bcurve(
   CUVpt_list& uvpts,
   Map2D3D* surf,
   CLMESHptr& mesh,
   CARRAY<double>& t,
   int res_lev,
   Bpoint* b1,
   Bpoint* b2
   ) : _map(0),
       _skin(0),
       _stroke_3d(0),
       _got_style(0),
       _max_height(0),
       _shadow_visible(true),
       _move_by(0),
       _cur_reshape_mode(RESHAPE_NONE)
{

   assert(mesh);
   assert(uvpts.num() > 1);

   set_mesh(mesh);

   bool debug = Config::get_var_bool("DEBUG_BCURVE_EMBEDDED",false);
   if (debug) 
      {
         iostream i(cerr.rdbuf()); STDdstream e(&i); 
         //STDdstream e(&cerr);
         cerr << " -- in embedded bcurve constructor with uvpts: " << endl;
         e << uvpts;
         cerr << endl << " and t vals" << endl;
         e << t;
         cerr << endl;
      }

   // If the endpoints are not given to us and uvpts does not
   // represent a closed curve, then we need to create bpoints that
   // live in this surface (SurfacePtMap).
   if (!uvpts.is_closed()) {
      if (b1) {
         if (b1->constraining_surface() != surf) 
            // Maybe the endpoints don't really need to live in the surface..
            cerr << "Warning: bcurve endpoint 1 doesn't live in "
                 << "the same surface.." << endl;
      } else {
         b1 = new Bpoint(mesh, surf, uvpts.first());
         if (debug)
            cerr << "Created new endpoint b1 with uvpt"
                 << uvpts.first() << endl;
      }
      
      // same thing for b2
      if (b2) {
         if (b2->constraining_surface() != surf)
            if (debug)
               cerr << "Warning: bcurve endpoint 2 doesn't live in "
                    << "the same surface.." << endl;
         //assert(surf == b2->constraining_surface());
      } else {
         b2 = new Bpoint(mesh, surf, uvpts.last());
         if (debug)
            cerr << "Created new endpoint b2 with uvpt "
                 << uvpts.last() << endl;
      }

      assert(b1->constraining_surface() == b2->constraining_surface());

   } else {
      // If we have a closed curve, endpoints must be null
      assert(b1 == NULL);
      assert(b2 == NULL);
   }

   _map = new SurfaceCurveMap(surf, uvpts,
                              b1 ? b1->map() : NULL,
                              b2 ? b2->map() : NULL);

   if (res_lev < 0) {
      err_adv(debug_res_level,
              "Bcurve.C line %d Warning using default res level = 3", __LINE__);
      res_lev = 3;
   }
   set_res_level(res_lev);

   resample(t, b1, b2);

   if (is_closed())
      _vmemes.pin();

   // Make sure we'll be recomputed
   invalidate();

   // set up dependencies on inputs (i.e. _map)
   hookup();
   
   if (Config::get_var_bool("DEBUG_BCURVE_CONSTRUCTOR",false))
      err_msg("Bcurve::Bcurve: curve in sruface, res level %d, %s",
              _res_level, is_closed() ? "closed" : "not closed");        
   _stroke_3d = new WpathStroke(mesh);
   _got_style = false;
}


inline double
get_param( CUVpt_list& uvs, int k ) 
{
   if (uvs.is_closed())
      return uvs.partial_length(k)/uvs.length()+1e-12;
   else
      return uvs.partial_length(k)/uvs.length();
}

inline double
get_param( CWpt_list& pts, int k )
{
   if (pts.is_closed())
      return pts.partial_length(k)/pts.length()+1e-12;
   else
      return pts.partial_length(k)/pts.length();
}

//! verts is the vert list on skeleton faces
//! or the vert list on skin faces
Bcurve::Bcurve(
   CBvert_list& verts,
   Bsurface* surf,
   int res_lev
   ) : _map(0),
       _skin(surf),
       _stroke_3d(0),
       _got_style(0),
       _max_height(0),
       _shadow_visible(true),
       _move_by(0),
       _cur_reshape_mode(RESHAPE_NONE)
{
   assert(Skin::isa(surf));

   // create bcurve that lives on a specified set of vertices on a skin
   bool debug = Config::get_var_bool("DEBUG_BCURVE_ON_SKIN",false);

   LMESH* m = LMESH::upcast(verts.mesh());
   assert(m);
   set_mesh(m);

   bool is_closed = (verts.last()==verts.first()) && 
      !Bcurve::find_controller(verts.last());

   if (is_closed) {
      err_adv(debug, "vert list is closed");
   }   
   
   Skin* skin = (Skin*)surf;
   Bvert_list s_verts = skin->get_mapper()->a_to_b(verts);
   if (s_verts.empty())
      s_verts = verts;

   // if the endpoints are not given to us and verts does not
   // represent a closed curve, then we need to create bpoints that
   // live in this CURVE (CurvePtMap) .. except for when this
   // is a closed curve

   Bpoint* b1 = 0;
   Bpoint* b2 = 0;
   Bvert* first = s_verts.first();
   Bvert* last = s_verts.last();
   if (first->get_all_faces().empty()) first = verts.first();
   if (last->get_all_faces().empty()) last = verts.last();

   if ( !is_closed ) {
     
      b1 = Bpoint::find_owner(first);
      if (!b1) {
         Bcurve* c = Bcurve::find_controller(first);
         if (!c) {
            b1 = new Bpoint((Lvert*)first, res_lev);
         } else {
            Bvert_list cur_verts = c->cur_subdiv_verts();
            Wpt_list cur_pts = cur_verts.pts();

            for (int i = 0; i < cur_verts.num(); i++) {
               double t = get_param(cur_pts, i);
               if (cur_verts[i] == first)
                  b1 = new Bpoint((Lvert*)first, t, c->map(), res_lev);
               if (!Bpoint::find_controller(cur_verts[i]) && 
                  !Bbase::find_boss_meme(cur_verts[i])) {
                  new CurveMeme(c, (Lvert*)cur_verts[i], t, true);
               }
            }
            if (!b1) {
               double t = 0.5;
               c->map()->invert(first->loc(), t, t);
               b1 = new Bpoint((Lvert*)first, t, c->map(), res_lev);
            }
         }
      }
      
      // same thing for b2
      b2 = Bpoint::find_owner(last);
      if (!b2) {
         Bcurve* c = Bcurve::find_controller(last);
         if (!c) {
            b2 = new Bpoint((Lvert*)last, res_lev);
         } else {
            Bvert_list cur_verts = c->cur_subdiv_verts();
            Wpt_list cur_pts = cur_verts.pts();

            for (int i = 0; i < cur_verts.num(); i++) {
               double t = get_param(cur_pts, i);
               if (cur_verts[i] == last)
                  b2 = new Bpoint((Lvert*)last, t, c->map(), res_lev);
               if (!Bpoint::find_controller(cur_verts[i]) && 
                  !Bbase::find_boss_meme(cur_verts[i])) {
                  new CurveMeme(c, (Lvert*)cur_verts[i], t, true);
               }
            }
            if (!b2) {            
               double t = 0.5;
               c->map()->invert(last->loc(), t, t);
               b2 = new Bpoint((Lvert*)last, t, c->map(), res_lev);
            }
         }
      }

   }

   Bsimplex_list simps;
   ARRAY<Wvec> bcs;
   for (int i = !is_closed; i < s_verts.num()-!is_closed; i++) {
      assert(SkinMeme::isa(Bbase::find_boss_meme(s_verts[i])));
      SkinMeme* meme = (SkinMeme*)Bbase::find_boss_meme(s_verts[i]);
      simps += (meme->track_simplex());
      bcs += meme->get_bc();
   }
   _map = new SkinCurveMap(simps, bcs, skin,
      b1 ? b1->map() : NULL,
      b2 ? b2->map() : NULL);
   Wpt_list pts = _map->get_wpts();

   if (res_lev < 0) {
      err_adv(debug_res_level,
              "Bcurve.C line %d Warning using default res level = 3", __LINE__);
      res_lev = 3;
   }
   set_res_level(res_lev);

   // create vert memes
   // 1st vertex:

   CurveMeme* cm;
   if (is_closed)
      cm = new CurveMeme(this, (Lvert*)s_verts.first(), get_param(pts, 0), true);
   else
      cm = new CurveMeme(this, (Lvert*)first, get_param(pts, 0), false);

   // Compute the index of the last vertex for which we want to create a meme:
   // if the curve is closed, leave out the last vertex (which is the same as 
   // the first) to avoid adding a duplicate meme to the first vertex.
   int n = verts.num();
   if (is_closed) 
      --n;

   // Interior vertices: 2nd param to last
   for (int k=1; k<n; k++) {
      if (is_closed || k < n-1)
         cm = new CurveMeme(this, (Lvert*)s_verts[k], get_param(pts, k), true);
      else if (verts.first() != verts.last())
         cm = new CurveMeme(this, (Lvert*)last, 1, false);
   }

   build_strip();

   //Make sure we'll be recomputed
   invalidate();

   // set up dependencies on inputs.. what are our inputs?
   hookup();

   _stroke_3d = new WpathStroke((CLMESHptr&)m);
   _got_style = false;
}
     
//! create bcurve that lives on a specified set of vertices
Bcurve::Bcurve(
   CBvert_list& verts,
   UVpt_list uvpts,
   Map2D3D* surf,
   int res_lev
   ) : _map(0),
       _skin(0),
       _stroke_3d(0),
       _got_style(0),
       _max_height(0),
       _shadow_visible(true),
       _move_by(0),
       _cur_reshape_mode(RESHAPE_NONE)
{

   bool debug = Config::get_var_bool("DEBUG_BCURVE_ON_VERTS",false);

   LMESH* m = LMESH::upcast(verts.mesh());
   assert(m);
   set_mesh(m);

   assert(verts.num()>1 && verts.num()==uvpts.num());
   bool is_closed = (verts.last()==verts.first());

   if (is_closed) {
      err_adv(debug, "vert list is closed");
      if (uvpts.last() == uvpts.first()) {
         err_adv(debug, "uv pt list is closed");
      }
      assert(uvpts.last() == uvpts.first());
   }

   // if the endpoints are not given to us and uvpts does not
   // represent a closed curve, then we need to create bpoints that
   // live in this surface (SurfacePtMap) .. except for when this
   // is a closed curve

   Bpoint* b1 = 0;
   Bpoint* b2 = 0;

   if ( !is_closed ) {
     
      b1 = Bpoint::find_owner(verts.first());
      b2 = Bpoint::find_owner(verts.last());

      if (b1) {
         if (b1->constraining_surface() != surf) 
            // maybe the endpoints don't really need to live in the surface..
            err_adv(debug, "Warning: bcurve endpoint 1 doesn't live in the same surface..");
         //assert(surf == b1->constraining_surface());
      } else {
         // XXX -- here and below, is cast to Lvert* definitely safe?
         b1 = new Bpoint((Lvert*)verts.first(), uvpts.first(), surf, res_lev);
         if (debug)
            cerr << "Created new endpoint b1 with uvpt" << uvpts.first() << endl;
      }
      
      // same thing for b2
      if (b2) {
         if (b2->constraining_surface() != surf)
            err_adv(debug, "Warning: bcurve endpoint 2 doesn't live in the same surface..");
         //assert(surf == b2->constraining_surface());
      } else {
         b2 = new Bpoint((Lvert*)verts.last(), uvpts.last(), surf, res_lev);
         if (debug)
            cerr << "Created new endpoint b2 with uvpt "
                 << uvpts.last() << endl;
      }

      //assert(b1->constraining_surface() == b2->constraining_surface());

   }

   // at this point we are guaranteed that b1 and b2 point to
   // something meaningful, ie, either they point to meaningful
   // bpoints or the UV point list is closed, in which case we can
   // just pass them as nulls along to the SurfaceCurveMap constructor
   // below

   _map = new SurfaceCurveMap( surf, 
                               uvpts, 
                               b1 ? b1->map() : NULL,
                               b2 ? b2->map() : NULL);

   if (res_lev < 0) {
      err_adv(debug_res_level,
              "Bcurve.C line %d Warning using default res level = 3", __LINE__);
      res_lev = 3;
   }
   set_res_level(res_lev);

   // create vert memes
   // 1st vertex:

   new CurveMeme(this, (Lvert*)verts.first(), get_param(uvpts, 0), is_closed);

   // Compute the index of the last vertex for which we want to create a meme:
   // if the curve is closed, leave out the last vertex (which is the same as 
   // the first) to avoid adding a duplicate meme to the first vertex.
   int n = verts.num();
   if (is_closed) 
      --n;

   // Interior vertices: 2nd param to last
   for (int k=1; k<n; k++) {
      new CurveMeme(this, (Lvert*)verts[k], get_param(uvpts, k), k<n-1 || is_closed);
   }

   build_strip();

   //Make sure we'll be recomputed
   invalidate();

   // set up dependencies on inputs.. what are our inputs?
   hookup();
   
   if (Config::get_var_bool("DEBUG_BCURVE_CONSTRUCTOR",false))
      err_msg("Bcurve::Bcurve: curve on verts, res level %d, %s",
              _res_level, is_closed ? " closed" : "not closed");

   _stroke_3d = new WpathStroke((CLMESHptr&)m);
   _got_style = false;
}

//! Create a child Bcurve of the given parent:
Bcurve::Bcurve(Bcurve* parent) :
   _map(0),
   _skin(0),
   _stroke_3d(0),
   _got_style(0),
   _max_height(0),
   _shadow_visible(true),
   _move_by(0),
   _cur_reshape_mode(RESHAPE_NONE)
{

   assert(parent);
   _map = parent->_map;

   // Record the parent and produce children if needed:
   set_parent(parent);

   // Make sure we'll be recomputed
   invalidate();

   hookup();

   // Child Bcurves don't use their EdgeStrip and don't get in the
   // BMESH drawables list.

   // XXX - in fact, all those methods that use the EdgeStrip to get
   // at the sequence of edges and vertices don't work for child
   // Bcurves and shouldn't be called.

   if (Config::get_var_bool("DEBUG_BCURVE_CONSTRUCTOR",false))
      err_msg("Bcurve::Bcurve: child curve, res level %d, %s",
              _res_level, is_closed() ? " closed" : "not closed");
   _stroke_3d = new WpathStroke(parent->_mesh);
   _got_style = false;
}


Bcurve::~Bcurve()
{
   destructor();

   // delete it?
   _map = 0;

   // Bbase destructor removes vert memes.
   // We have to remove our edge memes.
   static bool debug = Config::get_var_bool("DEBUG_BNODE_DESTRUCTOR",false);
   err_adv(debug, "Bcurve::~Bcurve: deleting %d edge memes", _ememes.num());
   _ememes.delete_all();
}

bool
Bcurve::apply_update()
{
   for (int i = 0; i < _vmemes.num(); i++) {
      if (CurveMeme::isa(_vmemes[i]))
         ((CurveMeme*)_vmemes[i])->reset_count();
   }
   return Bbase::apply_update();
}

//! Get the edge strip at level k relative to the control curve.
CEdgeStrip* 
Bcurve::strip(int k) const 
{

   if (k < 0) {
      err_msg("Bcurve::strip: bad level (%d)", k);
      return 0;
   }
   Bcurve* c = ctrl_curve();
   if (!c) {
      err_msg("Bcurve::strip: null control curve");
      return 0;
   }
   CEdgeStrip* ret = c->_strip.sub_strip(k);
   if (!ret) {
      err_msg("Bcurve::strip: sub strip at level %d is null", k);
   }
   return ret;
}

CEdgeStrip* 
Bcurve::cur_strip() const 
{
   Bcurve* c = ctrl_curve();
   if (!c) {
      err_msg("Bcurve::strip: null control curve");
      return 0;
   }
   return strip(c->rel_cur_level());
}

// XXX - move to method of Bedge_list
inline void
set_creases(CBedge_list& edges, unsigned short k)
{
   for (int i=0; i<edges.num(); i++)
      edges[i]->set_crease(k);
}

void 
Bcurve::set_res_level(int r)
{
   // Set the "resolution level":

   Bbase::set_res_level(r);

   // in case memes are wimping out early, also set crease flags
   // on the control edges, down to the given level
   if (is_control()) {
      set_creases(edges(), (unsigned short)r);
   }
}

ARRAY<double> 
Bcurve::tvals() const
{
   ARRAY<double> ret(_vmemes.num());
   for (int i=0; i<_vmemes.num(); i++)
      ret += ((CurveMeme*)_vmemes[i])->t();
   return ret;
}

/*****************************************************************
 * refinement
 *****************************************************************/
void
Bcurve::produce_child()
{
   if (_child)
      return;
   if (!_mesh->subdiv_mesh()) {
      err_msg("Bcurve::produce_child: Error: no subdiv mesh");
      return;
   }

   // It hooks itself up and takes care of everything...
   // even setting the child pointer is not necessary here.
   _child = new Bcurve(this);
}

/*****************************************************************
 * adjacency
 *****************************************************************/

//! Find adjacent surfaces registered on adjacent faces at top-level
//! subdivision mesh (i.e. the control mesh).
Bsurface_list 
Bcurve::surfaces() const
{

   // XXX - probably should not favor top level.

   Bsurface* surf=0;
   Bsurface_list ret;
   CARRAY<Bedge*>& edges = _strip.edges();
   for (int k=0; k<edges.num(); k++) {
      if ((surf = Bsurface::find_controller(edges[k]->f1())))
         ret.add_uniquely(surf);
      if ((surf = Bsurface::find_controller(edges[k]->f2())))
         ret.add_uniquely(surf);
   }
   return ret;
}

//! The curve is "isolated" if there are no adjacent faces and the
//! set of control edges is "maximal" (no adjacent foreign edges):
bool
Bcurve::is_isolated() const
{
   //
   // XXX -
   //   Doesn't address the possibility that at some 
   //   subdivision level > 0 the story might be different.
   return is_polyline() && is_maximal(edges());
}

//! return unit vector originating at v pointing along e.
//! e must contain v.
inline Wvec
edge_vec(CBvert* v, CBedge* e)
{
   assert(e && v);
   Bvert* u = e->other_vertex(v);
   assert(u);
   return (v->loc() - u->loc()).normalized();
}

inline Bcurve*
next_curve(Bpoint* b, Bcurve* c, bool ccw)
{
   static bool debug = Config::get_var_bool("DEBUG_CRV_BOUNDARIES",false);

   assert(b && c && c->contains(b) && b->vert());
   Bvert* v = b->vert();

   // vector from b along c:
   Wvec vec = (c->b1() == b) ?
      edge_vec(v, c->edges().first()) :
      edge_vec(v, c->edges().last());

   Wvec n = VIEW::eye_vec(b->loc()); // unit vector from point to eye
   if (ccw)
      n = -n;

   // edges to check for next curve:
   Bedge_list adj = b->vert()->get_manifold_edges();
   Bcurve* ret = 0;
   double min_angle = 0;
   for (int i=0; i<adj.num(); i++) {
      Bedge*    e = adj[i];
      Bcurve* crv = Bcurve::find_owner(e);
      if (!crv || crv->is_hidden() || crv == c)
         continue;
      double a = signed_angle(vec, edge_vec(v,e),n);
      if (a < 0)
         continue;
      if (!ret || a < min_angle) {
         ret = crv;
         min_angle = a;
      }
   }
   if (debug) {
      cerr << "next_curve (" << (ccw ? "ccw" : "cw") << "): curve: "
           << (ret ? ret->identifier() : str_ptr("null"))
           << ", angle: " << rad2deg(min_angle) << endl;
   }
   return ret;
}

inline bool
trace_curves(Bcurve* crv, Bcurve_list& result, bool ccw, bool debug=false)
{
   result.clear();

   if (!crv)
      return false;

   Bcurve* c = crv;
   Bpoint* b = c->b1();
   assert(b);
   do {
      result += c;
      b = c->other_point(b);
      c = next_curve(b,c,ccw);
      if (c == crv) {
         err_adv(debug, "trace_curves: succeeded (%s)",
                 (ccw ? "ccw" : "cw"));
         return true;
      }
   } while (c);
   result.clear();
   return false;
}

//! Given a Bcurve, try to extend it to form a closed
//! boundary. If it's possible, put the list of bcurves in
//! result and return true. Otherwise return false.
bool
Bcurve::extend_boundaries(Bcurve_list& result)
{
   static bool debug = Config::get_var_bool("DEBUG_CRV_BOUNDARIES",false);

   result.clear();

   // if this curve is closed, it's the boundary
   if (is_closed()) {
      result += this;
      return true;
   }

   // try winding CCW, then CW:
   if (trace_curves(this, result, true,  debug) ||
       trace_curves(this, result, false, debug)) {
      err_adv(debug, "Bcurve::extend_boundaries: succeeded");
      return true;
   } else {
      err_adv(debug, "Bcurve::extend_boundaries: failed");
      return false;
   }
}

Bcurve_list
Bcurve::extend_boundaries()
{
   Bcurve_list ret;
   extend_boundaries(ret);
   return ret;
}

/*****************************************************************
 * accessing vert lists and Wpt lists
 *****************************************************************/

//! Vertices of this curve.

//! The Bvert_list in the EdgeStrip omits the last vertex
//! because it's stored implicitly (found in the last edge).
//! We get it explicitly below, unless the curve is closed, 
//! in which case it's redundant anyway.
Bvert_list
Bcurve::verts() const
{
   

   EdgeStrip    s = get_strip();
   Bvert_list ret = s.verts();
   if (!(is_closed() || s.empty()))
      ret += s.last();
   return ret;
}

//! Same as verts(), but for closed curves first vertex
//! appears again at the end of the list.
Bvert_list
Bcurve::full_verts() const
{

   EdgeStrip    s = get_strip();
   Bvert_list ret = s.verts();
   if (!s.empty())
      ret += s.last();
   return ret;
}

//! Edges of the curve at the current subdivision level.
CBedge_list&
Bcurve::cur_subdiv_edges() const
{
   

   Bcurve* cur = cur_curve();
   if (cur) {
      return cur->edges();
   }
   static Bedge_list ret;
   return ret;
}

//! Vertices of the curve at the current subdivision level.
//! (For a closed curve, does not repeat the first vertex at the end).
Bvert_list
Bcurve::cur_subdiv_verts() const
{

   // See note in Bcurve::verts() above.

   Bvert_list ret;
   CEdgeStrip* cur = cur_strip();
   if (cur) {
      ret = cur->verts();
      if (!(is_closed() || cur->empty()))
         ret += cur->last();
   } else {
      err_msg("Bcurve::cur_subdiv_verts: can't get cur strip");
   }
   return ret;
}

/*****************************************************************
 * Bnode methods
 *****************************************************************/
Bnode_list 
Bcurve::inputs() const 
{
   Bnode_list ret = Bbase::inputs();

   if (_map)    ret += (Bnode*) _map;
   return ret;
}

void
Bcurve::recompute()
{
   if (_map)
      Bbase::recompute();
}

void
Bcurve::delete_elements()
{
   static bool debug = Config::get_var_bool("DEBUG_BCURVE_DELETE_ELS",false);

   if (_child)
      _child->delete_elements();

   // XXX - can't use Bbase::delete_elements() until Bcurves start
   //       using VertMemes. Should switch.
   // XXX - the above comment must be years old...

   // Wipe out edges and interior vertices

   // First thing we do is kill all the memes:
   _vmemes.delete_all();
   _ememes.delete_all();

   // Get this before possibly eliminating the edge (below)
   // because the last vert is found via the edge.
   Bvert_list ctrl = verts();

   // Normally the edges will get destroyed when interior vertices do
   // ... but not if there's just one edge with no interior vertices.
   if (num_edges() == 1) {
      if (debug)
         err_msg("Bcurve::delete_elements: removing single edge");
      _mesh->remove_edge(edge(0));
   }

   // Eliminate first vertex:
   if (!ctrl.empty() && !Bpoint::lookup(ctrl[0])) {
      if (debug)
         err_msg("Bcurve::delete_elements: removing 1st vertex");
      _mesh->remove_vertex(ctrl[0]);
   }

   // Eliminate last vertex:
   if (ctrl.num() > 1 && !Bpoint::lookup(ctrl.last())) {
      if (debug)
         err_msg("Bcurve::delete_elements: removing last vertex");
      _mesh->remove_vertex(ctrl.last());
   }

   // Eliminate interior vertices:
   for (int i=1; i<ctrl.num()-1; i++) {
      if (debug)
         err_msg("Bcurve::delete_elements: removing interior vertex %d/%d",
                 i, ctrl.num()-1);
      _mesh->remove_vertex(ctrl[i]);
   }

   // The deed is done ... now reset the strip
   _strip.reset();
}



/*****************************************************************
 * resampling
 *****************************************************************/
inline Wpt_list 
sample(const Wpt_list& pts, double max_err, bool closed)
{
   Wpt_list result = pts;
   Wpt_list   orig = pts;
   orig.update_length();
   
   for (int num = closed ? 4 : 1; num < orig.num(); num *= 2) {

      double dt = 1.0/num;
      
      // resample the polyline with num samples
      int i;
      result.clear();
      for (i=0; i<num; i++)
         result += orig.interpolate(i*dt);
      result += orig.last();

      // find the average error
      double error = 0;
      for (i=0; i<orig.num(); i++)
         error += result.closest(orig[i]).dist(orig[i]);
      error /= orig.num();

      // if error is acceptable, return this result.
      // otherwise we'll double the number of samples and try again.
      if (error < max_err)
         return result;
   }
   // we were going to use more samples than in the original, so
   // instead of that, just return the original. after all, it has
   // "zero error".
   return orig;
}

bool 
Bcurve::can_resample() const
{
   // Preconditions for being able to resample a Bcurve:
   //   * isn't embedded
   
   static bool debug = Config::get_var_bool("DEBUG_BCURVE_RESAMPLE",false);

   if (!is_control()) {
      if (debug)
         err_msg("Bcurve::can_resample: this is not a control curve");
      return false;
   }

   // For now just support Bcurves with no adjacent faces
   if (!(_strip.empty() || is_polyline())) {
      if (debug)
         err_msg("Bcurve::can_resample: can't resample embedded curve");
      return false;
   }

   if (_map == NULL) {
      if (debug)
         err_msg("Bcurve::can_resample: no map!");
      return false;
   }

   return true;
}

bool
Bcurve::resample(int n)
{
   if (num_edges() == n)
      return true;

   if (!can_resample())
      return false;

   int min_edges = _map->is_closed() ? MIN_EDGES_FOR_CLOSED_CURVE :
      MIN_EDGES_FOR_OPEN_CURVE ;

   // Ensure number of edges >= 1
   if (n < min_edges)
      return false;

   return resample(make_params(n), NULL, NULL);
}

bool 
Bcurve::resample(CARRAY<double>& t, CBpoint* bpt1, CBpoint* bpt2)
{
   // Check preconditions
   if (!can_resample()   ||
       !is_increasing(t) ||
       t[0]     != 0.0   ||
       t.last() != 1.0)
      return false;

   this->_resample(t, bpt1, bpt2);
   return true;
}


//! Internal resampling that does actual work.
//! Assumes this bcurve can be resampled.
void
Bcurve::_resample(CARRAY<double>& t, CBpoint* bpt1, CBpoint* bpt2) 
{   
   if (_map->is_closed()) {
      if (bpt1 || bpt2) {
         err_msg("Bcurve::_resample: Unexpected Bpoints for a closed curve");
         assert(0);
      }
   } else {
      if (bpt1 == NULL && bpt2 == NULL) {
         bpt1 = b1();
         bpt2 = b2();
      } 
        
      // we don't want one to be NULL and not the other
      assert(bpt1 != NULL);
      assert(bpt2 != NULL);
      assert(bpt1 != bpt2); // sanity check.. this was a problem once
   }

   // kills the mesh elements owned by this bcurve. as well as vert
   // meme list and edge memes too
   delete_elements();

   // 1st vertex:
   Lvert* v = (bpt1 ? bpt1->vert() :
               (Lvert*)_mesh->add_vertex(_map->map(t[0])));
   new CurveMeme(this, v, t[0], bpt1 == NULL);

   // Interior vertices: 2nd param to second to last
   for (int k=1; k<t.num() - 1; k++) {
      v = (Lvert*)_mesh->add_vertex(_map->map(t[k]));
      new CurveMeme(this, v, t[k], true);
   }


   // Last one: for closed curves we're done. Otherwise put
   // a non-boss meme on the Bpoint's vertex:
   if (bpt2)
      new CurveMeme(this, bpt2->vert(), t.last(), false);

   build_strip();
}

//! Build the strip from the vert meme array
//! assume vert meme array is in order of connectivity
void
Bcurve::build_strip()
{

   if (!is_control()) {
      err_msg("Bcurve::build_strip: error: called on non-control curve");
      return;
   }
   // Start fresh
   _strip.reset();

   // Create the edges and build the strip
   for (int k=0; k<_vmemes.num()-1; k++) {
      // Create the edge and slap an edge meme on it:
      EdgeMeme* e = add_edge(_vmemes[k]->vert(),_vmemes[k+1]->vert());
      assert(e);

      // Add it to the strip:
      _strip.add(_vmemes[k]->vert(), e->edge());
   }
   if (_map->is_closed()) {
      EdgeMeme* e = add_edge(_vmemes.last()->vert(),_vmemes[0]->vert());
      assert(e);
      _strip.add(_vmemes.last()->vert(), e->edge());
   }
}


/*****************************************************************
 * Bbase methods
 *****************************************************************/
VertMeme*
Bcurve::add_vert_meme(VertMeme* v)
{
   if (Bbase::add_vert_meme(v)) {

      // XXX - if we wanted the vert memes kept in sorted order
      //       we could enforce that here.

      return v;
   }
   return 0;
}

void 
Bcurve::rem_edge_meme(EdgeMeme* e) 
{
   if (!e)
      err_msg("Bcurve::rem_edge_meme: Error: meme is nil");
   else if (e->bbase() != this)
      err_msg("Bcurve::rem_edge_meme: Error: meme owner not this");
   else {
      _ememes -= e;
      delete e;
      err_adv(Config::get_var_bool("DEBUG_MEME_DESTRUCTOR",false),
              "%s::rem_edge_meme: %d left at level %d",
              **class_name(),
              _ememes.num(),
              bbase_level()
         );

      // The curve should not be drawn now:
      // XXX - necessary?
      if (is_control() && !_strip.empty()) {
         _strip.reset();
         _mesh->drawables() -= this;
      }
   }
}

EdgeMeme* 
Bcurve::add_edge_meme(EdgeMeme* e)
{
   if (e && e->bbase() == this) {
      _ememes += e;
      return e;
   }
   return 0;
}


EdgeMeme* 
Bcurve::add_edge_meme(Ledge* e) 
{
   // Screen out the wackos
   if (!e)
      return 0;

   // Don't create a duplicate meme:
   EdgeMeme* em = find_edge_meme(e);
   if (em)
      return em;

   // No more excuses...
   return new EdgeMeme(this, e, true); // it adds itself to the _ememe list
}

void 
Bcurve::set_selected() 
{
   Bbase::set_selected();
   if (is_skel())
      texbody()->add_post_drawer(this);
}

void 
Bcurve::unselect() 
{
   Bbase::unselect();
   if (is_skel())
      texbody()->rem_post_drawer(this);
}

/*****************************************************************
 * drawing
 *****************************************************************/
Bcurve_list 
Bcurve::selected_curves()
{
   return Bcurve_list(_selection_list);
}

Bcurve* 
Bcurve::selected_curve()
{
   Bcurve_list curves = selected_curves();
   return (curves.num() == 1) ? curves[0] : 0;
}

CCOLOR& 
Bcurve::selection_color() const 
{
   return Color::blue_pencil_d;
}

CCOLOR& 
Bcurve::regular_color() const 
{
   return Color::grey2;
}

bool
Bcurve::should_draw() const
{
   if (!(is_control() && _is_shown))
      return false;

   if (is_selected())
      return true;

   if (is_inner_skel() || is_in_surface_any_level())
      return false;

   return true;
}

static const double BCURVE_WIDTH = 4;

int 
Bcurve::draw(CVIEWptr& v)
{
   // Ensure it's up-to-date
   update();

   // Curves embedded in surfaces look ugly...
   // use a hack to avoid drawing them:
   if (!should_draw()) {
//      draw_debug();     // usually a no-op, unless debug mode
      return 0;
   }

   // if stylized, drawing will occur during final pass, not now:
   static bool draw_curves = !Config::get_var_bool("HIDE_BCURVES",false);
   if (draw_curves && !is_stylized()) {
      // turn on antialiasing and set line width
      GLfloat w = GLfloat(BCURVE_WIDTH * v->line_scale());
      GL_VIEW::init_line_smooth(w, GL_CURRENT_BIT);
      glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
      glColor3dv(draw_color().data());     // GL_CURRENT_BIT

      // draw with a simple strip callback object:
      GLStripCB cb;
      _strip.draw(&cb);

      GL_VIEW::end_line_smooth();
   }     
   
   // draw shadow here

   // but draw the memes if requested (for debugging)
   // even if it is embedded:
   draw_debug();

   return _strip.num();
}

int 
Bcurve::draw_vis_ref()
{
   // draw to the visibility reference image (for picking)
   if (!is_control()) {
      err_msg("Bcurve::draw_vis_ref: error: called on non-control curve");
      return 0;
   }
   if (!_is_shown)
      return 0;
   update();
   return ColorIDTexture::draw_edges(&_strip, 4.0);
}

int
Bcurve::draw_final(CVIEWptr& v)
{
   // Curves embedded in surfaces look ugly...
   // use a hack to avoid drawing them:
   if (!should_draw()) {
      return 0;
   }

   if (is_stylized()) {
      if(!_got_style){
         get_style();         // finds needed info from ffs style file
         _got_style = true;
      }
      return _stroke_3d->draw(v);
   }
   return 0;
}

void
Bcurve::request_ref_imgs()
{    
   if (is_stylized())
      IDRefImage::schedule_update();
}

int
Bcurve::draw_id_ref_pre1()
{          
   if (is_stylized()){
      _stroke_3d->set_stroke_3d(cur_strip());
      return _stroke_3d->draw_id_ref_pre1();
   } else {
      return 0;
   }
}

int
Bcurve::draw_id_ref_pre2()
{
   if (is_stylized())
      return _stroke_3d->draw_id_ref_pre2();
   else
      return 0;
}

int
Bcurve::draw_id_ref_pre3()
{
   if (is_stylized())
      return _stroke_3d->draw_id_ref_pre3();
   else
      return 0;
}

int
Bcurve::draw_id_ref_pre4()
{
   if (is_stylized())
      return _stroke_3d->draw_id_ref_pre4();
   else
      return 0;
}

void Bcurve::get_style ()
{
   _stroke_3d->get_style("nprdata/ffs_style/bcurve.ffs");
}

void
Bcurve::cycle_reshape_mode() 
{
   cerr << "Bcurve::cycle_reshape_mode(), cur mode: " 
        << _cur_reshape_mode << endl;

   _cur_reshape_mode = (reshape_mode_t)( (_cur_reshape_mode + 1)%
                                         NUM_RESHAPE_MODES );

   cerr << "after cycle, cur mode: " 
        << _cur_reshape_mode << endl;
}


bool
t_val(CWpt_list& pts, int i, double& ret_t)
{
   if (!pts.valid_index(i)) {
      cerr << "t_val(), invalid index" << endl;
      return false;
   }

   if (pts.empty()) {
      cerr << "t_val(), pts empty" << endl;
      return false;
   }

   if ( isZero(pts.length()) ) {
      cerr << "t_val(), zero length point list" << endl;
      return false;
   }

   ret_t = pts.partial_length(i)/pts.length();

   return true;
}

//! Returns a set of 3D lines given by the set of the map's world
//! points and their associated normals.  If get_binormals is true,
//! then the binormals (instead of the normals) are used for the line
//! directions.
bool
Bcurve::get_map_normal_lines(
   ARRAY<Wline>& ret_lines, 
   double len, // desired world length
   bool get_binormals)
{
   // need to have a map
   if (!_map)
      return false;

   Wpt_list pts = get_wpts();
  
   // XXX -- bug in map?  
   // get_wpts() should return point list with valid lengths 
   pts.update_length();

   bool total_success = true;

   for ( int i=0; i<pts.num(); i++ ) {
      double t = 0.0;
      if ( !t_val(pts, i, t) ) {
         cerr << "Bcurve::draw_normals(), WARNING: failed to get t val for pt " 
              << i << endl;
         total_success = false;
         continue;
      }
      Wpt start = _map->map(t);
      Wvec dir = ( get_binormals ? _map->binorm(t) : _map->norm(t) );
      Wpt end = start + (len * dir);

      ret_lines +=  Wline(start, end);
   }

   return total_success;
}

bool
Bcurve::get_reshape_constraint_lines(ARRAY<Wline>& ret_lines, 
                                     reshape_mode_t mode,
                                     double len  )
{
   ret_lines.clear();

   Bvert_list verts = cur_subdiv_verts();

   if (verts.num() < 2) 
      return false;

   switch(mode){

    case RESHAPE_NONE: 
    {
       return false;
       break;
    } 
    case RESHAPE_MESH_NORMALS: 
    {
       for ( int i=0; i<verts.num(); i++ ) {
          Wpt start = verts[i]->loc();
          Wpt end = start + (len * verts[i]->norm());
          ret_lines +=  Wline(start, end);
       }
       return true;
       break;
    }
    case RESHAPE_MESH_BINORMALS: 
    {
       bool total_success = true;

       for ( int i=0; i<verts.num(); i++ ) {
          Wpt start = verts[i]->loc();

          // Use the direction of the curve edge containing this vertex
          // for the tangent direction to compute the binormal.  Find
          // the edge connecting this vertex to the next one in the
          // curve.  (In the case of the last vertex, find the edge
          // connecting to the previous vertex.)

          // find the index of the 
          int other_i = (i < verts.num()-1) ? i+1 : i-1;
          Bedge *e = verts[i]->lookup_edge(verts[other_i]);

          if (!e) {
             cerr << "Bcurve::get_reshape_constraint_lines(): "
                "warning, couldn't get edge for tangent" 
                  << endl;
             total_success = false;
             continue;
          }

          // get the tangent
          Wvec t = e->vec();

          if (t.is_null()) {
             cerr << "Bcurve::get_reshape_constraint_lines(): "
                "warning, zero edge vec found" 
                  << endl;
             total_success = false;
             continue;
          }

          Wvec bi_norm = cross(t, verts[i]->norm());
          bi_norm = bi_norm.normalized();

          // make sure binorm is valid

          if (bi_norm.is_null()) {
             cerr << "Bcurve::get_reshape_constraint_lines(): "
                "warning, zero length binormal computed" 
                  << endl;
             total_success = false;
             continue;
          }

          Wpt end = start + (len * bi_norm);
          ret_lines +=  Wline(start, end);
       }
       return total_success;

       break;
    }
    case RESHAPE_MESH_RIGHT_FACE_TAN: 
    case RESHAPE_MESH_LEFT_FACE_TAN: 
    {
       bool total_success = true;

       for ( int i=0; i<verts.num(); i++ ) {

          Wpt start = verts[i]->loc();

          // Use the direction of the curve edge containing this vertex
          // for the tangent direction to compute the binormal.  Find
          // the edge connecting this vertex to the next one in the
          // curve.  (In the case of the last vertex, find the edge
          // connecting to the previous vertex.)

          // find the index of the other vertex connected by the edge
          int other_i = (i < verts.num()-1) ? i+1 : i-1;
          Bedge *e = verts[i]->lookup_edge(verts[other_i]);

          if (!e) {
             cerr << "Bcurve::get_reshape_constraint_lines(): "
                "warning, couldn't get edge for tangent" 
                  << endl;
             total_success = false;
             continue;
          }

          // get the tangent
          Wvec t = e->vec();

          if (t.is_null()) {
             cerr << "Bcurve::get_reshape_constraint_lines(): "
                "warning, zero edge vec found" 
                  << endl;
             total_success = false;
             continue;
          }

          // get the vertex on one of e's adjoining faces that's
          // on the desired side with respect to the tangent

          Bface* adj_face = 0;

          if ( mode == RESHAPE_MESH_RIGHT_FACE_TAN ) {
             adj_face = e->other_face(e->ccw_face());
          } else {
             adj_face = e->ccw_face();
          }

          if (!adj_face) {
             cerr << "Bcurve::get_reshape_constraint_lines(): "
                "warning, no adjacent face found" 
                  << endl;
             total_success = false;
             continue;
          }

          Bvert* side_vert = adj_face->other_vertex(e);

          if (!side_vert) {
             cerr << "Bcurve::get_reshape_constraint_lines(): "
                "warning, no side vert found" 
                  << endl;
             total_success = false;
             continue;
          }

          Wvec side_face_tan = side_vert->loc() - verts[i]->loc();

          side_face_tan = side_face_tan.orthogonalized(t);

          side_face_tan = side_face_tan.normalized();

          // make vector is valid

          if (side_face_tan.is_null()) {
             cerr << "Bcurve::get_reshape_constraint_lines(): "
                "warning, zero length tangent to side face computed" 
                  << endl;
             total_success = false;
             continue;
          }

          Wpt end = start + (len * side_face_tan);
          ret_lines +=  Wline(start, end);
       }
       return total_success;

       break;
    }

    default: 
    {
       cerr << "Bcurve::get_reshape_constraint_lines(): ERROR, "
          "unknown reshape mode" 
            << endl;
       return false;
    }

   } // end switch

   return false;
}

void
Bcurve::draw_normals(double len, 
                     CCOLOR& col,
                     bool) 
{
   if (!_is_shown)
      return;

   // create a line along the normal through each point on the map
   ARRAY<Wline> norm_lines;
   //get_map_normal_lines(norm_lines, len, draw_binormals);
   get_reshape_constraint_lines(norm_lines, 
                                //RESHAPE_MESH_NORMALS,
                                //RESHAPE_MESH_BINORMALS,
                                //RESHAPE_MESH_RIGHT_FACE_TAN,
                                _cur_reshape_mode,
                                len);

   GL_VIEW::draw_lines(norm_lines,
                       col, 
                       1.0,  // alpha
                       2,    // width
                       true  // do stipple
      );
}

void
Bcurve::draw_points(int size,
                    CCOLOR& col) 
{
   if (!_is_shown)
      return;

   Wpt_list pts = get_wpts();

   if (pts.empty())
      return;

   double a = 0.8; // alpha
   GL_VIEW::draw_wpt_list(pts, col, a, size, false); // false = no stippling
}


void
Bcurve::draw_debug()
{
   if (!_is_shown)
      return;

   static bool debug_norms = Config::get_var_bool("DEBUG_CURVE_NORMS",false);
   if (debug_norms) {
      draw_normals(1, Color::grey5);
   }
   static bool debug_binorms = Config::get_var_bool("DEBUG_CURVE_BINORMS",false);
   if (debug_binorms) {
      bool draw_binorms = true;
      draw_normals(1.0, Color::yellow, draw_binorms);
   }
   static bool debug_pts = Config::get_var_bool("DEBUG_CURVE_PTS",false);
   if (debug_pts) {
      draw_points(1, Color::green);
   }

   if (_show_memes) {
      const double BASE_SIZE = 8.0;
      double size = BASE_SIZE * pow(0.75, cur_level());
      show_memes(Color::blue_pencil_d, Color::blue_pencil_l,(float) size);
   }
}

/*****************************************************************
 * coordinate frames
 *****************************************************************/

//! Associated normal vector (e.g. for planar curves):
Wvec 
Bcurve::normal() const
{

   Wvec ret;
   if (_map)
      ret = _map->norm();
   if (!ret.is_null())
      return ret;

   err_mesg(ERR_LEV_WARN, "Bcurve::normal: no cached normal... computing");

   if (control_pts().get_plane_normal(ret))
      return ret;

   err_mesg(ERR_LEV_ERROR, "Bcurve::normal: can't compute plane normal");
   return Wvec();
}

Wvec 
Bcurve::vector() const
{
	Wvec ret;
	if((b1() == b2()) || is_closed()) {
		err_mesg(ERR_LEV_ERROR, "Bcurve::vector: curve is closed.");
	}
	
	ret = b2()->o() - b1()->o();
	
	return ret;
}

//! Compute the "tangent" vector at the given vertex.
Wvec
Bcurve::t(CBvert* v) const
{

   // Note: This relies on the fact that each edge of a Bcurve is
   // "directed" in the forward direction along the Bcurve. That
   // is, the forward direction of an edge runs from v1 to v2
   // (its two vertices), which have the same order in the edge
   // that they have in the Bcurve. Further, we require that this
   // holds true for all subdivision levels as well.
   //
   // So if someone later changes the code to start switching
   // around the order of vertices in Bcurve edges, there's going
   // to be hell to pay.

   // Let's hope this is never actually necessary:
   if (!v)
      return Wvec();

   // Find the 0, 1 or 2 edges of this Bcurve that are adjacent
   // to the given vertex and average their vectors. If the given
   // vertex does not belong to this Bcurve the result will be
   // the null vector:
   Wvec ret;
   for (int k=0; k<v->degree(); k++)
      if (find_controller(v->e(k)) == this) // XXX - controller or owner?
         ret += v->e(k)->vec().normalized();
   return ret.normalized();
}

//! Compute the "tangent" vector at the given edge.
Wvec
Bcurve::t(CBedge* e) const
{

   // Same caveat as Bcurve::t(CBvert*) -- see above.
   // (Namely, We rely on the ordering of vertices in the edge
   // remaining as the Bcurve built them.)

   // XXX - controller or owner?
   return (find_controller(e) == this) ? e->vec().normalized() : Wvec();
}


Bcurve* 
Bcurve::hit_curve(CNDCpt& p, double rad, Wpt& hit, Bedge** edge)
{
   // get the visibility reference image:
   VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());
   if (!vis_ref) {
      err_msg("Bcurve::hit_curve: error: can't get vis ref image");
      return 0;
   }
   vis_ref->update();

   // Find an edge of a Bcurve in the search region, and get the Bcurve:
   Bedge* e = (Bedge*)vis_ref->find_near_simplex(p, rad, BcurveFilter());

   Bcurve* ret = find_controller(e);
   if (ret) {
      // Fill in the hit point itself
      double dist, d2d; Wvec n; // dummies
      e->view_intersect(p, hit, dist, d2d, n);
      if (edge)
         *edge = e;
   }
   return ret;
}

void 
Bcurve::notify_xform(BMESH*, CWtransf& xf, CMOD& mod)
{
   apply_xf(xf, mod);
}

bool
Bcurve::apply_xf(CWtransf& xf, CMOD& mod)
{
   bool debug = Config::get_var_bool("DEBUG_BBASE_XFORM");
   if (!_map) {
      err_adv(debug, "Bcurve::apply_xf: error: null map");
      return false;
   }
   if (!is_control())
      return true;
   if (!_map->can_transform()) {
      if (debug) {
         cerr << "Bcurve::apply_xf: can't transform map: "
              << _map->class_name() << endl;
      }
      return false;
   }
   _map->transform(xf, mod);
   return true;
}

/*****************************************************************
 * tests
 *****************************************************************/
class InSurfaceEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->num_all_faces() > 0;
   }
};

//! Return true if every edge has at least one adjacent face
//! (counting f1, f2, and "multi" edges)
bool 
Bcurve::is_in_surface() const 
{

   return edges().all_satisfy(InSurfaceEdgeFilter());
}

bool 
Bcurve::is_closed() const 
{
   if (_map) 
      return _map->is_closed();

   CEdgeStrip* s = strip();
   if (!s || s->empty())
      return false;

   return (s->num_line_strips() == 1 && s->first() == s->last());
}

//! Return true if the curve or any of its children satisfies the test
bool
Bcurve::satisfies_any_level(crv_test_meth sat) const
{

   return ((this->*sat)()  ? true : child() ? (child()->*sat)() : false);
}

bool 
Bcurve::get_plane(Wplane& P, double len_scale) const 
{ 
   if (is_straight(len_scale)) {
      P = Wplane(control_pts().average(), normal());
      return P.is_valid();
   } else {
      return control_pts().get_plane(P, len_scale); 
   }
}

/*****************************************************************
 * Bcurve_list
 *****************************************************************/

//! Are the curves connected in the order of the list?
bool 
Bcurve_list::is_connected() const
{

   if (empty())
      return false;
   for (int i=1; i<_num; i++)
      if (!::is_connected(_array[i], _array[i-1]))
         return false;
   return true;
}

//! Are curves are connected together, one after the other?
bool 
Bcurve_list::forms_chain() const
{

   return !get_chain().empty();
}

//! Do curves form a chain, with last connected to first?
bool 
Bcurve_list::forms_closed_chain() const
{

   if (num() == 1 && first()->is_closed())
      return true;
   Bpoint_list chain = get_chain();
   return (chain.num() > 1 && chain.first() == chain.last());
}

//! If the curves form a chain, return the bpoints in
//! order. (If chain is closed, last point == first).
Bpoint_list 
Bcurve_list::get_chain() const
{

   Bpoint_list ret(num() + 1);
   Bpoint* b = _array[0]->b1();
   if (num() > 1 && _array[1]->contains(b))
      b = _array[0]->b2();
   ret += b;
   for (int i=0; i<num(); i++) {
      b = _array[i]->other_point(b);
      if (!b) {
         return Bpoint_list();
      }
      ret += b;
   }
   return ret;
}

bool 
Bcurve_list::is_planar() const
{
   if (empty())
      return false;

   for (int k=0; k<_num; k++)
      if (!(_array[k]->plane().is_valid() &&
            (k < 1 || _array[k]->plane().is_equal(_array[k-1]->plane()))))
         return false;

   return true;
}

Wplane
Bcurve_list::get_plane() const
{
   if (!is_planar())
      return Wplane();
   assert(!empty());
   return first()->plane();
}

bool 
Bcurve_list::is_each_straight() const
{
   if (empty())
      return false;

   for (int k=0; k<_num; k++)
      if (!_array[k]->is_straight())
         return false;

   return true;
}

//! Returns total length of curves
double 
Bcurve_list::total_length() const 
{

   double ret = 0;
   for (int i=0; i<_num; i++)
      ret += _array[i]->length();
   return ret;
}

//! Length of shortest curve:
double 
Bcurve_list::min_length() const 
{

   if (empty())
      return 0;
   double ret = first()->length();;
   for (int i=1; i<_num; i++)
      ret = min(ret, _array[i]->length());
   return ret;
}

//! Length of longest curve:
double 
Bcurve_list::max_length() const 
{

   if (empty())
      return 0;
   double ret = first()->length();;
   for (int i=1; i<_num; i++)
      ret = max(ret, _array[i]->length());
   return ret;
}

//! Returns average edge length of curves at given
//! subdivision level (relative to each curve)
double 
Bcurve_list::avg_edge_len(int level) const 
{

   double ret = 0;
   for (int i=0; i<_num; i++)
      ret += _array[i]->avg_edge_length(level);
   return _num ? ret/_num : 0.0;
}

Wpt
Bcurve_list::center() const
{
   Wpt ret;
   for (int i=0; i<_num; i++)
      ret = ret + _array[i]->center();
   return _num ? ret/_num : ret;
}

//! Can they all be resampled?
bool 
Bcurve_list::can_resample() const
{

   for (int i=0; i<_num; i++)
      if (!_array[i]->can_resample())
         return false;
   return true;
}

//! Given a connected sequence of Bcurves forming a closed path,
//! extract the control vertices, arranged to run in a consistent
//! order around the path (agreeing with the first Bcurve).
bool
Bcurve_list::extract_boundary(Bvert_list& ret) const
{

   ret.clear();

   if (empty())
      return false;

   if (!forms_closed_chain())
      return false;

   if (num() == 1) {
      ret = first()->verts();
      return true;
   }

   for (int i=0; i<_num; i++) {
      Bvert_list v = _array[i]->verts();
      if (v.num() < 2)
         return false;
      if (ret.num() > 0) {
         // If it's not the first time thru the loop,
         // this curve must connect to the previous one...
         // is it oriented forward or backward?
         if (ret.last() == v.first()) {
            // It's in the right order (oriented forward)
            // now pull out the duplicate 1st vertex:
            v.pull_index(0);
         } else if (ret.last() == v.last()) {
            // It's backwards -- fix it
            v.pop();
            v.reverse();
         } else {
            // There is no else
            return false;
         }
      }
      // add new verts via array concatenation:
      ret += v;
   }

   if (ret.num() < 3)
      return false;

   if (ret.first() == ret.last())
      ret.pop();

   return true;
}

//! Similar to above, but the control vertices of each Bcurve are
//! returned in a separate list (1 for each Bcurve):
bool
Bcurve_list::extract_boundary(ARRAY<Bvert_list>& ret) const
{

   ret.clear();

   if (empty())
      return false;

   if (!forms_closed_chain())
      return false;

   for (int i=0; i<_num; i++) {
      Bvert_list v = _array[i]->verts();
      if (v.num() < 2)
         return false;
      if (ret.num() > 0) {
         // If it's not the first time thru the loop,
         // this curve must connect to the previous one...
         // is it oriented forward or backward?
         if (v.first() == ret.last().last()) {
            // It's in the right order (oriented forward)
         } else if (v.last() == ret.last().last()) {
            // It's backwards -- fix it
            v.reverse();
         } else {
            // There is no else
            return false;
         }
      }
      ret += v;
   }

   return true;
}

inline bool
can_fill_ccw(CBvert_list& verts)
{
   static bool debug = Config::get_var_bool("DEBUG_BCURVE_BOUNDARY",false);
   if (!verts.forms_chain()) {
      err_adv(debug, "can_fill_ccw: error: verts do not form a chain");
      return false;
   }

   // get the chain of edges connecting the vertices. 'true' means
   // also get the edge connecting last and first vertices:
   Bedge_list chain = verts.get_chain(true);

   if (chain.all_satisfy(PolylineEdgeFilter())) {
      return true;
   }
   if (chain.any_satisfy(InteriorEdgeFilter())) {
      err_adv(debug, "can_fill_ccw: 1 or more edges are interior");
      return false;
   }
   assert(verts.num() == chain.num()   ||
          verts.num() == chain.num()+1);
   for (int i=0; i<chain.num(); i++) {
      if (chain[i]->ccw_face(verts[i]) != NULL) {
         err_adv(debug, "can_fill_ccw: a face lies in the interior");
         return false;
      }
   }
   return true;
}

inline bool
can_fill_ccw(ARRAY<Bvert_list>& contour)
{
   // assumes the contour is legitimate
   // (it forms a closed chain of vertices).

   for (int i=0; i<contour.num(); i++)
      if (!can_fill_ccw(contour[i]))
         return false;
   return true;
}

inline void
do_reverse(Bvert_list& ret)
{
   // reverses order of the vertices in a special way so:
   //   0 1 2 3 4 
   // becomes:
   //   0 4 3 2 1

   assert(ret.num() > 0);
   ret += ret.first();
   ret.reverse();
   ret.pop();
}

inline void
do_reverse(ARRAY<Bvert_list>& ret)
{
   // reverses order in a special way so:
   //   0 1 2 3 4 
   // becomes:
   //   0 4 3 2 1
   // and each Bvert_list is also reversed

   assert(ret.num() > 0);
   ret += ret.first();
   ret.reverse();
   ret.pop();
   for (int i=0; i<ret.num(); i++)
      ret[i].reverse();
}

inline bool
is_cw(CBvert_list& chain)
{
   Wpt  o = chain.center();
   Wvec n = -VIEW::eye_vec(o);
   return chain.pts().winding_number(o,n) < 0;
}

bool
Bcurve_list::extract_boundary_ccw(Bvert_list& ret) const
{
   if (!extract_boundary(ret))
      return false;

   if (ret.get_chain(true).all_satisfy(PolylineEdgeFilter())) {
      // can orient how we prefer...  make it run CCW around its
      // center as seen from the camera:
      if (is_cw(ret)) {
         do_reverse(ret);
      }
      return true;
   }
   if (!can_fill_ccw(ret)) {
      do_reverse(ret);
   }
   if (!can_fill_ccw(ret)) {
      return false;
   }
   return true;
}

bool
Bcurve_list::extract_boundary_ccw(ARRAY<Bvert_list>& ret) const
{
   if (!extract_boundary(ret))
      return false;

   if (all_edges().all_satisfy(PolylineEdgeFilter())) {
      // can orient how we prefer...  make it run CCW around its
      // center as seen from the camera:
      if (is_cw(boundary_verts())) {
         do_reverse(ret);
      }
      return true;
   }
   if (!can_fill_ccw(ret)) {
      do_reverse(ret);
   }
   if (!can_fill_ccw(ret)) {
      return false;
   }
   return true;
}

void 
Bcurve_list::draw_debug() const
{
   for (int k=0; k<_num; k++)
      _array[k]->draw_debug();
}

Bedge_list 
Bcurve_list::all_edges() const
{
   Bedge_list ret;
   for (int k=0; k<_num; k++)
      ret += _array[k]->edges();
   return ret;
}

/*****************************************************************
 * oversketch methods
 *****************************************************************/

//! Given a list of world pts, replace the curve from
//! base_start to base_end % verts.num(). Working mod
//! verts.num() is necessary for closed curves, where verts
//! is the list of vertices of the curve without duplicates.
//! Vertices affected are put into the array 'affected', and
//! the index of each (WRT the control vertex list) is put
//! into the array 'indices'.
void 
Bcurve::rebuild_seg(
   Wpt_list&      new_pts,
   int            base_start,
   int            base_end,
   ARRAY<Lvert*>& affected,
   ARRAY<int>&    indices
   )
{

   static bool debug = Config::get_var_bool("OVERSKETCH_DEBUG",false);
   new_pts.update_length();
   Bvert_list vertices = verts();
   if (is_closed())
      vertices.pop();
   int n = vertices.num();

   // Build list of affected vertices and their indices in the
   // control verts list:
   int k;
   affected.clear();
   for (k = base_start; k <= base_end; k++) {
      // If there are endpoints, skip over them. I.e.,
      // endpoint verts are not put in the "affected" list.
      // (Probably because the Bcurve doesn't control them.)
      if (!is_closed() && (k%n == 0 || k%n == n-1))
         continue;
      affected += (Lvert*)vertices[k%n];
      indices  += k%n;
   }

   // Build a Wpt_list from the affected vertex locations:
   Wpt_list orig_arc(affected.num());
   for (k=0; k<affected.num(); k++)
      orig_arc += affected[k]->loc();
   orig_arc.update_length();

   // Find the parameter value of each affected vertex WRT the
   // orig_arc polyline... Then use that same parameter to get
   // the interpolated point from the new_pts list, and set
   // each vertex location to that.
   //
   // XXX - The following is Probably a bad idea.
   //
   // Better: based on the mumber of affected vertices, just
   // uniformly resample the new_pts list to get the new
   // locations for the affected vertices.
   //
   for (k=0; k<affected.num(); k++) { 
      double t = orig_arc.partial_length(k)/orig_arc.length();
      if (debug) cerr << "k=" << indices[k] << ", t=" << t <<endl;
      affected[k]->set_loc(new_pts.interpolate(t));
   }
}

int
direction_of_curve(
   CPIXEL_list& crv_a,
   CPIXEL_list& crv_b,
   double thresh,
   int point_a, //!< the point in a we are interested in the direction of
   int point_b) //!< the closest point in b to this point in a
{

   if ( !(crv_a.num() > 1 && crv_b.num() > 1) ) {
      cerr << "direction asked with too short curves, returning 0" << endl;
      return 0;
   }

   if (    point_a < 0 || point_a >= crv_a.num()
           || point_b < 0 || point_b >= crv_b.num() ) {
      cerr << "direction asked at wrong place, returning 0" << endl;
      return 0;
   }

   VEXEL v_a;
   if (point_a < crv_a.num() - 1)
      v_a = (crv_a[point_a + 1] - crv_a[point_a]).normalized();
   else
      v_a = (crv_a[point_a] - crv_a[point_a - 1]).normalized();

   double pos_amt;
   if (point_b + 1 < crv_b.num())
      pos_amt = (crv_b[point_b + 1] - crv_b[point_b]).normalized() * v_a;
   else
      pos_amt = (crv_b[point_b] - crv_b[point_b - 1]).normalized() * v_a;
   
   double neg_amt = 0;
   if (point_b - 1 >= 0)
      neg_amt = (crv_b[point_b - 1] - crv_b[point_b]).normalized() * v_a;
   else
      neg_amt = (crv_b[point_b] - crv_b[point_b + 1]).normalized() * v_a;

   if (    (pos_amt < thresh && neg_amt < thresh)
           || (pos_amt > thresh && thresh > 0.75) )
      return 0;
   return(pos_amt > thresh)?( 1 ):( -1 );
}

//! Splice crv_a into crv_b, but only if endpoints of crv_a are
//! within thresh distance of crv_b.  If splicing is successful,
//! store the result in new_crv and return true.
bool
Bcurve::splice_curves(
   PIXEL_list   crv_a, //!< non-const because curve may need to be reversed
   CPIXEL_list& crv_b, 
   double thresh,
   PIXEL_list& new_crv)
{
   bool debug = false;

   // Check that curves are sufficiently long
   if (crv_a.num() < 2 || crv_b.num() < 2) {
      if (debug) cerr << "splice_curves(): curves are not both sufficiently long" << endl;
      return false;
   }

   // Check if the endpoints of crv_a are sufficiently close to crv_b

   int start_i = -1; // closest point on crv_b to start of crv_a
   int end_i = -1;   // closest point on crv_b to end of crv_a

   double dist_start, dist_end; // distances of crv_a start and end points to crv_b
   PIXEL dummy;
   crv_b.closest(crv_a[0],     dummy,   dist_start, start_i);
   crv_b.closest(crv_a.last(), dummy,   dist_end,   end_i);

   if (dist_start > thresh && dist_end > thresh) {
      if (debug) cerr << "splice_curves(): curves too far apart, aborting" << endl;
      return false;
   }

   int before_start_i = start_i; //the point before we want to splice
   int after_end_i = end_i; //the point after we're done splicing

// the distaces that were too far get are set to the proper endpoint:
   double DIR_THRESH = Config::get_var_dbl("SPLICE_DIR_THRESH", 0.75);
   if (dist_start > thresh) {
      int end_dir = direction_of_curve(crv_a, crv_b, DIR_THRESH, crv_a.num() - 1, end_i);
      if (end_dir == 0) {
         if (debug) cerr << "direction for end not defined, bailing" << endl;
         return false;
      }
      start_i = (end_dir == 1)?(0):(crv_b.num()-1);
      before_start_i = (end_dir == 1)?(-1):(crv_b.num());
   }

   if (dist_end > thresh) {
      int start_dir = direction_of_curve(crv_a, crv_b, DIR_THRESH, 0, start_i);
      if (start_dir == 0) {
         if (debug) cerr << "direction for start not defined, bailing" << endl;
         return false;
      }
      end_i = (start_dir == 1)?(crv_b.num()-1):(0);
      after_end_i = (start_dir == 1)?(crv_b.num()):(-1);
   }

   //now a pass to clean up before_start and after_end
   if (dist_start <= thresh)
      if ( (crv_b[start_i] - crv_a[0]) * (crv_a[1] - crv_a[0]) > 0 )
         before_start_i += (start_i > end_i)?(1):(-1);

   if (dist_end <= thresh)
      if ( (crv_b[end_i] - crv_a.last()) * (crv_a[crv_a.num() - 2] - crv_a.last()) > 0 )
         after_end_i += (start_i > end_i)?(-1):(1);

   // sanity check
   assert(start_i >= 0 && start_i < crv_b.num());
   assert(end_i >= 0 && end_i < crv_b.num());

   // Find direction
   if (start_i > end_i) {
      crv_a.reverse();
      swap(start_i, end_i);
      swap(before_start_i, after_end_i);
   }

   // Do the splicing: crv_b pixels with indices between start_i and
   // end_i will be replaced by crv_a pixels.

   bool added = false;

   for (int i = 0; i < crv_b.num(); i++) {
      if (i < before_start_i) {
         // keep original crv_b pixels before the splice start point
         new_crv += crv_b[i];
      } else if (i > after_end_i) {
         // keep original crv_b pixels past the splice end point
         new_crv += crv_b[i];
      } else {
         if (i == before_start_i) new_crv += crv_b[i];
         if (!added) {
            // insert crv_a pixels
            int k;
            for (k = 0; k < crv_a.num(); k++) {
               new_crv += crv_a[k];
            }
            added = true;
         }
         if (i != before_start_i && i == after_end_i) new_crv += crv_b[i];
      }
   }


   return true;
}


// for now, have a separate function to deal with closed curve,
// till I get this to work
bool
Bcurve::splice_curve_on_closed(
   PIXEL_list   crv_a, // non-const because curve may need to be reversed
   PIXEL_list   crv_b, // non-const because curve may need to be shifted
   double thresh,
   PIXEL_list& new_crv)
{
   bool debug = false;

   if (debug) cerr << "splice_curve_on_closed()" << endl;

   if (debug) cerr << "crv_a num: " << crv_a.num() << endl;
   if (debug) cerr << "crv_b num: " << crv_b.num() << endl;

   new_crv.clear();

   // record original starting point of crv_b to help 
   // "ushifting" the result curve below
   CPIXEL crv_b_orig_start = crv_b.first();

   // Splice crv_a into crv_b, but only if endpoints of crv_a are
   // within thresh distance of crv_b.  If splicing is successful,
   // store the result in new_crv and return true.

   // Check that curves are sufficiently long
   if (crv_a.num() < 2 || crv_b.num() < 2) {
      if (debug) cerr << "splice_curve_on_closed(): curves are not both sufficiently long"
           << endl;
      return false;
   }

   // Check if the endpoints of crv_a are sufficiently close to crv_b

   int start_i = -1; // closest point on crv_b to start of crv_a
   int end_i = -1;   // closest point on crv_b to end of crv_a
   int start_dir = 0; //these two flags show the direction
   int end_dir = 0;   //(which is good to know for later) --Jim

   double dist_start, dist_end; // distances of crv_a start and end points to crv_b
   PIXEL dummy;
   crv_b.closest(crv_a[0],     dummy,   dist_start, start_i);
   crv_b.closest(crv_a.last(), dummy,   dist_end,   end_i);


   if (debug) cerr << "start_i: " << start_i << endl;
   if (debug) cerr << "end_i: " << end_i << endl;

   // Reject if the crv_a is too far from crv_b.
   if (dist_start > thresh || dist_end > thresh) {
      if (debug) cerr << "splice_curve_on_closed(): curves too far apart, aborting" << endl;
      return false;
   }

//for determining the direction flags above --Jim

   //this might need some tweaking:
   double DIR_THRESH = Config::get_var_dbl("SPLICE_DIR_THRESH", 0.75);
   { //determine start_dir:
      if (crv_a[1] == crv_a[0]) {
         if (debug) cerr << "Degenerate curve start" << endl;
         return false;
      }
      double pos_amt =   (crv_b[(start_i + 1) % crv_b.num()] - crv_b[start_i]).normalized()
         * (crv_a[1] - crv_a[0]).normalized();
      double neg_amt =   (crv_b[(start_i + crv_b.num() - 1) % crv_b.num()] - crv_b[start_i]).normalized()
         * (crv_a[1] - crv_a[0]).normalized();
      if ( (pos_amt < DIR_THRESH && neg_amt < DIR_THRESH) || (pos_amt > DIR_THRESH && DIR_THRESH > 0.75) )
         if (debug) cerr << " start is too close to call, will have to wait for end..." << endl;
      else start_dir = (pos_amt > DIR_THRESH)?( 1 ):( -1 );
   }

   { //determine end_dir:
      if (crv_a[crv_a.num() - 2] == crv_a.last()) {
         if (debug) cerr << "Degenerate curve end" << endl;
         return false;
      }
      double pos_amt =   (crv_b[(end_i + 1) % crv_b.num()] - crv_b[end_i]).normalized()
         * (crv_a[crv_a.num() - 2] - crv_a.last()).normalized();
      double neg_amt =   (crv_b[(end_i + crv_b.num() - 1) % crv_b.num()] - crv_b[end_i]).normalized()
         * (crv_a[crv_a.num() - 2] - crv_a.last()).normalized();
      if ( (pos_amt < DIR_THRESH && neg_amt < DIR_THRESH) || (pos_amt > DIR_THRESH && neg_amt > DIR_THRESH) )
         if (start_dir == 0) {
            if (debug) cerr << "both start and end too close to call, aborting" << endl;
            return false;
         } else {
            if (debug) cerr << "guessing end direction from start direction" << endl;
            end_dir = -start_dir;
         }
      else {
         end_dir = (pos_amt > DIR_THRESH)?( 1 ):( -1 );
         if (start_dir == 0) {
            start_dir = -end_dir;
            if (debug) cerr << "guessing start direction from end direction" << endl;
         }
      }
   }

   if (start_dir == end_dir) {
      if (debug) cerr << "curve wants to start and end in the same direction, aborting" << endl;
      return false;
   }
//end of added code chunk --Jim

   // sanity check
   assert(start_i >= 0 && start_i < crv_b.num());
   assert(end_i >= 0 && end_i < crv_b.num());

   // fix the direction of crv_a, if necessary
   //if (start_i > end_i) {
   if (start_dir == -1) { // We should always start heading in the
                          // positive direction --Jim
      if (debug) cerr << "reversing crv_a" << endl;
      crv_a.reverse();
      swap(start_i, end_i);
      swap(start_dir, end_dir); //--Jim
      if (debug) cerr << "start_i: " << start_i << endl;
      if (debug) cerr << "end_i: " << end_i << endl;
   }

   // Do the splicing: crv_b pixels with indices between start_i and
   // end_i will be replaced by crv_a pixels.

   bool added = false;

   for (int i = 0; i < crv_b.num(); i++) {
      //if (i < start_i) {
      if ((start_i <= end_i) && ((i < start_i) || (i > end_i))) { //--Jim
         // keep original crv_b pixels before the splice start point
         new_crv += crv_b[i];
         //} else if (i > end_i) {
      } else if ((start_i >= end_i) && ((i < start_i) && (i > end_i))) { //--Jim
         // keep original crv_b pixels past the splice end point
         new_crv += crv_b[i];
      } else {
         if (!added) {
            // insert crv_a pixels
            for (int k = 0; k < crv_a.num(); k++) {
               new_crv += crv_a[k];
            }
            added = true;
         }
      }
   }

   assert(added); // " am I sane, am I sane; but she just won't go
                  // away, and I feel fine " - Noise Elysium (added by Jim)

   // I'm not sure if the following is the correct approach, but I'm
   // going to leave it in anyway. --Jim

   // Find the closest point, p_closest, on new_crv to the
   // original crv_b start pixel.  Shift new_crv so that it
   // begins at p_closest.

   int closest_i = -1;   // index of closest point on new_crv to crv_b_orig_start

   double dummy_dist;   // not used
   PIXEL dummy_closest; // not used
   new_crv.closest(crv_b_orig_start, dummy_closest,   dummy_dist, closest_i);

   if (debug) cerr << "closest_i" << closest_i << endl;

   new_crv.shift(-closest_i);


   new_crv += new_crv[0]; //make it closed. --Jim

   return true;
}


bool 
Bcurve::oversketch(CPIXEL_list& sketch_pixels)
{   
   bool debug = Config::get_var_bool("OVERSKETCH_DEBUG",false);
   err_adv(debug, "Bcurve::oversketch");

   // First, check parameters 

   if (sketch_pixels.num() < 2) {
      cerr << "Bcurve::oversketch(): too few sketch pixels" << endl;
      return false;
   }

   // Attempt to slice the sketch pixels into the current screen path of the curve
   PIXEL_list curve_pixels;

   bool not_smoothed = Config::get_var_bool("OVERSKETCH_REAL_CURVE",false);

   if ( not_smoothed || constraining_surface() || constraining_skin()) {
      // if the curve has a constraining surface map, project the curve
      // points given by the map to pixel space 
      get_pixels(curve_pixels);
   } else {
      // otherwise, project the vertex locations at the current
      // subdivision level to pixel space
      Bvert_list verts = cur_subdiv_verts();     
      for (int i=0; i<verts.num(); i++) {
         curve_pixels += PIXEL(verts[i]->loc());
      } 
   }

   if (curve_pixels.num()<2) {
      cerr << "Bcurve::oversketch(): insufficient number of curve pixel points"
           << endl;
      return false;
   }

   double DIST_THRESH = 15.0;
   PIXEL_list new_curve;

   if (is_closed()) {
      if (!splice_curve_on_closed(sketch_pixels, curve_pixels,
                                  DIST_THRESH, new_curve)) {
         err_adv(debug, "Bcurve::oversketch: oversketch closed curve failed");
         return false;
      }
   } else if (!splice_curves(sketch_pixels, curve_pixels,
                             DIST_THRESH, new_curve)) {
      err_adv(debug, "Bcurve::oversketch: can't splice in oversketch stroke");
      return false;
   }

   // Now reshape the curve to the new desired pixel path.
   //
   // If the curve lies within a constraining surface, oversketch
   // within that surface. Otherwise, try to oversketch a planar curve
   // within its plane, or try an oversketch that displaces the curve
   // along the surface normals of the surface surrounding the curve
   // (if any).
   if (constraining_surface()) {
      return reshape_on_surface(new_curve);
   } else if (constraining_skin()) {
      return reshape_on_skin(new_curve);
   } else {
      // alexni - if planar curve, oversketch onto the plane
      return reshape_on_plane(new_curve) || reshape_along_normals(new_curve);
   } 
}


UVpt
random_uv()
{
   return UVpt(drand48(), drand48()); 
}


bool
intersect_surface(CPIXEL_list& pixels,
                  Map2D3D* map,
                  CUVpt& first_guess,
                  UVpt_list& out_list,
                  int max_num_attempts)
{
   if (!map) {
      cerr << "intersect_surface(), ERROR: NULL map" << endl;
      return false;
   }

   if (!map) {
      cerr << "intersect_surface(), ERROR: no pixels" << endl;
      return false;
   }
   UVpt latest_guess = first_guess;

   for ( int i=0; i<pixels.num(); i++ ) {
      UVpt result_uv;
      int num_attempts = 0;

      while ( (!map->invert_ndc(pixels[i], latest_guess, result_uv, 25))
         ){//|| (VIEW::eye_vec(map->map(result_uv))*map->norm(result_uv) >= -1e-2)) {
         cerr << "intersect_surface(), ERROR: invert_ndc failed, attempt "
              << num_attempts << "; the " << i << "th pixel" << endl;
         ++num_attempts;
         if (num_attempts > max_num_attempts){
            // give up, we failed
            return false;
         }
         latest_guess = random_uv();
      }

      // if we got here, we succeded 
      out_list += result_uv;
      latest_guess = result_uv;
   }

   return true;
}

//! If this Bcurve is planar, try to reshape the curve within its plane.
bool
Bcurve::reshape_on_plane(const PIXEL_list &new_curve)
{
   //
   // XXX - should test wether the plane is sufficiently aligned
   // toward the camera. Otherwise projecting screen points into the
   // plane gives bad results.

   bool debug = Config::get_var_bool("RESHAPE_ON_PLANE_DEBUG",false);
   Wpt_listMap* m = Wpt_listMap::upcast(_map);
   if (!m) {
      err_adv(debug, "Bcurve::reshape_on_plane: rejecting non Wpt_list map");
      return false;
   }
   Wplane P;
   if (!get_plane(P)) {
      err_adv(debug, "Bcurve::reshape_on_plane: rejecting non-planar curve");
      return false;
   }

   Wpt_list pts;
   new_curve.project_to_plane(P, pts);

   // get old shape from map: m->get_wpts();
   // new shape:              pts
   // create WPT_LIST_RESHAPE_CMDptr from old, new, and m
   // WORLD::add_command(cmd)
   WPT_LIST_RESHAPE_CMDptr  cmd= new WPT_LIST_RESHAPE_CMD(m,pts);
   WORLD::add_command(cmd);

   //or can be represented in one line as  //   WORLD::add_command(new (WPT_LIST_RESHAPE_CMD(m,pts)));
   // m->set_pts(pts);

   err_adv(debug, "Bcurve::reshape_on_plane: succeeded");
   return true;
}

bool
Bcurve::reshape_on_skin(const PIXEL_list &new_curve)
{
   bool debug = Config::get_var_bool("RESHAPE_ON_SKIN_DEBUG", true);
   err_adv(debug, "Bcurve::reshape_on_skin()");

   if (new_curve.num() < 2) {
      cerr << "Bcurve::reshape_on_skin(): insufficient number of pix in new curve"
           << endl;
      return false;
   }

   if (_skin == NULL) {
      // this function should only be called if curve is constrained to a surface
      cerr << "Bcurve::reshape_on_skin(): ERROR, no constraining skin for curve" 
           << endl;
      return false;
   }

   Bface_list skel_faces = ((Skin*)_skin)->skel_faces();
   Bsimplex_list faces;
   ARRAY<Wvec> bcs;
   for (int i = 0; i < new_curve.num(); i++) {
      int inter_face = -1;
      double inter_depth = 1e+10;
      Wvec bc; double depth; Wpt hit;
      for (int j = 0; j < skel_faces.num(); j++) {
         if (skel_faces[j]->ray_intersect(Wline(new_curve[i]), hit, depth) && (depth < inter_depth)) {
            skel_faces[j]->project_barycentric(hit, bc);
            inter_depth = depth;
            inter_face = j;
         }
      }
      if (inter_face == -1) {
         hit = skel_faces[0]->plane_intersect(new_curve[i]);
         skel_faces[0]->project_barycentric(hit, bc);
         faces += skel_faces[0];
         bcs += bc;
      } else {
         faces += skel_faces[inter_face];
         bcs += bc;
      }
   }

   if (is_closed()) {
      faces += faces.first();
      bcs += bcs.first();
   }
   ((SkinCurveMap*)_map)->set_pts(faces, bcs);
  
   err_adv(debug, "end Bcurve::reshape_on_skin()");

   return true;
}

bool 
Bcurve::reshape_on_surface(const PIXEL_list &new_curve)
{
   bool debug = Config::get_var_bool("RESHAPE_ON_SURFACE_DEBUG",false);
   err_adv(debug, "Bcurve::reshape_on_surface()");

  
   if (new_curve.num() < 2) {
      cerr << "Bcurve::reshape_on_surface(): insufficient number of pix in new curve"
           << endl;
      return false;
   }

   Map2D3D* surf_map = constraining_surface();

   if (surf_map == NULL) {
      // this function should only be called if curve is constrained to a surface
      cerr << "Bcurve::reshape_on_surface(): ERROR, no constraining surface for curve" 
           << endl;
      return false;
   }  

   // get the first guess as to a nearby uv
   // (use the uv of the first vertex)

   // try to get the uv surface either from the vert or one of it's adjoining faces
   UVsurface* uv_surf = UVsurface::find_controller(verts()[1]);

   if (!uv_surf) {
      ARRAY<Bface*> faces;
      verts()[1]->get_faces(faces);
      if ( !faces.empty() ){
         // just get the surface from the first face
         uv_surf = UVsurface::find_controller(faces[0]); 
      }
   }

   UVpt first_guess = random_uv();

   // if we have a uv surface, try to find the uv location of the first vert to use as a guess
   if (uv_surf) {
      err_adv(debug, "have a UV surface for curve");
      if (!uv_surf->get_uv(verts()[1], first_guess) ) {
         err_adv(debug, "failed to get first guess uv"); 
      }
   }

   UVpt_list new_uvs; 
   const int MAX_NUM_ATTEMPTS = 200;

   if ( !intersect_surface(new_curve, surf_map, first_guess, new_uvs, MAX_NUM_ATTEMPTS) ) {
      cerr << "Bcurve::reshape_on_surface(): ERROR, failed to intersect surface" 
           << endl;
      return false;
   }

   // if the curve is closed, we must provide a closed uv list
   if (is_closed()) {
      new_uvs += new_uvs.first();
   }

   assert(SurfaceCurveMap::isa(_map));
   ((SurfaceCurveMap*)_map)->set_uvs(new_uvs);
  
   err_adv(debug, "end Bcurve::reshape_on_surface()");

   return true;
}
 

//! Attempt to intersect line with each segment of the polyline.
//! If intersection succees, return all intersection points in out_pts
//! and return true.
bool
intersect_line_polyline(const PIXELline&  line,
                        CPIXEL_list& polyline,
                        PIXEL_list& out_pts // the result
   )
{

   if (polyline.num() < 2)
      return false;

   bool success = false;

   for (int i=0; i<polyline.num()-1; i++) {
      // create the current polyline segment
      PIXELline seg(polyline[i], polyline[i+1]);
      PIXEL intersect_pt;
      // attempt to intersect segment with the line
      if (seg.intersect_seg_line(line, intersect_pt)) {
         success = true;
         out_pts += intersect_pt;  // return the result
      }
   }

   return true;
}

//! Intersects the screen projections of the given world lines
//! with the pixel curve.  For each screen intersection point, the 
//! corresponding world point along the line is computed and returned
//! in out_new_pts.  If a line does not intersect the screen curve,
//! the line's base point is returned instead.
void
intersect_lines_with_curve(CARRAY<Wline>& lines,
                           CPIXEL_list& pix_curve,
                           Wpt_list& out_new_pts)
{

   out_new_pts.clear();

   if (lines.empty() || pix_curve.empty())
      return;
  
   // Convert the world lines to pixel space.
   ARRAY<PIXELline> pix_lines(lines.num());

   int i=0; // loop index

   for (i=0; i<lines.num(); i++) {
      pix_lines += PIXELline(PIXEL(lines[i].point()),
                             PIXEL(lines[i].endpt()) );
   }

   // For each pixel line, determine if the line intersects any segment
   // of the pixel curve.  If more than one segment is intersected,
   // return the closest intersection.

   // For what follows, there must be a one-to-one correspondence
   // between lines and pix_lines
   assert(pix_lines.num() == lines.num());

   PIXEL_list intersect_pixels;

   for (i=0; i<pix_lines.num(); i++) {
      intersect_pixels.clear();
      intersect_line_polyline(pix_lines[i], pix_curve, intersect_pixels);
    
      // find intersect pixel that's closest to the line base point
      double closest_dist = DBL_MAX;  // closest distance so far
      int closest_i = 0;              // index of closest pix
      for (int j=0; j<intersect_pixels.num(); j++) {
         double cur_dist = pix_lines[i].point().dist(intersect_pixels[j]);
         if ( cur_dist < closest_dist ) {
            closest_dist = cur_dist;
            closest_i = j;
         }
      }

      if (!intersect_pixels.empty()) {

         // Find closest world point along the line that projects to this pixel.
         // Create a world line from eye to screen point 
         Wpt wp = XYpt(intersect_pixels[closest_i]); // conversion from screen to world currently requires XYpt?
         Wline view_line(VIEW::eye(), wp);
         Wpt closest(lines[i], view_line);

         out_new_pts += closest;
      } else {
         // no itersections, so use the line's original base point
         out_new_pts += lines[i].point();
      }
   }

   // we guarantee that we provide one point for every line
   assert(out_new_pts.num() == lines.num());
}


bool 
Bcurve::reshape_along_normals(const PIXEL_list &new_curve)
{
   cerr << "Bcurve::reshape_along_normals()" << endl;

   if (!Wpt_listMap::isa(_map)) {
      cerr << "Bcurve::reshape_along_normals(): ERROR " 
         "must have a Wpt_listMap" 
           << endl;
      return false;
   }

   ARRAY<Wline> lines;  

   if ( !get_reshape_constraint_lines(lines, _cur_reshape_mode) ) {
      cerr << "Bcurve::reshape_along_normals(): WARNING: "
         "error getting normal lines" 
           << endl;      
   }

   cerr << "num norm lines " << lines.num() << endl;

   // set new world points in the curve map

   Wpt_list new_pts(lines.num());

   intersect_lines_with_curve(lines, new_curve, new_pts);

   if ( new_pts.empty() ) {
      cerr << "Bcurve::reshape_along_normals(): no intersection points "
           << endl;
      return false;
   }

   new_pts.update_length();

   // set the endpoints
   if (b1())
      b1()->move_to(new_pts.first());
   if (b2())
      b2()->move_to(new_pts.last());


/*   if (b1() && b1()->map()) */
/*     b1()->map()->invalidate(); */
/*   if (b2() && b2()->map()) */
/*     b2()->map()->invalidate(); */

   // find the top level parent
   Bcurve* p = parent();
   while(p && p->parent()) {
      p = p->parent();
   }

   if (p) {
      assert(p->mesh());
      cerr << "top level parent rel cur level " 
           << p->mesh()->rel_cur_level()
           << endl;

      if (p->mesh()->rel_cur_level()) {
         p->set_res_level(p->mesh()->rel_cur_level()); 
      }

   } else {
      assert(mesh());
      cerr << "this curve rel cur level " 
           << mesh()->rel_cur_level()
           << endl;
      if (mesh()->rel_cur_level()) {
         set_res_level(mesh()->rel_cur_level()); 
      }
   }
  
   ((Wpt_listMap*)_map)->set_pts(new_pts);

   return true;
}

/*****************************************************************
 * obsolete
 *****************************************************************/

//! sets this bcurve's map to specified one, and makes sure children
//! get the same map.
void 
Bcurve::set_map(Map1D3D* c) 
{

   unhook();
   _map = c;
   hookup();
   if (_child)
      ((Bcurve*)_child)->set_map(c);

   invalidate();
}

inline UVpt_list
interp_uvpts(CUVpt& uv1, CUVpt& uv2, int num_verts)
{
   if (num_verts <0) {
      cerr << "interp_uvpts, negative num_verts" << endl;
      return UVpt_list();
   }

   UVpt_list ret(num_verts);

   double delt = 1.0/(num_verts-1);

   for (int i=0; i<num_verts; i++)
      ret += interp(uv1, uv2, i*delt);
   ret.update_length();
   return ret;
}

Wpt_list 
Bcurve::get_wpts() const 
{
   if ( _map ) {
      return _map->get_wpts();
   }
   
   cerr << "Bcurve::get_wpts(): ERROR, _map is null" << endl;

   return Wpt_list();
}

ARRAY<double> 
Bcurve::get_map_tvals() const {

   ARRAY<double> ret;
   
   // for now, we need to handle Wpt basd Maps and UV based maps
   // this is sort of undefined for RayMaps
   int i;

   if (Wpt_listMap::isa(_map)) {
      Wpt_list wpts = ((Wpt_listMap*)_map)->pts();
      for (i = 0; i < wpts.num(); i++) {
         ret += wpts.partial_length(i) / wpts.length();
      }
   } else if (SurfaceCurveMap::isa(_map)) {
      UVpt_list uvs = ((SurfaceCurveMap*)_map)->uvs();
      for (i = 0; i < uvs.num(); i++) {
         ret += uvs.partial_length(i) / uvs.length();
      }
   } else {
      cerr << "Bcurve::get_map_tvals() : TODO : Handle other types of maps";
   }

   return ret;
}  


bool
Bcurve::get_pixels(PIXEL_list& list) const {

   Wpt_list wpl = get_wpts();
   
   list.clear();

   int i;
   for (i = 0; i < wpl.num(); i++) {
      list.add(PIXEL(wpl[i]));
   }

   list.update_length();

   return true;

}

//! This function attempts to resample the curves so they
//! have the same number of edges. If possible they will
//! both be resampled with 'preferred_num_edges'. Returns
//! true if at the end they have the same number of edges.
bool
ensure_same_sampling(Bcurve* a, Bcurve* b, int preferred_num_edges)
{

   if (!(a && b))
      return false;

   if (!(a->can_resample() || b->can_resample()))
      return a->num_edges() == b->num_edges();
   else if (!a->can_resample())
      b->resample(a->num_edges());
   else if (!b->can_resample())
      a->resample(b->num_edges());
   else {
      a->resample(preferred_num_edges);
      b->resample(preferred_num_edges);
   }

   return true;
}

void
Bcurve_list::grow_connected(Bpoint* b, Bcurve_list& ret)
{
   if (!b) 
      return;

   Bcurve_list adj = b->adjacent_curves();

   for ( int i=0; i<adj.num(); i++) {
      if (ret.add_uniquely(adj[i]))
         grow_connected(adj[i]->other_point(b), ret);
   }
}

Bcurve_list
Bcurve_list::reachable_curves(Bpoint* b)
{
   Bcurve_list ret;

   if (!b) {
      return ret;
   }

   grow_connected(b, ret);

   return ret;
}

// end of file bcurve.C
