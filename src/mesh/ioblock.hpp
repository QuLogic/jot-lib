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
#include "std/support.H"

#include <vector>

/*
   This class deals with reading in text bookended by:
#BEGIN X
#END Y
*/

#define IOBlockList vector<IOBlock *>
#define CIOBlock const IOBlock
class IOBlock {
   protected:
      string _name;
   public:
      IOBlock(const string &name) : _name(name) {}
      virtual ~IOBlock() {}
      
      // Callback for when #BEGIN NAME is encountered
      // (where NAME is what is contained in _name)
      virtual int consume_block(istream &, vector<string> &leftover) = 0;

      const string &name() const {return _name;}
      virtual int operator==(CIOBlock &b) {return b._name == _name;}
      static int consume(istream &is, const IOBlockList &list,
                         vector<string> &leftover);
};

template <class T>
class IOBlockMeth: public IOBlock {
   public:
      typedef int (T::*METH)(istream &is, vector<string> &leftover);
   protected:
      METH  _meth;
      T    *_inst;
   public:
      IOBlockMeth(const string &name, METH meth, T *inst)
         : IOBlock(name), _meth(meth), _inst(inst) {}
      virtual int consume_block(istream &is, vector<string> &leftover)
         {return (_inst->*_meth)(is, leftover);}
      void set_obj(T *inst) { _inst = inst;}
};

template <class T>
class IOBlockMethList : public vector<IOBlockMeth<T> *> {
   public:
      IOBlockMethList(int num = 5) : vector<IOBlockMeth<T> *>() { vector<IOBlockMeth<T> *>::reserve(num); }
      void set_obj(T *inst) {
         for (size_t i = 0; i < vector<IOBlockMeth<T>*>::size(); i++) {
            (*this)[i]->set_obj(inst);
         }
      }
};
