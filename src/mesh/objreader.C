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
/*!
 *  \file objreader.C
 *  \brief Contains the implementation of the OBJReader class.
 *
 *  \sa objreader.H
 *
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <limits>
#include <algorithm>

using namespace std;

#include "geom/texturegl.H"
#include "mesh/bmesh.H"
#include "mesh/patch.H"
#include "mlib/points.H"
#include "std/config.H"

using namespace mlib;

#include "objreader.H"

static bool debug = Config::get_var_bool("DEBUG_OBJ_READER",false);

/*!
 *  \brief Ignore all the characters in stream \p in until the character \p delim
 *  is encountered.  By default this ignores all characters until the next
 *  new-line is encountered, effectively "eating" the rest of the line.
 *
 */
inline void
eat_line(istream& in, int delim = '\n')
{
   in.ignore(numeric_limits<std::streamsize>::max(), delim);
}

//----------------------------------------------------------------------------//

/*!
 *  \brief A semi-generic parser class for reading files formatted similiarly to
 *  .obj and .mtl files.
 *
 *  A parser class that reads files that are formatted such that each line starts
 *  with an identifying token that is followed by data.  Blank lines, comments
 *  starting with the '#' character, and line continuations with the '\' character
 *  are also allowed.
 *
 *  Exactly what actions are performed for each token are customizable using
 *  registered callback functions.
 *
 *  The class is templated on a type which is passed as an additional parameter
 *  to each callback function.  Typically, this type will be the type of a class
 *  which has all of the callback functions as static members.
 *
 */
template <typename T>
class Reader {
   
 public:
      
   Reader()
      : read_succeeded(false)
      { }
      
   bool read(istream &in, T *object);
      
   void add_reader_function(const string &name,
                            bool (*func)(istream&, T*));
      
   bool was_successful() const
      { return read_succeeded; }
      
 private:
      
   bool read_succeeded;
      
   typedef map<string, bool (*)(istream&, T*)> reader_map_t;
   reader_map_t reader_map;
      
   //! \name Parsing Functions
   //@{
      
   string get_next_token(istream &in);
      
   void ignore_element(istream &in);
      
   void extract_line(istream &in, string &line);
      
   //@}
   
};

template <typename T>
bool
Reader<T>::read(istream &in, T *object)
{
   
   // Reset reader state:
   
   read_succeeded = true;
   
   // Read stream:
   
   string token;
   string line;
   
   typename reader_map_t::iterator reader_itor;
   
   while(read_succeeded && !(token = get_next_token(in)).empty()){
      
      reader_itor = reader_map.find(token);
      
      if(reader_itor == reader_map.end()){
         
         // Skip the element for this token if we don't know how to read it:
         ignore_element(in);
         
         cerr << "Reader::read() - Warning:  Skipped element with token "
              << token << endl;
         
      } else {
         
         extract_line(in, line);
         
         istringstream line_stream(line);
         
         read_succeeded = read_succeeded &&
            (*(reader_itor->second))(line_stream, object);
         
      }
      
   }
   
   if(!read_succeeded){
      
      cerr << "Reader::read() - Error:  Failed while reading element with token "
           << token << endl;
      
   }
   
   return read_succeeded;
   
}

template <typename T>
void
Reader<T>::add_reader_function(const string &name,
                               bool (*func)(istream&, T*))
{
   
   reader_map[name] = func;
   
}

/*!
 *  \brief Skips over comments until it finds a whitespace delimited token.
 *  \return The token if one is found.  Otherwise the empty string.
 *
 */
template <typename T>
string
Reader<T>::get_next_token(istream &in)
{
   
   string buf;
   
   // Get the next whitespace delimited token:
   while(in >> buf){
      
      // See if the token is actually the beginning of a comment:
      if(buf[0] == '#')
         eat_line(in);  // Discard the rest of the line (it's a comment)
      else
         break;         // If we don't have a comment, we're done
      
   }
   
   // Return the empty string if the stream went bad before we found anything:
   return in ? buf : string();
   
}

/*!
 *  \brief Skips over the next element in the stream \p in including any comments
 *  and line continuations.
 *
 */
template <typename T>
void
Reader<T>::ignore_element(istream &in)
{
   
   char c;
   
   while(in.get(c)){
      
      if(c == '\\')
         eat_line(in);
      else if(c == '\n')
         break;
      
   }
   
}

