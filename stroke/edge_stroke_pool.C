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
#include "edge_stroke_pool.H"
#include "edge_stroke.H"
#include "gtex/gl_extensions.H"
#include "mesh/bmesh.H"
#include "std/config.H"

using mlib::Point2i;

 /*****************************************************************
 * EdgeStrokePool
 *****************************************************************/

static int foobag1 = DECODER_ADD(EdgeStrokePool);

TAGlist*    EdgeStrokePool::_esp_tags   = NULL;

// XXX hack for serialization
BMESH *     EdgeStrokePool::_mesh = 0;
int         EdgeStrokePool::foo = 0;

/////////////////////////////////////////////////////////////////
// EdgeStrokePool Methods
/////////////////////////////////////////////////////////////////

/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
EdgeStrokePool::tags() const
{
   if (!_esp_tags) {
      _esp_tags = new TAGlist;
      *_esp_tags += BStrokePool::tags();    
      *_esp_tags += new TAG_meth<EdgeStrokePool>(
         "edge_strip",
         &EdgeStrokePool::put_edge_strip,
         &EdgeStrokePool::get_edge_strip,  1);
   }

   return *_esp_tags;
}

/////////////////////////////////////
// decode()
/////////////////////////////////////

STDdstream  &
EdgeStrokePool::decode(STDdstream &ds)
{

   static bool fix_offsets = Config::get_var_bool("FIX_CREASE_OFFSETS",false,true);

   if (fix_offsets) verify_offsets();

   STDdstream  &ret =  BStrokePool::decode(ds);

   return ret;
}

/////////////////////////////////////
// get_edge_strip()
/////////////////////////////////////

void
EdgeStrokePool::get_edge_strip(TAGformat &d)
{
   assert(_mesh);

   ARRAY<Point2i> edge_strip;
   *d >> edge_strip;

   for (int i=0; i<edge_strip.num(); i++)
      add_to_strip(_mesh->bv(edge_strip[i][0]), _mesh->be(edge_strip[i][1]));

}

/////////////////////////////////////
// put_edge_strip()
/////////////////////////////////////

void
EdgeStrokePool::put_edge_strip(TAGformat &d) const
{
   ARRAY<Point2i> edge_strip;

   CARRAY<Bvert*> &verts = _strip.verts();
   CARRAY<Bedge*> &edges = _strip.edges();

   assert(verts.num() == edges.num());

   for (int i=0; i<verts.num(); i++) 
      edge_strip += Point2i(verts[i]->index(),edges[i]->index());

   d.id();
   *d << edge_strip;
   d.end_id();

}

/*

ostream &
EdgeStrokePool::write_stream(ostream &os) const 
{   
   CARRAY<Bvert*> &verts = _strip.verts();
   CARRAY<Bedge*> &edges = _strip.edges();
   ARRAY<int> vert_inds;
   ARRAY<int> edge_inds;
   int i;
   assert(verts.num() == edges.num());
   for (i =0;i < verts.num(); i++) {
      vert_inds += verts[i]->index();
      edge_inds += edges[i]->index();
   }


   os << vert_inds << edge_inds;
         
   return os;
}
*/






void
EdgeStrokePool::draw_flat(CVIEWptr& v) {

   //This really fails with noise... so...
   assert(get_num_protos()==1);

   if (_hide) return;
   OutlineStroke *prot = get_active_prototype();
   if (!prot) return;

   // A OutlineStroke::draw_start() call has a different effect depending
   // on whether or not the stroke has texture.  So textured and
   // untextured strokes must be rendered in different passes.

   // Check whether we have textured or untextured strokes, or both
   // kinds of strokes.

   bool have_textured_strokes = false;
   bool have_untextured_strokes = false;
   int i;
   for (i = 0; i < _num_strokes_used; i++) 
   {
      if (_array[i]->get_texture()) have_textured_strokes = true;
      else                          have_untextured_strokes = true;
   }

   if (have_untextured_strokes) 
   {
      // If proto has a texture, temporarily unset the texture so it
      // calls draw_start() appropriately for untextured strokes.

      TEXTUREptr proto_tex =     prot->get_texture();
      Cstr_ptr proto_tex_file =  prot->get_texture_file();

      if (prot->get_texture()) prot->set_texture(NULL, NULL_STR);

      prot->draw_start();

      glDepthFunc(GL_ALWAYS);
      for (i = 0; i < _num_strokes_used; i++) 
         if (!_array[i]->get_texture()) _array[i]->draw(v);
      glDepthFunc(GL_LESS);

      prot->draw_end();

      // If we saved the proto's texture, reset it now.
      if (proto_tex) prot->set_texture(proto_tex, proto_tex_file);

   }
   
   if (have_textured_strokes) 
   {
      // Have each textured stroke call its own draw_begin().
      // XXX -- this is a little inefficient.
      for (int j = 0; j < _num_strokes_used; j++) 
      {
         if (_array[j]->get_texture()) 
         {
            _array[j]->draw_start();
            glDepthFunc(GL_ALWAYS);
            _array[j]->draw(v);
            glDepthFunc(GL_LESS);
            _array[j]->draw_end();
         }
      }
   }
}

