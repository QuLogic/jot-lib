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



#include "gtex/ref_image.H"
#include "stroke/edge_stroke.H"
#include "mesh/simplex_filter.H"
#include "std/config.H"

using namespace mlib;

static int es = DECODER_ADD(EdgeStroke);

// XXX hack for serialization
BMESH *EdgeStroke::_mesh = 0;

EdgeStroke::EdgeStroke() : 
   _strip(0),
   _vis_step_pix_size(15.0)
{
   _vis_type = VIS_TYPE_SCREEN | VIS_TYPE_SUBCLASS;

   _draw_verts.set_proto(new EdgeStrokeVertexData);
   _refine_verts.set_proto(new EdgeStrokeVertexData);
   _verts.set_proto(new EdgeStrokeVertexData);
}

// Subclass this to change offset LOD, etc.
void   
EdgeStroke::apply_offset(BaseStrokeVertex* v, 
                         BaseStrokeOffset* o)
{
   //mak, sublcass this...
   //Use pressures? 
   //Different LOD policy?

   // XXX - Arbitrary policy for now -- scale offsets
   // as ratio of original base length, _pix_len, 
   // to current base length, but never magnify
   //This should be precomputed once...
   
   assert(_offsets->get_pix_len());

   // XXXXXXXX - mak, you used to actually modify the pix_len via mesh ratio
   // in Outline::draw, this is 'evil' and has been removed.
   // You should manually apply the mesh ratio here if desired.
   double scale = _scale * min( (_ndc_length/(_offsets->get_pix_len()*_scale)), 1.0);
 
   if(_press_vary_width)
      v->_width = (float)o->_press;
   else
      v->_width = 1.0f;

   if(_press_vary_alpha)
      v->_alpha = (float)o->_press;
   else
      v->_alpha = 1.0f;
   
   v->_loc = v->_base_loc + v->_dir * scale * o->_len;
}



int
EdgeStroke::draw(CVIEWptr& view)
{
   if (!_strip) return 0;

   if (_strip->edges().empty() )       
      return 0;
   
   assert(_strip->edges().num() == _strip->verts().num());

   // cache per frame constants
   static uint   cache_stamp   = UINT_MAX;
   static double scale         = 0.0;
   //static double pix_step_size = get_var_dbl("CREASE_REFINE_STEP", 15.0);
   static double step_size     = 0.0;

   if (cache_stamp != VIEW::stamp()) {
      scale     = VIEW::pix_to_ndc_scale();
      //step_size = pix_step_size * scale;
      step_size = _vis_step_pix_size * scale;
      cache_stamp = VIEW::stamp();
   }

   if (_dirty) {

      OutlineStroke::clear();  // clears all the vertices

      NDCZpt cur_p;
      NDCZpt prev_p;

      NDCZvec vec;
      double  len = 0.0;

         
      for ( int i=0; i<_strip->verts().num(); i++) {

         cur_p = _strip->vert(i)->ndc();

         // To refine visibility testing, add vertices along the
         // current edge of this stroke, if needed.
         if (i>0) {
            vec = cur_p - prev_p;
            len = vec.length();
            vec = vec.normalized();

            refine(prev_p, vec, len, step_size, 
                   _strip->edge(i-1));
         } 

         add(cur_p);
         
         prev_p = cur_p;

         // Get the vertex we just added, so we can set its data.
         BaseStrokeVertex& stroke_v = _verts[_verts.num()-1];
         assert(stroke_v._data);

         ((EdgeStrokeVertexData *)stroke_v._data)->sim
            = _strip->verts()[i];
      }

      // Must add the last vertex of the crease strip,
      // adding refinement verts if needed

      cur_p = _strip->last()->ndc();

      vec = cur_p - prev_p;
      len = vec.length();
      vec = vec.normalized();
      refine(prev_p, vec, len, step_size, _strip->edges().last());
         
      add(cur_p);

      // Set the data on the stroke vertex just added.
      BaseStrokeVertex& last_stroke_v = _verts[_verts.num()-1];
      assert(last_stroke_v._data);

      ((EdgeStrokeVertexData *)last_stroke_v._data)->sim 
         = _strip->last();
   }

   BaseStroke::draw(view);
   return 0;
}