/*!
 *  \brief Extracts an entire line from the stream \p in and puts it in the
 *  string \p line.  This function will collapse multiple lines into one if there
 *  are line continuations.
 *
 */
template <typename T>
void
Reader<T>::extract_line(istream &in, string &line)
{
   
   line.clear();
   
   string buf;
   
   string::size_type continuation_loc;
   
   while(getline(in, buf)){
      
      if((continuation_loc = buf.rfind('\\')) != string::npos){
         
         buf.resize(continuation_loc);
         
         line.push_back(' ');
         line.append(buf);
         
      } else {
         
         line.append(buf);         
         break;
         
      }
      
   }
   
}

//----------------------------------------------------------------------------//

/*!
 *  \brief A class to represent a face in a .obj file.
 *
 */
class OBJFace {
   
 public:
      
   OBJFace(long mtl_index_in = -1)
      : mtl_index(mtl_index_in)
      { }
      
   unsigned long num_vertices() const { return vertices.size(); }
   unsigned long num_texcoords() const { return texcoords.size(); }
   unsigned long num_normals() const { return normals.size(); }
      
   unsigned long get_vertex_idx(unsigned long idx) const
      { return vertices[idx]; }
   unsigned long get_texcoord_idx(unsigned long idx) const
      { return texcoords[idx]; }
   unsigned long get_normal_idx(unsigned long idx) const
      { return normals[idx]; }
      
   bool has_texcoords() const { return num_texcoords() > 0; }
   bool has_normals() const { return num_normals() > 0; }
      
   bool add_vertex(long vertex, unsigned long total_vertices);
   bool add_texcoord(long texcoord, unsigned long total_texcoords);
   bool add_normal(long normal, unsigned long total_normals);
      
   //! \brief Checks ot see if this face has consistent numbers of vertices,
   //! texture coordinates, and normals.
   bool good() const;
      
 private:
      
   bool process_index(long index, unsigned long max_val,
                      vector<unsigned long> &index_list);
      
   long mtl_index;
      
   vector<unsigned long> vertices;
   vector<unsigned long> texcoords;
   vector<unsigned long> normals;
   
};

bool
OBJFace::add_vertex(long vertex, unsigned long total_vertices)
{
   
   return process_index(vertex, total_vertices, vertices);
   
}

bool
OBJFace::add_texcoord(long texcoord, unsigned long total_texcoords)
{
   
   return process_index(texcoord, total_texcoords, texcoords);
   
}

bool
OBJFace::add_normal(long normal, unsigned long total_normals)
{
   
   return process_index(normal, total_normals, normals);
   
}

bool
OBJFace::good() const
{
   
   return ((vertices.size() > 2) &&
           ((vertices.size() == texcoords.size()) || texcoords.empty()) &&
           ((vertices.size() == normals.size())   || normals.empty()));
   
}

/*!
 *  \brief Process a vertex position, texture coordinate or normal index to see
 *  if it is valid and, if so, to normalize it and add it to the given vector.
 *
 *  \param[in] index The index to check and add (if the check succeeds).
 *  \param[in] max_val The maximum value that the index can take (this should
 *  be equal to the number of elements (vertices, texture coordinates, or normals)
 *  that have been read in the .obj file at the time this face is read.
 *  \param[in,out] index_list The vector of indices to add the vertex to if it
 *  is valid.
 *
 *  \return \c true if the index is valid and \c false otherwise.
 *
 */
bool
OBJFace::process_index(long index, unsigned long max_val,
                       vector<unsigned long> &index_list)
{
   
   // Check for invalid cases:
   if((index == 0) ||
      (index > static_cast<long>(max_val)) ||
      (index < -static_cast<long>(max_val))){
         
      return false;
      
   }
      
   if(index < 0)
      index = max_val + index + 1;
   
   index_list.push_back(static_cast<unsigned long>(index));
   
   return true;
   
}

//----------------------------------------------------------------------------//

/*!
 *  \brief A class to represent a material in a .obj file.
 *
 */
class OBJMtl {
   
 public:
      
   enum color_component_t {COLOR_RED = 0, COLOR_GREEN = 1, COLOR_BLUE = 2};
      
   OBJMtl(const string &mtl_name = "",
          double ambt_r = 0.2, double ambt_g = 0.2, double ambt_b = 0.2,
          double diff_r = 0.8, double diff_g = 0.8, double diff_b = 0.8,
          double spec_r = 1.0, double spec_g = 1.0, double spec_b = 1.0,
          double trans = 1.0, double shin = 0.0, int illum = 1,
          const string &texture_color_diffuse_filename = "");
      
