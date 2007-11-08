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
/********************************************************************************
 * obj2jot:
 *
 *   Convert .obj file to .jot file. 
 *
 *
 *   Usage: % obj2jot [new jotfile] [objFile frame1] [objFile frame2] [objFile frame3] ... [objFile frameN]
 *
 *
 ********************************************************************************/

/********************************************************************************
 * The descriptions of .obj and .mtl formats provided below are from:
 *
 *    http://www.csit.fsu.edu/~burkardt/txt/obj_format.txt   
 *
 *
 ********************************************************************************/

/********************************************************************************
 * .obj file:
 *
 * TOKEN                        MEANING
 *
 * v float float float          Vertex position v (x) (y) (z)
 * 
 * vn float float float         Vertex normal vn (normal x) (normal y) (normal z)
 * 
 * vt float float               Texture coordinate vt (tu) (tv)
 * 
 * f int int int f (v) (v) (v)  Faces are stored as a series of three vertices
 *                              in clockwise order. Vertices are described by
 *                              their position, optional texture coordinate, and
 *                              optional normal, encoded as indices into the
 *                              respective component lists.
 * 
 * f int/int int/int int/int    f (v)/(vt) (v)/(vt) (v)/(vt)
 * 
 * f int/int/int int/int/int int/int/int        
 *                              f (v)/(vt)/(vn) (v)/(vt)/(vn) (v)/(vt)/(vn)
 * 
 * mtllib string                Material file (MTL) mtllib (filename.mtl)
 *                              References the MTL file for this mesh. MTL files
 *                              contain illumination variables and texture
 *                              filenames.
 * 
 * usemtl string                Material selection usemtl (material name) Faces
 *                              which are listed after this point in the file
 *                              will use the selected material
 * 
 * g group_name1 group_name2    Specifies the group name for the elements that 
 *                              follow it. You can have multiple group names. 
 *                              If there are multiple groups on one line, the 
 *                              data that follows belong to all groups. Group 
 *                              information is optional.
 *
 ********************************************************************************/

/********************************************************************************
 * .mtl file:
 *
 * TOKEN                        MEANING
 * 
 * newmtl string                Material newmtl (material name). Begins a new
 *                              material description.
 * 
 * Ka float float float         Ambient color Ka (red) (green) (blue)
 * 
 * Kd float float float         Diffuse color Kd (red) (green) (blue)
 * 
 * Ks float float float         Specular color Ks (red) (green) (blue)
 * 
 * d float Tr float             Transparency Tr (alpha)
 * 
 * Ns int                       Shininess Ns (specular power)
 * 
 * illum int                    Illumination model illum (1 / 2); 1 if specular
 *                              disabled, 2 if specular enabled.
 * 
 * map_Kd string                Texture map_Kd (filename)
 ********************************************************************************/
#include <string>
#include <sstream>
#include <limits>
#include "mesh/lmesh.H"
#include "std/config.H"
#include "net/io_manager.H"

static bool debug = true; //Config::get_var_bool("DEBUG_OBJ2JOT",false,true);

inline void
skip_line(istream& in)
{
   in.ignore(numeric_limits<std::streamsize>::max(), '\n');
}

inline void
read_vert(LMESH* mesh, istream& in)
{
   double x, y, z;
   in >> x >> y >> z;
   skip_line(in);
   assert(mesh);
   mesh->add_vertex(Wpt(x,y,z));
}

inline void
read_texcoord(UVpt_list& uvs, istream& in)
{
   double u, v;
   in >> u >> v;
   skip_line(in);
   uvs += UVpt(u,v);
}

class vtn {
 public:
   int _v, _t, _n; // vertex, texture coordinate, and normal indices

   vtn(int v = -1, int t = -1, int n = -1) : _v(v), _t(t), _n(n) {}

   bool operator==(const vtn& v) const {
      return v._v == _v && v._t == _t && v._n == _n;
   }

   bool is_valid() const { return _v >= 0; }
};

inline istream&
operator>>(istream& in, vtn& v)
{
   // read a vertex index, texcoord index, and normal index.
   // the last two are optional.

   in >> v._v;
   v._v -= 1;   // do correction to start counting from 0, not 1
   if (in.peek() == '/') {
      char slash; 
      in >> slash >> v._t;
      v._t -= 1;
      if (in.peek() == '/') {
         in >> slash >> v._n;
         v._n -= 1;
      }
   }
   return in;
}

