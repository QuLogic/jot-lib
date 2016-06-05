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
 * bmesh.C
 **********************************************************************/
#include <fstream>
#include "std/run_avg.H"
#include "std/stop_watch.H"
#include "std/config.H"

#include "disp/ray.H"
#include "net/io_manager.H"

#include "mesh/patch.H"
#include "mesh/uv_data.H"
#include "mesh/base_ref_image.H"
#include "mesh/ioblock.H"
#include "mesh/gtexture.H"
#include "mesh/bfilters.H"
#include "mesh/lmesh.H"      // because of DECODER_ADD(LMESH) hack, below
#include "mesh/patch_blend_weight.H"

using namespace mlib;

#include <string>
using namespace std;

//******** STATICS ********
string BODY::_base_name = BMESH::static_name();
TAGlist BODY::_body_tags;

// This makes sure that BMESH is used to create BODY's -
// sub-classes can override this, but it should not be done in a
// static constructor, since static constructor order is fickle
static int dummy = BODY::set_factory(make_shared<BMESH>());

bool     BMESH::_random_sils = !Config::get_var_bool("NO_RANDOM_SILS",false);
bool     BMESH::_freeze_sils  = false;
TAGlist* BMESH::_bmesh_tags   = nullptr;
TAGlist* BMESH::_bmesh_update_tags   = nullptr;
BMESHptr BMESH::_focus;
bool     BMESH::_show_secondary_faces = false;

bool use_new_bface_io = Config::get_var_bool("JOT_USE_NEW_BFACE_IO",true);

// XXX - Moved these definitions above
// the LMESH decoder to insure the
// BMESHobs statics are available
// when the LMESH is constructed.
// On WIN32 this wasn't happening...

BMESHobs_list BMESHobs::_all_observers;
map<BMESH*,BMESHobs_list*> BMESHobs::_hash;

// add BMESH to decoder hash table:
static class DECODERS {
 public:
   DECODERS() {
      DECODER_ADD(BMESH);
      DECODER_ADD(LMESH);
   }
}
   DECODERS_static;

/*****************************************************************
 * NameLookup<BMESH> static data:
 *****************************************************************/

// XXX - this declaration doesn't work:
//map<string,BMESHptr> NameLookup<BMESH>::_map; // used in name lookups

// these do (why??):
template<class T> map<string,T*> NameLookup<T>::_map; // used in name lookups
template<class T> bool NameLookup<T>::_debug =
   Config::get_var_bool("DEBUG_NAME_LOOKUP",false);

/***************************************************************** 
 * BMESH
 *****************************************************************/
BMESH::BMESH(int num_v, int num_e, int num_f) :
   NameLookup<BMESH>(),
   _verts(num_v),
   _edges(num_e),
   _faces(num_f),
   _version(1),
   _creases(nullptr),
   _borders(nullptr),
   _polylines(nullptr),
   _lone_verts(nullptr),
   _sil_stamp(0),
   _zx_stamp(0),
   _draw_enabled(1),
   _pix_size(0),
   _pix_size_stamp(0),
   _type(EMPTY_MESH),
   _type_valid(1),
   _geom(nullptr),
   _pm_stamp(0),
   _eye_local_stamp(0),
   _curv_data(nullptr),
   _avg_edge_len(0),
   _avg_edge_len_valid(0),
   _edit_level(0),
   _patch_blend_weights_valid(false),
   _patch_blend_smooth_passes(0),
   _blend_remap_value(2.0),
   _do_patch_blend(true),
   _is_occluder(false),
   _is_reciever(false),   
   _shadow_scale(1),
   _shadow_softness(0.5),
   _shadow_offset(0.0)
{
   _drawables.set_unique();
}

BMESH::BMESH(CBMESH& m) :
   NameLookup<BMESH>(),
   _version(1),
   _polylines(nullptr),
   _lone_verts(nullptr),
   _sil_stamp(0),
   _zx_stamp(0),
   _draw_enabled(1),
   _pix_size(0),
   _pix_size_stamp(0),
   _type(EMPTY_MESH),
   _type_valid(1),
   _geom(nullptr),
   _pm_stamp(0),
   _eye_local_stamp(0),
   _curv_data(nullptr),
   _avg_edge_len(0),
   _avg_edge_len_valid(0),
   _edit_level(0),
   _patch_blend_weights_valid(false),
   _patch_blend_smooth_passes(0),
   _is_occluder(false),
   _is_reciever(false),   
   _shadow_scale(1),
   _shadow_softness(0.5),
   _shadow_offset(0.0)
{
   _drawables.set_unique();
        
   *this = m;
}

BMESH::~BMESH()
{
   static bool debug = Config::get_var_bool("DEBUG_BMESH_DESTRUCTOR",false);
   if (debug)
      err_msg("BMESH::~BMESH called...");

   // Notify observers that it's gonna happen
   BMESHobs::broadcast_delete(this);
   
   // then delete mesh elements
   delete_elements();

   if (focus().get() == this)
      set_focus(nullptr);
}

void
BMESH::set_focus(BMESHptr m) 
{
   set_focus(m, (m && m->patches().num() == 1) ? m->patches().first() : nullptr);
}

void
BMESH::set_focus(BMESHptr m, Patch* p) 
{
   _focus = m;
   Patch::set_focus(p);
}

Bvert*
BMESH::add_vertex(Bvert* v)
{
   if (v) {
      v->set_mesh(shared_from_this());
      _verts.push_back(v);
   } else
      err_msg("BMESH::add_vertex:: error: vertex is nil");
   return v;
}

Bvert*
BMESH::add_vertex(CWpt& loc)
{
   return add_vertex(new_vert(loc));
}

Bvert_list 
BMESH::add_verts(CWpt_list& pts)
{
   Bvert_list ret(pts.size());
   for (Wpt_list::size_type i=0; i<pts.size(); i++)
      ret.push_back(add_vertex(pts[i]));
   return ret;
}

Bedge*
BMESH::add_edge(Bedge* e)
{
   if (e) {
      e->set_mesh(shared_from_this());
      _edges.push_back(e);
   } else
      err_msg("BMESH::add_edge:: error: edge is nil");
   return e;
}

Bedge*
BMESH::add_edge(Bvert* u, Bvert* v)
{
   if (!(u && v)) {
      err_msg("BMESH::add_edge: Error: vertices are nil");
   } else if (u == v) {
      err_msg("BMESH::add_edge: Error: repeated vertex");
   } else if (!((u->mesh().get() == this) && (v->mesh().get() == this))) {
      err_msg("BMESH::add_edge: Error: foreign vertices not allowed");
   } else {
      Bedge* ret = u->lookup_edge(v);
      return ret ? ret : add_edge(new_edge(u,v));
   }
   return nullptr;
}

Bedge*
BMESH::add_edge(int i, int j)
{
   Bedge* ret = nullptr;
   if (valid_vert_indices(i,j))
      ret = add_edge(_verts[i],_verts[j]);
   else
      err_msg("BMESH::add_edge: invalid vertex indices (%d,%d)", i, j);
   return ret;
}

Bedge*
BMESH::lookup_edge (const Point2i &p)
{
   if (p[0] < 0 || p[0] >= (int)_verts.size() || p[1] < 0 || p[1] >= (int)_verts.size())
      return nullptr;
   return (::lookup_edge (bv(p[0]), bv(p[1])));

}


Bface*
BMESH::add_face(Bface* f, Patch* p)
{
   if (!f) {
      err_msg("BMESH::add_face: error: face is nil");
      return nullptr;
   }

   f->set_mesh(shared_from_this());
   _faces.push_back(f);

   // deal with the patch.
   // first do error-check
   if (p && p->mesh().get() != this) {
      cerr << "BMESH::add_face: error: patch specified "
           << "belongs to a different mesh"  << endl;
      p = nullptr; // reject it
   }

   // use the patch if it is given
   if (p)
      p->add(f);

   return f;
}

Bface*
BMESH::add_face(Bvert* u, Bvert* v, Bvert* w, Patch* p)
{
   // Screen for wackos:
   if (!(u && v && w)) {
      err_msg("BMESH::add_face: Error: vertices are nil");
      return nullptr;
   } else if (u==v || u==w || v==w) {
      err_msg("BMESH::add_face: Error: repeated vertex");
      return nullptr;
   }
   if (!((u->mesh().get() == this) && (v->mesh().get() == this) && (w->mesh().get() == this))) {
      err_msg("BMESH::add_face: Error: foreign vertices not allowed");
      return nullptr;
   }

   // Is the face already defined?
   Bface* tmp = ::lookup_face(u,v,w);
   if (tmp) {
      // The proposed face already exists.
      // This is not considered a problem.
      // (Return the existing face).
      return tmp;
   }

   // Create the edges for the face:
   Bedge *e1=add_edge(u,v), *e2=add_edge(v,w), *e3=add_edge(w,u);
   if (!(e1 && e2 && e3)) {
      err_msg("BMESH::add_face: Error: can't create edges");
      return nullptr;
   }
   return add_face(new_face(u,v,w,e1,e2,e3), p);
}

Bface*
BMESH::add_face(int i, int j, int k, Patch* p)
{
   if (!valid_vert_indices(i,j,k)) {
      err_msg("BMESH::add_face: Error: invalid vertex indices (%d,%d,%d)",
              i, j, k);
      return nullptr;
   }
   return add_face(_verts[i],_verts[j],_verts[k], p);
}

Bface*
BMESH::add_face(Bvert* u, Bvert* v, Bvert* w, CUVpt& a, CUVpt& b, CUVpt& c, Patch* p)
{
   Bface* ret = add_face(u,v,w,p);
   if (ret)
      UVdata::set(ret, u, v, w, a, b, c);
   return ret;
}

Bface*
BMESH::add_face(int i, int j, int k, CUVpt& a, CUVpt& b, CUVpt& c, Patch* p)
{
   Bface* ret = add_face(i,j,k,p);
   if (ret)
      UVdata::set(ret, _verts[i], _verts[j], _verts[k], a, b, c);
   return ret;
}

/*!
 * Create the quad defined as follows:
 * 
 *      x --------  w
 *      |         / |
 *      |       /   |
 *      |     /     |
 *      |   /       |
 *      | /         |
 *      u --------  v
 *
 * u,v,w,x should form a cycle, i.e., (u,v), (v,w), (w,x) and (x,u) should the edges.
 * {u,v,w} and {w,x,u} are the faces created and added.
 */

Bface*
BMESH::add_quad(Bvert* u, Bvert* v, Bvert* w, Bvert* x, Patch* p)
{
   Bface* f1 = add_face(u,v,w,p);
   Bface* f2 = add_face(u,w,x,p);

   if (f1 && f2) {
      Bedge* weak_edge = u->lookup_edge(w);
      assert(weak_edge);
      weak_edge->set_bit(Bedge::WEAK_BIT);
      assert(f1->is_quad());
      return f1->quad_rep();
   }
   if (f1 || f2)
      err_msg("BMESH::add_quad: Error: created one face, not the other");
   else
      err_msg("BMESH::add_quad: Error: couldn't create either face");
   return nullptr;
}

Bface*
BMESH::add_quad(
   Bvert* u, Bvert* v, Bvert* w, Bvert* x,
   CUVpt& a, CUVpt& b, CUVpt& c, CUVpt& d, Patch* p)
{
   Bface* ret = add_quad(u,v,w,x,p);

   if (ret && !UVdata::set
       (u,v,w,x,a,b,c,d))
      err_msg("BMESH::add_quad: Error: could not set UV coordinates");

   return ret;
}

Bface*
BMESH::add_quad(int i, int j, int k, int l, Patch* p)
{
   return (
      valid_vert_indices(i,j,k,l) ?
      add_quad(_verts[i],_verts[j],_verts[k],_verts[l],p) :
      nullptr);
}

Bface*
BMESH::add_quad(int i, int j, int k, int l,
                CUVpt& a, CUVpt& b, CUVpt& c, CUVpt& d, Patch* p)
{
   return (
      valid_vert_indices(i,j,k,l) ?
      add_quad(_verts[i],_verts[j],_verts[k],_verts[l],a,b,c,d,p) :
      nullptr);
}

Bface*
BMESH::lookup_face (const Point3i &p)
{
   if (p[0] < 0 || p[0] >= (int)_verts.size()
    || p[1] < 0 || p[1] >= (int)_verts.size()
    || p[2] < 0 || p[2] >= (int)_verts.size()) {
      cerr << "BMESH::lookup_face - invalid vert index\n";
      return nullptr;
   }
   return (::lookup_face (bv(p[0]), bv(p[1]), bv(p[2])));

}


void
BMESH::Cube(CWpt& a, CWpt& b, Patch* p)
{
   // XXX - desired policy?
//   delete_elements();

   // Create vectors we'll use:
   Wvec diag = (b - a);
   Wvec dx(diag[0],0,0);
   Wvec dy(0,diag[1],0);
   Wvec dz(0,0,diag[2]);

   // Create the 8 vertices:
   add_vertex(a + dz);
   add_vertex(a);
   add_vertex(a + dx + dz);
   add_vertex(a + dx);
   add_vertex(a + dx + dy + dz);
   add_vertex(a + dx + dy);
   add_vertex(a + dy + dz);
   add_vertex(a + dy);

   // setup uv coords:
   UVpt u00(0,0), u10(1,0), u11(1,1), u01(0,1);

   // Set up the Patch:
   if (p && p->mesh().get() != this) {
      err_msg("BMESH::Cube: foreign patch, rejecting...");
      p = nullptr;
   }
   if (!p)
      p = new_patch();

   // Create the 6 quads with uv-coords:
   add_quad(0,1,3,2,u00,u10,u11,u01,p);
   add_quad(2,3,5,4,u00,u10,u11,u01,p);
   add_quad(4,5,7,6,u00,u10,u11,u01,p);
   add_quad(6,7,1,0,u00,u10,u11,u01,p);
   add_quad(6,0,2,4,u00,u10,u11,u01,p);
   add_quad(5,3,1,7,u00,u10,u11,u01,p);

   changed(TOPOLOGY_CHANGED);
}

void
BMESH::Octahedron(CWpt& bot, CWpt& m0, CWpt& m1,
                  CWpt& m2,  CWpt& m3, CWpt& top, Patch* p)
{
   // XXX - desired policy?
//   delete_elements();

   add_vertex(bot);
   add_vertex(m0);
   add_vertex(m1);
   add_vertex(m2);
   add_vertex(m3);
   add_vertex(top);

   // Set up the Patch:
   if (p && p->mesh().get() != this) {
      err_msg("BMESH::Octahedron: foreign patch, rejecting...");
      p = nullptr;
   }
   if (!p)
      p = new_patch();

   add_face(0,1,4,p);
   add_face(0,2,1,p);
   add_face(0,3,2,p);
   add_face(0,4,3,p);

   add_face(5,1,2,p);
   add_face(5,2,3,p);
   add_face(5,3,4,p);
   add_face(5,4,1,p);

   changed(TOPOLOGY_CHANGED);
}




void
BMESH::UV_BOX(Patch* &p)
{
   /*
     The following code creates a canonical cube centered at 0,0,0
     it has UV mapping that maps a single texture as a side projection at 45*
     It is used as the skybox
   */

   const double B = 1.0; 

   // Set up the Patch:
   if (p && p->mesh().get() != this) {
      err_msg("BMESH::Sphere: foreign patch, rejecting...");
      p = nullptr;
   }
   if (!p)
      p = new_patch();


   add_vertex(Wpt(-B,B,-B)); //0
   add_vertex(Wpt(B,B,-B)); //1
   add_vertex(Wpt(-B,B,B)); //2
   add_vertex(Wpt(B,B,B)); //3
   
   add_vertex(Wpt(-B,-B,-B)); //4
   add_vertex(Wpt(B,-B,-B)); //5
   add_vertex(Wpt(-B,-B,B)); //6
   add_vertex(Wpt(B,-B,B)); //7
/*
  add_vertex(Wpt(B,-B,-B)); //0
  add_vertex(Wpt(B,B,-B)); //1
  add_vertex(Wpt(B,-B,B)); //2
  add_vertex(Wpt(B,B,B)); //3
   
  add_vertex(Wpt(-B,-B,-B)); //4
  add_vertex(Wpt(-B,B,-B)); //5
  add_vertex(Wpt(-B,-B,B)); //6
  add_vertex(Wpt(-B,B,B)); //7
*/
   const double f = 0.00390625;

   UVpt t1 = UVpt(0.0+f,1.0-f);
   UVpt t2 = UVpt(1.0-f,1.0-f);
   UVpt t3 = UVpt(0.0+f,0.0+f);
   UVpt t4 = UVpt(1.0-f,0.0+f);
   UVpt t5 = UVpt(0.5,0.9);
   UVpt t6 = UVpt(0.5,0.1);


   //part 1

   add_face(1,3,2,t1,t5,t2,p);
  
   add_face(2,3,7,t2,t5,t6,p);
   add_face(2,7,6,t2,t6,t4,p);

   add_face(3,1,5,t5,t1,t3,p);
   add_face(3,5,7,t5,t3,t6,p);

   add_face(5,6,7,t3,t4,t6,p);

   //part 2
/*
//alternative mapping
add_face(1,2,0,t2,t1,t5,p);

add_face(0,2,6,t5,t1,t3,p);
add_face(0,6,4,t5,t3,t6,p);

add_face(1,0,4,t2,t5,t6,p);  
add_face(1,4,5,t2,t6,t4,p);

add_face(5,4,6,t4,t6,t3,p);
*/


   add_face(1,2,0,t1,t2,t5,p);

   add_face(0,2,6,t5,t2,t4,p);
   add_face(0,6,4,t5,t4,t6,p);

   add_face(1,0,4,t1,t5,t6,p);  
   add_face(1,4,5,t1,t6,t3,p);

   add_face(5,4,6,t3,t6,t4,p);


/*
  Another UV mapping using less polygons
  but it looks far worse

*/


/*

add_vertex(Wpt(-B,-B,B)); //0
add_vertex(Wpt(0,B,0)); //1
add_vertex(Wpt(0,-B,-B)); //2
add_vertex(Wpt(B,-B,0)); //3
  
UVpt t0 = UVpt(0.0,0.0);
UVpt t1 = UVpt(0.0,1.0);
UVpt t2 = UVpt(1.0,1.0);
UVpt t3 = UVpt(1.0,0.0);

add_face(0,1,2,t0,t1,t2,p); //A
add_face(0,2,3,t0,t2,t3,p); //B
   
add_face(0,1,3,t0,t1,t3,p); //C
add_face(1,2,3,t1,t2,t3,p); //D

*/

   changed(TOPOLOGY_CHANGED);

};

