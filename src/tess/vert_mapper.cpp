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
#include "std/config.H"
#include "vert_mapper.H"

static bool debug = Config::get_var_bool("DEBUG_VERT_MAPPER",false);

VertMapper::VertMapper(bool dual) :
   _dual(dual)
{
}

VertMapper::VertMapper(CBvert_list& A, CBvert_list& B, bool dual) :
   _dual(dual)
{
   set(A,B);
}

VertMapper::VertMapper(CVertMapper& mapper) :
   _dual(false)
{
   *this = mapper;
}

VertMapper&
VertMapper::operator=(CVertMapper& mapper)
{
   if (&mapper != this) {
      _dual = mapper._dual;
      set(mapper.A(), mapper.B());
   }
   return *this;
}

bool
VertMapper::set(CBvert_list& A, CBvert_list& B)
{
   if (A.size() != B.size()) {
      err_msg("VertMapper::set: error: unequal sizes: %d, %d", A.size(), B.size());
      return false;
   }
   if (A.has_duplicates()) {
      err_msg("VertMapper::set: error: duplicates in A");
      return false;
   }
   if (_dual && B.has_duplicates()) {
      err_msg("VertMapper::set: error: duplicates in B");
      return false;
   }
   _a_verts = A;
   _b_verts = B;
   return true;
}

bool
VertMapper::add(CBvert_list& A, CBvert_list& B)
{
   if (A.size() != B.size()) {
      err_msg("VertMapper::add: error: unequal sizes: %d, %d", A.size(), B.size());
      return false;
   }
   if (A.has_duplicates()) {
      err_msg("VertMapper::add: error: duplicates in A");
      return false;
   }
   if (_dual && B.has_duplicates()) {
      err_msg("VertMapper::add: error: duplicates in B");
      return false;
   }
   _a_verts.append(A);
   _b_verts.append(B);
   return true;
}

bool
VertMapper::add(Bvert* a, Bvert* b) 
{
   // Add a new pair of corresponding vertices:
   if (!(a && b)) {
      err_msg("VertMapper::add: error: null vert(s)");
      return false;
   }
   if (std::find(_a_verts.begin(), _a_verts.end(), a) != _a_verts.end()) {
      err_msg("VertMapper::add: error: 'from' vert is duplicate");
      return false;
   }
   if (_dual && std::find(_b_verts.begin(), _b_verts.end(), b) != _b_verts.end()) {
      err_msg("VertMapper::add: error: 'to' vert is duplicate");
      return false;
   }
   _a_verts.push_back(a);
   _b_verts.push_back(b);
   return true;
}

Bvert_list
VertMapper::map_verts(CBvert_list& in, map_vert_meth_t fn) const
{
   // fn allows us to map A --> B or vice versa

   Bvert_list ret;
   if (in.empty())
      return ret;    // "success"
   for (Bvert_list::size_type i=0; i<in.size(); i++) {
      Bvert* v = (this->*fn)(in[i]);
      if (v) {
         ret.push_back(v);
      } else {
         return Bvert_list();
      }
   }
   return ret;
}

Bedge_list
VertMapper::map_edges(CBedge_list& in, map_edge_meth_t fn) const
{
   // fn allows us to map A --> B or vice versa

   Bedge_list ret;
   if (in.empty())
      return ret;    // "success"
   for (Bedge_list::size_type i=0; i<in.size(); i++) {
      Bedge* e = (this->*fn)(in[i]);
      if (e) {
         ret.push_back(e);
      }
   }
   return ret;
}

Bface_list
VertMapper::map_faces(CBface_list& in, map_face_meth_t fn) const
{
   // fn allows us to map A --> B or vice versa

   Bface_list ret;
   if (in.empty())
      return ret;    // "success"
   for (Bface_list::size_type i=0; i<in.size(); i++) {
      Bface* f = (this->*fn)(in[i]);
      if (f) {
         ret.push_back(f);
      }
   }
   return ret;
}

EdgeStrip
VertMapper::map_strip(
   CEdgeStrip& in, 
   map_vert_meth_t vfn,
   map_edge_meth_t efn) const 
{
   // fn allows us to map A --> B or vice versa

   EdgeStrip ret;
   if (in.empty())
      return ret;      // "success"
   for (int i=0; i<in.num(); i++) {
      Bvert* v = (this->*vfn)(in.vert(i));
      Bedge* e = (this->*efn)(in.edge(i));
      if (v && e) {
         ret.add(v,e);
      }
   }
   return ret;
}

class MappableEdgeFilter : public SimplexFilter {
   CVertMapper& _mapper;
 public:
   MappableEdgeFilter(CVertMapper& m) : _mapper(m) {}
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && _mapper.a_to_b((Bedge*)s) != nullptr;
   }
};

Bedge_list
VertMapper::a_edges() const
{
   return _a_verts.get_inner_edges().filter(MappableEdgeFilter(*this));
   
}

// end of file vert_mapper.C