   const string &get_name() const
      { return name; }
      
   double get_ambient(color_component_t comp) const
      { return color_ambient[comp]; }
      
   void set_ambient(color_component_t comp, double val)
      { color_ambient[comp] = val; }
      
   void set_ambient(double ambient[3])
      { copy(ambient, ambient + 3, color_ambient); }
      
   double get_diffuse(color_component_t comp) const
      { return color_diffuse[comp]; }
      
   void set_diffuse(color_component_t comp, double val)
      { color_diffuse[comp] = val; }
      
   void set_diffuse(double diffuse[3])
      { copy(diffuse, diffuse + 3, color_diffuse); }
      
   double get_specular(color_component_t comp) const
      { return color_specular[comp]; }
      
   void set_specular(color_component_t comp, double val)
      { color_specular[comp] = val; }
      
   void set_specular(double specular[3])
      { copy(specular, specular + 3, color_specular); }
      
   double get_transparency() const
      { return transparency; }
      
   void set_transparency(double trans)
      { transparency = trans; }
      
   double get_shininess() const
      { return shininess; }
      
   void set_shininess(double shin)
      { shininess = shin; }
      
   int get_illumination_model() const
      { return illumination_model; }
      
   void set_illumination_model(int illum)
      { illumination_model = illum; }
      
   const string &get_diffuse_texture_map() const
      { return texmap_color_diffuse; }
      
   void set_diffuse_texture_map(const string &texmap)
      { texmap_color_diffuse = texmap; }
      
 private:
      
   string name;
      
   double color_ambient[3];
   double color_diffuse[3];
   double color_specular[3];
      
   double transparency;
      
   double shininess;
      
   int illumination_model;
      
   string texmap_color_diffuse;
   
};

OBJMtl::OBJMtl(const string &mtl_name,
               double ambt_r, double ambt_g, double ambt_b,
               double diff_r, double diff_g, double diff_b,
               double spec_r, double spec_g, double spec_b,
               double trans, double shin, int illum,
               const string &texture_color_diffuse_filename)
   : name(mtl_name), transparency(trans), shininess(shin),
     illumination_model(illum),
     texmap_color_diffuse(texture_color_diffuse_filename)
{
   
   color_ambient[0] = ambt_r;
   color_ambient[1] = ambt_g;
   color_ambient[2] = ambt_b;
   
   color_diffuse[0] = diff_r;
   color_diffuse[1] = diff_g;
   color_diffuse[2] = diff_b;
   
   color_specular[0] = spec_r;
   color_specular[1] = spec_g;
   color_specular[2] = spec_b;
   
}

//----------------------------------------------------------------------------//

/*!
 *  \brief A class that can read .mtl files and create OBJMtl objects from them.
 *
 */
class MTLReader {
   
 public:
      
   MTLReader();
      
   bool read(istream &in);
      
   bool has_material(const string &mtl_name) const
      { return mtl_name2id.count(mtl_name) > 0; }
      
   const OBJMtl &get_material(const string &mtl_name) const
      { assert(has_material(mtl_name));
      return materials[mtl_name2id.find(mtl_name)->second]; }
      
 private:
      
   Reader<MTLReader> reader;
      
   //! \name Element Reading Functions
   //@{
      
   static bool read_newmtl(istream &in, MTLReader *self);
   static bool read_Ka(istream &in, MTLReader *self);
   static bool read_Kd(istream &in, MTLReader *self);
   static bool read_Ks(istream &in, MTLReader *self);
   static bool read_d(istream &in, MTLReader *self);
   static bool read_Tr(istream &in, MTLReader *self);
   static bool read_Ns(istream &in, MTLReader *self);
   static bool read_illum(istream &in, MTLReader *self);
   static bool read_map_Kd(istream &in, MTLReader *self);
      
   bool exists_current_material();
      
   //@}
      
   //! \name Reader State
   //@{
      
   vector<OBJMtl> materials;
      
   typedef map<string, long> mtl_name2id_map_t;
   mtl_name2id_map_t mtl_name2id;
      
   //@}
   
};

