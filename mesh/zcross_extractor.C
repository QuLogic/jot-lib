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
 *  \file zcross_extractor.C
 *  \brief Contains the implementation of the ZCrossExtractor class.
 *
 *  \sa zcross_extractor.H
 *
 */

#include "mesh/zcross_extractor.H"

ZCrossPreviousFaceGenerator::ZCrossPreviousFaceGenerator()
{
   
   gen_face = gen_all_faces;
   
}

void
ZCrossPreviousFaceGenerator::init(const std::vector<ZXseg> &segs)
{
   
   prev_faces.clear();
   
   if(segs.empty()){
      
      gen_face = gen_all_faces;
      
   } else {
      
      gen_face = gen_prev_faces;
      
      prev_faces.resize(segs.size());
      
      for(unsigned long i = 0; i < segs.size(); ++i){
         
         if(segs[i].f()) prev_faces[i] = segs[i].f()->index();
         
      }
      
   }
      
}

int
ZCrossPreviousFaceGenerator::gen_prev_faces(ZCrossPreviousFaceGenerator *self)
{
   
   int val = -1;
   
   if(!self->prev_faces.empty()){
      
      val = self->prev_faces.back();
      self->prev_faces.pop_back();
      
   }
   
   return val;
   
}
