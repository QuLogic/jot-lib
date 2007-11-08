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
/*****************************************************************
* proxy_surface.C
*****************************************************************/
#include "disp/colors.H"
#include "disp/bbox.H"
#include "geom/gl_view.H"
#include "geom/world.H"
#include "mesh/bvert.H"
#include "mesh/mi.H"
#include "mesh/uv_data.H"
#include "npr/hatching_group_base.H"
#include "std/run_avg.H"

#include "proxy_surface.H"

static bool debug = Config::get_var_bool("DEBUG_PROXY_SURFACE",false);

inline VEXEL
cmult(CVEXEL& a, CVEXEL& b)
{
   // complex number multiplication:
   return VEXEL(a[0]*b[0] - a[1]*b[1], a[0]*b[1] + a[1]*b[0]);
}

/*****************************************************************
 * ProxySurface
 *****************************************************************/
ProxySurface::ProxySurface(Patch* p) 
{
   set_patch(p);
   _scale = 1.0;
  
   
}

ProxySurface::~ProxySurface() 
{
   // _proxy_mesh goes out of scope here and is deleted automatically
}

void 
ProxySurface::set_patch(Patch* p)
{
   err_adv(debug && _patch, "ProxySurface::set_patch: replacing patch");
   _patch = p;
}

void 
ProxySurface::create_proxy_surface()
{
   // create it only if needed
   if (_proxy_mesh || !_patch) {
      return;
   }
   _proxy_mesh = new BMESH;
   
    mlib::NDCZpt a_t, c_t;
    _patch->mesh()->get_bb().ndcz_bounding_box(_patch->obj_to_ndc(),a_t,c_t);

    // Clamp the min and max to screen
    a_t[0] = max(a_t[0],-1.0);
    a_t[1] = max(a_t[1],-1.0);
    c_t[0] = min(c_t[0], 1.0);
    c_t[1] = min(c_t[1], 1.0);

    BBOXpix bb = BBOXpix(PIXEL(a_t), PIXEL(c_t));
    double len = max(bb.width(),bb.height());

    PIXEL a_p(a_t);
    XYpt a = XYpt(a_p);
    XYpt b = XYpt(a_p + VEXEL(len,  0));
    XYpt c = XYpt(a_p + VEXEL(len,len));
    XYpt d = XYpt(a_p + VEXEL(  0,len));
 
    Wpt cc = VIEW::peek_cam()->data()->center();
   // create a quad organized in XY space
   // first create vertices (XY square in CCW order):
   _proxy_mesh->add_vertex(Wpt(a, cc));
   _proxy_mesh->add_vertex(Wpt(b, cc));
   _proxy_mesh->add_vertex(Wpt(c, cc));
   _proxy_mesh->add_vertex(Wpt(d, cc));
  
   // now create the quad:
   Patch* pa = _proxy_mesh->new_patch();
   _proxy_mesh->add_quad(0,1,2,3,
                         UVpt(0,0), UVpt(1,0), UVpt(1,1), UVpt(0,1),
                         pa);

   UVdata::set(_proxy_mesh->bv(0), (UVpt(0,0)));
   UVdata::set(_proxy_mesh->bv(1), (UVpt(1,0)));
   UVdata::set(_proxy_mesh->bv(2), (UVpt(1,1)));
   UVdata::set(_proxy_mesh->bv(3), (UVpt(0,1)));

   put_vert_grid(UVpt(0,0), _proxy_mesh->bv(0));
   put_vert_grid(UVpt(1,0), _proxy_mesh->bv(1));
   put_vert_grid(UVpt(1,1), _proxy_mesh->bv(2));
   put_vert_grid(UVpt(0,1), _proxy_mesh->bv(3));

   _o = _proxy_mesh->bv(0)->pix();
   _u_o = _proxy_mesh->bv(1)->pix();
   _v_o = _proxy_mesh->bv(3)->pix();  

   // grow more quads if needed, though it is never needed (yet):
   while (grow_proxy_surface() > 0)
      ; 

   // finish up:
   _proxy_mesh->changed();  
 }

inline bool
is_good(CBvert* v)
{
   return v && v->is_front_facing() && v->wloc().in_frustum();
}

inline double
depth(CWpt& p, CCAMdataptr& camdata)
{
   // return distance to given world-space point, measured
   // along the camera's line of sight (forward direction):

   return (p - camdata->from())*camdata->at_v();
}