template <class T>
inline bool
all_valid_indices(const T& l, int i, int j, int k)
{
   return l.valid_index(i) && l.valid_index(j) && l.valid_index(k);
}

template <class T>
inline bool
any_valid_indices(const T& l, int i, int j, int k)
{
   return l.valid_index(i) || l.valid_index(j) || l.valid_index(k);
}

inline void
add_tri(LMESH* mesh, Patch* p, CUVpt_list& uvs, const vtn& v1, const vtn& v2, const vtn& v3)
{
   assert(mesh);
   //cerr << "add_tri: " << v1._v << " "<< v2._v << " " <<  v3._v << endl;
   assert(all_valid_indices(mesh->verts(), v1._v, v2._v, v3._v));

   if (all_valid_indices(uvs, v1._t, v2._t, v3._t)) {
      mesh->add_face(v1._v, v2._v, v3._v, uvs[v1._t], uvs[v2._t], uvs[v3._t], p);
   } else {
      mesh->add_face(v1._v, v2._v, v3._v, p);
   }
}

inline void
add_poly(LMESH* mesh, Patch* p, CUVpt_list& uvs, const ARRAY<vtn>& vtns)
{
   assert(vtns.num() > 2);

   for (int k=2; k < vtns.num(); k++) {
      add_tri(mesh, p, uvs, vtns[0], vtns[k-1], vtns[k]);
   }
   // if it is a quad, mark the quad diagonal as "weak"
   if (vtns.num() == 4) {
      Bedge* e = lookup_edge(mesh->bv(vtns[0]._v), mesh->bv(vtns[2]._v));
      assert(e);
      e->set_bit(Bedge::WEAK_BIT);
   }
}

inline void
read_face(LMESH* mesh, CUVpt_list& uvs, Patch* p, istream& in)
{
   // The docs say each face has 3 vertices, but out there in the
   // world we're finding some with 4 or more.  So here we read
   // an arbitrary number of vertices following the "f" token,
   // then create a triangle fan for cases where there are > 3
   // vertices. If there are exactly 4 we make a quad.

   // use an istringstream to read vertices until the end of the line:
   ARRAY<vtn> vtns;
   const int BUF_SIZE = (1 << 12);
   char buf[BUF_SIZE] = {};
   in.getline(buf, BUF_SIZE);
   if (in.fail())
      return;
   istringstream is(buf);
   while (!(is.eof() || is.fail())) {
      vtn v;
      is >> v;
      if (v.is_valid())
         vtns += v;
      else break;
   }

   // create a polygon (triangle fan) from the vertices:
   if (vtns.num() < 3) {
      cerr << "read_face: error: read " << vtns.num() << " vertices for face" << endl;
      return;
   }
   add_poly(mesh, p, uvs, vtns);
}

inline void
read_obj(ARRAY<LMESHptr>& meshes, istream& in)
{
   UVpt_list    uvs;
  
   meshes.add(new LMESH);
   int curr_mesh = 0;
   Patch* curr_patch = 0;

   string token;
   while (!(in.eof() || in.fail())) {
      in >> token;
      if(token == "g") {   
         //curr_patch = meshes[curr_mesh]->new_patch();
         skip_line(in);
         //cerr << "Starting a new Mesh: " << endl;
         //meshes.add(new LMESH);
         //curr_mesh++;
      } else if(token == "usemtl") {         
         //cerr << "Starting a new patch: " << endl;
         //curr_patch = meshes[curr_mesh]->new_patch();
         skip_line(in);
      } else if (token == "v") {
         read_vert(meshes[curr_mesh], in);
      } else if (token == "vt") {
         read_texcoord(uvs, in);
      } else if (token == "f") {
         read_face(meshes[curr_mesh], uvs, curr_patch, in);
      } else {
         // can add more cases...
         skip_line(in);
      }
   }
}



inline
void output_update_mesh_start(ostream& os, str_ptr name)
{
   os << "UPDATE_GEOM	{ " << name << endl;
   os << "TEXBODY {"       << endl; 
	os << "name " << name   << endl; 
	os << "xform 	{{1 0 0 0 }{0 1 0 0 }{0 0 1 0 }{0 0 0 1 }}" << endl;
	os << "mesh_data_update 	{"    << endl;   
}