//this should generate a radius 1.0 sphere 
//with normals and texture coordinates
//used by skybox
void   
BMESH::Sphere(Patch* p)
{
   // XXX - desired policy?
//   delete_elements();

   //sphere mesh resolution 
   //values below 4 are impractical
   const int RESOLUTION = 16;

   //needs a way to delete existing data here before it starts
        

   //stage #1
   //creates all of the point rings
   //special case at the poles creates single point instead of a ring

   for (int i=0; i<(RESOLUTION+1); i++) { //north to south
      for (int j=0; j<(RESOLUTION*2); j++) { //one ring around the sphere
         //spherical coordinates of the points
         double alpha = (double(j)/double(RESOLUTION*2))*TWO_PI;//(0-360)
         double beta  = (double(i)/double(RESOLUTION))*M_PI;//(0-180)

         //3d coordinate
         double x = sin(alpha) * sin(beta);
         double y = cos(beta);
         double z = cos(alpha) * sin(beta);  

         add_vertex(Wpt(x,y,z));
                
         if((beta==0)||(beta==M_PI))
            break;  //don't put a bunch of identical points at the poles
      }
   }
   // Set up the Patch:
   if (p && p->mesh().get() != this) {
      err_msg("BMESH::Sphere: foreign patch, rejecting...");
      p = nullptr;
   }
   if (!p)
      p = new_patch();

   //stage #2
   //construct faces

   //pole caps, two triangle fans at the poles
   //each triangle fan has a special case triangle 
   //that is between the begining and endpoint of a point ring
   //texture is somewhat distorted because there is no way to 
   //smoothly connect it at the center of the fan

   
   for (int i=1; i<=(RESOLUTION*2); i++)
      add_face(0,i,((i)%(RESOLUTION*2)+1),   
               UVpt((double(i)/double(RESOLUTION*2)),1.0),     
               UVpt((double(i)/double(RESOLUTION*2)),1.0 -(1.0/double(RESOLUTION+1))),      
               UVpt((double(i+1)/double(RESOLUTION*2)),1.0 -(1.0/double(RESOLUTION+1))),    
               p);  
                
   //south cap is a little more complicated..
   int start=((RESOLUTION-2)*((RESOLUTION*2))+1);
   int stop= ((RESOLUTION-1)*((RESOLUTION*2)));

   for (int i=start; i<=stop; i++)
      add_face((RESOLUTION-1)*((RESOLUTION*2))+1,i,(i==stop ? start : i+1),   
               UVpt((double(i)/double(RESOLUTION*2)),0.0),                    
               UVpt((double(i+1)/double(RESOLUTION*2)),1.0 -(double(RESOLUTION-1)/double(RESOLUTION+1))),            
               UVpt((double(i)/double(RESOLUTION*2)),1.0 -(double(RESOLUTION-1)/double(RESOLUTION+1))),          
               p);

        
   //quads around the regular sphere rings
   for (int j=0; j<(RESOLUTION-2); j++) { //iterate over rings
      for (int i=1; i<=(RESOLUTION*2); i++) { //build quad ring
         int k = ((RESOLUTION*2)*j)+(i); //index to the points
         if (i!= (RESOLUTION*2)) {
            add_quad(k,k+(RESOLUTION*2),((k+1))+(RESOLUTION*2),((k+1)),
                     UVpt((double(i)/double(RESOLUTION*2)),1.0-(double(j+1)/double(RESOLUTION+1))),
                     UVpt((double(i)/double(RESOLUTION*2)),1.0-(double(j+2)/double(RESOLUTION+1))),
                     UVpt((double(i+1)/double(RESOLUTION*2)),1.0-(double(j+2)/double(RESOLUTION+1))),
                     UVpt((double(i+1)/double(RESOLUTION*2)),1.0-(double(j+1)/double(RESOLUTION+1))),
                     p);                                                                                
         } else { //ring seem, quad between beginning and end of the ring
            add_quad(k,k+(RESOLUTION*2),((k+1)),((k+1))-(RESOLUTION*2),
                     UVpt((double(i)/double(RESOLUTION*2)),1.0-(double(j+1)/double(RESOLUTION+1))),
                     UVpt((double(i)/double(RESOLUTION*2)),1.0-(double(j+2)/double(RESOLUTION+1))),
                     UVpt((double(i+1)/double(RESOLUTION*2)),1.0-(double(j+2)/double(RESOLUTION+1))),
                     UVpt((double(i+1)/double(RESOLUTION*2)),1.0-(double(j+1)/double(RESOLUTION+1))),
                     p);
         }
      }
   }



   changed(TOPOLOGY_CHANGED);


}// end of sphere function 

void
BMESH::Icosahedron(Patch* p)
{
   // XXX - desired policy?
//   delete_elements();

   add_vertex(Wpt(-.525731112119133606, 0,  .850650808352039932));
   add_vertex(Wpt( .525731112119133606, 0,  .850650808352039932));
   add_vertex(Wpt(-.525731112119133606, 0, -.850650808352039932));
   add_vertex(Wpt( .525731112119133606, 0, -.850650808352039932));

   add_vertex(Wpt(0,  .850650808352039932,  .525731112119133606));
   add_vertex(Wpt(0,  .850650808352039932, -.525731112119133606));
   add_vertex(Wpt(0, -.850650808352039932,  .525731112119133606));
   add_vertex(Wpt(0, -.850650808352039932, -.525731112119133606));

   add_vertex(Wpt( .850650808352039932,  .525731112119133606, 0));
   add_vertex(Wpt(-.850650808352039932,  .525731112119133606, 0));
   add_vertex(Wpt( .850650808352039932, -.525731112119133606, 0));
   add_vertex(Wpt(-.850650808352039932, -.525731112119133606, 0));

   // Set up the Patch:
   if (p && p->mesh().get() != this) {
      err_msg("BMESH::Icosahedron: foreign patch, rejecting...");
      p = nullptr;
   }
   if (!p)
      p = new_patch();

   add_face( 0,  1,  4, p);
   add_face( 0,  4,  9, p);
   add_face( 9,  4,  5, p);
   add_face( 4,  8,  5, p);
   add_face( 4,  1,  8, p);
   add_face( 8,  1, 10, p);
   add_face( 8, 10,  3, p);
   add_face( 5,  8,  3, p);
   add_face( 5,  3,  2, p);
   add_face( 2,  3,  7, p);
   add_face( 7,  3, 10, p);
   add_face( 7, 10,  6, p);
   add_face( 7,  6, 11, p);
   add_face(11,  6,  0, p);
   add_face( 0,  6,  1, p);
   add_face( 6, 10,  1, p);
   add_face( 9, 11,  0, p);
   add_face( 9,  2, 11, p);
   add_face( 9,  5,  2, p);
   add_face( 7, 11,  2, p);

   changed(TOPOLOGY_CHANGED);
}

double
BMESH::area() const
{
   // XXX - ?
   // send_update_notification();

   // sum up the area

   double ret = 0;
   for (Bface_list::size_type k=0; k<_faces.size(); k++)
      ret += _faces[k]->area();

   return ret;
}

double
BMESH::volume() const
{
   // sum up the volume
   // using the divergence theorem

   double ret = 0;
   for (Bface_list::size_type k=0; k<_faces.size(); k++)
      ret += _faces[k]->volume_el();

   return ret;
}


/*! \brief Return the approximate memory used for this mesh.
 *
 * The extra 4 bytes per vertex, edge, etc. is for the
 * pointer in the corresponding list (_verts, _edges, etc.).
 * For each Bvert we also count the expected number of Bedge*
 * pointers in the adjacency list.
 */
int
BMESH::size() const
{

   return (
      (nverts()   * (sizeof(Bvert) + 6*sizeof(Bedge*) + 4)) +
      (nedges()   * (sizeof(Bedge) + 4)) +
      (nfaces()   * (sizeof(Bface) + 4)) +
      (npatches() * (sizeof(Patch) + 4)) +
      sizeof(BMESH)
      );
}

/*! Print info about this mesh to the error text stream */
void
BMESH::print() const
{
   err_msg("%s: ", class_name().c_str());
   err_msg("\tverts: %6d", _verts.size());
   err_msg("\tedges: %6d", _edges.size());
   err_msg("\tfaces: %6d", _faces.size());
   if (Config::get_var_bool("PRINT_MESH_SIZE",false))
      err_msg("\tMbytes used:  %6.1lf", size()/1e6);
   cerr << "\ttype: ";
   if (is_points())
      cerr << "points ";
   if (is_polylines())
      cerr << "polylines ";
   if (is_open_surface())
      cerr << "open surface ";
   if (is_closed_surface())
      cerr << "closed surface ";
   cerr << endl;
}

int
BMESH::set_crease(int i, int j) const
{
   Bedge* e = nullptr;
   if (valid_vert_indices(i,j) && (e = bv(i)->lookup_edge(bv(j))))
      e->set_crease();
   else
      err_msg("BMESH::set_crease: error: no such edge (%d,%d)", i, j);
   return e ? 1 : 0;
}

int
BMESH::set_weak_edge(int i, int j) const
{
   Bedge* e = nullptr;
   if (valid_vert_indices(i,j) && (e = bv(i)->lookup_edge(bv(j))))
      e->set_bit(Bedge::WEAK_BIT);
   else
      err_msg("BMESH::set_weak_edge: error: no such edge (%d,%d)", i, j);
   return e ? 1 : 0;
}

int
BMESH::set_patch_boundary(int i, int j) const
{
   Bedge* e = nullptr;
   if (valid_vert_indices(i,j) && (e = bv(i)->lookup_edge(bv(j))))
      e->set_patch_boundary();
   else
      err_msg("BMESH::set_patch_boundary: error: no such edge (%d,%d)", i, j);
   return e ? 1 : 0;
}

Bface*
BMESH::pick_face(
   CWline&      world_ray,      // ray in world space
   Wpt&         world_hit       // returned: hit point in world space
   ) const
{
   // Brute force intersection:
   //   Given ray in world space, return intersected face (if any)
   //   and intersection point in world space:

   static bool debug = Config::get_var_bool("DEBUG_PICK_FACE",false);
   if (debug)
      err_msg("BMESH::pick_face: doing brute force intersection");

   Wpt  p = inv_xform() * world_ray.point();
   Wvec n = inv_xform() * world_ray.direction();

   Bface* ret = nullptr;        // face to return
   double d, min_d = -1;        // distance, min distance
   Wpt    h;                    // (temp) hit point
   for (int i=0; i<nfaces(); i++) {
      if (bf(i)->ray_intersect(p, n, h, d) && (!ret || d < min_d)) {
         min_d     = d;
         ret       = bf(i);
         world_hit = xform()*h;
      }
   }
   return ret;
}

int
BMESH::intersect(
   RAYhit   &r,         // ray-surface intersection data structure
   CWtransf &,          // object to world space xform 
   Wpt      &nearpt,    // ray-surface intersection, in world space
   Wvec     &n,         // surface normal, world space
   double   &d,         // distance to hit point, world space
   double   &d2d,       // unused screen-space distance
   XYpt     &uvc        // unused texture coordinate
   ) const
{
   // find visibility reference image for picking:
   BaseVisRefImage *vis_ref = BaseVisRefImage::lookup(VIEW::peek());

   // but for special requests we'll still do brute force:
   static bool force_face_pick
      = Config::get_var_bool("FORCE_FACE_PICK",false);

   if (force_face_pick || !r.from_camera() || !vis_ref) {

      Bface* f = pick_face(r.line(), nearpt);
      if (!f)
         return 0;

      d = nearpt.dist(r.point());

      // If we know the GEOM, set the RAYhit appropriately
      if (_geom && r.test(d,1,0)) {
         uvc = XYpt(0,0);
         n  = (inv_xform().transpose() * f->norm()).normalized();
         Wpt obj_pt = inv_xform() * nearpt;
         r.check(d, 1, 0, _geom, n, nearpt, obj_pt, f->patch(), uvc);
         BMESHray::set_simplex(r, f); // set 'face' data iff BMESHray
      } else {
         return 1;
      }
   }

   // Do reference image picking
   vis_ref->vis_update();
   Bsimplex* sim = vis_ref->vis_simplex(r.screen_point());
   if (!(sim && sim->mesh().get() == this)) {
      return 0;
   }

   Wvec norm;
   if (sim->view_intersect(r.screen_point(),nearpt,d,d2d,norm) &&
       r.test(d,1,0)) {
      uvc  = XYpt();
      n = norm;         // it's already in world space

      // get the patch in case anyone cares:
      // XXX - returning the control patch:
      Patch* p = get_ctrl_patch(sim);

      r.check(d, 1, d2d, _geom, n, nearpt, inv_xform()*nearpt, p, uvc);
      BMESHray::set_simplex(r, sim);
      return 1;
   }

   return 0;
}

void
BMESH::clear_creases()
{
   bool changed_occurred = 0;
   for (Bedge_list::size_type i=0; i<_edges.size(); i++) {
      if (be(i)->is_crease()) {
         be(i)->set_crease(0);
         changed_occurred=1;
      }
   }

   if (changed_occurred)
      changed(CREASES_CHANGED);
}

void
BMESH::compute_creases()
{
   double thresh = Config::get_var_dbl("JOT_CREASE_THRESH", 0.5, true);

   for (Bedge_list::size_type k=0; k<_edges.size(); k++)
      be(k)->compute_crease(thresh);

   changed(CREASES_CHANGED);
}

int
BMESH::build_polyline_strips()
{
   // if mesh has "polylines" (edges with no adjacent faces)
   // then calculate polyline strips so they can be rendered.

   // called e.g. in read_polylines() if needed.

   // reset or allocate polyline edge strip
   if (_polylines)
      _polylines->reset();
   else
      _polylines = new_edge_strip(); // an LMESH uses an LedgeStrip

   // add polyline edge strip to 1st patch by default
   // (make sure there is at least one patch)
   make_patch_if_needed();
   _patches[0]->add
      (_polylines);

   // find polyline ends; i.e., polyline edges that are
   // not adjacent to other polyline edges at both ends.
   // prefer to start strips from these polyline ends.
   vector<Bedge*> polyline_edges; polyline_edges.reserve(1024);
   vector<Bedge*> polyline_ends; polyline_ends.reserve(128);
   Bedge_list::size_type k;
   for (k=0; k<_edges.size(); k++) {
      // also clear all edge flags.
      _edges[k]->clear_flag();
      if (_edges[k]->is_polyline()) {
         // collect regular polyline edges in one list,
         // and polyline ends into another
         if (_edges[k]->is_polyline_end())
            polyline_ends.push_back(_edges[k]);
         else
            polyline_edges.push_back(_edges[k]);
      }
   }

   // put the polyline ends last
   polyline_edges.insert(polyline_edges.end(), polyline_ends.begin(), polyline_ends.end());

   // work backward to get polyline ends first:
   UnreachedSimplexFilter unreached;
   PolylineEdgeFilter     polyline;
   AndFilter       wanted = unreached + polyline;
   for (k=polyline_edges.size()-1; ; k--) {
      Bedge* e = polyline_edges[k];
      Bvert* v = e->v2()->is_polyline_end() ? e->v2() : e->v1();
      _polylines->build(v, e, wanted);
      if (k == 0) break;
   }

   err_msg("BMESH::build_polyline_strips: got %d edges",
           _polylines->edges().size());
   err_msg("Warning: BMESH does not currently draw polylines");

   return _polylines->num();
}

