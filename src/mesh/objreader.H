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
#ifndef OBJ_READER_H_IS_INCLUDED
#define OBJ_READER_H_IS_INCLUDED

/*!
 *  \file objreader.H
 *  \brief Contains the definition of the OBJReader class.  A class for reading
 *  .obj files into meshes.
 *
 *  \sa objreader.C
 *
 */

#include <iostream>

class BMESH;

class OBJReaderImpl;

/*!
 *  \brief A class that can read .obj files and create Jot meshes from them.
 *
 */
class OBJReader {

 public:
      
   OBJReader();
      
   ~OBJReader();
      
   //! \brief Attempt to read the contents of a .obj file from the stream
   //! \p in.
   bool read(std::istream &in);
      
   //! \brief Create a new BMESH object containing the data read in with the
   //! read function.
   BMESH *get_mesh() const;
      
   //! \brief Clear the given mesh object and fill it with the data read in
   //! with the read function.
   void get_mesh(BMESH *mesh) const;
      
   unsigned long get_num_vertices() const;
   unsigned long get_num_texcoords() const;
   unsigned long get_num_normals() const;
   unsigned long get_num_faces() const;
   unsigned long get_num_materials() const;
   unsigned long get_num_material_libs() const;
      
 private:
      
   OBJReaderImpl *impl;

};

#endif // OBJ_READER_H_IS_INCLUDED