MTLReader::MTLReader()
{
   
   reader.add_reader_function("newmtl", &MTLReader::read_newmtl);
   reader.add_reader_function("Ka", &MTLReader::read_Ka);
   reader.add_reader_function("Kd", &MTLReader::read_Kd);
   reader.add_reader_function("Ks", &MTLReader::read_Ks);
   reader.add_reader_function("d", &MTLReader::read_d);
   reader.add_reader_function("Tr", &MTLReader::read_Tr);
   reader.add_reader_function("Ns", &MTLReader::read_Ns);
   reader.add_reader_function("illum", &MTLReader::read_illum);
   reader.add_reader_function("map_Kd", &MTLReader::read_map_Kd);
   
}

bool
MTLReader::read(istream &in)
{
   
   // Clear reader state:
   
   materials.clear();
   mtl_name2id.clear();
   
   return reader.read(in, this);
   
}

bool
MTLReader::read_newmtl(istream &in, MTLReader *self)
{
   
   string name;
   
   if(!(in >> name))
      return false;
   
   // Make sure that the new material name hasn't been used before:
   if(self->mtl_name2id.count(name) != 0)
      return false;
   
   self->materials.push_back(OBJMtl(name));
   
   self->mtl_name2id[name] = self->materials.size() - 1;
   
   return true;
   
}

bool
MTLReader::read_Ka(istream &in, MTLReader *self)
{
   
   if(!self->exists_current_material())
      return false;
   
   double ambient[3];
   
   if(!(in >> ambient[0] >> ambient[1] >> ambient[2]))
      return false;
   
   self->materials.back().set_ambient(ambient);
   
   return true;
   
}

bool
MTLReader::read_Kd(istream &in, MTLReader *self)
{
   
   if(!self->exists_current_material())
      return false;
   
   double diffuse[3];
   
   if(!(in >> diffuse[0] >> diffuse[1] >> diffuse[2]))
      return false;
   
   self->materials.back().set_diffuse(diffuse);
   
   return true;
   
}

bool
MTLReader::read_Ks(istream &in, MTLReader *self)
{
   
   if(!self->exists_current_material())
      return false;
   
   double specular[3];
   
   if(!(in >> specular[0] >> specular[1] >> specular[2]))
      return false;
   
   self->materials.back().set_specular(specular);
   
   return true;
   
}

bool
MTLReader::read_d(istream &in, MTLReader *self)
{
   
   return read_Tr(in, self);
   
}

bool
MTLReader::read_Tr(istream &in, MTLReader *self)
{
   
   if(!self->exists_current_material())
      return false;
   
   double trans;
   
   if(!(in >> trans))
      return false;
   
   self->materials.back().set_transparency(trans);
   
   return true;
   
}

bool
MTLReader::read_Ns(istream &in, MTLReader *self)
{
   
   if(!self->exists_current_material())
      return false;
   
   double shin;
   
   if(!(in >> shin))
      return false;
   
   self->materials.back().set_shininess(shin);
   
   return true;
   
}

bool
MTLReader::read_illum(istream &in, MTLReader *self)
{
   
   if(!self->exists_current_material())
      return false;
   
   int illum;
   
   if(!(in >> illum))
      return false;
   
   self->materials.back().set_illumination_model(illum);
   
   return true;
   
}

bool
MTLReader::read_map_Kd(istream &in, MTLReader *self)
{
   
   if(!self->exists_current_material())
      return false;
   
   string texmap;
   
   if(!(in >> texmap))
      return false;
   
   self->materials.back().set_diffuse_texture_map(texmap);
   
   return true;
   
}

bool
MTLReader::exists_current_material()
{
   
   return materials.size() > 0;
   
}

//----------------------------------------------------------------------------//

/*!
 *  \brief The implementation of the OBJReader class.
 *
 */
class OBJReaderImpl {

 public:
      
   OBJReaderImpl();
      
   bool read(istream &in);
      
   BMESH *get_mesh() const;
      
   void get_mesh(BMESH *mesh) const;
      
   friend class OBJReader;
      
 private:
      
   Reader<OBJReaderImpl> reader;
      
   bool read_mtl_files();
      
   //! \name Element Reading Functions
   //@{
      
   static bool read_v(istream &in, OBJReaderImpl *self);
   static bool read_vn(istream &in, OBJReaderImpl *self);
   static bool read_vt(istream &in, OBJReaderImpl *self);
   static bool read_f(istream &in, OBJReaderImpl *self);
   static bool read_mtllib(istream &in, OBJReaderImpl *self);
   static bool read_usemtl(istream &in, OBJReaderImpl *self);
      
   //@}
      
   //! \name Reader State
   //@{
      