int
BMESH::build_vert_strips()
{
   // if mesh has isolated vertices, put them in vertext strips so
   // they can be rendered.

   // called e.g. in read_stream() if needed.

   // reset or allocate vertex strip
   if (_lone_verts)
      _lone_verts->reset();
   else
      _lone_verts = new_vert_strip(); // an LMESH uses an LvertStrip

   // add vertex strip to 1st patch by default
   // (make sure there is at least one patch)
   if (_patches.empty())
      new_patch();
   _patches[0]->add
      (_lone_verts);

   for (Bvert_list::size_type k=0; k<_verts.size(); k++)
      if (bv(k)->degree() == 0)
         _lone_verts->add
            (bv(k));

   return _lone_verts->num();
}

int
BMESH::build_zcross_strips()
{
   // do nothing if silhouette strips are up-to-date
   // or there are no surfaces

   static bool HACK_EYE_POS = Config::get_var_bool("HACK_EYE_POS",false);
   static bool DEBUGADAPT = Config::get_var_bool("DEBUG_ADAPTIVE",false);
   if ((!(HACK_EYE_POS || DEBUGADAPT ) && ( _zx_stamp == VIEW::stamp())) ||
       nfaces() == 0 || _freeze_sils)
      return 0;

   static bool debug = Config::get_var_bool("DEBUG_BUILD_ZX_STRIPS",false);
   if (debug)
      err_msg("BMESH::build_zcross_strips: frame number %d",
              (int)VIEW::stamp());

   // clear sil strip lists in patches
   int k;
   for (k = 0; k<_patches.num(); k++)
      _patches[k]->zx_sils().reset();

   // extract sil strips
   get_zcross_strips();

   if (_zx_sils.empty())
      return 0;
   // distribute sils to patches

   Patch*  p = nullptr;
   Patch* lp = _zx_sils.seg(0).f()->patch(); // last p
   assert (lp != nullptr);


   for (k = 0; k < _zx_sils.num(); k++) {

      Bface* f = _zx_sils.seg(k).f();
      if ( !f && k > 0 )
         assert ( _zx_sils.seg(k-1).f() ); //assert for double nullptrs

      // null f means that point belongs to last f;
      // this accessor is probably less than safe...
      if (f) {

         p = f->patch();
         assert(p);

         p->zx_sils().add_seg (_zx_sils.seg(k));

         if  ( p != lp ) {
            //patch boundary fixit

            if (!lp->zx_sils().segs().empty() && lp->zx_sils().segs().back().f()) {

               lp->zx_sils().add_seg (
                  nullptr, _zx_sils.seg(k-1).p(), _zx_sils.seg(k-1).v(),
                  _zx_sils.seg(k-1).g() , _zx_sils.seg(k-1).bc()
                  );
            }
         }
      } else {

         p = lp;
         p->zx_sils().add_seg (_zx_sils.seg(k));     //add the termination  marker to the last patch

      }
      lp = p;
   }

   // call this AFTER get_zcross_strips():
   _zx_stamp = VIEW::stamp();

   return _zx_sils.num();
}

RunningAvg<double> rand_secs(0);  // secs per randomized extraction
RunningAvg<double> brute_secs(0); // secs per brute-force extraction
RunningAvg<double> zx_secs(0);    // secs per zero-cross extraction (randomized)
RunningAvg<double> rand_sils(0);  // avg num sils found randomized
RunningAvg<double> brute_sils(0); // avg num sils found brute-force
RunningAvg<double> zx_sils(0);    // avg num sils found randomized zero-crossing
RunningAvg<double> all_edges(0);  // avg number of total mesh edges

int
BMESH::get_zcross_strips()
{
   static bool REALLY_HACK_EYE_POS = Config::get_var_bool("REALLY_HACK_EYE_POS",false);
   static bool HACK_EYE_POS = Config::get_var_bool("HACK_EYE_POS",false);
   static bool ALLOW_CHECK_ALL = Config::get_var_bool("ALLOW_CHECK_ALL",false);
   static int CHECK_ALL_UNDER = Config::get_var_int ("CHECK_ALL_UNDER", 1000, true);

   vector<ZXseg> old_segs = _zx_sils.segs();

   _zx_sils.reset();
   if (HACK_EYE_POS && (geom()->has_transp() | REALLY_HACK_EYE_POS)) {
      // get the silhouettes from the position of light4
      // (instead of actual camera )
      Wpt light_pos;
      VIEWptr v = VIEW::peek();
      if (v->light_get_in_cam_space(3))
         light_pos = v->cam()->xform().inverse() *
            v->light_get_coordinates_p(3);

      else
         light_pos = v->light_get_coordinates_p(3);

      light_pos = inv_xform() * light_pos;

      _zx_sils.set_eye(light_pos);
   } else
      _zx_sils.set_eye(eye_local());

   int n = _faces.size();
   bool CHECK_ALL = ALLOW_CHECK_ALL && (n < CHECK_ALL_UNDER);
   static int min_rand_faces = Config::get_var_int("RANDOMIZED_MIN_FACES", 4000, true);
   if (n < min_rand_faces               ||      // if too few faces
       _zx_stamp < VIEW::stamp() - 1     ||      // or the old sils are too old
       old_segs.empty()                 ||      // or there are no old ones
       !_random_sils || CHECK_ALL) {            // or sposed to do brute force

      // check all edges to find new sils:
      for (int k = 0; k < n ; k++)
         _zx_sils.start_sil(_faces[k]);

   } else {
      size_t i;
      vector<ZXseg>::size_type k;

      // We'll time it:
      stop_watch clock;

      // get crease and border lists from the mesh.
      // check triangles neighboring these edges.
      // solves the 'missing silhouettes on cylinders' problem

      // Get the crease and border edges of the mesh via the
      // cached strip. (It's lightweight after the first time,
      // until creases change again.)
      //
      CBedge_list& creases = get_creases();
      for (i=0; i<creases.size(); i++) {
         Bedge* e = creases[i];
         if (e && e->f(1))
            _zx_sils.start_sil(e->f(1));
         if (e && e->f(2))
            _zx_sils.start_sil(e->f(2));
      }
      CBedge_list& borders = get_borders();
      for (i=0; i<borders.size(); i++) {
         Bedge* e = borders[i];
         if (e && e->f(1))
            _zx_sils.start_sil(e->f(1));
         if (e && e->f(2))
            _zx_sils.start_sil(e->f(2));
      }

      // Now check old silhoutte triangles:
      for (k=0 ; k<old_segs.size(); k++) {
         // some old faces are nullptr
         if (old_segs[k].f())
            _zx_sils.start_sil(old_segs[k].f());
      }

      // Now do the randomized part:
      int maxj = n - 1;
      for (k = (int)(2. * sqrt((double)n)); k>0; k--) {
         // XXX - On WIN32 drand48() sometimes returns 1.0,
         //       so we're careful below to ensure j < n:
         int j = min((int)(drand48()*n), maxj);
         _zx_sils.start_sil(_faces[j]);
      }

      // Record the time taken and number of silhouettes found:
      zx_secs.add(clock.elapsed_time());
      zx_sils.add(_zx_sils.num());
   }

   _zx_stamp = VIEW::stamp();

   return _faces.size();
}

int
BMESH::build_sil_strips()
{
   // do nothing if silhouette strips are up-to-date
   if (_sil_stamp == VIEW::stamp() || _freeze_sils)
      return 0;

   // clear sil strip lists in patches
   int k;
   for (k = 0; k<_patches.num(); k++)
      _patches[k]->sils().reset();

   // get sil strips
   get_sil_strips();

   // distribute sils to patches
   Patch* p;
   for (k = 0; k < _sils.num(); k++) {
      Bedge* e = _sils.edge(k);
      Bface* f = e->frontfacing_face();
      // if it's a border edge, there may be no front-facing triangle.
      // so just take the non-null face:
      if (!f)
         f = e->get_face();
      if (f && (p = f->patch()))
         p->sils().add(_sils.vert(k), e);
   }

   // call this AFTER get_sil_strips():
   _sil_stamp = VIEW::stamp();

   return _sils.num();
}

int
BMESH::get_sil_strips()
{
   // Record number of mesh edges this time:
   all_edges.add(nedges());

   // Copy the old sil edges into a temporary list:
   Bedge_list old_sils = _sils.edges();

   // clear the old sil strip (clears edge list):
   _sils.reset();

   // Make a filter that accepts NEW silhouettes
   NewSilEdgeFilter filter(VIEW::stamp(), !show_secondary_faces());

   static int min_rand_edges =
      Config::get_var_int("RANDOMIZED_MIN_EDGES", 4000, true);

   if (nedges() < min_rand_edges        ||  // if too few edges
       _sil_stamp < VIEW::stamp() - 1   ||  // or old sils are too old
       old_sils.empty()                 ||  // or there are no old sils
       !_random_sils) {                     // or we're not doing random sils

      // We'll time it:
      stop_watch clock;

      // Check all edges to find new sils:
      for (Bedge_list::size_type k=0; k<_edges.size(); k++)
         if (_edges[k]->is_sil())
            _sils.build(nullptr, _edges[k], filter);  // get all connected sils

      // Record the time taken and number of silhouettes found:
      brute_secs.add(clock.elapsed_time());
      brute_sils.add(_sils.num());

   } else {
      // Randomized algorithm described in:
      //    "Real-Time Nonphotorealistic Rendering," by
      //    Lee Markosian, Michael A. Kowalski, Samuel J. Trychin,
      //    Lubomir D. Bourdev, Daniel Goldstein, John F. Hughes.
      //    Proceedings of SIGGRAPH 97.
      //
      // Assuming the silhouette edges themselves number O(sqrt(n))
      // (where n is the total number of edges), then the following
      // code runs in O(sqrt(n)) steps.

      // We'll time it:
      stop_watch clock;

      // 1. First check all the old ones.
      size_t k;
      for (k = 0; k < old_sils.size(); k++)
         if (old_sils[k]->is_sil())
            _sils.build(nullptr, old_sils[k], filter);

      // 2. Now check a small fraction of the edges.
      //    (If n is the total number of edges,
      //     we'll check 2 * sqrt(n) edges.)

      int n = _edges.size();
      k = (size_t)(2*sqrt((double)n));
      for (int maxj = n - 1; k>0; k--) {
         // XXX - On WIN32 drand48() sometimes returns 1.0,
         //       so we're careful below to ensure j < n:
         int j = min((int)(drand48()*n), maxj);
         if (_edges[j]->is_sil())
            _sils.build(nullptr, _edges[j], filter);  // get all connected sils
      }

      // Record the time taken and number of silhouettes found:
      rand_secs.add(clock.elapsed_time());
      rand_sils.add(_sils.num());
   }

   // Record the frame number for this silhouette extraction:
   _sil_stamp = VIEW::stamp();

   return _sils.num();
}

EdgeStrip 
BMESH::sil_strip() 
{
   // extract silhouette edge strip and return a copy of it:
   build_sil_strips();
   return _sils;
}

void
BMESH::set_geom(GEOM *geom)
{
   if (_geom)
      err_msg("BMESH::set_geom: warning: overwriting old geom");
   if (!geom)
      err_msg("BMESH::set_geom: warning: new geom is null");
   _geom = geom;
}

CWtransf&
BMESH::xform() const 
{
   // FIXME: const
   const_cast<BMESH*>(this)->_obj_to_world = (_geom ? _geom->obj_to_world() : Identity);
   return _obj_to_world;
}

CWtransf&
BMESH::inv_xform() const 
{
   // FIXME: const
   const_cast<BMESH*>(this)->_world_to_obj = (_geom ? _geom->world_to_obj() : Identity);
   return _world_to_obj;
}

CWtransf& 
BMESH::obj_to_ndc() const 
{
   if (_pm_stamp != VIEW::stamp()) {
      // FIXME: const
      const_cast<BMESH*>(this)->_pm_stamp = VIEW::stamp();
      const_cast<BMESH*>(this)->_pm = VIEW::peek_cam()->ndc_projection() * xform();
   }
   return _pm;
}

CWpt& 
BMESH::eye_local() const 
{
   // Returns camera position ("eye") in object space ("local coordinates")

   // XXX - Hmm, if the mesh's xform changes, this should
   // be recomputed, right?  This stamp should include that
   // property... I think...(rdk)
   //
   // That is correct; though the following probably works in
   // practice, since it means the eye local position is updated
   // once each frame, if requested.

   if (_eye_local_stamp != VIEW::stamp()) {
      // FIXME: const
      const_cast<BMESH*>(this)->_eye_local_stamp = VIEW::stamp();
      CWpt& eye = VIEW::peek_cam()->data()->from();
      const_cast<BMESH*>(this)->_eye_local = inv_xform() * eye;
   }
   return _eye_local;
}

void
BMESH::make_patch_if_needed()
{
   Patch* p = nullptr;

   for (Bface_list::size_type k=0; k<_faces.size(); k++) {
      if (!bf(k)->patch()) {
         if (!p)
            p=new_patch();
         p->add
            (bf(k));
      }
   }
}

bool
BMESH::remove_patch(int k)
{
   if (!_patches.valid_index(k)) {
      err_msg("BMESH::remove_patch: it's not in the list of patches");
      return 0;
   }
   Patch* p = _patches[k];
   if (p->num_faces() != 0)
      err_msg("BMESH::remove_patch: removed patch, %d faces left stranded",
              p->num_faces());
   _patches.remove(k);
   _drawables -= p;
   delete p;
   return 1;
}

void
BMESH::clean_patches()
{
   // Clean out any empty patches.

   bool debug = Config::get_var_bool("DEBUG_CLEAN_PATCHES",false);

   // Work backward to remove from the list:
   for (int i=_patches.num()-1; i>=0; i--)
      if (_patches[i]->num_faces() == 0)
         if (remove_patch(i) && debug)
            err_msg("BMESH::clean_patches: got one");

}


void
BMESH::send_update_notification()
{
   // Send controllers a message to do their mesh modifications now:
   BMESHobs::broadcast_update_request(shared_from_this());

   // LMESH over-rides this to also compute subdivision meshes.
}

void
BMESH::request_ref_imgs()
{
   if (draw_enabled())
      _drawables.request_ref_imgs();
}

int
BMESH::draw_img(const RefImgDrawer& r)
{
   if (!draw_enabled())
      return 0;

   send_update_notification();

   return r.draw(&_drawables);
}

void
BMESH::transform(CWtransf &xf, CMOD& m)
{
   _verts.transform(xf);

   // XXX - Swapped order of these notifications... (rdk) why? (lem)
   BMESHobs::broadcast_xform(shared_from_this(), xf, m);

   changed(VERT_POSITIONS_CHANGED);
}

CWpt_list &
BMESH::vertices()
{
   if (!_vert_locs.empty() || _verts.empty())
      return _vert_locs;

   _vert_locs.reserve(nverts());
   for (int k=nverts()-1; k>=0; k--)
      _vert_locs.push_back(_verts[k]->loc());
   return _vert_locs;
}

void
BMESH::triangulate(Wpt_list &verts, FACElist &faces)
{
   verts.clear();
   faces.clear();
   if (nverts() > 0)
      verts.reserve(_verts.size());
   if (nfaces() > 0)
      faces.resize(_faces.size());

   int i;
   for (i=0; i<nverts(); i++)
      verts.push_back(bv(i)->loc());
   for (i=0; i<nfaces(); i++)
      faces[i] = Point3i(bf(i)->v(1)->index(),
                         bf(i)->v(2)->index(),
                         bf(i)->v(3)->index());
}

BMESHptr
BMESH::read_jot_file(const char* filename, BMESHptr ret)
{
   // Opens the file and calls BMESH::read_jot_stream().
   
   if (!filename) {
      err_msg("BMESH::read_jot_file() - Filename is NULL");
      return nullptr;
   }
   fstream fin;
#if (defined (WIN32) && (defined(_MSC_VER) && (_MSC_VER <=1300))) /*VS 6.0*/

   fin.open(filename, ios::in | ios::nocreate);
#else

   fin.open(filename, ios::in);
#endif

   if (!fin) {
      err_mesg(ERR_LEV_WARN, "BMESH::read_jot_file() - Could not open file '%s'", filename);
      return nullptr;
   }

   return read_jot_stream(fin, ret);
}