inline PIXEL_list
get_pixels(CBvert_list& verts, ProxySurface* p)
{
   PIXEL_list ret(verts.num());
   for (int i=0; i<verts.num(); i++) {
      ret += PixelsData::get_pix(verts[i], p);
   }
   return ret;
}



inline void
set_pix(Bvert* v, ProxySurface* p, CPIXEL& pix)
{
   // change the vert location to match the given pixel location,
   // not changing depth:
   v->set_loc(Wpt(XYpt(pix)));
   PixelsData::set_pix(v, p, pix);  // also store it for next time
}

void    
ProxySurface::apply_xform()
{
   if(!_patch)
      return;
   // apply the transform to the vertices of the proxy mesh
   assert(_proxy_mesh);
 
   PIXEL o = _patch->get_old_sample_center();
   PIXEL n = _patch->get_sample_center();
   VEXEL z = _patch->get_z();


   CBvert_list&  verts = _proxy_mesh->verts();    // vertices of the proxy mesh
   PIXEL_list   pixels = get_pixels(verts, this); // their old pixel locations

   // compute and apply the transform to each vertex:
   for (int i=0; i<verts.num(); i++) {
      set_pix(verts[i], this, n + cmult(pixels[i] - o, z));
   }

   //cache _uv_orig, _uv_u_pt, _uv_v_pt so that we can grow new quads
   _o = n + cmult(_o - o, z);
   _u_o = n + cmult(_u_o - o, z);
   _v_o = n + cmult(_v_o - o, z);

   // take care of bizness:
   _proxy_mesh->changed(); //BMESH::VERT_POSITIONS_CHANGED
}

void 
ProxySurface::update_proxy_surface()
{
   if(!_proxy_mesh)
      create_proxy_surface();

   apply_xform();  

   while (grow_proxy_surface() > 0)
      ; 
   trim_proxy_surface();
   
   lod_update();
}

void 
ProxySurface::lod_update()
{
	//cerr << "scale: " << _scale << endl;

	if(_scale > 2.0)
	{
		//make new mesh where current z is 1.0
        //_proxy_mesh_other
		//fade out _proxy_mesh / fade in _proxy_mesh_new
		
		//fade in
		//_proxy_mesh_old = _proxy_mesh;
		
	}
	else if(_scale < .7)
	{
		//_scale = 1.0;
	}

}

void 
ProxySurface::grow_quad_from_edge(BMESH* m, EdgeStrip* boundary, int i)
{
   // i is the position of the edge in the boundary
   Bedge* e = boundary->edge(i);
   assert(e);
   
   //make sure is still a boundary edge if not, return...
   if(e->nfaces() > 1){
      return;
   }

   Bvert* v2 = boundary->vert(i);
   Bvert* v1 = boundary->next_vert(i);
   assert(v1);
   assert(v2);

   if(!(UVdata::is_continuous(e)))
   {
      cerr << "ProxySurface::grow_quad_from_edge: e is discontinous!!!" << endl;
      return;
   }
 
   UVpt uv_1, uv_2, uv_3, uv_4;

   if(!(UVdata::get_uv(v2, uv_2)))
   {   
      cerr << "ProxySurface::grow_quad_from_edge: v2 vertex does not have UVdata!!!" << endl;
      return;
   }
   
   if(!(UVdata::get_uv(v1, uv_1)))
   {   
      cerr << "ProxySurface::grow_quad_from_edge: v1 vertex does not have UVdata!!!" << endl;
      return;
   }

   // constant in u dir -> v line
   if(uv_1[0] == uv_2[0]){
      if(uv_1[1] > uv_2[1])
      {
         uv_3 = UVpt(uv_2[0]+1,uv_2[1]);
         uv_4 = UVpt(uv_1[0]+1,uv_1[1]);
      } else {
         uv_3 = UVpt(uv_2[0]-1,uv_2[1]);
         uv_4 = UVpt(uv_1[0]-1,uv_1[1]);
      }
   } else {
      assert(uv_1[1] == uv_2[1]);
      //constant in v dir
      if(uv_1[0] > uv_2[0])
      {
         uv_3 = UVpt(uv_2[0],uv_2[1]-1);
         uv_4 = UVpt(uv_1[0],uv_1[1]-1);
      } else {
         uv_3 = UVpt(uv_2[0],uv_2[1]+1);
         uv_4 = UVpt(uv_1[0],uv_1[1]+1);
      }
   }      
   
   // double dist = e->length();
   //cerr << "grow quad: " << i << " " << uv_1 << " " << uv_2 << " " << uv_3 << " " << uv_4 << endl;
   Bvert* v_1 = vertFromUV(uv_1);
   Bvert* v_2 = vertFromUV(uv_2);
   Bvert* v_3 = vertFromUV(uv_3);
   Bvert* v_4 = vertFromUV(uv_4);
   assert(v_1 && v_2 && v_3 && v_4);
   //cerr << "grow quad: " << i << " " << v_1->wloc() << " " << v_2->wloc() << " " << v_3->wloc() << " " << v_4->wloc() << endl;

   m->add_quad(v_1, v_2, v_3, v_4,
               uv_1, uv_2, uv_3, uv_4,
               m->patch(0));
   UVdata::set(v_1, uv_1);
   UVdata::set(v_2, uv_2);
   UVdata::set(v_3, uv_3);
   UVdata::set(v_4, uv_4);
   
 
   //PixelsData::get_pix(v_1, this);
   //PixelsData::get_pix(v_2, this);
   //PixelsData::get_pix(v_3, this);
   //PixelsData::get_pix(v_4, this);

   PixelsData::set_pix(v_1, this,v_1->pix());
   PixelsData::set_pix(v_2, this,v_2->pix());
   PixelsData::set_pix(v_3, this,v_3->pix());
   PixelsData::set_pix(v_4, this,v_4->pix());


}