   bool read_succeeded;
      
   vector<Wpt> vertices;
   vector<Wvec> normals;
   vector<UVpt> texcoords;
   vector<OBJFace> faces;
      
   vector<string> mtl_files;
   vector<OBJMtl> materials;
   vector< vector<long> > material_faces;
      
   typedef map<string, long> mtl_name2id_map_t;
   mtl_name2id_map_t mtl_name2id;
      
   long current_material;
      
   //@}
      
   //! \name Mesh Building Functions
   //@{
      
   void add_vertices(BMESH *mesh) const;
      
   void add_patches(BMESH *mesh) const;

   void set_vert_normals(BMESH *mesh) const;
      
   void add_face(BMESH *mesh, Patch *patch, unsigned long index) const;
      
   void add_tri(BMESH *mesh, Patch *patch, const OBJFace &face,
                unsigned long idx0, unsigned long idx1, unsigned long idx2) const;
      
   void add_creases(BMESH *mesh) const;
      
   //@}
      
   //! \name Mesh Building State:
   //@{
      
   //! Lookup .obj face index from mesh face index:
   mutable vector<unsigned long> mesh_faces2obj_faces;
      
   //@}

};

OBJReaderImpl::OBJReaderImpl()
   : read_succeeded(false), current_material(-1)
{
   
   // Initialize reader functions pointers:
   
   reader.add_reader_function("v", &OBJReaderImpl::read_v);
   reader.add_reader_function("vn", &OBJReaderImpl::read_vn);
   reader.add_reader_function("vt", &OBJReaderImpl::read_vt);
   reader.add_reader_function("f", &OBJReaderImpl::read_f);
   reader.add_reader_function("mtllib", &OBJReaderImpl::read_mtllib);
   reader.add_reader_function("usemtl", &OBJReaderImpl::read_usemtl);
   
}

bool
OBJReaderImpl::read(istream &in)
{
   
   // Reset reader state:
   
   read_succeeded = false;
   
   vertices.clear();
   normals.clear();
   texcoords.clear();
   faces.clear();
   mtl_files.clear();
   materials.clear();
   mtl_name2id.clear();
   
   // Create default material (note that it is not in the name to id map because
   // the default material cannot be referenced by name):
   materials.push_back(OBJMtl());
   material_faces.push_back(vector<long>());
   current_material = 0;
   
   read_succeeded = reader.read(in, this);
   
   // Make sure we have a face list for each material:
   assert(materials.size() == material_faces.size());
   
   read_succeeded = read_succeeded && read_mtl_files();
   
   return read_succeeded;
   
}
      
BMESH*
OBJReaderImpl::get_mesh() const
{
   
   if(!read_succeeded) return 0;
   
   BMESH *mesh = new BMESH;
   
   get_mesh(mesh);
   
   return mesh;
   
}

void
OBJReaderImpl::get_mesh(BMESH *mesh) const
{
   
   // Do nothing if the read didn't succeed:
   if(!read_succeeded)
      return;
   
   // Clear the mesh's current data:
   mesh->delete_elements();
   
   // Clear mesh builder state:
   mesh_faces2obj_faces.clear();
   
   // Fill the mesh with the new data:
   
   add_vertices(mesh);
   
   add_patches(mesh);
   
   add_creases(mesh);

   set_vert_normals(mesh);

   mesh->changed();
}

bool
OBJReaderImpl::read_mtl_files()
{
   
   if(!read_succeeded)
      return false;
   
   MTLReader mtl_reader;
   
   for(unsigned long i = 0; i < mtl_files.size(); ++i){
      
      ifstream mtl_stream(mtl_files[i].c_str());
      
      if(!mtl_stream.is_open()){
         
         cerr << "OBJReader::read_mtl_files() - Warning:  Couldn't open material file "
              << mtl_files[i] << "!" << endl;
         
         continue;
         
      }
      
      if(!mtl_reader.read(mtl_stream))
         return false;
      
      for(unsigned long j = 0; j < materials.size(); ++j){
         
         if(mtl_reader.has_material(materials[j].get_name())){
            
            materials[j] = mtl_reader.get_material(materials[j].get_name());
            
         }
         
      }
      
   }
   
   return true;
   
}

/*!
 *  \brief Reads the data for a vertex position element in a .obj file (not
 *  including the starting 'v' token).
 *
 */
