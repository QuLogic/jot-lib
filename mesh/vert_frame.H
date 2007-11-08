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
 * vert_frame.H
 *
 *                         n                                   
 *                         |                                   
 *                         |                                   
 *                         |                                   
 *                         o - - - b                          
 *                        /                                    
 *                       /                                     
 *                      t                                      
 *                                                             
 *      VertFrame: a coordinate frame associated with a Bvert.
 *      Origin is vertex location. Directions t and n are
 *      given orthonormal vectors. Direction b is determined
 *      by them: b = n x t.
 *
 *****************************************************************/
#ifndef VERT_FRAME_H_IS_INCLUDED
#define VERT_FRAME_H_IS_INCLUDED

#include "simplex_frame.H"

/*****************************************************************
 * VertFrame:
 *
 *      see diagram above.
 *****************************************************************/
class VertFrame : public SimplexFrame {
 public:

   //******** MANAGERS ********

   VertFrame(uint key, Bvert* v, CWvec& t=mlib::Wvec::X(), mlib::CWvec& n=mlib::Wvec::Z()) :
      SimplexFrame(key, v, t, n) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("VertFrame", VertFrame*, SimplexFrame, CSimplexData*);

   //******** ACCESSORS ********

   Bvert* vert()        const   { return (Bvert*) _simplex; }

   //******** CoordFrame VIRTUAL METHODS ******** 

   virtual Wpt o()      { return vert()->loc(); }
};
typedef const VertFrame CVertFrame;

#endif // VERT_FRAME_H_IS_INCLUDED

// end of file vert_frame.H