EdgeStrokePool::EdgeStrokePool(EdgeStroke* proto) :
   BStrokePool(proto)
{
   assert(_edit_proto == 0);
   assert(_draw_proto == 0);
   if (proto != NULL)
      set_prototype(proto);
}

EdgeStrokePool::~EdgeStrokePool()
{
   int i = 0; // loop index
   for (i = 0; i < _strip.edges().num(); i++) {
      Bedge* edge = _strip.edges()[i];
      assert(edge);
      //SimplexData* d = edge->find_data(this->static_name());
      SimplexData* d = edge->find_data((unsigned int)&(this->foo));

      edge->rem_simplex_data(d);
   }

   i = 0;
   while (i < _num) {
      assert( _array[i]->is_of_type(EdgeStroke::static_name()));
      ((EdgeStroke*)_array[i++])->clear_simplex_data();
   }
}

void EdgeStrokePool::set_stroke_strips()
{
   assert(_edit_proto == 0);
   assert(_draw_proto == 0);

   assert(get_prototype());
   assert(get_prototype()->is_of_type(EdgeStroke::static_name()));  
   ((EdgeStroke*)get_prototype())->set_edge_strip(&_strip);
 
   for (int i = 0; i < _num; i++) {
      assert(_array[i]->is_of_type(EdgeStroke::static_name()));   
      ((EdgeStroke*)_array[i])->set_edge_strip(&_strip);
   }
}


void EdgeStrokePool::set_edge_strip(CEdgeStrip& strip)
{
   _strip = strip;
   set_stroke_strips();

   for(int i=0; i<_strip.edges().num(); i++){
      Bedge* e = _strip.edges()[i];
      new EdgeStrokePool::EdgeData(this, e);
   }
}


// Add a vertex to which current stroke is assigned.
// Enforces rule that the added vertex must be adjacent to the previous
// one in the list.  Adds edge data to edge newly "covered" by this stroke.

void
EdgeStrokePool::add_to_strip(Bvert* vert, Bedge* edge)
{
   assert(vert);
   assert(edge);

   set_stroke_strips();

   // check that the vert being added is contiguous to last vert in edge
   // strip

   if (_strip.verts().num() > 0) {
      // Edge (if any) connecting this vert to previous vert;
      Bedge* last_edge = vert->lookup_edge(_strip.verts().last());

      // Should have edge connecting v to last vert on the list
      if(!last_edge) {
         cerr << "EdgeStrokePool::add_to_strip(), " 
              << "WARNING: vertex not adjacent to previous vertex:" << endl;
         cerr << "Old vertex: " << _strip.verts().last()->index() << endl;
         cerr << "New vertex: " << vert->index() << endl;
         cerr << "returning" << endl;
         return;
      }

      // reject adjacent v's with duplicate locations
      if (_strip.verts().last()->loc() == vert->loc()) {
         cerr << "EdgeStrokePool::add_to_strip(), " 
              << "WARNING: adjacent verts identical:" << endl;
         cerr << "returning" << endl;
         return;
      }
   }

   
   new EdgeStrokePool::EdgeData(this, edge);
   _strip.add(vert, edge);
   
}


void
EdgeStrokePool::verify_offsets()
{
   cerr << "verifying offsets" << endl;

   int i=0;
   int j=0;

   for (i=0; i < _num_strokes_used; i++) {
      // see if any stroke has all 0 pressures in offsets
      BaseStrokeOffsetLISTptr o = 0;

      if(_array[i]->get_offsets())
         o = _array[i]->get_offsets();
      else
         continue;

      bool good = false;

      for (j=0; j < o->num(); j++) {
         if((*o)[j]._press > 0.0) {
            good = true;
            break;
         }
      }
       
      if(!good) {
         cerr << "resetting pressures" << endl;
         // set all pressures to 1.0
         for (j=0; j < o->num(); j++) {
            (*o)[j]._press = 1.0;
         }
      }
   }
}






istream &
EdgeStrokePool::read_stream(istream &is)        
{
   BStrokePool::read_stream(is);

   bool fix_offsets 
      = Config::get_var_bool("FIX_CREASE_OFFSETS",false,true);

   if (fix_offsets) 
      verify_offsets();

   ARRAY<int> vert_inds;
   ARRAY<int> edge_inds;
   is >> vert_inds >> edge_inds;
   if (_mesh) {
      for (int i =0; i < vert_inds.num(); i++) {
         add_to_strip(_mesh->bv(vert_inds[i]), _mesh->be(edge_inds[i]));
      }
   } else {
      cerr << "EdgeStrokePool::read_stream - no mesh?" << endl;
   }

   return is;
}


ostream &
EdgeStrokePool::write_stream(ostream &os) const 
{   
   BStrokePool::write_stream(os);

   CARRAY<Bvert*> &verts = _strip.verts();
   CARRAY<Bedge*> &edges = _strip.edges();
   ARRAY<int> vert_inds;
   ARRAY<int> edge_inds;
   int i;
   assert(verts.num() == edges.num());
   for (i =0;i < verts.num(); i++) {
      vert_inds += verts[i]->index();
      edge_inds += edges[i]->index();
   }
   //XXX These endl's might cause trouble...
   os << endl;
   os << vert_inds << edge_inds;
   os << endl;
         
   return os;
}


/* end of file crease_stroke_pool.C */