bool
OBJReaderImpl::read_v(istream &in, OBJReaderImpl *self)
{
   
   double vals[3];
   
   if(!(in >> vals[0] >> vals[1] >> vals[2]))
      return false;
   
   self->vertices.push_back(Wpt(vals[0], vals[1], vals[2]));
   
   return true;
   
}

/*!
 *  \brief Reads the data for a vertex normal element in a .obj file (not
 *  including the starting 'vn' token).
 *
 */
bool
OBJReaderImpl::read_vn(istream &in, OBJReaderImpl *self)
{
   
   double vals[3];
   
   if(!(in >> vals[0] >> vals[1] >> vals[2]))
      return false;
   
   self->normals.push_back(Wvec(vals[0], vals[1], vals[2]));
   
   return true;
   
}

/*!
 *  \brief Reads the data for a vertex texture coordinate element in a .obj file
 *  (not including the starting 'vt' token).
 *
 */
bool
OBJReaderImpl::read_vt(istream &in, OBJReaderImpl *self)
{
   
   double vals[2];
   
   if(!(in >> vals[0] >> vals[1]))
      return false;
   
   self->texcoords.push_back(UVpt(vals[0], vals[1]));
   
   return true;
   
}

/*!
 *  \brief Reads the data for a face element in a .obj file (not including the
 *  starting 'f' token).
 *
 */
bool
OBJReaderImpl::read_f(istream &in, OBJReaderImpl *self)
{
   
   long pos, tcoord, norm;
   
   char slash;
   
   OBJFace face(self->current_material);
   
   while(in >> pos){
   
      face.add_vertex(pos, self->vertices.size());
      
      if(in.peek() == '/'){
         
         in >> slash;
         
         if(in.peek() != '/'){
            
            in >> tcoord;
            face.add_texcoord(tcoord, self->texcoords.size());
            
         }
            
         if(in.peek() == '/'){
            
            in >> slash >> norm;
            face.add_normal(norm, self->normals.size());
            
         }
         
      }
      
   }
   
   if(!face.good())
      return false;
   
   self->faces.push_back(face);
   self->material_faces[self->current_material].push_back(self->faces.size() - 1);
   
   return true;
   
}

/*!
 *  \brief Reads the data for a material library element in a .obj file (not
 *  including the starting 'mtllib' token).
 *
 */
bool
OBJReaderImpl::read_mtllib(istream &in, OBJReaderImpl *self)
{
   
   string buf;
   
   bool valid = false;
   
   while(in >> buf){
      
      if(buf[0] == '#')
         break; // Break out if we hit a comment
      
      self->mtl_files.push_back(buf);
      
      valid = true;
      
   }
   
   return valid;
   
}

/*!
 *  \brief Reads the data for a "use material" directive in a .obj file (not
 *  including the starting 'usemtl' token).
 *
 */
bool
OBJReaderImpl::read_usemtl(istream &in, OBJReaderImpl *self)
{
   
   string mtl_name;
   
   // Fail on failed extraction or if a comment is extracted:
   if(!(in >> mtl_name) || (mtl_name[0] == '#'))
      return false;
   
   mtl_name2id_map_t::iterator name2id_itor = self->mtl_name2id.find(mtl_name);
   
   // See if this is the first time this material has been used:
   if(name2id_itor == self->mtl_name2id.end()){
      
      self->materials.push_back(OBJMtl(mtl_name));
      self->material_faces.push_back(vector<long>());
      
      name2id_itor =
         self->mtl_name2id.insert(self->mtl_name2id.begin(),
                                  make_pair(mtl_name, self->materials.size() - 1));
      
   }
   
   self->current_material = name2id_itor->second;
   
   return true;
   
}

void
OBJReaderImpl::add_vertices(BMESH *mesh) const
{
   
   for(unsigned long i = 0; i < vertices.size(); ++i){
      
      mesh->add_vertex(vertices[i]);
      
   }
   
}
      
