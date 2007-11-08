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
 * obj2sm:
 *
 *   Convert .obj file to .sm file. 
 *
 *   Reads obj file from stdin, writes .sm file to stdout.
 *
 *   Usage: % obj2sm < model.obj > model.sm
 *
 *   Preliminary version 1/2005:
 *      Reads vertices, texture coordinates, and triangles.
 *      Ignores everything else.
 *
 *   TODO: read material info and create patches with 
 *         textures and colors.
 *
 ********************************************************************************/

/********************************************************************************
 * The descriptions of .obj and .mtl formats provided below are from:
 *
 *    http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/\
 *    directx9_c_Summer_04/directx/graphics/tutorialsandsamples/samples/\
 *    meshfromobj.asp
 *
 * See also: 
 *
 *    http://www.nada.kth.se/~asa/Ray/matlabobj.html#4    
 *    http://netghost.narod.ru/gff/graphics/summary/waveobj.htm
 *    http://astronomy.swin.edu.au/~pbourke/geomformats/obj/
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

static bool debug = Config::get_var_bool("DEBUG_OBJ2SM",false,true);

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
add_tri(LMESH* mesh, CUVpt_list& uvs, const vtn& v1, const vtn& v2, const vtn& v3)
{
   assert(mesh);
   assert(all_valid_indices(mesh->verts(), v1._v, v2._v, v3._v));

   if (all_valid_indices(uvs, v1._t, v2._t, v3._t)) {
      mesh->add_face(v1._v, v2._v, v3._v, uvs[v1._t], uvs[v2._t], uvs[v3._t]);
   } else {
      mesh->add_face(v1._v, v2._v, v3._v);
   }
}

inline void
add_poly(LMESH* mesh, CUVpt_list& uvs, const ARRAY<vtn>& vtns)
{
   assert(vtns.num() > 2);

   for (int k=2; k < vtns.num(); k++) {
      add_tri(mesh, uvs, vtns[0], vtns[k-1], vtns[k]);
   }
   // if it is a quad, mark the quad diagonal as "weak"
   if (vtns.num() == 4) {
      Bedge* e = lookup_edge(mesh->bv(vtns[0]._v), mesh->bv(vtns[2]._v));
      assert(e);
      e->set_bit(Bedge::WEAK_BIT);
   }
}

inline void
read_face(LMESH* mesh, CUVpt_list& uvs, istream& in)
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
   add_poly(mesh, uvs, vtns);
}

static LMESHptr
read_obj(istream& in)
{
   UVpt_list    uvs;

   LMESHptr mesh = new LMESH;

   string token;
   while (!(in.eof() || in.fail())) {
      in >> token;
      if (token == "v") {
         read_vert(mesh, in);
      } else if (token == "vt") {
         read_texcoord(uvs, in);
      } else if (token == "f") {
         read_face(mesh, uvs, in);
      } else {
         // can add more cases...
         skip_line(in);
      }
   }

   return mesh;
}

int
main(int argc, char *argv[])
{
   if (argc != 1) {
      err_msg("usage: %s < mesh.obj > mesh.sm", argv[0]);
      return 1;
   }

   LMESHptr mesh = read_obj(cin);
   if (!mesh)
      return 1;

   if (Config::get_var_bool("JOT_RECENTER"))
      mesh->recenter();

   mesh->write_stream(cout);

   return 0;
}

// end of file obj2sm.C