BMESHptr
BMESH::read_jot_stream(istream& in, BMESHptr ret)
{
   // Read a mesh from a stream and return it.  Handles new
   // .jot or .sm formats, as well as old .sm format. If ret
   // is null, a new mesh will be generated (BMESH or LMESH,
   // depending on what type is specified in the stream). If
   // ret is a BMESH and the file specifies LMESH, an LMESH is
   // generated, read from the stream, and then the mesh data
   // is copied into ret.

   // Run thru initial whitespace
   while (isspace(in.peek()))
      in.get();

   // If the first character isn't printable, reject it
   char firstchar = in.peek();

   if (!isprint(firstchar)) {
      err_msg("BMESH::read_jot_stream() - Unreadable: Non-printable first character.");
      return nullptr;
   } else if (isdigit(firstchar)) {
      // If it's numerical digit then try to load as old .sm
      // This will go away...
      if (!ret) {
         ret = make_shared<LMESH>(); // use LMESH by default for old .sm files
      }
      if (ret->read_stream(in)) {
         return ret;
      } else {
         err_msg("BMESH::read_jot_stream() - Error: Failed parsing old-style file.");
         return nullptr;
      }
   } else if (firstchar == '#') {
      // If it's a # then fail out...
      char file_header[2048];
      in.get();
      in >> file_header;

      err_mesg(ERR_LEV_INFO,
               "BMESH::read_jot_stream() - Warning: Rejecting file with '#%s' header.",
               file_header);
      return nullptr;
   } else {
      // Otherwise, it better be the new .sm format...
      STDdstream stream(&in);

      // Check the class name:
      string class_name;
      stream >> class_name;

      // If we don't yet have a mesh, or the mesh we have is the wrong
      // type, then get one of the right type:
      if (!(ret && ret->is_of_type(class_name))) {
         DATA_ITEM* di = DATA_ITEM::lookup(class_name);
         if (!di) {
            err_msg(
               "BMESH::read_jot_stream() - Error: Supposed mesh class '#%s' not found.",
               class_name.c_str());
            return nullptr;
         }

         // Get the correct type (BMESH or LMESH) as specified in the file:
         ret = dynamic_cast<BMESH*>(di->dup())->shared_from_this();
         if (!ret) {
            err_msg(
               "BMESH::read_jot_stream() - Error: Class '#%s' is not a BMESH subclass.",
               class_name.c_str());
            return nullptr;
         }
      }

      ret->decode(stream);
      return ret;
   }
}

bool
BMESH::read_file(const char* filename)
{
   // Read a mesh file into *this* mesh:

   BMESHptr mesh = read_jot_file(filename, shared_from_this());
   if (!mesh)
      return false;

   // In case this is a BMESH, but the file specified an LMESH,
   // then read_jot_file() would have returned an LMESH. Now
   // copy its data into this mesh:
   if (this != mesh.get())
      *this = *mesh;

   return true;
}

// XXX - Deprecated (now TEXBODY's mesh_data_update_file tag
//       also just uses read_file)
int
BMESH::read_update_file(const char* filename)
{
   if (!filename) {
      err_ret( "BMESH::read_update_file: filename is NULL");
      return 0;
   }

   ifstream fin;
   fin.open(filename);
   if (!fin) {
      err_ret( "BMESH::read_update_file: could not open file %s", filename);
      return 0;
   }

   int ret = 1;

   if (!read_update_stream(fin)) {
      err_msg("BMESH::read_update_file: error reading file %s", filename);
      ret = 0;
   }

   return ret;
}

// XXX - Deprecated
int
BMESH::read_update_stream(istream& is)
{
   // read header
   if (!read_header(is))
      return 0;                 // stop if an error occurred

   // read vertices
   int ret = read_vertices(is);

   if (ret) {
      // must do this after changing mesh:
      changed(VERT_POSITIONS_CHANGED);
   }


   return ret;
}


// XXX - Deprecated
int
BMESH::read_stream(istream& is)
{
   // free up current data
   delete_elements();

   // read header
   if (!read_header(is))
      return 0;                 // stop if an error occurred

   // read vertices
   int ret = read_vertices(is);

   // read faces
   ret = ret && read_faces(is);

   if (!ret) {
      // must do this after changing mesh:
      changed(TOPOLOGY_CHANGED);
      make_patch_if_needed();
      return 0;
   }

   // read creases
   // XXX - changed so errors here don't abort the process
   read_creases(is);

   // read edges without faces
   // XXX - changed so errors here don't abort the process
   read_polylines(is);

   // must do this after changing mesh:
   changed(TOPOLOGY_CHANGED);

   // now the topological type is determined; if there are lone
   // vertices store them in vertex strips.
   if (is_points() && Config::get_var_bool("BMESH_BUILD_VERT_STRIPS",false))
      build_vert_strips();

   // continue reading the file but now check for tokens
   ret = ret && read_blocks(is);

   // If faces were read in, but no Patches were created,
   // make a Patch now and put all the faces in:
   make_patch_if_needed();

   // Problem:
   //   read_faces() creates a default patch.  Then sometimes
   //   patches are read in from file, whereupon they take the
   //   faces away from the default patch, which is left
   //   empty. For reasons no one understands, this leads to seg
   //   faults in the optimized build (on linux).
   //
   // Solution:
   //   Don't do that. I.e., remove the empty patch.
   clean_patches();

   return ret;
}

int
BMESH::read_blocks(istream &is)
{
   if (Config::get_var_bool("JOT_SKIP_IOBLOCKS",false))
      return true;

   static IOBlockList blocklist;
   if (blocklist.empty()) {
      blocklist.push_back(new IOBlockMeth<BMESH>("COLORS",
                                                 &BMESH::read_colors, this));
      blocklist.push_back(new IOBlockMeth<BMESH>("TEX_COORDS2",
                                                 &BMESH::read_texcoords2, this));
      blocklist.push_back(new IOBlockMeth<BMESH>("WEAK_EDGES",
                                                 &BMESH::read_weak_edges, this));
      blocklist.push_back(new IOBlockMeth<BMESH>("PATCH",
                                                 &BMESH::read_patch, this));
      blocklist.push_back(new IOBlockMeth<BMESH>("INCLUDE",
                                                 &BMESH::read_include, this));

   } else {
      for (auto & elem : blocklist) {
         ((IOBlockMeth<BMESH> *) elem)->set_obj(this);
      }
   }
   vector<string> leftover;
   int ret = IOBlock::consume(is, blocklist, leftover);
   // Put leftover words back on stream
   for (string & str : leftover) {
      is.putback(' ');
      for (string::size_type i = str.length(); i > 0; i--) {
         is.putback(str[i-1]);
      }
   }


   return ret;
}

int
BMESH::read_include(istream& is, vector<string> &/*leftover*/)
{
   const int len = 1024;
   char buff[len];
   is >> buff;
   ifstream fin;
   fin.open(buff);
   if (!fin) {
      err_ret( "BMESH::read_include: could not open include file %s", buff);
      return 0;
   }
   return read_blocks(fin);
}

int
BMESH::read_header(istream& is)
{
   char file_header[256];
   char firstchar = is.peek();

   // If the first character isn't printable, bad!
   if (!isprint(firstchar))
      return 0;

   // Not a #<header>, then return a success...
   if (firstchar != '#')
      return 1;


   is.get();
   is >> file_header;
   if (is.bad() || is.eof())
      return 0;

   if (strstr(file_header, "jot")) {
      err_msg("BMESH::read_header() - Trying to read a file with #jot header, stopping...");
      return 0;
   } else {
      err_msg("BMESH::read_header() - Reading a file with %s header...",file_header);
      return 1;
   }

}

int
BMESH::read_vertices(istream& is)
{
   int n;
   is >> n;
   if (is.bad() || is.eof()) {
      return 0;
   }

   _verts.reserve(n);
   _edges.reserve(3*n);

   for (int i=0 ; i<n; i++) {
      double x, y, z;
      is >> x >> y >> z;

      if (i<(int)_verts.size())
         _verts[i]->set_loc(Wpt(x,y,z));
      else
         add_vertex(Wpt(x,y,z));
   }

   return 1;
}

int
BMESH::read_faces(istream& is)
{
   int n = 0, i, j, k;
   is >> n;
   if (is.bad())
      return 0;

   if (n>0)
      _faces.reserve(n);

   int ret = 1; // really it's a bool (1 == success)

   for ( ; n>0 && !is.eof(); n--) {

      // Read 3 vertex indices:
      is >> i >> j >> k;

      // Create the face:
      if (!add_face(i,j,k,nullptr))
         ret = 0;       // not success
   }

   return ret;
}

int
BMESH::read_creases(istream& is)
{
   // read vertex pairs corresponding to "crease" edges
   // mark vertices and edges w/ crease tag.

   int n;
   is >> n;

   int ret = 1;
   if (is.bad()) {
      err_ret( "BMESH::read_creases: istream is bad");
      return 0;
   }
   for (int k=0; k<n && !is.eof(); k++) {
      int i, j;
      is >> i >> j;
      if (!set_crease(i,j))
         ret = 0;
   }

   return ret;
}

int
BMESH::read_weak_edges(istream& is, vector<string> &leftover)
{
   leftover.clear();

   if (is.bad()) {
      err_ret( "BMESH::read_weak_edges: istream is bad");
      return 0;
   }

   int n, ret = 1;
   is >> n;
   for (int k=0; k<n && !is.eof(); k++) {
      int i, j;
      is >> i >> j;
      if (!set_weak_edge(i,j))
         ret = 0;
   }

   return ret;
}

int
BMESH::read_polylines(istream& is)
{
   // read vertex pairs corresponding to
   // lone edges (without faces)

   int n;
   is >> n;

   int ret = 1;
   if (is.bad()) {
      err_ret( "BMESH::read_creases: istream is bad");
      ret = 0;
   } else {
      for (int k=0; k<n && !is.eof(); k++) {
         int i, j;
         is >> i >> j;
         if (!add_edge(i,j))
            ret = 0;
      }
   }

   if (Config::get_var_bool("BMESH_BUILD_POLYSTRIPS",false) && n > 0)
      build_polyline_strips();

   return ret;
}

int
BMESH::read_colors(istream& is, vector<string> &leftover)
{
   leftover.clear();
   for (int i = 0; i< nverts(); i++) {
      double r,g,b;
      is >> r >> g >> b;
      bv(i)->set_color( COLOR(r,g,b));
   }

   return 1;
}


int
BMESH::read_texcoords2(istream& is, vector<string> &leftover)
{
   static double UV_RESOLUTION = Config::get_var_dbl("UV_RESOLUTION",0,true);

   leftover.clear();
   int n;
   is >> n;
   for (int i = 0; i<n; i++) {
      UVpt uv[3];
      int k;
      is >> k >> uv[0] >> uv[1] >> uv[2];
      if (0 <= k && k < (int)_faces.size()) {
         // Round UV's to nearest UV_RESOLUTION
         // A value of 0.00001 is nice...
         if (UV_RESOLUTION>0) {
            for (auto & elem : uv) {
               double r;
               r = fmod(elem[0], UV_RESOLUTION);
               elem[0] -= r;
               if (2.0*fabs(r) > UV_RESOLUTION)
                  elem[0] += (r > 0) ? UV_RESOLUTION : -UV_RESOLUTION;
               r = fmod(elem[1],UV_RESOLUTION);
               elem[1] -= r;
               if (2.0*fabs(r) > UV_RESOLUTION)
                  elem[1] += (r > 0) ? UV_RESOLUTION : -UV_RESOLUTION;
            }
         }

         UVdata::set
            (bf(k),uv[0], uv[1], uv[2]);
      } else
         err_msg("BMESH::read_texcoords2: invalid face index %d", k);
   }

   return 1;
}

int
BMESH::read_patch(istream& is, vector<string> &leftover)
{
   return new_patch()->read_stream(is, leftover);
}

int
BMESH::write_file(const char* filename)
{
   fstream fout;
   fout.open(filename, ios::out);

   if (!fout) {
      err_msg("BMESH::write_file: error: could not open file: %s", filename);
      return 0;
   }

   STDdstream stream(&fout);
   format(stream);
   fout.close();

   return 1;
}

int
BMESH::write_stream(ostream& os)
{
   // Write new-style .sm file:

   STDdstream out(&os);
   format(out);
   os << endl;

   return 1;
}

int
BMESH::write_vertices(ostream& os) const
{
   os << nverts() << endl;
   if (os.bad())
      return 0;

   bool xform_mesh = Config::get_var_bool("JOT_SAVE_XFORMED_MESH",false);

   Wpt p;
   for (int i = 0; i< nverts(); i++) {
      if (xform_mesh)
         p = bv(i)->wloc();
      else
         p = bv(i)->loc();
      os << p[0] << " " << p[1] << " " << p[2] << endl;
   }

   return 1;
}

int
BMESH::write_colors(ostream& os) const
{
   // if there are colors, write them

   if (!_verts.empty() && bv(0)->has_color()) {
      //if one vertex has a color, they all do
      if (os.bad())
         return 0;

      os << "#BEGIN COLORS" << endl;
      for (int i = 0; i< nverts(); i++) {
         const COLOR& c = bv(i)->color();
         os << c[0] << " "
            << c[1] << " "
            << c[2] << " " << endl;
      }
      os << "#END COLORS" << endl;
   }

   return 1;
}

int
BMESH::write_texcoords2(ostream& os) const
{
   // New version replacing write_texcoords(), but for now
   // leaving that one there in case some old files use it

   if (os.bad())
      return 0;

   // Find out how many faces have tex coords:
   int count=0, k;
   for (k=0; k<nfaces(); k++)
      if (UVdata::lookup(bf(k)))
         count++;

   // If none, bail early
   if (count == 0)
      return 1;

   // Need to work it
   os << "#BEGIN TEX_COORDS2" << endl;
   os << count << endl;
   for (k=0; k<nfaces(); k++) {
      UVdata* uvdata = UVdata::lookup(bf(k));
      if (uvdata) {
         os << k << " "
            << uvdata->uv(1) << " "
            << uvdata->uv(2) << " "
            << uvdata->uv(3) << endl;
      }
   }
   os << "#END TEX_COORDS2" << endl;

   return 1;
}

int
BMESH::write_faces(ostream& os) const
{
   os << nfaces() << endl;
   if (os.bad())
      return 0;

   for (int i = 0; i< nfaces(); i++)
      os << *bf(i);

   return 1;
}

int
BMESH::write_creases(ostream& os) const
{
   if (nedges()<1 || os.bad())
      return 0;

   vector<Bedge*> creases; creases.reserve(512);

   creases.clear();

   int k;
   for (k=0; k<nedges(); k++)
      if (be(k)->is_crease())
         creases.push_back(be(k));

   os << creases.size() << endl;
   while (!creases.empty()) {
      os << *creases.back() << endl;
      creases.pop_back();
   }

   os << endl;

   return 1;
}

int
BMESH::write_weak_edges(ostream& os) const
{
   if (os.bad())
      return 0;

   vector<Bedge*> weak_edges; weak_edges.reserve(nedges());

   for (int k=0; k<nedges(); k++)
      if (be(k)->is_weak())
         weak_edges.push_back(be(k));

   if (!weak_edges.empty()) {

      os << "#BEGIN WEAK_EDGES" << endl;
      os << weak_edges.size() << endl;
      while (!weak_edges.empty()) {
         os << *weak_edges.back() << endl;
         weak_edges.pop_back();
      }
      os << endl;

      os << "#END WEAK_EDGES" << endl;
   }

   return 1;
}

int
BMESH::write_patches(ostream& os) const
{
   if (_patches.empty() || Config::get_var_bool("TMOD_DONT_WRITE_PATCHES",false))
      return 1;

   // Write out patch info
   if (_patches[0]->cur_tex()) {
      if (os.bad())
         return 0;

      _patches.write_stream(os);
   }
   return 1;
}

int
BMESH::write_polylines(ostream& os) const
{
   if (nedges()<1 || os.bad())
      return 0;

   // XXX - use _polylines
   vector<Bedge*> polylines; polylines.reserve(256);

   polylines.clear();

   int k;
   for (k=0; k<nedges(); k++)
      if (be(k)->is_polyline())
         polylines.push_back(be(k));

   os << polylines.size() << endl;
   while (!polylines.empty()) {
      os << *polylines.back() << endl;
      polylines.pop_back();
   }

   os << endl;

   return 1;
}

void
BMESH::delete_elements()
{
   _drawables.clear();

   _patches.delete_all();

   // delete mesh elements, faces first,
   // since deleting a vert will also delete adjacent
   // edges and faces.
   _faces.delete_all();
   _edges.delete_all();
   _verts.delete_all();

   _type = EMPTY_MESH;
   _type_valid = 1;
}

BMESH&
BMESH::operator =(CBMESH& m)
{
   delete_elements();

   _type       = m._type;
   _type_valid = m._type_valid;

   if (m.nverts() > 0)
      _verts.reserve(m._verts.size());
   if (m.nedges() > 0)
      _edges.reserve(m._edges.size());
   if (m.nfaces() > 0)
      _faces.reserve(m._faces.size());

   // copy verts
   int k;
   for (k=0; k<m.nverts(); k++) {
      Bvert* v_old = m.bv(k);
      Bvert* v_new = add_vertex(v_old->loc());
      if (v_old->has_color())
         v_new->set_color(v_old->color());
   }

   // copy edges
   for (k=0; k<m.nedges(); k++) {
      Bedge* e_old = m.be(k);
      Bedge* e_new = add_edge(e_old->v1()->index(), e_old->v2()->index());
      if (e_old->is_crease())
         e_new->set_crease(e_old->crease_val());
      if (e_old->is_patch_boundary())
         e_new->set_patch_boundary();
      if (e_old->is_weak())
         e_new->set_bit(Bedge::WEAK_BIT);
   }

   // XXX - doesn't copy patches yet
   Patch* p = new_patch();

   // copy faces
   for (k=0; k<m.nfaces(); k++) {
      Bface* f_old = m.bf(k);
      Bface* f_new = add_face(f_old->v1()->index(),
                              f_old->v2()->index(),
                              f_old->v3()->index(), p);
      if (f_old->is_secondary())
         f_new->make_secondary();
   }

   changed(TRIANGULATION_CHANGED);

   return *this;
}