void
OBJReaderImpl::add_patches(BMESH *mesh) const
{
   err_adv(debug, "OBJReaderImpl::add_patches: %d materials", int(materials.size()));

   for(unsigned long i = 0; i < materials.size(); ++i){
      
      if(material_faces[i].size() == 0)
         continue;
      
      Patch *patch = mesh->new_patch();
      
      if(materials[i].get_name().size() > 0)
         patch->set_name(materials[i].get_name().c_str());

      // the following is wrong when there are no materials...
      // haven't tracked down what part is wrong (materials come out
      // blinding bright) so just commenting it out now. --sapo 2/19/2006
/*      
      patch->set_ambient_color(COLOR(materials[i].get_ambient(OBJMtl::COLOR_RED),
                                     materials[i].get_ambient(OBJMtl::COLOR_GREEN),
                                     materials[i].get_ambient(OBJMtl::COLOR_BLUE)));
      
      patch->set_color(COLOR(materials[i].get_diffuse(OBJMtl::COLOR_RED),
                             materials[i].get_diffuse(OBJMtl::COLOR_GREEN),
                             materials[i].get_diffuse(OBJMtl::COLOR_BLUE)));
      
      patch->set_specular_color(COLOR(materials[i].get_specular(OBJMtl::COLOR_RED),
                                      materials[i].get_specular(OBJMtl::COLOR_GREEN),
                                      materials[i].get_specular(OBJMtl::COLOR_BLUE)));
      
      patch->set_shininess(materials[i].get_shininess());
      
      patch->set_transp(materials[i].get_transparency());
      
      if(materials[i].get_diffuse_texture_map().size() > 0){
         
         patch->set_texture_file(materials[i].get_diffuse_texture_map().c_str());
         patch->set_texture(new TEXTUREgl(patch->texture_file()));
         
      }
*/      
      for(unsigned long j = 0; j < material_faces[i].size(); ++j)
         add_face(mesh, patch, material_faces[i][j]);
      
   }
}

void
OBJReaderImpl::set_vert_normals(BMESH *mesh) const
{
   assert(mesh);
   if (vertices.empty() || vertices.size() != normals.size()) {
      err_adv(debug, "OBJReaderImpl::set_vert_normals: skipping...");
      return;
   }
   err_adv(debug, "OBJReaderImpl::set_vert_normals: assigning normals to vertices");
   assert((unsigned long)(mesh->nverts()) == normals.size());
   for (unsigned long i=0; i<normals.size(); i++) {
      mesh->bv(int(i))->set_norm(normals[i]);
   }
}

void
OBJReaderImpl::add_face(BMESH *mesh, Patch *patch, unsigned long index) const
{
   
   assert(faces[index].good());

   for(unsigned long k = 2; k < faces[index].num_vertices(); ++k){
      
      add_tri(mesh, patch, faces[index], 0, k-1, k);
      mesh_faces2obj_faces.push_back(index);
      
   }
   
   // If the face is a quad, mark the quad diagonal as "weak":
   if (faces[index].num_vertices() == 4) {
      
      Bedge* e = lookup_edge(mesh->bv(faces[index].get_vertex_idx(0) - 1),
                             mesh->bv(faces[index].get_vertex_idx(2) - 1));
      assert(e);
      e->set_bit(Bedge::WEAK_BIT);
      
   }
   
}

/*!
 *  \brief Adds a triangle to BMESH \p mesh and Patch \p patch from the OBJFace
 *  \p face.  \p idx0, \p idx1 and \p idx2 are the indices (with respect to the
 *  face) of the three vertices that make up the triangle.
 *
 */
void
OBJReaderImpl::add_tri(BMESH *mesh, Patch *patch, const OBJFace &face,
                       unsigned long idx0, unsigned long idx1, unsigned long idx2) const
{
   
   // Make sure vertex indices are good:
   assert((face.get_vertex_idx(idx0) > 0) &&
          (face.get_vertex_idx(idx0) <= vertices.size()) &&
          (face.get_vertex_idx(idx1) > 0) &&
          (face.get_vertex_idx(idx1) <= vertices.size()) &&
          (face.get_vertex_idx(idx2) > 0) &&
          (face.get_vertex_idx(idx2) <= vertices.size()));
   
   // Make sure texture coordinate indices are good:
   assert(!face.has_texcoords() ||
          ((face.get_texcoord_idx(idx0) > 0) &&
           (face.get_texcoord_idx(idx0) <= texcoords.size()) &&
           (face.get_texcoord_idx(idx1) > 0) &&
           (face.get_texcoord_idx(idx1) <= texcoords.size()) &&
           (face.get_texcoord_idx(idx2) > 0) &&
           (face.get_texcoord_idx(idx2) <= texcoords.size())));
   
   // Make sure normal indices are good:
   assert(!face.has_normals() ||
          ((face.get_normal_idx(idx0) > 0) &&
           (face.get_normal_idx(idx0) <= normals.size()) &&
           (face.get_normal_idx(idx1) > 0) &&
           (face.get_normal_idx(idx1) <= normals.size()) &&
           (face.get_normal_idx(idx2) > 0) &&
           (face.get_normal_idx(idx2) <= normals.size())));
   
   if(face.has_texcoords()){
      
      mesh->add_face(face.get_vertex_idx(idx0) - 1,
                     face.get_vertex_idx(idx1) - 1,
                     face.get_vertex_idx(idx2) - 1,
                     texcoords[face.get_texcoord_idx(idx0) - 1],
                     texcoords[face.get_texcoord_idx(idx1) - 1],
                     texcoords[face.get_texcoord_idx(idx2) - 1],
                     patch);
      
   } else {
      
      mesh->add_face(face.get_vertex_idx(idx0) - 1,
                     face.get_vertex_idx(idx1) - 1,
                     face.get_vertex_idx(idx2) - 1,
                     patch);
      
   }
   
}

