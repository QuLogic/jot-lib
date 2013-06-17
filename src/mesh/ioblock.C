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

string
get_word(istream &is, vector<string> &list)
{
   if (list.size() > 0) {
      string result = list.back();
      list.pop_back();
      return result;
   }
   char buf[1024];
//   is >> buf;
//   return string(buf);
   return (is >> buf) ? string(buf) : "";
}

/*************************************************************************
 * Function Name: IOBlock::consume
 * Parameters: istream &is, const IOBlockList &lis
 * Returns: int
 * Effects: 
 *************************************************************************/
int
IOBlock::consume(istream &is, const IOBlockList &list, vector<string> &leftover)
{
   leftover.clear();
   if (is.bad()) return 0;
   if (is.eof()) return 1;

   static const string endtoken  ("#END");
   static const string begintoken("#BEGIN");
   string str1;
   string str2;
   string starttype;
   int consumed = 0;

   str1 = get_word(is, leftover);
   str2 = get_word(is, leftover);
   if (is.eof())
      return 1;
   while(str1 == begintoken && !is.eof()) {

      starttype = str2;
      consumed = 0;
      for (IOBlockList::size_type i = 0; !consumed && i < list.size(); i++) {
         if (starttype == list[i]->name()) {
            if (leftover.size()) {
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
      if (str1 == "") {
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
            str2 = "";
         }
         
         // Get type
         if (str2 == "") {
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
      leftover.push_back(str2);
      leftover.push_back(str1);
   }
   return 1;
}
