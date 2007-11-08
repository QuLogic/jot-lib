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
 *  \file mesh_select_cmd.C
 *  \brief Contains the implementation of the MESH_SELECT_CMD class.
 *
 *  \sa mesh_select_cmd.H
 *
 */

#include "mesh/bvert.H"
#include "mesh/bedge.H"
#include "mesh/bface.H"
#include "mesh/bmesh.H"
#include "mesh/mesh_global.H"

#include "mesh_select_cmd.H"

MESH_SELECT_CMD::MESH_SELECT_CMD(Bvert *v)
{
   
   vlist += v;
   
}

MESH_SELECT_CMD::MESH_SELECT_CMD(Bedge *e)
{
   
   elist += e;
   
}

MESH_SELECT_CMD::MESH_SELECT_CMD(Bface *f)
{
   
   flist += f;
   
}

MESH_SELECT_CMD::MESH_SELECT_CMD(const Bvert_list &vl)
   : vlist(vl)
{
   
}

MESH_SELECT_CMD::MESH_SELECT_CMD(const Bedge_list &el)
   : elist(el)
{
   
}

MESH_SELECT_CMD::MESH_SELECT_CMD(const Bface_list &fl)
   : flist(fl)
{
   
}

MESH_SELECT_CMD::MESH_SELECT_CMD(const Bvert_list &vl, const Bedge_list &el)
   : vlist(vl), elist(el)
{
   
}

MESH_SELECT_CMD::MESH_SELECT_CMD(const Bvert_list &vl, const Bface_list &fl)
   : vlist(vl), flist(fl)
{
   
}

MESH_SELECT_CMD::MESH_SELECT_CMD(const Bedge_list &el, const Bface_list &fl)
   : elist(el), flist(fl)
{
   
}

MESH_SELECT_CMD::MESH_SELECT_CMD(const Bvert_list &vl,
                                 const Bedge_list &el,
                                 const Bface_list &fl)
   : vlist(vl), elist(el), flist(fl)
{
   
}

bool
MESH_SELECT_CMD::doit()
{
   
   if(!vlist.empty())
      MeshGlobal::select(vlist);
   
   if(!elist.empty())
      MeshGlobal::select(elist);
   
   if(!flist.empty())
      MeshGlobal::select(flist);
   
   return COMMAND::doit();
   
}

bool
MESH_SELECT_CMD::undoit()
{
   
   if(!vlist.empty())
      MeshGlobal::deselect(vlist);
   
   if(!elist.empty())
      MeshGlobal::deselect(elist);
   
   if(!flist.empty())
      MeshGlobal::deselect(flist);
   
   return COMMAND::undoit();
   
}

MESH_DESELECT_ALL_CMD::MESH_DESELECT_ALL_CMD()
   : MESH_DESELECT_CMD(MeshGlobal::selected_verts(),
                       MeshGlobal::selected_edges(),
                       MeshGlobal::selected_faces())
{
   
}