BMESH&
BMESH::operator =(BODY& body)
{
   if (body.is_of_type(static_name()))
      return *this = *((BMESH*)&body);

   delete_elements();

   Wpt_list pts;
   FACElist tris;
   body.triangulate(pts, tris);
   if (!pts.empty() && !tris.empty()) {
      _verts.reserve(pts.size());
      _faces.reserve(tris.size());
      _edges.reserve(tris.size()*3/2+10);
      size_t i;
      for (i = 0; i < pts.size(); i++)
         add_vertex(pts[i]);
      Patch* p = new_patch();
      for (i = 0; i < tris.size(); i++)
         add_face(tris[i][0], tris[i][1], tris[i][2], p);

      // Set normals etc.
      changed(TOPOLOGY_CHANGED);

      // automatically mark creases by dihedral angle
      if (!Config::get_var_bool("SUPPRESS_CREASES",false)) {
         for (int k= nedges()-1; k>=0; k--)
            be(k)->compute_crease(0.5);
      }
   }

   return *this;
}


void
BMESH::delete_patches()
{
   _patches.delete_all();
}

int
BMESH::check_type()
{
   // check what type of mesh this is:
   //   POINTS,         (has isolated vertices)
   //   POLYLINES,      (has edges w/ no faces)
   //   OPEN_SURFACE,   (has some edges adjacent to just 1 face)
   //   CLOSED_SURFACE  (has faces; no edge adjacent to just 1 face)
   //
   // can also be a combination of the above types
   // (e.g. closed surface with additional isolated vertices)
   // except OPEN_SURFACE and CLOSED_SURFACE are mutually exclisive.

   // set _type_valid because we're checking the type now
   _type_valid = 1;
   _type = EMPTY_MESH;

   // get ready to count stuff
   int num_isolated_verts = 0,
      num_isolated_edges = 0,
      num_border_edges = 0,
      num_inconsistent_edges = 0;

   // check for isolated vertices:
   size_t k;
   for (k=0; k<_verts.size(); k++)
      if (bv(k)->degree() == 0)
         num_isolated_verts++;

   if (num_isolated_verts > 0)
      _type |= POINTS;

   // check for isolated edges, border edges,
   // and inconsistently oriented edges:
   for (k=0; k<_edges.size(); k++) {
      if (be(k)->nfaces() == 0)
         num_isolated_edges++;
      else if (be(k)->nfaces() == 1)
         num_border_edges++;
      else if (!be(k)->consistent_orientation())
         num_inconsistent_edges++;
   }

   // check for "polyline" edges
   if (num_isolated_edges > 0)
      _type |= POLYLINES;

   // check for closed/open surface.
   // (for closed surfaces can do backface culling, e.g.):
   if (num_border_edges > 0)
      _type |= OPEN_SURFACE;
   else if (nfaces() > 0)
      _type |= CLOSED_SURFACE;

   // XXX - may not be needed
   for (k=0; (int)k<nfaces(); k++)
      _faces[k]->orient_strip(nullptr);

   // deal with inconsistently oriented edges if needed.
   if (num_inconsistent_edges > 0) {
      err_msg("BMESH::check_type: inconsistently oriented faces found...");
      if (Config::get_var_bool("NO_FIX_ORIENTATION",false)) {
         err_msg("...leaving it as is");
      } else {
         if (fix_orientation())
            err_msg("fixed it");
         else
            err_msg("can't fix it");
      }
   }

   return _type;
}

bool
BMESH::fix_orientation()
{
   if (Config::get_var_bool("NO_FIX_ORIENTATION",false))
      return false;

   bool debug = Config::get_var_bool("DEBUG_FIX_ORIENTATION",false);

   _faces.clear_flags();

   vector<Bface*> pos_faces;
   vector<Bface*> neg_faces;
   for (Bface_list::size_type k=0; k<_faces.size(); k++) {
      if (!_faces[k]->flag()) {
         if (debug)
            err_msg("fix_orientation: starting on untouched face...");
         pos_faces.clear();
         neg_faces.clear();
         _faces[k]->set_flag(1);
         pos_faces.push_back(_faces[k]);
         grow_oriented_face_lists(_faces[k], pos_faces, neg_faces);
         reverse_faces((pos_faces.size() < neg_faces.size()) ?
                       pos_faces : neg_faces);
      }
   }
   changed(TRIANGULATION_CHANGED);

   // check consistency (e.g. surface is not orientable)
   int num_inconsistent_edges = 0;
   for (Bedge_list::size_type j=0; j<_edges.size(); j++)
      if (!_edges[j]->consistent_orientation())
         num_inconsistent_edges++;

   if (num_inconsistent_edges > 0)
      err_msg("BMESH::fix_orientation: still found %d inconsistent edges!",
              num_inconsistent_edges);

   return (num_inconsistent_edges == 0);
}

void
BMESH::grow_oriented_face_lists(
   Bface* f,
   vector<Bface*>& pos_faces,
   vector<Bface*>& neg_faces)
{
   for (int i=1; i<=3; i++) {
      Bface* nbr = f->nbr(i);
      if (nbr && !nbr->flag()) {
         nbr->set_flag(1);
         if (f->e(i)->consistent_orientation()) {
            pos_faces.push_back(nbr);
            grow_oriented_face_lists(nbr, pos_faces, neg_faces);
         } else {
            neg_faces.push_back(nbr);
            grow_oriented_face_lists(nbr, neg_faces, pos_faces);
         }
      }
   }
}

Bedge*
BMESH::nearest_edge(CWpt &p)
{
   if (_edges.empty())
      return nullptr;

   Wpt q;
   Bedge *ret = _edges[0];
   double dist = (p - _edges[0]->project_to_simplex(p, q)).length_sqrd(), d;

   for (Bedge_list::size_type i=1; i<_edges.size(); i++) {
      if ((d = (p - _edges[i]->project_to_simplex(p, q)).length_sqrd()) < dist) {
         ret = _edges[i];
         dist = d;
      }
   }

   return ret;
}

Bvert*
BMESH::nearest_vert(CWpt &p)
{
   if (_verts.empty())
      return nullptr;

   Bvert *ret = _verts[0];
   double dist = (p - _verts[0]->loc()).length_sqrd(), d;

   for (Bvert_list::size_type i=1; i<_verts.size(); i++) {
      if ((d = (p - _verts[i]->loc()).length_sqrd()) < dist) {
         ret = _verts[i];
         dist = d;
      }
   }

   return ret;
}

void
BMESH::get_enclosed_verts(
   CXYpt_list& boundary,
   Bface* startface,
   vector<Bvert*>& ret)
{
   // Do a search on the mesh to find out vertices that
   // lie inside the boundary, with the vertices of the
   // given startface as starting point.

   // start fresh
   ret.clear();

   // error-check
   if (startface->mesh().get() != this) {
      err_msg("BMESH::get_enclosed_verts: startface is from wrong mesh");
      return;
   }

   // clear all vertex flags
   for (int j=0; j<nverts(); j++)
      bv(j)->clear_flag();

   vector<Bvert*> frontier; // expandable frontier of vertices to check

   // start with on-screen vertices, since others
   // cannot be projected reliably to screen
   // coordinates:
   if (startface->v1()->ndc().in_frustum())
      frontier.push_back(startface->v1());
   if (startface->v2()->ndc().in_frustum())
      frontier.push_back(startface->v2());
   if (startface->v3()->ndc().in_frustum())
      frontier.push_back(startface->v3());

   while (!frontier.empty()) {
      Bvert* v = frontier.back();
      frontier.pop_back();

      // note: v->ndc() accessor takes into account
      // mesh transform if any
      XYpt xy = NDCpt(v->ndc());

      if (boundary.contains(xy)) {
         ret.push_back(v);
         for (int i=0 ;i<v->degree(); i++) {
            Bvert *nbr = v->nbr(i);
            if (!nbr->flag()) {
               nbr->set_flag();
               frontier.push_back(nbr);
            }
         }
      }
   }
}

int
BMESH::remove_face(Bface* f)
{
   Bface_list::iterator it;
   it = std::find(_faces.begin(), _faces.end(), f);
   _faces.erase(it);
   delete_face(f);

   return 1;
}

int
BMESH::remove_edge(Bedge* e)
{
   Bedge_list::iterator it;
   it = std::find(_edges.begin(), _edges.end(), e);
   _edges.erase(it);
   delete_edge(e);

   return 1;
}

int
BMESH::remove_vertex(Bvert* v)
{
   Bvert_list::iterator it;
   it = std::find(_verts.begin(), _verts.end(), v);
   _verts.erase(it);
   delete_vert(v);

   return 1;
}

int
BMESH::remove_faces(CBface_list& faces)
{
   if (!faces.empty() && faces.mesh().get() != this) {
      cerr <<"BMESH::remove_faces(): ERROR, faces not all from this mesh"
           << endl;
      return 0;
   }

   for (Bface_list::size_type i=0; i<faces.size(); i++) {
      remove_face(faces[i]);
   }

   return 1;
}

int
BMESH::remove_edges(CBedge_list& edges)
{
   if (!edges.empty() && edges.mesh().get() != this) {
      cerr <<"BMESH::remove_edges(): ERROR, edges not all from this mesh"
           << endl;
      return 0;
   }

   for (Bedge_list::size_type i=0; i<_edges.size(); i++) {
      remove_edge(edges[i]);
   }

   return 1;
}

int
BMESH::remove_verts(CBvert_list& verts)
{
   if (!verts.empty() && verts.mesh().get() != this) {
      cerr <<"BMESH::remove_verts(): ERROR, verts not all from this mesh"
           << endl;
      return 0;
   }

   for (Bvert_list::size_type i=0; i<verts.size(); i++) {
      remove_vertex(verts[i]);
   }

   return 1;
}

void
BMESH::merge_vertex(Bvert* v, Bvert* u, bool keep_vert)
{
   // Identify vertex v to vertex u,
   // destroy v unless keep_vert == 1.
   // then v is left as a dangling vert.

   // Get faces around v and detach them
   Bface_list star_faces(6);
   v->get_all_faces(star_faces);
   size_t k;
   for (k=0; k<star_faces.size(); k++)
      star_faces[k]->detach();

   // redefine edges coming out of v,
   // removing those that can't be redefined
   // (because they would duplicate an existing edge).
   vector<Bedge*> star_edges = v->get_adj();
   for (k=0; k<star_edges.size(); k++)
      if (!star_edges[k]->redefine(v,u))
         remove_edge(star_edges[k]);

   for (k=0; k<star_faces.size(); k++)
      if (!star_faces[k]->redefine(v,u))
         remove_face(star_faces[k]);

   // get rid of pathetic isolated vertex
   if (!keep_vert)
      remove_vertex(v);
}


int
compare_locs_lexicographically(const Bvert *va, const Bvert *vb)
{
   // used in BMESH::remove_duplicate_vertices() below

   CWpt& a = va->loc();
   CWpt& b = vb->loc();

   return ((a[0] > b[0]) ?  1 :
           (a[0] < b[0]) ? -1 :
           (a[1] > b[1]) ?  1 :
           (a[1] < b[1]) ? -1 :
           (a[2] > b[2]) ?  1 :
           (a[2] < b[2]) ? -1 :
           0);
}

void
BMESH::remove_duplicate_vertices(bool keep_verts)
{
   // zero-length edges cause problems -- remove them and their
   // adjacent zero-area faces.  NOTE: because of the way vector
   // deletion works, we have to work backwards.
   for (int j=nedges()-1; j>=0; j--) {
      if (_edges[j]->length() == 0)
         remove_edge(_edges[j]);
   }

   // we need to process vertices in sorted order, but
   // removing vertices has the side effect
   // of re-shuffling the order of the _verts array.
   //
   // so we make a temporary copy of the _verts array,
   // which will not be reshuffled, and operate on that:
   Bvert_list verts = _verts;

   // sort verts by increasing (x,y,z) order:
   std::sort(verts.begin(), verts.end(), compare_locs_lexicographically);

   // remove duplicate vertices, which now lie
   // next to each other in the array:
   int count = 0;
   Bvert* prev = verts[0];
   for (Bvert_list::size_type k=1; k<verts.size(); k++) {
      if (verts[k]->loc() == prev->loc()) {
         merge_vertex(verts[k], prev, keep_verts);
         count++;
      } else
         prev = verts[k];
   }

   if (count > 0) {
      // if anything happened, invalidate cached data:
      changed(TRIANGULATION_CHANGED);

      // this mesh was effed up -- better fix face normals
      // to consistent orientation or else the triangle
      // stripping code will crash:

      int fixed = fix_orientation()
         ;

      // tell the humans:
      err_msg("BMESH::remove_duplicate_vertices:");
      err_msg("       removed %d verts", count);
      err_msg("       %s orientation", fixed ? "fixed" : "warning: can't fix")
         ;
   }
}


Bvert*
BMESH::split_edge(Bedge* edge, CWpt &p)
{
   /*
   //            c
   //         /  | \
   //       /    |   \
   //      d  f1 v f2  b
   //       \    |    /
   //        \   |   /
   //          \ | /
   //            a
   */

   Bface* f1 = ccw_face(edge);
   Bface* f2 = edge->other_face(f1);

   Bvert* a = edge->v1();
   Bvert* c = edge->v2();

   // preserve UV coords, if any
   UVdata* uvd1 = UVdata::lookup(f1);
   UVdata* uvd2 = UVdata::lookup(f2);

   UVpt uv1a, uv1c, uv1d;
   UVpt uv2a, uv2b, uv2c;

   if (uvd1) {
      uv1a = uvd1->uv(a);
      uv1c = uvd1->uv(c);
      uv1d = uvd1->uv(f1->other_vertex(a,c));
   }
   if (uvd2) {
      uv2a = uvd2->uv(a);
      uv2b = uvd2->uv(f2->other_vertex(a,c));
      uv2c = uvd2->uv(c);
   }

   // detach faces
   if (f1)
      f1->detach();
   if (f2)
      f2->detach();

   // allocate new vertex
   Bvert* v = add_vertex(p);

   // propagate tess data
   // from edge to new vertex:
   edge->notify_split(v);

   // redefine edge, replacing a with v:
   edge->redefine(a,v);

   // create new edge from v to a:
   Bedge* av = add_edge(v,a);

   // propagate tess data from
   // old to new edge:
   edge->notify_split(av);

   if (f1) {
      Bvert* d = f1->other_vertex(a,c);

      // add edge from d to v, and propagate
      // tess data from f1 to new edge:
      f1->notify_split(add_edge(v,d));

      // redefine f1, replacing a with v:
      f1->redefine(a,v);

      // create new face joining v,d,a
      Bface* avd = add_face(v,d,a,f1->patch());

      // propagate tess data:
      f1->notify_split(avd);

      if (uvd1) {
         UVpt uv1v = (uv1a + uv1c)/2;
         UVdata::set
            (f1,  v, c, d, uv1v, uv1c, uv1d);
         UVdata::set
            (avd, v, d, a, uv1v, uv1d, uv1a);
      }
   }
   if (f2) {
      Bvert* b = f2->other_vertex(a,c);
      f2->notify_split(add_edge(v,b));
      f2->redefine(a,v);
      Bface* abv = add_face(v,a,b,f2->patch());
      f2->notify_split(abv);

      if (uvd2) {
         UVpt uv2v = (uv2a + uv2c)/2;
         UVdata::set
            (f2,  v, b, c, uv2v, uv2b, uv2c);
         UVdata::set
            (abv, v, a, b, uv2v, uv2a, uv2b);
      }
   }

   return v;
}

Bvert*
BMESH::split_face(Bface *face, CWpt &pos)
{
   /*
   //                    
   //                   c                              
   //                 /   \                            
   //               /      \                           
   //             /         \                          
   //           /      p     \                         
   //         /               \
   //       /                  \
   //      a ------------------ b                     
   //                                                  
   //    face is (a,b,c) is ccw order
   */

   Bvert* a = face->v1();
   Bvert* b = face->v2();
   Bvert* c = face->v3();

   // keep uv data if it exists
   UVpt uva, uvb, uvc, uvp;
   UVdata* uvd = UVdata::lookup(face);
   if (face) {
      uva = uvd->uv1();
      uvb = uvd->uv2();
      uvc = uvd->uv3();
      Wvec bc;
      face->project_barycentric(pos, bc);
      uvd->bc2uv(bc, uvp);
   }
   face->detach();

   Bvert *v = add_vertex(pos);
   Bedge *av = add_edge(a,v);
   Bedge *bv = add_edge(b,v);
   Bedge *cv = add_edge(c,v);

   face->redefine(c,v);

   Bface *bcv = add_face(b,c,v,face->patch());
   Bface *cav = add_face(c,a,v,face->patch());

   if (uvd) {
      UVdata::set
         (face, uva, uvb, uvp);
      UVdata::set
         (bcv,  uvb, uvc, uvp);
      UVdata::set
         (cav,  uvc, uva, uvp);
   }
   face->notify_split(v);
   face->notify_split(av);
   face->notify_split(bv);
   face->notify_split(cv);
   face->notify_split(bcv);
   face->notify_split(cav);

   return v;
}


