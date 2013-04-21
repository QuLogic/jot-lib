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



#include "decal_line_stroke.H"
#include "mesh/patch.H"
#include "gtex/ref_image.H"

using namespace mlib;

static int dcl = DECODER_ADD(DecalLineStroke);

TAGlist*            DecalLineStroke::_dls_tags = 0;

// XXX hack for serialization
BMESH *DecalLineStroke::_mesh = 0;

DecalLineStroke::DecalLineStroke(Patch* p) :
   OutlineStroke(p)
{
   _vis_type = VIS_TYPE_SCREEN | VIS_TYPE_SUBCLASS;

   _draw_verts.set_proto(new DecalVertexData);
   _refine_verts.set_proto(new DecalVertexData);
   _verts.set_proto(new DecalVertexData);

}

CTAGlist &
DecalLineStroke::tags() const
{
   if (!_dls_tags) {
      _dls_tags = new TAGlist;
      *_dls_tags += OutlineStroke::tags();    

      *_dls_tags += new TAG_meth<DecalLineStroke>(
         "locs",
         &DecalLineStroke::put_vert_locs,
         &DecalLineStroke::get_vert_locs,
         1);
      *_dls_tags += new TAG_meth<DecalLineStroke>(
         "vertex_loc",
         &DecalLineStroke::put_vertex_locs,
         &DecalLineStroke::get_vertex_loc,
         1);

   }
   return *_dls_tags;
}

int
DecalLineStroke::draw(CVIEWptr& v)
{
   if (_vert_locs.num() < 2)       
      return 0;

   if (_dirty) {
      OutlineStroke::clear();  // clears all the vertices

      // recompute the vertex ndc locations for the current frame
      for ( int i=0; i<_vert_locs.num(); i++) {
         // A null simplex indicates that this is a "bad" vert,
         // i.e., a break in the stroke.

         if (!_vert_locs[i].sim) {
            // XXX -- hack: add a "bad" dummy vert to cause a break
            // in the stroke when it's rendered.
            NDCZpt dummy;
            add(dummy, false);
         }
         else {
            assert(_patch);
            NDCZpt p(_vert_locs[i].loc, 
                     _patch->mesh()->obj_to_ndc());

            // add vert with location p
            add(p);  

            BaseStrokeVertex& last_v = _verts[_verts.num()-1];

            if (_press_vary_width) 
               last_v._width = (float)_vert_locs[i].press;

            if (_press_vary_alpha) 
               last_v._alpha = (float)_vert_locs[i].press;

            ((DecalVertexData *)last_v._data)->sim
               = _vert_locs[i].sim;
         }
      }
   }

   return BaseStroke::draw(v);
}

BaseStroke*
DecalLineStroke::copy() const
{
   DecalLineStroke* s = new DecalLineStroke(_patch);
   assert(s);
   s->copy(*this);
   return s;
}

void
DecalLineStroke::clear()
{
   _vert_locs.clear();
   OutlineStroke::clear();
}



void
DecalLineStroke::interpolate_refinement_vert(
        BaseStrokeVertex *v, 
        BaseStrokeVertex **vl,
        double u)
{

   assert(v);

   assert(vl[1] && vl[2]);

   v->_good = true;

   // Linearly interpolate normals and locations.

   if (_vis_type & VIS_TYPE_NORMAL) 
      v->_norm = ((1-u) * vl[1]->_norm 
                  + u * vl[2]->_norm).normalized();

   v->_base_loc = vl[1]->_base_loc 
      + (u * (vl[2]->_base_loc - vl[1]->_base_loc));
   
   if (!v->_data) {
      return;
   }

   DecalVertexData* d 
      = (DecalVertexData*) v->_data;
   d->sim = 0;

   DecalVertexData* v1_d 
      = (DecalVertexData*)(vl[1]->_data);

   DecalVertexData* v2_d 
      = (DecalVertexData*)(vl[2]->_data);

   if( !v1_d || !v1_d->sim || !v2_d || !v2_d->sim ) {
      return;
   }

   if(is_face(v2_d->sim) && is_face(v1_d->sim) ) { 
      
      if(v2_d->sim != v1_d->sim) {
         cerr << "warning:  interpolating between "
              << " 2 different faces!!, returning" << endl;
         return;
      }
      
      // set the interpolated vert's simplex to this face
      d->sim = v1_d->sim;
   }

   else if(is_edge(v2_d->sim) && is_edge(v1_d->sim) ){

      if (v2_d->sim == v1_d->sim) {
         cerr << "warning:  interpolating between "
              << " pointers to same edge, returning" << endl;
         return;
      }

      // get the shared face
      Bface * f = ((Bedge*)v2_d->sim)
         ->lookup_face((Bedge*)v1_d->sim);

      if (!f) {
         cerr << "found no shared face, returning" << endl;
         return;
      }

      d->sim = f;
   }
   else if (is_face(v2_d->sim)) {
      d->sim = v2_d->sim;
   }
   else if (is_face(v1_d->sim)) {
      d->sim = v1_d->sim;
   } else {
      cerr << "invalid sim type, returning" << endl;
      return;
   }

   assert(d->sim);
}