void
OBJReaderImpl::add_creases(BMESH *mesh) const
{
   
   for(int ei = 0; ei < mesh->nedges(); ++ei){
      
      Bedge *cur_edge = mesh->be(ei);
      
      // Skip boundary edges:
      if((cur_edge->f1() == 0) || (cur_edge->f2() == 0))
         continue;
      
      // Skip edges that inside a single obj face:
      if(mesh_faces2obj_faces[cur_edge->f1()->index()] ==
         mesh_faces2obj_faces[cur_edge->f2()->index()])
         continue;
      
      // Skip edges adjacent to faces that have no normals:
      if(!faces[mesh_faces2obj_faces[cur_edge->f1()->index()]].has_normals() ||
         !faces[mesh_faces2obj_faces[cur_edge->f2()->index()]].has_normals())
         continue;
      
      int vertex_indices[2];
      vertex_indices[0] = cur_edge->v1()->index();
      vertex_indices[1] = cur_edge->v2()->index();
      
      // Indexed by vertex, then by face:
      int normal_indices[2][2] = {{-1, -1}, {-1, -1}};
      
      for(int fi = 0; fi < 2; ++fi){
         
         unsigned long obj_face_idx = mesh_faces2obj_faces[cur_edge->f(fi + 1)->index()];
         
         for (uint vi = 0; vi < faces[obj_face_idx].num_vertices(); ++vi){
            
            for (int evi = 0; evi < 2; ++evi){
               
               if (vertex_indices[evi] ==
                   int(faces[obj_face_idx].get_vertex_idx(vi) - 1)) {
                  
                  normal_indices[evi][fi] =
                     faces[obj_face_idx].get_normal_idx(vi);
                  
               }
               
            }
         
         }
         
      }
      
      // Make sure all normal indices were set:
      assert(normal_indices[0][0] != -1);
      assert(normal_indices[0][1] != -1);
      assert(normal_indices[1][0] != -1);
      assert(normal_indices[1][1] != -1);
      
      if((normal_indices[0][0] != normal_indices[0][1]) ||
         (normal_indices[1][0] != normal_indices[1][1])){
         
         cur_edge->set_crease();
         
      }
      
   }
   
}

//----------------------------------------------------------------------------//

OBJReader::OBJReader()
   : impl(0)
{
   
   impl = new OBJReaderImpl();
   
}

OBJReader::~OBJReader()
{
   
   delete impl;
   
}
      
/*!
 *  \return \c true if .obj file data was read off the stream \p in successfully
 *  and \c false otherwise.
 *
 */
bool
OBJReader::read(istream &in)
{
   
   return impl->read(in);
   
}

BMESH*
OBJReader::get_mesh() const
{
   
   return impl->get_mesh();
   
}

void
OBJReader::get_mesh(BMESH *mesh) const
{
   
   impl->get_mesh(mesh);
   
}

//----------------------------------------------------------------------------//

unsigned long
OBJReader::get_num_vertices() const
{
   
   return impl->vertices.size();
   
}

unsigned long
OBJReader::get_num_texcoords() const
{
   
   return impl->texcoords.size();
   
}

unsigned long
OBJReader::get_num_normals() const
{
   
   return impl->normals.size();
   
}

unsigned long
OBJReader::get_num_faces() const
{
   
   return impl->faces.size();
   
}

unsigned long
OBJReader::get_num_materials() const
{
   
   return impl->materials.size();
   
}

unsigned long
OBJReader::get_num_material_libs() const
{
   
   return impl->mtl_files.size();
   
}