int
BMESH::split_faces(
   CXYpt_list &pts,
   vector<Bvert *> &verts,
   Bface* start_face)
{
   // preconditions:
   // the first point intersects with this mesh
   // there is at least a single point

   // if the first and last point are equal, and there is
   // more than two points, then treat pts as a closed curve.
   int closed = pts.size() > 2 && pts[0] == pts.back();

   cerr << (closed? " It's closed " : "Open!")<<endl;
   verts.clear();

   if (pts.empty()) {
      return 0;
   }

   Bface* cur=nullptr;
   Wpt cur_pt;
   if (!start_face) {
      BaseVisRefImage *vis_ref = BaseVisRefImage::lookup(VIEW::peek());
      if (!vis_ref) {
         err_msg("BMESH::split_faces: error: can't get BaseVisRefImage");
         return 0;
      }
      vis_ref->vis_update();
      cur = vis_ref->vis_intersect(pts[0], cur_pt);
   } else {
      cur = start_face;
      cur_pt = cur->plane_intersect(inv_xform() * Wline(pts[0]));
   }

   if (!cur || cur->mesh().get() != this) {
      return 0;
   }

   verts.push_back(split_face(cur, cur_pt));
   NDCpt new_ndc;

   Bface_list star_faces(16);

   for (CXYpt_list::size_type i=1; i < pts.size(); ) {
      // walk from prev point to this point, splitting edges and faces along
      // the way, accumulating new vertices.

      int success = 0;
      verts.back()->get_faces(star_faces);
      NDCpt last_ndc = xform() * verts.back()->loc();

      if (closed && i==pts.size()-1) {
         cerr << "In closed search" <<endl;
         // see if there is an adjacent vertex at the final position
         // already.  if so, declare success and we're done.
         for (int j=0; j < verts.back()->degree(); j++) {
            if (verts.back()->nbr(j) == verts[0]) {
               i++;
               success = 1;
               Bvert* v= verts[0];
               verts.push_back(v);
               break;
            }
         }
         if (!success)
            cerr << "BMESH::split_faces: Closed SearchFailed" << endl;
         else {
            cerr << "BMESH::split_faces: Closing correctly" << endl;
            assert(verts[0]==verts.back());
         }
      }

      // As the first vertex may not be immediately adjacent to this vertex
      // so the closing can possibly fail but succeed in subsequent triangles

      if (!success) {
         for (Bface_list::size_type j=0; j<star_faces.size(); j++) {
            if (star_faces[j]->ndc_contains(pts[i]) ) {
               if (i==pts.size()-1)
                  cerr << "BMESH::split_faces: Why here" << endl;
               Wpt new_pt = star_faces[j]->plane_intersect(
                  inv_xform() * Wline(pts[i]));
               verts.push_back(split_face(star_faces[j], new_pt));
               cerr << "BMESH::split_faces: Split face insert "
                    << verts.size()
                    << endl;
               i++;

               success = 1;
               break;

            } else if (star_faces[j]->opposite_edge(verts.back())->
                       ndc_intersect(last_ndc, pts[i], new_ndc)) {

               if (i==pts.size()-1)
                  cerr << "Kept going" << endl;
               Wpt new_pt = star_faces[j]->plane_intersect(
                  inv_xform() * Wline(new_ndc));
               verts.push_back(split_edge(
                  star_faces[j]->opposite_edge(verts.back()),new_pt));

               cerr << "BMESH::split_faces: Split edge insert "
                    << verts.size()
                    << endl;
               success = 1;
               break;
            }
         }
      }

      if (!success) {
         cerr << "i fail " << i << endl;
         cerr << "pts.num() " << pts.size() <<endl;
         return 0;
      }
   }

   cerr << "BMESH::Split_faces :: Success.. returning "
        << verts.size()
        << " vertices"
        << endl;
   changed(TRIANGULATION_CHANGED);
   return 1;
}

int
BMESH::try_swap_edge(Bedge* edge, bool favor)
{
   /*
   //                  c
   //                / | \
   //              /   |   \
   //            /     |     \
   //          /       |       \
   //        d    f1  /|\   f2   b
   //          \       |       /
   //            \     |     /
   //              \   |   /
   //                \ | /
   //                  a
   //
   //         edge goes from a to c.
   //  "swapping" the edge means deleting it
   //   and putting in an edge from d to b.
   //  an edge is "swapable" if the operation
   //  is both legal (no change to topology)
   //   and preferable (e.g. it leads to a
   //           bigger minimum angle).
   */

   Bface *f1=nullptr, *f2=nullptr;
   Bvert *a=nullptr, *b=nullptr, *c=nullptr, *d=nullptr;
   if (!edge->swapable(f1,f2,a,b,c,d, favor))
      return 0;

   // do it
   f1->detach();
   f2->detach();

   // disconnect edge from its vertices
   edge->set_new_vertices(d,b);

   f1->redefine(a,b);
   f2->redefine(c,d);

   return 1;
}

int
BMESH::try_collapse_edge(Bedge* e, Bvert* v)
{
   // vertex v will be removed
   // vertex u will absorb its adjacent edges and faces
   Bvert *u = nullptr;

   if (v) {
      // caller wants v to be destroyed
      u = e->other_vertex(v);
      assert(u);
   } else {
      // caller doesn't care which vertex is destroyed
      v = e->v(1);
      u = e->v(2);
   }

   // see hoppe et al (SIG93): can't proceed if:
   //    1) fewer than 5 vertices in mesh
   //    2) both verts are border verts, but edge isn't border edge
   //    3) there exists a vertex adjacent to both endpoints of the edge,
   //       and this vertex is not in either face adjacent to the edge.

   //  case 1)
   if (nverts()<5)
      return 0;

   // case 2)
   static bool debug = Config::get_var_bool("SKIN_DEBUG",false);
   if (v->is_border() && u->is_border() && !e->is_border()) {
      if (debug)
         cerr << "BMESH::try_collapse_edge: Case 2 failure" <<endl;
      return 0;
   }

   // case 3) - only matters if there is a face
   if (e->f1() || e->f2()) {
      Bvert* op1 = e->opposite_vert1();
      Bvert* op2 = e->opposite_vert2();
      for (int k=v->degree()-1; k>=0; k--) {
         Bvert* vnbr = v->nbr(k);
         if (vnbr != u && u->is_adjacent(vnbr) && vnbr != op1 && vnbr != op2) {
            if (debug)
               cerr << "BMESH::try_collapse_edge: Case 3 failure" <<endl;
            return 0;
         }
      }
   }

   // do it
   merge_vertex(v, u);

   return 1;
}


Patch*
BMESH::new_patch()
{
   _patches += new Patch(shared_from_this());
   if (subdiv_level() == 0)
      _drawables += _patches.last();

   return _patches.last();
}

bool
BMESH::unlist(Patch* p)
{
   // Remove the Patch from the patch list; returns 1 on success

   if (!(p && p->mesh() && p->mesh().get() == this))
      return false;
   return _patches -= p;
}

void
BMESH::changed(change_t change)
{
   if (change == NO_CHANGE)
      return;

   if (change == PATCHES_CHANGED) {

      // XXX -- obsolete -- fix this

      // assume this means that some triangles have
      // been re-labeled (re: which patch they are in).
      // this means line strips are invalid,
      // but bounding box is okay.

      // mark sil strips invalid:
      _sil_stamp = 0;
      _zx_stamp = 0;

      _patch_blend_weights_valid = false;
      // todo: re-do patch boundaries...
   }

   switch (change) {
      // cases arranged in order of decreasing severity.
      // breaks are not used on purpose
    case TOPOLOGY_CHANGED:

      _type_valid = 0;

    case TRIANGULATION_CHANGED:

      // mark various edge strips invalid:
      _sil_stamp = 0;
      _sils.reset();

      _zx_sils.reset();
      _zx_stamp = 0;

      delete _borders;
      _borders = nullptr;

      delete _creases;
      _creases = nullptr;

      _patches.triangulation_changed();

      _patch_blend_weights_valid = false;

    case VERT_POSITIONS_CHANGED:

      //XXX (rdk) - I added this... cool?
      // XXX (lem):
      //  This means randomized zx sil detection
      //  is turned OFF for deforming meshes.
      _zx_stamp = 0;

      // invalidate BODY stuff:
      _vert_locs.clear();
      BODY::_edges.reset();
      _body = BODYptr(nullptr);
      _bb.reset();
      _avg_edge_len_valid = 0;
      
      // invalidate curvature data:
      delete _curv_data;
      _curv_data = nullptr;
      
      break;

    case CREASES_CHANGED:
      // Invalidate crease strips in patches:
      _patches.creases_changed();
      delete _creases;
      _creases = nullptr;

    default:
      ;
   }

   BMESHobs::broadcast_change(shared_from_this(), change);

   // Invalidate display lists:
   _version++;
}

const BBOX &
BMESH::get_bb()
{
   if (_bb.valid())
      return _bb;

   for (int i = 0; i< nverts(); i++) {
      _bb.update(bv(i)->loc());
   }
   return _bb;
}

CTAGlist &
BMESH::tags() const
{
   // Tags used for MESHes loaded/saved during animations.
   // (e.g. only the vertex positions change)
   if  ( (IOManager::state() == IOManager::STATE_PARTIAL_LOAD) ||
         (IOManager::state() == IOManager::STATE_PARTIAL_SAVE) ) {
      if (!_bmesh_update_tags) {
         _bmesh_update_tags  = new TAGlist(BODY::tags());

         _bmesh_update_tags->push_back(new TAG_meth<BMESH>(
            "vertices", &BMESH::put_vertices, &BMESH::get_vertices, 1
            ));
      }

      return *_bmesh_update_tags;
   }

   // Full set of tags used in all other circumstances...
   if (!_bmesh_tags) {
      _bmesh_tags  = new TAGlist(BODY::tags());

      // XXX - vertices & faces must be written/read first
      _bmesh_tags->push_back(new TAG_meth<BMESH>(
         "vertices", &BMESH::put_vertices, &BMESH::get_vertices, 1
         ));
      _bmesh_tags->push_back(new TAG_meth<BMESH>(
         "faces", &BMESH::put_faces, &BMESH::get_faces, 1
         ));
      _bmesh_tags->push_back(new TAG_meth<BMESH>(
         "uvfaces", &BMESH::put_uvfaces, &BMESH::get_uvfaces, 1
         ));
      _bmesh_tags->push_back(new TAG_meth<BMESH>(
         "creases", &BMESH::put_creases, &BMESH::get_creases, 1
         ));
      _bmesh_tags->push_back(new TAG_meth<BMESH>(
         "polylines", &BMESH::put_polylines, &BMESH::get_polylines, 1
         ));
      _bmesh_tags->push_back(new TAG_meth<BMESH>(
         "weak_edges", &BMESH::put_weak_edges, &BMESH::get_weak_edges, 1
         ));
      _bmesh_tags->push_back(new TAG_meth<BMESH>(
         "secondary_faces", &BMESH::put_sec_faces, &BMESH::get_sec_faces, 1
         ));
      _bmesh_tags->push_back(new TAG_meth<BMESH>(
         "colors", &BMESH::put_colors, &BMESH::get_colors, 1
         ));
      _bmesh_tags->push_back(new TAG_meth<BMESH>(
         "texcoords2", &BMESH::put_texcoords2, &BMESH::get_texcoords2, 1
         ));
      _bmesh_tags->push_back(new TAG_meth<BMESH>(
         "patch", &BMESH::put_patches, &BMESH::get_patches, 1
         ));
      _bmesh_tags->push_back(new TAG_meth<BMESH>(
         "render_style", &BMESH::put_render_style, &BMESH::get_render_style, 1
         ));

      // SHADOW TAGS
      _bmesh_tags->push_back(new TAG_val<BMESH,bool>("Shadow_Occluder",&BMESH::occluder));

      _bmesh_tags->push_back(new TAG_val<BMESH,bool>("Shadow_Reciever",&BMESH::reciever));

      _bmesh_tags->push_back(new TAG_val<BMESH,double>(
         "Shadow_Scale",&BMESH::shadow_scale
         ));

      _bmesh_tags->push_back(new TAG_val<BMESH,double>(
         "Shadow_Softness",&BMESH::shadow_softness
         ));

      _bmesh_tags->push_back(new TAG_val<BMESH,double>(
         "Shadow_Offset",&BMESH::shadow_offset
         ));
   }

   return *_bmesh_tags;
}

void
BMESH::put_vertices(TAGformat &d) const
{
   Wpt_list verts(nverts());

   if (Config::get_var_bool("JOT_SAVE_XFORMED_MESH",false))
      for (int i = 0; i< nverts(); i++)
         verts[i] = bv(i)->wloc();
   else
      for (int i = 0; i< nverts(); i++)
         verts[i] = bv(i)->loc();

   // XXX - copied code from net_type.H,
   //       want to avoid super long lines that may be
   //       causing bugs...
   d.id();
   (*d) << "{";
   for (Wpt_list::size_type i=0; i<verts.size();i++) {
      (*d) << " " << verts[i];
      (*d).write_newline();
   }
   (*d) << "}";
   (*d).write_newline();
   d.end_id();
}

void
BMESH::put_faces(TAGformat &d) const
{
   vector<vector<int> > faces(nfaces());
   
   for (int i=0; i<nfaces(); i++) {
      vector<int> face;
      Bface *f = bf(i);
      if (use_new_bface_io) {
         // Faces with uv coords will be written out in
         // BMESH::put_uvfaces(), so we can skip writing them here.
         if (UVdata::has_uv(f))
            continue;
         if (!f->is_quad()) {
            // Write an ordinary triangle
            face.push_back(f->v1()->index());
            face.push_back(f->v2()->index());
            face.push_back(f->v3()->index());
            faces[i] = face;
         } else if (f->quad_rep() == f) {
            // Write a quad:
            Bvert *a=nullptr, *b=nullptr, *c=nullptr, *d=nullptr;
            f->get_quad_verts(a,b,c,d);
            assert(a && b && c && d);
            face.push_back(a->index());
            face.push_back(b->index());
            face.push_back(c->index());
            face.push_back(d->index());
            faces[i] = face;
         }
      } else {
         // Old I/O: write a plain triangle
         face.push_back(f->v1()->index());
         face.push_back(f->v2()->index());
         face.push_back(f->v3()->index());
         faces[i] = face;
      }
   }

   // XXX - copied code from net_type.H,
   //       want to avoid super long lines that may be
   //       causing bugs...
   d.id();
   (*d) << "{";
   for (auto & face : faces) {
      (*d) << " " << face;
      (*d).write_newline();
   }
   (*d) << "}";
   (*d).write_newline();
   d.end_id();
}

class UVforIO2
{
 public:
   vector<int>  _face;   // indices of vertices making up the triangle or quad
   vector<UVpt> _uvs;    // corresponding UV coords

   UVforIO2() {}
   UVforIO2(CBface* f)
      {
         if (!(f && UVdata::has_uv(f)))
            return;
         if (f->is_quad()) {
            // Only write out the quad rep
            // (i.e. write out the quad just once, not twice)
            if (!f->is_quad_rep())
               return;
            Bvert *a=nullptr, *b=nullptr, *c=nullptr, *d=nullptr;
            UVpt ua, ub, uc, ud;
            if (f->get_quad_verts(a,b,c,d) && UVdata::get_quad_uvs(a,b,c,d,ua,ub,uc,ud)) {
               assert(a && b && c && d);
               _face.push_back(a->index());
               _face.push_back(b->index());
               _face.push_back(c->index());
               _face.push_back(d->index());
               _uvs.push_back(ua);
               _uvs.push_back(ub);
               _uvs.push_back(uc);
               _uvs.push_back(ud);
            } else {
               // could just as well assert(0)
               err_msg("UVforIO2::UVforIO2: error getting quad uv's; skipping...");
            }
         } else {
            UVpt ua, ub, uc;
            if (UVdata::get_uvs(f, ua, ub, uc)) {
               _face.push_back(f->v1()->index());
               _face.push_back(f->v2()->index());
               _face.push_back(f->v3()->index());
               _uvs.push_back(ua);
               _uvs.push_back(ub);
               _uvs.push_back(uc);
            } else {
               // could just as well assert(0)
               err_msg("UVforIO2::UVforIO2: error getting triangle uv's; skipping...");
            }
         }
      }

   int  num()     const { return _face.size(); }
   bool is_good() const { return (num() == 3 || num() == 4) && (num() == (int)_uvs.size()); }

   // Needed by vector<UVforIO2>::get_index():
   bool operator==(const UVforIO2 &u) const
   {
      // Use operator==(vector<T>, vector<T>)
      return (_face == u._face && _uvs == u._uvs);
   }
};

inline STDdstream&
operator<<(STDdstream &ds, const UVforIO2& v)
{
   if (v.is_good()) {
      ds << "{" << v._face << v._uvs << "}";
   }
   return ds;
}

inline STDdstream&
operator>>(STDdstream &ds, UVforIO2 &v)
{
   char brace;
   ds >> brace >> v._face >> v._uvs >> brace;
   return ds;
}


