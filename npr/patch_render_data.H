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
#ifndef _PATCH_RENDER_DATA_
#define _PATCH_RENDER_DATA_

#include "mesh/patch.H"

class CompBody;

// Stores rendering information for a single patch.
// (Just a shell at this point.)

class PatchRenderData : public PatchData
{
public:
    PatchRenderData(CompBody* b = 0, Patch* p = 0) :
       _comp_body(b),
       _patch(p) { }

    virtual ~PatchRenderData() {}
    
    CompBody* comp_body()                   { return _comp_body; }

    DEFINE_RTTI_METHODS2("PatchRenderData", PatchData, CPatchData*);
    
protected:
   CompBody*            _comp_body;	
   Patch*               _patch;
};

#endif

/* end of file patch_render_data.H */
