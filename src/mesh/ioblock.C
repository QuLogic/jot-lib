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
/*************************************************************************
 *    NAME: Loring Holden
 *    USER: lsh
 *    FILE: ioblock.C
 *    DATE: Wed Jan  6 17:10:03 US/Eastern 1999
 *************************************************************************/

#include "ioblock.H"

str_ptr
get_word(istream &is, str_list &list)
{
   if (list.num()) return list.pop();
   char buf[1024];
//   is >> buf;
//   return str_ptr(buf);
   return (is >> buf) ? str_ptr(buf) : NULL_STR;
}

/*************************************************************************
 * Function Name: IOBlock::consume
 * Parameters: istream &is, const IOBlockList &lis
 * Returns: int
 * Effects: 
 *************************************************************************/
int
IOBlock::consume(istream &is, const IOBlockList &list, str_list &leftover)
{
   leftover.clear();
   if (is.bad()) return 0;
   if (is.eof()) return 1;

   static Cstr_ptr endtoken  ("#END");
   static Cstr_ptr begintoken("#BEGIN");
   str_ptr str1;
   str_ptr str2;
   str_ptr starttype;
   int consumed = 0;

   str1 = get_word(is, leftover);
   str2 = get_word(is, leftover);
   if (is.eof())
      return 1;
   while(str1 == begintoken && !is.eof()) {

      starttype = str2;
      consumed = 0;
      for (int i = 0; !consumed && i < list.num(); i++) {
         if (starttype == list[i]->name()) {
            if (leftover.num()) {
               cerr << "IOBlock::consume - " 
                    << "warning: leftover data before consume"
                    << ", at byte " << is.tellg() << endl;
            }
            // XXX - deal with consume_block return value
            int ret = list[i]->consume_block(is, leftover);
            if (!ret) return 0;
            consumed = 1;
            if (is.bad()) {
               cerr << "IOBlock::consume - "
                    << "stream is bad after reading block "
                    << list[i]->name()
                    << ", at byte " << is.tellg() << endl;
               return 0;
            }
         }
      }
      if (!consumed) {
         cerr << "IOBlock::consume - skipping unknown block '"
              << starttype << "'"
              << ", at byte " << is.tellg() << endl;
      }
      
      if (is.eof()) return 1;

      str1 = get_word(is, leftover);
      if (str1 == NULL_STR) {
         cerr << "IOBlock::consume - empty string at beginning of block, "
              << "after block " << starttype
              << ", at byte " << is.tellg() << endl;
         return 0;
      }
      str2 = get_word(is, leftover);
      do {
         // Read until #END
         while (!is.eof() && !is.fail() && str1 != endtoken) {
            str1 = get_word(is, leftover);
            str2 = NULL_STR;
         }
         
         // Get type
         if (str2 == NULL_STR) {
            str2 = get_word(is, leftover);
            if (str2 != starttype) {
                  cerr << "IOBlock::consume - found " << str1 << " " << str2
                       << " before " << endtoken << " " << starttype
                       << ", at byte " << is.tellg() << endl;
            }
         }
      } while (!is.eof() && str2 != starttype);

      if (str1 == endtoken) {
         str1 = get_word(is, leftover);
         str2 = get_word(is, leftover);
      }
      if (is.eof()) break;
   }
   if (!is.eof()) {
      // Push onto stack in reverse order
      leftover += str_ptr(str2);
      leftover += str_ptr(str1);
   }
   return 1;
}