int  
ProxySurface::grow_proxy_surface()
{
   assert(_proxy_mesh);
   int n = 0; //number of edges that will be grown

   // Get the boundary of the current proxy surface
   EdgeStrip boundary = _proxy_mesh->faces().get_boundary();
   //cerr << "ProxySurface::grow_proxy_surface " << boundary.num() << " total "<< endl;
   for(int i=0; i < boundary.num(); ++i)
   {        
      //cerr << "ProxySurface::grow_proxy_surface now: " << i << endl;
      if(is_inside_bounding_box(boundary.edge(i))){ 
         grow_quad_from_edge(_proxy_mesh, &boundary, i);        
          _proxy_mesh->changed();
         n++;         
      }
      
   }
  
   return n;
}
void 
ProxySurface::trim_proxy_surface()
{
   assert(_proxy_mesh);

   int n = 0; //number of faces outside the bounding box   
   //get all the quads
   Bface_list faces = _proxy_mesh->faces();
   //clear out all the markings
   for(int i=0; i < faces.num(); ++i)
   {
      if(faces[i])
         ProxyData::set_mark(faces[i],this, false);
      //else
      //   cerr << "FACE is NULL" << endl;
   }
   //mark all the faces that do not overap bounding box
   for(int i=0; i < faces.num(); ++i)
   {
      if(faces[i]){
         bool t1 = (is_inside_bounding_box(faces[i]->e1())) ? true : false;
         bool t2 = (is_inside_bounding_box(faces[i]->e2())) ? true : false;
         bool t3 = (is_inside_bounding_box(faces[i]->e3())) ? true : false;
         
         // If all the edges are outside, then mark the face
         if(!t1 && !t2 && !t3){
            //cerr << "we can delete this face" << endl;
            ProxyData::set_mark(faces[i], this, true);
            n++;
         }
      }else {
         //cerr << "FACE is NULL" << endl;
      }
   }

   if(n < 1)
      return; 

   //for all verts check to see if all the faces that it is attached to has a marked
   Bvert_list verts = _proxy_mesh->verts();
   for(int i=0; i < verts.num(); ++i)
   {
      ARRAY<Bface*> ret;
      verts[i]->get_quad_faces(ret);
      // Make sure that all adjasent faces need to be deleted
      bool do_it=true;
      for(int k=0; k < ret.num(); ++k)
      {
         if(ret[k]){
            assert(ret[k]->is_quad());            
            if(!ProxyData::get_mark(ret[k], this) || !ProxyData::get_mark(ret[k]->quad_partner(), this))
            {
              // cerr << "vert degree " << verts[i]->p_degree() << endl;
               do_it = false;
               break;
            }
         }
      }
      if(do_it){
         UVpt remove_uv;
         UVdata::get_uv(verts[i], remove_uv);
         remove_vert_grid(remove_uv);

         _proxy_mesh->remove_vertex(verts[i]);
         _proxy_mesh->changed();
      }
   }
   //clean up faces
   Bface_list faces2 = _proxy_mesh->faces();
   for(int i=0; i < faces2.num(); ++i)
   {
      if(!(faces2[i]->is_quad()) || !(faces2[i]->quad_partner())){
         _proxy_mesh->remove_face(faces2[i]);
         _proxy_mesh->changed();
      }
      
   }

   //debug_grid();

   
}