void
EdgeStroke::copy(CEdgeStroke& s) 
{
   if(s._strip) {
      set_edge_strip(s._strip);  
   }

   OutlineStroke::copy(s);
}

BaseStroke*
EdgeStroke::copy() const
{
   EdgeStroke* s = new EdgeStroke();
   assert(s);
   s->copy(*this);
   return s;
}

void 
EdgeStroke::clear_simplex_data()
{
   // XXX -- trying to remove stroke data from edge.  Is this correct?   
   if (!_strip) return;

   for(int i=0; i<_strip->verts().num(); i++){
      Bvert* v = _strip->vert(i);
      assert(v);
      Bface_list faces;
      v->get_faces(faces);

      for ( int j=0; j<faces.num(); j++) {
         assert(faces[j]);
         SimplexData* d = faces[j]
            ->find_data(EdgeStroke::static_name());
         if(d) {
            faces[j]->rem_simplex_data(d);
         }
      }
   }
}


void
EdgeStroke::interpolate_refinement_vert(
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

   EdgeStrokeVertexData* d 
      = (EdgeStrokeVertexData*) v->_data;
   d->sim = 0;  

   EdgeStrokeVertexData* v1_d 
      = (EdgeStrokeVertexData*)(vl[1]->_data);

   EdgeStrokeVertexData* v2_d 
      = (EdgeStrokeVertexData*)(vl[2]->_data);

   if( !v1_d || !v1_d->sim || !v2_d || !v2_d->sim ) {
      return;
   }

   if(is_edge(v2_d->sim) && is_edge(v1_d->sim) ) { 
      
      if(v2_d->sim != v1_d->sim) {
         cerr << "EdgeStroke::interpolate_refinement_vert(), warning: "
              << "interpolating between 2 different edges!!, returning" 
              << endl;
         return;
      }

      // set the interpolated vert's simplex to this edge
      d->sim = v1_d->sim;
   }

   else if(is_vert(v2_d->sim) && is_vert(v1_d->sim) ){

      if (v2_d->sim == v1_d->sim) {
         cerr << "EdgeStroke::interpolate_refinement_vert(), " 
              << "warning:  interpolating between "
              << " pointers to same vert, returning" << endl;
         return;
      }

      // get the shared edge
      Bedge * e = ((Bvert*)v2_d->sim)
         ->lookup_edge((Bvert*)v1_d->sim);

      if (!e) {
         cerr << "EdgeStroke::interpolate_refinement_vert(), " 
              << "found no shared edge, returning" << endl;
         return;
      }

      d->sim = e;
   }

   else if (is_edge(v2_d->sim)) {
      d->sim = v2_d->sim;
   }
   else if (is_edge(v1_d->sim)) {
      d->sim = v1_d->sim;
   } else {
      cerr << "EdgeStroke::interpolate_refinement_vert(), " 
           << "invalid sim type, returning" << endl;
      return;
   }

   assert(d->sim);
}



void 
EdgeStroke::set_edge_strip(EdgeStrip* strip)
{
   if (!strip) {
      cerr << "WARNING: EdgeStroke::set_edge_strip(), null strip" 
           << endl;
   }

   if(_strip == strip){
      return;
   }

   if(_strip) {
      clear_simplex_data();
   }

   _strip = strip;

   for(int i=0; i<_strip->verts().num(); i++){
      Bvert* v = _strip->vert(i);
      assert(v);
      Bface_list faces;
      v->get_faces(faces);

      for ( int j=0; j<faces.num(); j++) {
         SimplexData* d = 
            faces[j]
            ->find_data(EdgeStroke::static_name());
         if (!d) {
            new EdgeStroke::FaceData(this, faces[j]);
         }
      }
   }

   _dirty = true;
}