bool    
DecalLineStroke::check_vert_visibility(CBaseStrokeVertex &v)
{
   if (!v._data) {
      cerr << "WARNING: " 
           << "DecalLineStroke::check_vert_visibility" 
           << " no vert data" << endl;      
      return false;
   }

   const Bsimplex * sim = ((DecalVertexData *)v._data)->sim;
      
   if(!v._good) {
      cerr << "bad vertex, returning" << endl;
      return false;
   }

   if (!sim) {
      return false;
   }

   static IDRefImage* id_ref       = 0;
   static uint        id_ref_stamp = UINT_MAX;

   // cache id ref image for the current frame
   if (id_ref_stamp != VIEW::stamp()) {
      IDRefImage::set_instance(VIEW::peek());
      id_ref = IDRefImage::instance();
      id_ref_stamp = VIEW::stamp();
   }
   assert(id_ref);

   if (is_edge(sim)) {
      const Bedge * e = (Bedge*)sim;

      if (!e->frontfacing_face())      
         return false;
      
      if ( (id_ref->is_simplex_near(v._base_loc, e)) ||
           (e->f1() && e->f1()->front_facing() &&
            id_ref->is_simplex_near(v._base_loc, e->f1())) ||
           (e->f2() && e->f2()->front_facing() &&
            id_ref->is_simplex_near(v._base_loc, e->f2())) )  {
         return true;
      }

   }
   
   else if (is_face(sim)) {
      const Bface * f = (Bface*)sim;
      if (!f->front_facing())
         return false;

      if (id_ref->is_simplex_near(v._base_loc, f)) {
         return true;
      }
   }

   // The quick id ref image visibility tests failed, so
   // do the "walking" test.

   if (is_face(sim)) {
      const Bface * f = (Bface*)sim;

      if (id_ref->is_face_visible(v._base_loc, f)) {
         return true;
      }
   } 

   else if (is_edge(sim)) {
      const Bedge * e = (Bedge*)sim;
      const Bface * f1 = e->f1();
      const Bface * f2 = e->f2();

      if ( f1->front_facing() && 
           id_ref->is_face_visible(v._base_loc, f1) ) {
         return true;
      }

      if ( f2->front_facing() && 
           id_ref->is_face_visible(v._base_loc, f2) ) {
         return true;
      }
   }

   return false;
}

// XXX - Deprecated
void
DecalLineStroke::get_vert_locs(TAGformat &d)
{
   assert(_mesh);

   int num_verts;

   *d >> num_verts;

   for (int i=0; i<num_verts; i++) {
      Wpt loc;
      double pr = 0.0;
      int e_index = 0;      
      int f_index = 0;      

      Bsimplex* s = 0;

      *d >> loc;

      *d >> pr;

      *d >> e_index >> f_index;

      if (e_index != -1) {
         s = _mesh->be(e_index);
      }
      if (f_index != -1) {
         assert(!s);
         s = _mesh->bf(f_index);
      }

      _vert_locs += vert_loc(loc, s, pr);

   }
}

// XXX - Deprecated
void
DecalLineStroke::put_vert_locs(TAGformat &d) const
{
/*
   // write num vert locs

   d.id();

   *d << _vert_locs.num() << "\n";

   // write out the vert locs

   for (int i=0; i<_vert_locs.num(); i++) {

      Wpt write_loc = _vert_locs[i].loc;

      if(_patch) 
         write_loc = _patch->mesh()->xform() * write_loc;

      *d << write_loc <<  "\n";

      *d << _vert_locs[i].press <<  "\n";

      if (is_edge(_vert_locs[i].sim)) 
         *d << ((Bedge*)_vert_locs[i].sim)->index();
      else
         *d << -1;

      *d << " ";

      if (is_face(_vert_locs[i].sim)) 
         *d << ((Bface*)_vert_locs[i].sim)->index();
      else
         *d << -1;

      *d << "\n";
   }

   d.end_id();
*/
}



