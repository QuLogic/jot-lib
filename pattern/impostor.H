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
 * impostor.H
 *****************************************************************/
#ifndef IMPOSTOR_H_IS_INCLUDED
#define IMPOSTOR_H_IS_INCLUDED

#include "mesh/bmesh.H" 
#include "disp/gel.H"

class Impostor : public GEL {
 public:
   //******** MANAGERS ********
   Impostor(BMESH* m=0);
   virtual ~Impostor();

   //******** GEL METHODS ********
   virtual int draw(CVIEWptr& v);
   virtual int  draw_final(CVIEWptr & v);
   virtual DATA_ITEM *dup() const { return new Impostor; }
   DEFINE_RTTI_METHODS3("Impostor", Impostor*, GEL, CDATA_ITEM*);

   
   void        set_mesh(BMESH* m) { _mesh = m; }

   //******** Helper Functions ********
   void    draw_start();
   void    draw_end();
 protected:
   BMESH*   _mesh;
  
};


#endif // IMPOSTOR_H_IS_INCLUDED

/* end of file impostor.H */