bool 
ProxySurface::is_inside_bounding_box(CBedge* e)
{
   //get the bouding box
    mlib::NDCZpt a, c;
    _patch->mesh()->get_bb().ndcz_bounding_box(_patch->mesh()->obj_to_ndc(),a,c);
    
    //clamp to the screen
    a[0] = (a[0] < -1.0) ? -1.0 : a[0];
    a[1] = (a[1] < -1.0) ? -1.0 : a[1];
    c[0] = (c[0] > 1.0) ? 1.0 : c[0];
    c[1] = (c[1] > 1.0) ? 1.0 : c[1];
    

    BBOXpix bb = BBOXpix(PIXEL(a), PIXEL(c)); 
    //cout << "bb is " <<  bb.min() <<  " " << bb.max() << endl;
    
    //If ether of the verts of the edge inside the box, then it's inside
    if(bb.contains(e->v1()->pix()) || bb.contains(e->v2()->pix())){
       return true;
    } else {
       //Check to see if the segment intersetcs will all the segments of the box
       PIXEL b(bb.max()[0], bb.min()[1]);
       PIXEL d(bb.min()[0], bb.max()[1]);

       PIXELline e_line = e->pix_line();
       
       PIXELline bb_line1 = PIXELline(bb.min(),b);
       if(e_line.intersect_segs(bb_line1))
          return true;
       PIXELline bb_line2 = PIXELline(b,bb.max());
       if(e_line.intersect_segs(bb_line2))
          return true;
       PIXELline bb_line3 = PIXELline(bb.max(),d);
       if(e_line.intersect_segs(bb_line3))
          return true;
       PIXELline bb_line4 = PIXELline(d,bb.min());
       if(e_line.intersect_segs(bb_line4))
          return true;
    }
    return false;
}

Wpt 
ProxySurface::getWptfromUV(CUVpt& uv)
{
   VEXEL u_vec = _u_o - _o;
   VEXEL v_vec = _v_o - _o;   
   PIXEL newpix = _o + u_vec*uv[0] + v_vec*uv[1];
   Wpt cc = VIEW::peek_cam()->data()->center();
   return Wpt(XYpt(newpix), cc);
}

UVpt     
ProxySurface::getUVfromNDC(CNDCpt& ndc)
{
   PIXEL pix(ndc);
   VEXEL u_vec = _u_o - _o;
   VEXEL v_vec = _v_o - _o; 
   
   UVpt pt;
   pt[0] = (pix - _o) * u_vec / (u_vec*u_vec);
   pt[1] = (pix - _o) * v_vec / (v_vec*v_vec);
   
   return pt; 

   //Wpt p(ndc);
   //CBface_list&  faces = _proxy_mesh->faces();
   //for(int i=0; i < faces.num(); ++i)
   //{
   //   if(faces[i]->contains(p,0))
   //   {
   //      Bface* f = (faces[i]->is_quad_rep()) ? faces[i] : faces[i]->quad_partner();
   //      UVpt base = baseUVpt(f);
}

Bvert* 
ProxySurface::vertFromUV(CUVpt& uv)
{
   Bvert* bv = get_vert_grid(uv);
   if(bv) {
      //cerr << "ProxySurface::vertFromUV: using existant vert " << endl;
      return bv;
   }

   //Lets create the new vertex   
  
   
   Wpt new_vert_wp = getWptfromUV(uv);
 
   //add the vertex to the mesh
   Bvert* new_vert = _proxy_mesh->add_vertex(new_vert_wp);   
   //add the vetex to the lookup table
   put_vert_grid(uv, new_vert);
   return new_vert;
}

void
ProxySurface::remove_vert_grid(CUVpt& uv)
{
   // XXX - need to fix map to use doubles for keys.
   //       for now just cast to int:
   _grid_map[int(uv[0])].erase(int(uv[1]));
   if(_grid_map[int(uv[0])].empty())
      _grid_map.erase(int(uv[0]));  
}

void 
ProxySurface::put_vert_grid(CUVpt& uv, CBvert* bvert)
{    
   _grid_map[int(uv[0])][int(uv[1])] = bvert; 
}