void
DecalLineStroke::get_vertex_loc(TAGformat &d)
{
   assert(_mesh);

   Wpt loc;
   double pr   = 0.0;
   int e_index = 0;      
   int f_index = 0;      

   Bsimplex *s = 0;

   *d >> loc;
   *d >> pr;
   *d >> e_index;
   *d >> f_index;

   if (e_index != -1) {
      s = _mesh->be(e_index);
   }
   if (f_index != -1) {
      assert(!s);
      s = _mesh->bf(f_index);
   }

   _vert_locs += vert_loc(loc, s, pr);

}


void
DecalLineStroke::put_vertex_locs(TAGformat &d) const
{
   // write out the vert locs

   for (int i=0; i<_vert_locs.num(); i++) {

      Wpt write_loc = _vert_locs[i].loc;

      // XXX - Keep things in mesh space unless the
      // mesh's transform is 'applied' during serialization
      // In general, we no longer do that... and decals should
      // 'notice' when this happens and deal with it automatically,
      // rather than here!

      //if(_patch) write_loc = _patch->mesh()->xform() * write_loc;

      d.id();

      *d << write_loc;
      *d << _vert_locs[i].press;
      *d << ((is_edge(_vert_locs[i].sim))?(((Bedge*)_vert_locs[i].sim)->index()):(-1));
      *d << ((is_face(_vert_locs[i].sim))?(((Bface*)_vert_locs[i].sim)->index()):(-1));
      
      d.end_id();
   }

}


// Validate vertices fanatically!

int
DecalLineStroke::add_vert_loc(CWpt loc, 
                              const Bsimplex* s, 
                              double press) 
{
   if (_vert_locs.empty()) {
      if(!s) {
         cerr << "DecalLineStroke::add_vert_loc: " 
              << " first vert is bad, returning" 
              << endl;
         return 0;
      }
      _vert_locs += vert_loc(loc, s, press);
      return 1;
   }

   if (loc == _vert_locs.last().loc) {
      cerr << "DecalLineStroke::add_vert_loc: " 
           << " attempting to add vert with duplicate loc, "
           << " returning" << endl;
      return 0;
   }

   if (s == 0 &&
       _vert_locs.last().sim == 0 ) {
      cerr << "DecalLineStroke::add_vert_loc: " 
           << " attempting to add 2 bad verts in a row, "
           << " returning" << endl;
      return 0;
   }

   if (_vert_locs.num() <= 2 &&
       _vert_locs.last().sim == 0 &&
       _vert_locs[_vert_locs.num() - 2].loc == loc ) {
      cerr << "DecalLineStroke::add_vert_loc: " 
           << " attempting to add duplicate vert after bad vert "
           << " returning" << endl;
      return 0;
   }

   if(is_edge(s) &&
      is_edge(_vert_locs.last().sim)) {
      //cerr << "sims both edges" << endl;
      if (s == _vert_locs.last().sim) {
         cerr << "DecalLineStroke::add_vert_loc: " 
              << "adding consecutive verts on same edge, returning" 
              << endl;
         return 0;
      }

      Bface* f = ((Bedge*)s)->lookup_face((Bedge*)_vert_locs.last().sim);

      if (f) {
         //cerr << "looked up face successfully" << endl;
      }
      else {
         cerr << "DecalLineStroke::add_vert_loc: " 
              << "consecutive verts on edges don't share face, returning" 
              << endl;
         return 0;
      }
   }

   if(is_face(s) &&
      is_face(_vert_locs.last().sim)) {
      //cerr << "sims both faces" << endl;
      if (s != _vert_locs.last().sim) {
         cerr << "DecalLineStroke::add_vert_loc: " 
              << "adding consecutive verts on different faces, returning" 
              << endl;
         return 0;
      }
   }
   
   _vert_locs += vert_loc(loc, s, press);

   return 1;
}



BaseStrokeVertex*
DecalLineStroke::refine_vert(int i, bool left)
{
   if ( !_verts[i]._good || !_verts[i+1]._good )
      return 0;

   return BaseStroke::refine_vert(i, left);
}



void
DecalLineStroke::interpolate_vert(
   BaseStrokeVertex *v, 
   BaseStrokeVertex **vl, 
   double u)
{
   BaseStroke::interpolate_vert(v, vl, u);

   if(_press_vary_width)
      v->_width = (float)((1-u) * vl[1]->_width 
                   + u * vl[2]->_width);

   if(_press_vary_alpha)
      v->_alpha = (float)((1-u) * vl[1]->_alpha 
                   + u * vl[2]->_alpha);

}


void                
DecalLineStroke::xform_locations(CWtransf& t)
{
   for ( int i=0; i < _vert_locs.num(); i++ ) {
      _vert_locs[i].loc = t * _vert_locs[i].loc;
   }
}


/* end of file decal_line_stroke.C */