// This filter accepts a face if it has been tagged
// with data from any EdgeStroke.

class EdgeStrokeSimplexFilter : public SimplexFilter {
 public:
        
   virtual bool accept(CBsimplex* s) const 
   { 
      if (!is_face(s)) return false;

      SimplexData* d = ((Bface*)s)
            ->find_data(EdgeStroke::static_name());
      
      if (d) return true;

      return false;
   }
};




bool 
EdgeStroke::check_vert_visibility(CBaseStrokeVertex &v)
{
   // The data for the stroke vertex v should provide a pointer to a
   // mesh vertex.  To determine (crudely) if vertex is visible, check if any
   // crease or silhouette edge sharing the vertex appears in the ID
   // ref image.

   if (!v._data) {
      return false;
   }

   if(!v._good) {
      return false;
   }

   const Bsimplex * sim = ((EdgeStrokeVertexData *)v._data)->sim;
      
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

   // XX -- next two lines are for faster ref image test, not
   // yet implemented 
//     static EdgeStrokeSimplexFilter filt;
//     return id_ref->find_near_simplex(v._base_loc, 1, filt);


   // If quick visibility check fails, we will record a frontfacing
   // face adjacent to the simplex to do the "walking" visibility
   // test.
   Bface* front_facing_face = 0;  

   // First, try the quick visibility test, through backface
   // culling and ref image test.

   if (is_vert(sim)) {

      CBvert* mesh_v = (Bvert*)sim;

      // get the edges adjacent to this vertex
      CARRAY<Bedge*>&  adj_e = mesh_v->get_adj();

      for (int i=0; i<adj_e.num(); i++) {
         front_facing_face = adj_e[i]->frontfacing_face();
         if (front_facing_face) {      
            break;
         }
      }

      // None of the adjacent faces is frontfacing,
      // so we cull this vertex.
      if (!front_facing_face) 
         return false;


      for (int j=0; j<adj_e.num(); j++) {

         Bedge* e = adj_e[j];

         if ( e->is_crease() ) {

            // If we find edge nearby in the id ref image, then
            // consider the vertex visible.

            if ( id_ref->is_simplex_near(v._base_loc, e) ) {
               return true;
            }

            // Or, if we find one of the edge's neighboring faces in
            // the id ref image, consider the vertex visible.

            if ( (e->f1() && e->f1()->front_facing() &&
                  id_ref->is_simplex_near(v._base_loc, e->f1())) ||
                 (e->f2() && e->f2()->front_facing() &&
                  id_ref->is_simplex_near(v._base_loc, e->f2())) )  {
               return true;
            }     
         }
      }
   }

   else if (is_edge(sim)) {

      const Bedge * e = (Bedge*)sim;
      
      front_facing_face = e->frontfacing_face();

      // Cull the edge if it has no adjoining frontfacing face.
      if (!front_facing_face)      
         return false;
      
      if ( id_ref->is_simplex_near(v._base_loc, e) ) {
         return true;
      }

      if ( (e->f1() && e->f1()->front_facing() &&
            id_ref->is_simplex_near(v._base_loc, e->f1())) ||
           (e->f2() && e->f2()->front_facing() &&
            id_ref->is_simplex_near(v._base_loc, e->f2())) )  {
         return true;
      }         

   }

   static bool exact_vis 
      = Config::get_var_bool("CREASE_EXACT_VISIBILITY",false,true);

   if ( exact_vis && front_facing_face) {

      Bface* f = id_ref->face(v._base_loc);
      if (!f) {
         Bedge* e = id_ref->edge(v._base_loc);
         if (e)
            f = e->frontfacing_face();
      }

      if (!f) return false;

      static Wpt obj_pt;
      Bsimplex* sim = 
         f->find_intersect_sim(v._base_loc, obj_pt);

      if (sim && sim->on_face(front_facing_face)) {
         return true;
      }
   }
   
   return false;
}




/* end of file edge_stroke.C */