Bvert* 
ProxySurface::get_vert_grid(CUVpt& uv)
{
   Bvert* b =  (Bvert*)(_grid_map[int(uv[0])][int(uv[1])]);
   //if(!b){
   //   cerr << "Vert NOT Found" << endl;
   //}
   return b;
}

inline void
print_uv(CBvert* v)
{
   assert(v);
   UVpt uv;
   if (UVdata::get_uv(v,uv)) {
      cerr << uv;
   } else if (UVdata::get_uv(v,v->get_face(),uv)) {
      cerr << "per face uv: " << uv;
   } else {
      cerr << "none/discontinuous uv";
   }
}

void 
ProxySurface::debug_grid()
{
   cerr << "debug_grid: " << endl;
  
   for(Grid_Map::iterator it = _grid_map.begin(); it != _grid_map.end(); ++it) {
      for(Col_Map::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
         cerr << "("<< it->first << ", " << it2->first << ") : ";
         if(it2->second)
            print_uv(it2->second);
         else
            cerr << "NULL" << endl;
         cerr << "   ";
      }
      cerr << endl;      
   }
 
}


//   2
// 3|_|1
//   0
Bface*   
ProxySurface::neighbor_face(int dir, Bface* face)
{
    UVpt base_uv = baseUVpt(face);
    
	UVpt uv1, uv2, uv3, uv4;
	switch (dir)
	{
	case 0:
		uv1 = UVpt(base_uv[0], base_uv[1]);
	    uv2 = UVpt(base_uv[0]+1, base_uv[1]);
		uv3 = UVpt(base_uv[0], base_uv[1]-1);
		uv4 = UVpt(base_uv[0]+1, base_uv[1]-1);
		break;
	case 1:
		uv1 = UVpt(base_uv[0]+1, base_uv[1]);
	    uv2 = UVpt(base_uv[0]+2, base_uv[1]);
		uv3 = UVpt(base_uv[0]+1, base_uv[1]+1);
		uv4 = UVpt(base_uv[0]+2, base_uv[1]+1);
		break;
	case 2:
		uv1 = UVpt(base_uv[0], base_uv[1]+1);
	    uv2 = UVpt(base_uv[0]+1, base_uv[1]+1);
		uv3 = UVpt(base_uv[0], base_uv[1]+2);
		uv4 = UVpt(base_uv[0]+1, base_uv[1]+2);
		break;
	case 3:
		uv1 = UVpt(base_uv[0]-1, base_uv[1]);
	    uv2 = UVpt(base_uv[0], base_uv[1]);
		uv3 = UVpt(base_uv[0], base_uv[1]+1);
		uv4 = UVpt(base_uv[0]-1, base_uv[1]+1);
		break;
	default:
		cerr << "ProxySurface::neighbor_face: invalid diraction" << endl;
		assert(0);
	}

    Bvert* v1 = get_vert_grid(uv1);
	Bvert* v2 = get_vert_grid(uv2);
	Bvert* v3 = get_vert_grid(uv3);
	Bvert* v4 = get_vert_grid(uv4);
	
	if(!v1 || !v2 || !v3 || !v4)
		return 0;

	Bface* n = lookup_quad(v1, v2, v3, v4);
	assert(n);
	assert(n->is_quad());
	n = (n->is_quad_rep()) ? n : n->quad_partner();
	return n;
}

//Static methods

UVpt 
ProxySurface::baseUVpt(Bface* f)
{
   UVpt uva, uvb, uvc, uvd;
   UVdata::get_quad_uvs(f, uva, uvb, uvc, uvd);
   UVpt_list uvpts;
   uvpts += uva;
   uvpts += uvb;
   uvpts += uvc;
   uvpts += uvd;
   return UVpt(uvpts.min_val(0),uvpts.min_val(1));
}

bool 
ProxySurface::baseUVpt(Bface* f, UVpt& uv)
{
   CBface_list& faces = _proxy_mesh->faces();
   for(int i=0; i < faces.num(); ++i)
   {  
      if(faces[i]->is_quad_rep() && faces[i]==f)
      {
         UVpt uva, uvb, uvc, uvd;
         UVdata::get_quad_uvs(f, uva, uvb, uvc, uvd);
         UVpt_list uvpts;
         uvpts += uva;
         uvpts += uvb;
         uvpts += uvc;
         uvpts += uvd;
         uv[0] = uvpts.min_val(0);
         uv[1] = uvpts.min_val(1);
         return true;
      }
   }
   return false;
   

}
// end of proxy_surface.C