void
BMESH::put_uvfaces(TAGformat &d) const
{
   // Writes out a record for each quad or triangle with uv coords.
   // When this is used, BMESH::put_faces() can skip writing out
   // faces that have assigned uv coords.

   // Don't write the new format unless requested
   if (!use_new_bface_io)
      return;

   vector<UVforIO2> uvs; uvs.reserve(nfaces());

   for (int i=0; i<nfaces(); i++) {
      UVforIO2 uv(bf(i));
      if (uv.is_good())
         uvs.push_back(uv);
   }

   if (!uvs.empty()) {
      d.id();
      *d << uvs;
      d.end_id();
   }
}

void
BMESH::get_uvfaces(TAGformat &d)
{
   vector<UVforIO2> uvs;
   *d >> uvs;

   if (uvs.empty())
      return;

   Patch* p = nullptr;
   for (const UVforIO2& uv : uvs) {
      if (!uv.is_good()) {
         err_msg("BMESH::get_uvfaces: skipping bad face:");

         iostream i(cerr.rdbuf());
         STDdstream e(&i);
         e << uv << "\n";
      }
      switch(uv.num()) {
       case 3:
         // triangle
         add_face(uv._face[0], uv._face[1], uv._face[2],
                  uv._uvs[0],  uv._uvs[1],  uv._uvs[2], p);
         break;
       case 4:
         // quad
         add_quad(uv._face[0], uv._face[1], uv._face[2], uv._face[3],
                  uv._uvs[0],  uv._uvs[1],  uv._uvs[2],  uv._uvs[3], p);
         break;
       default:
         assert(0);
      }
   }

   changed(TOPOLOGY_CHANGED);
}

void
BMESH::put_creases(TAGformat &d) const
{
   vector<Point2i> creases;

   for (int i=0; i<nedges(); i++) {
      Bedge *e = be(i);
      if (e->is_crease())
         creases.push_back(Point2i(e->v(1)->index(), e->v(2)->index()));
   }

   // No tag if no creases!
   if (creases.empty())
      return;

   d.id();
   *d << creases;
   d.end_id();
}


void
BMESH::put_polylines(TAGformat &d) const
{
   vector<Point2i> polylines;

   for (int i=0; i<nedges(); i++) {
      Bedge *e = be(i);
      if (e->is_polyline())
         polylines.push_back(Point2i(e->v(1)->index(), e->v(2)->index()));
   }

   //No tag if no creases!
   if (polylines.empty())
      return;

   d.id();
   *d << polylines;
   d.end_id();

}

void
BMESH::put_weak_edges(TAGformat &d) const
{
   if (use_new_bface_io)
      return;

   vector<Point2i> weak_edges;

   for (int i=0; i<nedges(); i++) {
      Bedge *e = be(i);
      if (e->is_weak())
         weak_edges.push_back(Point2i(e->v(1)->index(), e->v(2)->index()));
   }

   if (weak_edges.empty())
      return;

   d.id();
   *d << weak_edges;
   d.end_id();
}

void
BMESH::put_sec_faces(TAGformat &d) const
{
   vector<Point3i> sec_faces;

   for (int i=0; i<nfaces(); i++) {
      Bface *f = bf(i);
      if (f && f->is_secondary())
         sec_faces.push_back(Point3i(f->v1()->index(), f->v2()->index(), f->v3()->index()));
   }

   if (sec_faces.empty())
      return;

   d.id();
   *d << sec_faces;
   d.end_id();
}


class UVforIO
{
 public:
   int   _face;
   UVpt  _uv1;
   UVpt  _uv2;
   UVpt  _uv3;

   UVforIO() {}
   UVforIO(int f, CUVpt& uv1, CUVpt& uv2, CUVpt& uv3) : _face(f), _uv1(uv1), _uv2(uv2), _uv3(uv3) {}

   UVpt& uv(int i) {if (i==1)
         return _uv1; else if (i==2)
                       return _uv2; else { assert(i==3); return _uv3;} }

   bool operator ==(const UVforIO &p)   const
      { return _face==p._face && _uv1==p._uv1 && _uv2==p._uv2 && _uv3==p._uv3; }

};

inline STDdstream &operator<<(STDdstream &ds, const UVforIO  &v)
{
   ds << "{" << v._face << v._uv1 << v._uv2 << v._uv3 << "}";
   return ds;
}

inline STDdstream &operator>>(STDdstream &ds, UVforIO &v)
{
   char brace;
   ds >> brace >> v._face >> v._uv1 >> v._uv2 >> v._uv3 >> brace;
   return ds;
}

void
BMESH::put_texcoords2(TAGformat &d) const
{
   // Deprecated, if using BMESH::put_uvfaces()
   if (use_new_bface_io)
      return;

   vector<UVforIO> texcoords2;

   for (int i=0; i<nfaces(); i++) {
      UVdata* uvdata = UVdata::lookup(bf(i));
      if (uvdata)
         texcoords2.push_back(UVforIO(i, uvdata->uv(1), uvdata->uv(2), uvdata->uv(3)));
   }

   // No tag if no texcoords!
   if (texcoords2.empty())
      return;

   d.id();
   *d << texcoords2;
   d.end_id();
}


void
BMESH::put_render_style(TAGformat &d) const
{
   /* XXX - Saving this makes the texture menu ineffectual...
      string style;
    
      if (_render_style.num())
      // XXX - What does this mean?!?!?!?
      // this ain't right, but shifting out the vector _render_style
      // doesn't get decoded on the other side right because individual
      // elements aren't separatable.
      style = _render_style.back();
      else
      style = VIEW::peek()->rendering();
    
      d.id();
      *d << style;
      d.end_id();
   */
}

void
BMESH::put_colors(TAGformat &d) const
{
   // Assume if the first has color that they all do (bad assumption, probably)
   if ((nverts()==0) || (!_verts[0]->has_color()))
      return;

   vector<COLOR> colors(nverts());

   for (int i=0; i<nverts(); i++)
      colors[i] = bv(i)->color();

   d.id();
   *d << colors;
   d.end_id();
}

void
BMESH::put_patches(TAGformat &d) const
{
   // Ideally should skip writing a single patch with only
   // standard Gtextures; for now skipping single patches
   // with no Gtextures.
   if (_patches.num() == 1 && _patches[0]->gtextures().empty())
      return;

   for (int i=0;i<_patches.num();i++) {
      d.id();
      _patches[i]->format(*d);
      d.end_id();
   }
}


void
BMESH::get_vertices(TAGformat &d)
{
   // XXX - Should this be in BMESH::decode()?
   // NO! Let the caller of decode (or load file) do this...
   // delete_elements();

   // Read vertices
   Wpt_list locs;
   *d >> locs;

   // If verts already exist...
   if (nverts()) {
      if ((int)locs.size() == nverts()) {
         for (Wpt_list::size_type i=0; i < locs.size(); i++)
            _verts[i]->set_loc(locs[i]);
         changed(VERT_POSITIONS_CHANGED);
      } else {
         err_msg(
            "BMESH::get_vertices() - Error! Num verts in mesh (%d) doesn't match num in update (%d)!\n",
            nverts(), locs.size());
      }
   } else {
      _verts.reserve(locs.size());
      _edges.reserve(3*locs.size());

      for (Wpt_list::size_type i=0; i < locs.size(); i++) {
         // Bverts might be derived type,
         // so call this virtual method:
         Bvert* v = new_vert(locs[i]);
         add_vertex(v);
      }

      // XXX - Presumably there will be a matching
      // 'faces' and we'll call this there...
      //changed(TOPOLOGY_CHANGED);
   }
}

void
BMESH::get_faces(TAGformat &d)
{
   assert(nfaces() == 0);

   vector<vector<int> > faces;
   *d >> faces;

   _faces.reserve(faces.size());

   // Create default patch here and put all faces into it.
   // Later if Patches are specified in the file, faces will
   // be re-sorted into their correct patches and this default
   // patch will be removed:
   Patch* p = nullptr;  //new_patch();
   for (const vector<int>& face : faces) {
      switch (face.size()) {
       case 3:
         // triangle
         add_face(face[0], face[1], face[2], p);
         break;
       case 4:
         // quad
         add_quad(face[0], face[1], face[2], face[3], p);
         break;

       default: {
          err_msg("BMESH::get_faces: error: %d-gon found, skipping...", face.size());

          //XXX - This blows up on WIN32
          //STDdstream e((iostream*)&cerr);
          //XXX - This requires yet another steam class header
          //stdiostream s(stderr);  STDdstream e(&s);
          //Let's try this...
          iostream i(cerr.rdbuf());
          STDdstream e(&i);
          e << face << "\n";
       }
      }
   }

   changed(TOPOLOGY_CHANGED);
}

void
BMESH::get_creases(TAGformat &d)
{
   vector<Point2i> creases;
   *d >> creases;

   for (auto & crease : creases)
      set_crease(crease[0], crease[1]);

   changed(CREASES_CHANGED);
}

void
BMESH::get_polylines(TAGformat &d)
{
   vector<Point2i> polylines;
   *d >> polylines;

   for (auto & polyline : polylines)
      add_edge(polyline[0], polyline[1]);

   if (Config::get_var_bool("BMESH_BUILD_POLYSTRIPS",false) &&
       polylines.size() > 0)
      build_polyline_strips();
}

void
BMESH::get_weak_edges(TAGformat &d)
{
   vector<Point2i> weak_edges;
   *d >> weak_edges;

   bool debug = Config::get_var_bool("DEBUG_WEAK_EDGES",false);
   err_adv(debug, "BMESH::get_weak_edges: reading %d weak edges",
           weak_edges.size());

   int count=0;
   for (auto & weak_edge : weak_edges)
      if (set_weak_edge(weak_edge[0], weak_edge[1]))
         count++;
   err_adv(debug, "set %d/%d weak edges", count, weak_edges.size());
}

void
BMESH::get_sec_faces(TAGformat &d)
{
   vector<Point3i>::size_type i;
   vector<Point3i> sec_faces;

   *d >> sec_faces;

   Bface_list sec_faces_list;

   for (i=0; i<sec_faces.size(); i++) {
      Bface* face = lookup_face(sec_faces[i]);

      if (!face)
         cerr << "BMESH::get_sec_faces - error: could not find secondary face"
              << endl;
      else
         sec_faces_list.push_back(face);
   }

   if (sec_faces_list.empty())
      return;

   sec_faces_list.push_layer();

   for (i=0; (int)i<nedges(); i++)
      be(i)->fix_multi();
}


void
BMESH::get_texcoords2(TAGformat &d)
{
   static double UV_RESOLUTION = Config::get_var_dbl("UV_RESOLUTION",0,true);

   vector<UVforIO> texcoords2;
   *d >> texcoords2;

   for (auto & coord : texcoords2) {
      if (0 <= coord._face && coord._face < (int)_faces.size()) {
         // Round UV's to nearest UV_RESOLUTION
         // A value of 0.00001 is nice...
         if (UV_RESOLUTION>0) {
            for (int j=1; j<4; j++) {
               UVpt &uv = coord.uv(j);
               double r;
               uv[0] -= r = fmod(uv[0],UV_RESOLUTION);
               if (2.0*fabs(r) > UV_RESOLUTION)
                  uv[0] += ((r>0)?(UV_RESOLUTION):(-UV_RESOLUTION));
               uv[1] -= r = fmod(uv[1],UV_RESOLUTION);
               if (2.0*fabs(r) > UV_RESOLUTION)
                  uv[1] += ((r>0)?(UV_RESOLUTION):(-UV_RESOLUTION));
            }
         }

         UVdata::set
            (bf(coord._face),
             coord._uv1,
             coord._uv2,
             coord._uv3);
      } else {
         err_msg("BMESH::read_texcoords2() -  ERROR! Invalid face index %d",
                 coord._face);
      }
   }

   // N'existe pas...
   // changed(UVDATA_CHANGED);
}

void
BMESH::get_render_style(TAGformat &d)
{
   //XXX - Ignore this tag now -- see note in put_render_style

   // Get render style string, but the string may have spaces in it, which
   // makes things non-trivial
   string style = (*d).get_string_with_spaces();

   //   _render_style.clear();
   //   _render_style.push_back(style);

   //   changed(RENDERING_CHANGED);  // XXX - ?
}

void
BMESH::get_colors(TAGformat &d)
{
   // Read vertices
   vector<COLOR> colors;
   *d >> colors;

   if ((int)colors.size() != nverts()) {
      err_msg("BMESH::get_colors: warning: %d colors for %d verts",
              colors.size(), nverts());
      err_msg("   rejecting vert colors");
   } else {
      for (vector<COLOR>::size_type i=0; i < colors.size(); i++)
         _verts[i]->set_color(colors[i]);
      changed(VERT_COLORS_CHANGED);
   }
}

void
BMESH::get_patches(TAGformat &d)
{
   string patchname;
   *d >> patchname;

   //XXX - Check class name...

   Patch *patch = new_patch();

   patch->decode(*d);

   changed(PATCHES_CHANGED);
}

//We might put stuff in format/decode to cleanup
//things before/after actually doing IO

STDdstream  &
BMESH::format(STDdstream &ds) const
{
   STDdstream  &ret =  BODY::format(ds);

   return ret;
}

STDdstream  &
BMESH::decode(STDdstream &ds)
{

   STDdstream  &ret =  BODY::decode(ds);

   make_patch_if_needed();

   //clean_patches();

   // now the topological type is determined; if there are lone
   // vertices store them in vertex strips.
   if (is_points() && Config::get_var_bool("BMESH_BUILD_VERT_STRIPS",false))
      build_vert_strips();

   return ret;
}

void
BMESH::recenter()
{
   if (nverts() < 1) {
      err_msg("BMESH::recenter: mesh is empty");
      return;
   }

   static Wpt_list pts(8);
   if (!(get_bb().points(pts))) {
      err_msg("BMESH::recenter: can't get bounding box");
      return;
   }
   Wpt c = get_bb().center();

   double s = 0;
   for (int i=0; i<8; i++)
      s = max(s, pts[i].dist(c));

   if (s<1e-12) {
      err_msg("BMESH::recenter: mesh is too tiny");
      return;
   }

   double len = Config::get_var_dbl("JOT_REDENTER_LEN", 15);
   MOD::tick();
   transform(Wtransf::scaling(Wpt::Origin(), len/s) *
             Wtransf::translation(Wpt::Origin() - get_bb().center()), MOD());

   changed(VERT_POSITIONS_CHANGED);
}

double
BMESH::avg_len() const
{
   if (!_avg_edge_len_valid) {
      double ret = 0;
      for (Bedge_list::size_type k=0; k<_edges.size(); k++)
         ret += _edges[k]->length();
      if (!_edges.empty())
         ret /= _edges.size();
      // FIXME: const
      const_cast<BMESH*>(this)->_avg_edge_len_valid = 1;
      const_cast<BMESH*>(this)->_avg_edge_len = ret;
   }
   return _avg_edge_len;
}

BMESHptr
BMESH::merge(BMESHptr m1, BMESHptr m2)
{
   // merge the two meshes together if it's legal to do so. the result
   // is that one mesh ends up an empty husk, with its elements and
   // patches sucked into the other mesh. the smaller mesh is the one
   // that gets sucked dry. if the meshes are different types the
   // operation fails and a null pointer is returned. (in that case
   // the meshes are not changed).

   // this is the public method that handles error checking, then the
   // real work is done in the virtual method merge() that handles
   // anything specific to the derived mesh types being used.

   // error checking:
   if (!m1 && !m2) {
      err_msg("BMESH::merge: warning: both meshes are null.");
      return BMESHptr(nullptr);
   } else if (!m1) {
      return m2;        // don't fret it
   } else if (!m2) {
      return m1;        // or sweat it
   } else if (m1->class_name() != m2->class_name()) {
      cerr << "BMESH::merge: bad luck! the meshes are different types:\n   "
           << m1->class_name()
           << " and "
           << m2->class_name()
           << endl;
      return BMESHptr(nullptr);
   }

   // try to think of everything. they might be the same mesh.
   if (m1 == m2)
      return m1; // don't fuss about it

   // make sure m1 is the larger mesh (more vertices).
   // m2 will get sucked into m1 and be left as an empty husk.
//    if (m2->nverts() > m1->nverts())
//       swap(m1,m2);      // swap pointers

   // m2 is now the smallest. check it's not taking smallness to an
   // extreme:
   if (m2->empty()) {
      err_msg("BMESH::merge: smaller mesh is empty");
      return m1;
   }

   // transform m2 into the coordinate system of m1:
   m2->transform(m1->inv_xform()*m2->xform(), MOD());

   // now do the merging via the virtual method that lets LMESHes
   // (e.g.) deal with any LMESH-specific merge issues:
   m1->_merge(m2);

   return m1;
}

