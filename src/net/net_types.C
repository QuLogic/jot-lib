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

//#include <sys/types.h>

#include "std/support.H"
#include "stream.H"
#include "net.H"
#include "net_types.H"
#include "data_item.H"


STDdstream &
operator >> (STDdstream &ds, HASH &h)
{
   h.clear();
   int num;
   ds >> num;
   str_ptr key;
   for (int i = 0; i < num; i++) {
      ds >> key;
      DATA_ITEM *di = DATA_ITEM::Decode(ds);

      h.add(**key, (void *) di);
   }
   return ds;
}

STDdstream &
operator << (STDdstream &ds, CHASH &h)
{
   ARRAY<long>   keys;
   ARRAY<void *> items;
   h.get_items(keys, items);
   ds << items.num();
   for (int i = 0; i < items.num(); i++) {
      ds << str_ptr((char *) keys[i]) << *((DATA_ITEM *) items[i]);
   }
   return ds;
}
