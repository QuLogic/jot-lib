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
#ifndef MESH_SELECT_CMD_H_IS_INCLUDED
#define MESH_SELECT_CMD_H_IS_INCLUDED

/*!
 *  \file mesh_select_cmd.H
 *  \brief Contains the definition of the MESH_SELECT_CMD class.  A COMMAND class
 *  for selecting faces, edges and vertices on a mesh.
 *
 *  \sa mesh_select_cmd.C
 *
 */

#include "geom/command.H"
#include "mesh/bvert.H"
#include "mesh/bedge.H"
#include "mesh/bface.H"

MAKE_PTR_SUBC(MESH_SELECT_CMD,COMMAND);

/*!
 *  \brief A COMMAND class for selecting faces, edges and vertices on a mesh.
 *
 */
class MESH_SELECT_CMD : public COMMAND {

   public:
      
      //! \name Constructors
      //@{
      
      MESH_SELECT_CMD(Bvert *v);
      MESH_SELECT_CMD(Bedge *e);
      MESH_SELECT_CMD(Bface *f);
      
      MESH_SELECT_CMD(const Bvert_list &vl);
      MESH_SELECT_CMD(const Bedge_list &el);
      MESH_SELECT_CMD(const Bface_list &fl);
      
      MESH_SELECT_CMD(const Bvert_list &vl, const Bedge_list &el);
      MESH_SELECT_CMD(const Bvert_list &vl, const Bface_list &fl);
      MESH_SELECT_CMD(const Bedge_list &el, const Bface_list &fl);
      
      MESH_SELECT_CMD(const Bvert_list &vl,
                      const Bedge_list &el,
                      const Bface_list &fl);
      
      //@}
      
      //! \name Run-Time Type Id
      //@{
   
      DEFINE_RTTI_METHODS3("MESH_SELECT_CMD", MESH_SELECT_CMD*, COMMAND, CCOMMAND*);
      
      //@}
   
      //! \name Command Virtual Methods
      //@{
   
      virtual bool doit();
      virtual bool undoit();
      
      //@}
      
   private:
      
      Bvert_list vlist;
      Bedge_list elist;
      Bface_list flist;

};

MAKE_PTR_SUBC(MESH_DESELECT_CMD, UNDO_CMD);

/*!
 *  \brief A COMMAND class for deselecting faces, edges and vertices on a mesh.
 *
 */
class MESH_DESELECT_CMD : public UNDO_CMD {
   
   public:

      //! \name Constructors
      //@{
   
      MESH_DESELECT_CMD(Bvert *v)
         : UNDO_CMD(new MESH_SELECT_CMD(v)) { }
      MESH_DESELECT_CMD(Bedge *e)
         : UNDO_CMD(new MESH_SELECT_CMD(e)) { }
      MESH_DESELECT_CMD(Bface *f)
         : UNDO_CMD(new MESH_SELECT_CMD(f)) { }
      
      MESH_DESELECT_CMD(const Bvert_list &vl)
         : UNDO_CMD(new MESH_SELECT_CMD(vl)) { }
      MESH_DESELECT_CMD(const Bedge_list &el)
         : UNDO_CMD(new MESH_SELECT_CMD(el)) { }
      MESH_DESELECT_CMD(const Bface_list &fl)
         : UNDO_CMD(new MESH_SELECT_CMD(fl)) { }
      
      MESH_DESELECT_CMD(const Bvert_list &vl, const Bedge_list &el)
         : UNDO_CMD(new MESH_SELECT_CMD(vl, el)) { }
      MESH_DESELECT_CMD(const Bvert_list &vl, const Bface_list &fl)
         : UNDO_CMD(new MESH_SELECT_CMD(vl, fl)) { }
      MESH_DESELECT_CMD(const Bedge_list &el, const Bface_list &fl)
         : UNDO_CMD(new MESH_SELECT_CMD(el, fl)) { }
      
      MESH_DESELECT_CMD(const Bvert_list &vl,
                        const Bedge_list &el,
                        const Bface_list &fl)
         : UNDO_CMD(new MESH_SELECT_CMD(vl, el, fl)) { }
      
      //@}
   
      //! \name Run-Time Type Id
      //@{
   
      DEFINE_RTTI_METHODS3("MESH_DESELECT_CMD", MESH_DESELECT_CMD*, UNDO_CMD, CCOMMAND*);
      
      //@}

};

MAKE_PTR_SUBC(MESH_DESELECT_ALL_CMD, MESH_DESELECT_CMD);

/*!
 *  \brief A COMMAND class for deselecting faces, edges and vertices on a mesh.
 *
 */
class MESH_DESELECT_ALL_CMD : public MESH_DESELECT_CMD {
   
   public:

      //! \name Constructors
      //@{
   
      MESH_DESELECT_ALL_CMD();
      
      //@}
   
      //! \name Run-Time Type Id
      //@{
   
      DEFINE_RTTI_METHODS3("MESH_DESELECT_ALL_CMD", MESH_DESELECT_ALL_CMD*, UNDO_CMD, CCOMMAND*);
      
      //@}

};

#endif // MESH_SELECT_CMD_H_IS_INCLUDED