inline
void output_update_mesh_end(ostream& os, str_ptr name)
{
   os << "}" << endl; //mesh_data
   os << "}" << endl; //TEXBODY
   os << "}" << endl; //UPDATE_GEOM
}

inline
void output_mesh_start(ostream& os, str_ptr name)
{
   os << "TEXBODY {"       << endl; 
	os << "name " << name   << endl; 
	os << "xform 	{{1 0 0 0 }{0 1 0 0 }{0 0 1 0 }{0 0 0 1 }}" << endl;
	os << "mesh_data 	{"    << endl;   
}
inline
void output_mesh_end(ostream& os, str_ptr name)
{
   os << "}" << endl;
   //os << "mesh_data_file 	{ NULL_STR }" << endl;
   os << "}" << endl;
   os << "CREATE 	{  " << name << "  }" << endl; 
}

inline
void output_end_jot_file(ostream& os, str_ptr name, int frame_num)
{
   os << "CHNG_VIEW 	{" << endl;
	os << "VIEW 	{ " << endl;
	os << "  view_animator 	{ " << endl;
	os <<	"  Animator 	{ " << endl;
	os <<	"		fps 	12 " << endl;
	os <<	"		start_frame 	0 " << endl;
	os <<	"		end_frame 	"<< frame_num-1 << endl;
	os <<	"		name 	{ " << name << "  } " << endl;
	os <<	"		} } " << endl;
   os << "}" << endl;
   os << "}" << endl;
}

int
main(int argc, char *argv[])
{
   if (argc < 4) {
      err_msg("usage: %s [jot path] [filename] [obj path] [objFile1.obj] [objFile2.obj] [objFile3.obj] ... [objFileN.obj]", argv[0]);
      return 1;
   }
   int total_frames = argc-4; 
   str_ptr jot_path(argv[1]);
   str_ptr jot_filename(argv[2]);
   str_ptr obj_path(argv[3]);
   
   for(int i=0; i < total_frames; ++i)
   {  
      char bname[1024];           
      sprintf(bname, "%s%s[%05d]", **jot_path,**jot_filename, i);

      str_ptr in_name  = obj_path + str_ptr(argv[i+4]);      
      str_ptr jot_out_name = str_ptr(bname) + ".jot"; 
      
      str_ptr jot_out_name_first = jot_path + jot_filename + ".jot"; 
            
      ifstream in(**in_name);
      if(!in){
         cerr << in_name << " file is not found" << endl;
         return 1;
      }     
      ofstream out_jot(**jot_out_name, ios::out);
      if(!out_jot){
         cerr << jot_out_name << " file is not found" << endl;
         return 1;
      }

      ARRAY<LMESHptr> meshes;
      read_obj(meshes, in);      
      
      
      if(i == 0){
         //First Frames
         ofstream out_jot_first(**jot_out_name_first, ios::out);
         if(!out_jot_first){
            cerr << jot_out_name_first << " file is not found" << endl;
            return 1;
         }
         out_jot_first << "#jot" << endl;
         for(int j=0; j < meshes.num(); ++j)
         {  
            str_ptr m_name = str_ptr("mesh")+str_ptr(j);
            output_mesh_start(out_jot_first, m_name);
            //meshes[j]->recenter();
            meshes[j]->write_stream(out_jot_first);
            output_mesh_end(out_jot_first, m_name);           
         }
         output_end_jot_file(out_jot_first, jot_filename, total_frames);
      } 

      // Update Frames
      out_jot << "#jot" << endl;
      for(int j=0; j < meshes.num(); ++j)
      {  
         str_ptr m_name = str_ptr("mesh")+str_ptr(j);
         output_update_mesh_start(out_jot, m_name);
         //meshes[j]->recenter();
         IOManager::add_state(IOManager::STATE_PARTIAL_SAVE);
         meshes[j]->write_stream(out_jot);
         output_update_mesh_end(out_jot, m_name);           
      }       
      cerr << in_name << " to " << jot_out_name << endl;
   }
   return 0;
}



// end of file obj2sm.C
