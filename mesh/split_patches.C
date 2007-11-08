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
/**********************************************************************
 * split.C:
 *
 *      Takes an .sm file as input, splits it into disconnected
 *      pieces, and creates a separate patch for each piece.
 *
 **********************************************************************/
#include "std/config.H"
#include "mi.H"

int 
main(int argc, char *argv[])
{
   if (argc != 1) {
      cerr << "Usage: " << argv[0] << " < input.sm > output.sm" << endl;
      return 1;
   }

   BMESHptr mesh = BMESH::read_jot_stream(cin);
   if (!mesh || mesh->empty())
      return 1; // didn't work

   mesh->split_patches();

   mesh->write_stream(cout);

   return 0;
}

// end of file split_patches.C
