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
#ifndef _IMAGE_PLATE_H_
#define _IMAGE_PLATE_H_

#include "geom/geom.H"

MAKE_PTR_SUBC(ImagePlate, GEOM);
typedef const ImagePlateptr CImagePlateptr;

class ImagePlate : public GEOM {
public:
   ImagePlate(const string& filename);
   ImagePlate();
   virtual ~ImagePlate();

   //******** RUN-TIME TYPE IDENTIFICATION ********
   DEFINE_RTTI_METHODS3("ImagePlate", ImagePlate*, GEOM, CDATA_ITEM*);

   virtual int draw(CVIEWptr &v);

};

#endif