void
BMESH::_merge(BMESHptr m)
{
   // Merge the elements of mesh m into this one.
   //
   // For this protected method, it's okay to assume error checking
   // has already been done by the caller -- i.e. by the public method
   // BMESH::merge().

   assert(m);

   // Suck in verts, edges and faces:
   _verts.append(m->_verts);
   _edges.append(m->_edges);
   _faces.append(m->_faces);

   m->_verts.clear();
   m->_edges.clear();
   m->_faces.clear();

   // Suck in patches
   while (!m->_patches.empty()) {
      _patches += m->_patches.pop();
      _patches.last()->set_mesh(shared_from_this());
   }

   // Suck in drawables
   _drawables.operator+=(m->_drawables);
   m->_drawables.clear();

   // determine the "type" of the resulting mesh:
   if (_type_valid && m->_type_valid) {
      _type |= m->_type;
      // That pretty much works... but it can't be both an open and
      // closed surface:
      if (_type & OPEN_SURFACE)
         _type &= ~CLOSED_SURFACE;

      // Invalidation
      _vert_locs.clear();
      BODY::_edges.reset();
      _body = nullptr;
      _bb.reset();

      // So display lists get rebuilt:
      _version++;

   } else {
      // This will ensure the proper type is figured out when needed:
      changed(TOPOLOGY_CHANGED);
   }

   // That poor sucker definitely changed in either case:
   m->changed(TOPOLOGY_CHANGED);

   // Tell observers this mesh absorbed that one:
   BMESHobs::broadcast_merge(shared_from_this(), m);
}

// assumes flags were cleared previously
void
BMESH::grow_mesh_equivalence_class(
   Bvert* v,
   vector<Bface*> &faces,
   vector<Bedge*> &edges,
   vector<Bvert*> &verts)
{
   if (v->flag() == 0)
      return;

   for (int i=0; i<v->degree(); i++) {
      Bedge* nbr_e = v->e(i);
      Bvert* nbr_v = v->nbr(i);

      for (int j=1; j<=2; j++) {
         Bface *f = nbr_e->f(j);
         if (f && !f->flag()) {
            f->set_flag();
            faces.push_back(f);
         }
      }

      if (!nbr_e->flag()) {
         nbr_e->set_flag();
         edges.push_back(nbr_e);
      }

      if (!nbr_v->flag()) {
         nbr_v->set_flag();
         verts.push_back(nbr_v);

         grow_mesh_equivalence_class(nbr_v, faces, edges, verts);
      }
   }
}

void
BMESH::clear_flags()
{
   // clear flags
   _verts.clear_flags();
   _edges.clear_flags();
   _faces.clear_flags();
}

vector<BMESHptr>
BMESH::split_components()
{
   // Splits mesh into disconnected pieces.
   // Returns array of new meshes.

   vector<BMESHptr> new_meshes;

   clear_flags();

   // split up

   // Work from a copy of the vertices since the list will
   // get sucked out from under us otherwise:
   Bvert_list tmp_verts = _verts;

   for (Bvert_list::size_type i=0; i<tmp_verts.size(); i++) {
      Bvert* v = tmp_verts[i];
      if (!v->flag()) {

         // Mesh elements reachable from v:
         vector<Bface*> faces;
         vector<Bedge*> edges;
         vector<Bvert*> verts;

         // The flag being set means the vertex (or edge or face)
         // is in the list:
         v->set_flag();
         verts.push_back(v);

         // Collect all reachable elements:
         grow_mesh_equivalence_class(v, faces, edges, verts);

         // The 1st mesh is us... leave it alone.
         if (i == 0)
            continue;

         // Make a new mesh after the first time.
         BMESHptr m = dynamic_cast<BMESH*>(dup())->shared_from_this();
         new_meshes.push_back(m);

         size_t t;
         for (t=0; t<verts.size(); t++) {
            // remove from this mesh and add to the new mesh
            Bvert_list::iterator it;
            it = std::find(_verts.begin(), _verts.end(), verts[t]);
            _verts.erase(it);
            m->add_vertex(verts[t]);
         }

         for (t=0; t<edges.size(); t++) {
            // remove from this mesh and add to the new mesh
            Bedge_list::iterator it;
            it = std::find(_edges.begin(), _edges.end(), edges[t]);
            _edges.erase(it);
            m->add_edge(edges[t]);
         }

         // The new patch will take the face away from its
         // current patch, if any.
         Patch* p = m->new_patch();
         for (t=0; t<faces.size(); t++) {
            // remove from this mesh and add to the new mesh
            Bface_list::iterator it;
            it = std::find(_faces.begin(), _faces.end(), faces[t]);
            _faces.erase(it);
            m->add_face(faces[t],p);
         }

         if (m.get() != this)
            m->changed(TOPOLOGY_CHANGED);
      }
   }

   // Remove empty patches:
   clean_patches();

   changed(TOPOLOGY_CHANGED);

   BMESHobs::broadcast_split(shared_from_this(), new_meshes);

   return new_meshes;
}

void
BMESH::split_patches()
{
   // turn each separate mesh component into a separate patch

   // clear patches so no face belongs to a patch
   // for each face
   //   if patch is null
   //     get reachable faces
   //     assert all patches are null
   //     create new patch and add them all

   // clear patches
   delete_patches();

   // check each face
   for (Bface_list::size_type i=0; i<_faces.size(); i++) {
      // skip if already visited this face:
      if (_faces[i]->patch())
         continue;

      // get the reachable faces
      Bface_list component = Bface_list::reachable_faces(_faces[i]);

      // assert none of them are in a patch:
      assert(component.all_satisfy(PatchFaceFilter(nullptr)));

      // create a new patch and add all the faces we found:
      Patch* p = new_patch();
      assert(p);
      p->add(component);
   }

   changed(PATCHES_CHANGED);
}

vector<Bface_list>
BMESH::get_components() const
{
   // Returns separate Bface_lists, one for
   // each connected component of the mesh:

   vector<Bface_list> ret;

   // before calling Bface_list::grow_connected(),
   // must set all face flags to 1:
   _faces.set_flags(1);

   for (Bface_list::size_type i=0; i<_faces.size(); i++) {
      Bface* f = bf(i);
      if (f->flag() == 1) {
         Bface_list component;
         component.grow_connected(f);
         assert(!component.empty());
         ret.push_back(component);
      }
   }
   return ret;
}

void
BMESH::split_tris(Bface* start_face, Wplane plane, vector<Bvert*>& new_vs)
{
   Bedge* curr_edge = nullptr;
   for (int ed=1; ed<4; ed++)
      if (start_face->e(ed)->which_side(plane) == 0) {
         curr_edge = start_face->e(ed);
         break;
      }

   if (!curr_edge) {
      cerr << "BMESH::split_tris: lost my edge, can't go on" << endl;
      return;
   }

   Bvert *new_pt;
   Bvert *last_pt  = nullptr;
   Bvert *start_pt = nullptr;

   do {
      if (last_pt) {
         Bface_list faces;
         last_pt->get_faces(faces);
         for (Bface_list::size_type i=0; i<faces.size(); i++) {
            curr_edge = faces[i]->opposite_edge(last_pt);
            if (curr_edge->which_side(plane) == 0) {
               if (new_vs.empty())
                  break;
               else {
                  Bvert* oldv = new_vs[new_vs.size()-2];
                  if (!faces[i]->contains(oldv))
                     break;
               }
            }
         }
      }

      Wpt p1 = curr_edge->v1()->loc();
      Wpt p2 = curr_edge->v2()->loc();
      double d1 = fabs(plane.dist(p2));
      double d2 = fabs(plane.dist(p1));
      Wpt pt = ((d1 * p1) + (d2 * p2)) / (d1+d2);

      new_pt = split_edge(curr_edge, pt);
      new_vs.push_back(new_pt);

      if (!last_pt)
         start_pt = new_pt;

      last_pt = new_pt;
   } while (new_vs.size() < 3 || last_pt->lookup_edge(start_pt) == nullptr);
}


void
BMESH::kill_component(Bvert *start_vert)
{
   vector<Bface*> faces;
   vector<Bedge*> edges;
   vector<Bvert*> verts;

   int i;
   for (i=0; i<nfaces(); i++)
      _faces[i]->clear_flag();
   for (i=0; i<nedges(); i++)
      _edges[i]->clear_flag();
   for (i=0; i<nverts(); i++)
      _verts[i]->clear_flag();

   start_vert->set_flag();
   verts.push_back(start_vert);

   grow_mesh_equivalence_class(start_vert, faces, edges, verts);

   cerr << "removed " << faces.size() << " faces, "
        << edges.size() << " edges, " << verts.size() << " vertices" << endl;

   size_t t;
   for (t=0; t<faces.size(); t++)
      delete_face(faces[t]);

   for (t=0; t<edges.size(); t++)
      delete_edge(edges[t]);

   for (t=0; t<verts.size(); t++)
      delete_vert(verts[t]);

   changed(TOPOLOGY_CHANGED);
}


// Does a Patch with a current GTexture named name already exist in this
// mesh?
int
BMESH::tex_already_exists(const string &name) const
{
   int exists = 0;
   if (patches().num()) {
      for (int i = 0; !exists && i < patches().num(); i++) {
         exists = patches()[i]->cur_tex()->type() == name;
      }
   }
   return exists;
}

double
BMESH::z_span() const
{
   double a, b;
   return z_span(a, b);
}

double
BMESH::z_span(double& zmin, double& zmax) const
{
   // take the bounding box of this mesh,
   // collect its corners into a list of points,
   // transform these points to NDCZ,
   // find the min and max z values,
   // return the difference

   static Wpt_list pts(8);

   // FIXME: const
   if (!(const_cast<BMESH*>(this)->get_bb().points(pts)))
      return 2.0;       // z values lie between -1 and 1

   pts.xform(obj_to_ndc());

   zmin = pts[0][2];
   zmax = zmin;
   for (Wpt_list::size_type i=1; i<pts.size(); i++) {
      double z = pts[i][2];
      zmin = min(zmin, z);
      zmax = max(zmax, z);
   }

   return (zmax - zmin);
}


void
BMESH::compute_pix_size()
{
   // XXX -
   //   probably should use world_length so this will be more
   //   robust when the mesh is partly offscreen or behind the
   //   camera.

   _pix_size_stamp = VIEW::stamp();
   BBOX bb = xform()*get_bb();
   double size = bb.dim().length();
   Wpt pos = bb.center();
   Wvec perp = VIEW::peek_cam()->data()->right_v();
   assert(!perp.is_null());
   _pix_size = VEXEL(pos, size * perp).length();
}

//******** RENDERING STYLE ********

// the rendering style is only set on a mesh when you want
// to override the current rendering style of the view.
// i.e., almost never.
//
void
BMESH::set_render_style(const string& s)
{
   if (_render_style.empty())
      push_render_style(s) ;
   else
      _render_style.back() = s;
   changed(RENDERING_CHANGED);
}

void
BMESH::push_render_style(const string& s)
{
   _render_style.push_back(s);
   changed(RENDERING_CHANGED);
}

void
BMESH::pop_render_style()
{
   if (!_render_style.empty())
      _render_style.pop_back();
   changed(RENDERING_CHANGED);
}

const string&
BMESH::render_style() const
{
   static string empty("");
   return _render_style.empty() ? empty : _render_style.back();
}

void 
BMESH::update_patch_blend_weights()
{  
   if (_patch_blend_weights_valid)
      return;
    
   if (Config::get_var_bool("DEBUG_PATCH_BLEND_WEIGHTS",false)) {
      cerr << "BMESH::update_patch_blend_weights: level: "
           << subdiv_level()
           << ", passes: " << patch_blend_smooth_passes()
           << endl;
   }
   _patch_blend_weights_valid = true;
   PatchBlendWeight::compute_all(shared_from_this(), patch_blend_smooth_passes());
}

/*****************************************************************
 * BMESHray
 *****************************************************************/
void
BMESHray::check(
   double    d,
   int       is_surface,
   double    d_2d,
   CGELptr  &g,
   CWvec    &n,
   CWpt     &nearpt,
   CWpt     &surfl,
   APPEAR   *app,
   CXYpt    &tex_coord
   )
{
   if (test(d, is_surface, d_2d)) {
      // the RAYhit is being redefined ... clear the old simplex.
      set_simplex(nullptr);
      RAYhit::check(d, is_surface, d_2d, g, n, nearpt, surfl, app, tex_coord);
   }
}

/*************************************************************
 * BMESHobs
 ************************************************************/
void
BMESHobs::broadcast_change(BMESHptr m, BMESH::change_t change)
{
   // Notify observers who watch all meshes
   _all_observers.notify_change(m, change);

   // Notify observers of the given mesh
   bmesh_obs_list(m).notify_change(m, change);
}

void
BMESHobs::broadcast_xform(BMESHptr mesh, CWtransf& xf, CMOD& mod)
{
   // Notify observers who watch all meshes
   _all_observers.notify_xform(mesh, xf, mod);

   // Notify observers of the given mesh
   bmesh_obs_list(mesh).notify_xform(mesh, xf, mod);
}

void
BMESHobs::broadcast_merge(BMESHptr joined, BMESHptr removed)
{
   // The 'joined' mesh just sucked everything out of the
   // 'removed' mesh, which is now an empty husk.

   // Notify observers who watch all meshes
   _all_observers.notify_merge(joined, removed);

   // Notify observers of the joined mesh.
   //
   // Get observer list by copying, since some observers
   // might start hopping in and out of the original list
   // while we iterate over it.
   //
   // XXX - Actually the observers of the joined mesh will almost
   // certainly stay where they are, since their mesh wasn't
   // destroyed, it just jot bigger.
   BMESHobs_list joined_obs = bmesh_obs_list(joined);
   joined_obs.notify_merge(joined, removed);

   // Notify observers of the removed mesh.
   // Again work with a copy of the observer list.
   // This time they probably will jump off the sinking ship.
   BMESHobs_list removed_obs = bmesh_obs_list(removed);
   removed_obs.notify_merge(joined, removed);
}

void
BMESHobs::broadcast_split(BMESHptr m, const vector<BMESHptr>& new_meshes)
{
   // Notify observers who watch all meshes
   _all_observers.notify_split(m, new_meshes);

   // Notify observers of the given mesh.
   // Work from a copy of the list in case observers start
   // hopping between lists.
   BMESHobs_list list = bmesh_obs_list(m);
   list.notify_split(m, new_meshes);
}

void
BMESHobs::broadcast_subdiv_gen(BMESHptr m)
{
   // Notify observers who watch all meshes
   _all_observers.notify_subdiv_gen(m);

   // Notify observers of the given mesh
   bmesh_obs_list(m).notify_subdiv_gen(m);
}

void
BMESHobs::broadcast_delete(BMESH* m)
{
   // Notify observers who watch all meshes
   _all_observers.notify_delete(m);

   // Notify observers of the given mesh.
   // Work with a copy of the observer list in case
   // observers start hopping around.
   BMESHobs_list list = bmesh_obs_list(m);
   list.notify_delete(m);
}

void
BMESHobs::broadcast_sub_delete(BMESH* m)
{
   // Notify observers who watch all meshes
   _all_observers.notify_sub_delete(m);

   // Notify observers of the given mesh.
   bmesh_obs_list(m).notify_sub_delete(m);
}

void
BMESHobs::broadcast_update_request(BMESHptr m)
{
   // This is for observers who want to update the mesh
   // before it does something important like try to
   // draw itself.
   bmesh_obs_list(m).notify_update_request(m);

   // XXX - Notify observers who watch all meshes? Nah.
}

/*************************************************************
 * BMESHobs
 ************************************************************/
void
BMESHobs_list::notify_change(BMESHptr mesh, BMESH::change_t change) const
{
   BMESHobs_list::iterator i;
   for (i=begin(); i!=end(); ++i)
      (*i)->notify_change(mesh, change);
}

void
BMESHobs_list::notify_xform(BMESHptr mesh, CWtransf& xf, CMOD& m) const
{
   BMESHobs_list::iterator i;
   for (i=begin(); i!=end(); ++i)
      (*i)->notify_xform(mesh, xf, m);
}

void
BMESHobs_list::notify_merge(BMESHptr m1, BMESHptr m2) const
{
   BMESHobs_list::iterator i;
   for (i=begin(); i!=end(); ++i)
      (*i)->notify_merge(m1, m2);
}

void
BMESHobs_list::notify_split(BMESHptr mesh, const vector<BMESHptr>& pieces) const
{
   BMESHobs_list::iterator i;
   for (i=begin(); i!=end(); ++i)
      (*i)->notify_split(mesh, pieces);
}

void
BMESHobs_list::notify_subdiv_gen(BMESHptr mesh) const
{
   BMESHobs_list::iterator i;
   for (i=begin(); i!=end(); ++i)
      (*i)->notify_subdiv_gen(mesh);
}

void
BMESHobs_list::notify_delete(BMESH* mesh) const
{
   BMESHobs_list::iterator i;
   for (i=begin(); i!=end(); ++i)
      (*i)->notify_delete(mesh);
}

void
BMESHobs_list::notify_sub_delete(BMESH* mesh) const
{
   BMESHobs_list::iterator i;
   for (i=begin(); i!=end(); ++i)
      (*i)->notify_sub_delete(mesh);
}

void
BMESHobs_list::notify_update_request(BMESHptr mesh) const
{
   BMESHobs_list::iterator i;
   for (i=begin(); i!=end(); ++i)
      (*i)->notify_update_request(mesh);
}

void
BMESHobs_list::print_names() const
{
   BMESHobs_list::iterator i;
   for (i=begin(); i!=end(); ++i)
      cerr << (*i)->name() << " ";
   cerr << endl;
}

// end of file bmesh.C
