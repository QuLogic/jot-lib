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


/****************************************************************/
/*    NAME: Scott Klemmer                                       */
/*    ACCT: srk                                                 */
/*    FILE: linklist.H                                          */
/*    DATE: Wed Apr  7 19:51:04 1999                            */
/****************************************************************/



#ifndef LINKLIST_HEADER
#define LINKLIST_HEADER

/* The link list has a delimiter node _bound which, as in STL, functions
   as both the head and tail. Calling begin() on a non-empty list returns the
   first valid node. Calling end() returns the delimiter node.  For this
   reason, it often makes sense to compare to end() to check whether we've hit
   the end of the list. Iterator compare is overriden to memory compare the
   nodes, so if end() and another iterator evaluate to equal, it's hit the end
   of the list.
*/

template <class T>
class LINKLIST {
 public:
   class Iterator;
   friend class Iterator;
   
 protected:
   struct Node;
   friend struct Node;

// to get an Iterator from outside the LINKLIST class, it's necessary to
// preface Iterator with LINKLIST:
// e.g. LINKLIST<int>::Iterator iter = my_list.insert(begin(), 7);

 public:
   class Iterator {
    protected:
      friend class LINKLIST<T>;
      Node* _node;
      bool _reverse; //false(0) means forward, true(!0) means reverse
            
			
    public:

      Iterator() : _node(0) {};
      Iterator(Node* n, bool rev=0) : _node(n), _reverse(rev) {};

      Iterator(const Iterator& iter) : _node(iter._node), _reverse(iter._reverse) {}

      bool is_reverse() const { return _reverse; }
      
      bool operator==(const Iterator& iter) const {
         return _node == iter._node;
      }
	 
      bool operator!=(const Iterator& iter) const {
         return _node != iter._node;
      }
	 
      Iterator& operator++() {
         _node = _reverse ? _node->prev : _node->next;
         return *this;
      }		

      
      Iterator operator++(int) {
         Iterator tmp = *this;
         ++*this;
         return tmp;
      }		
      
      
      Iterator& operator--() {
         _node = _reverse ? _node->next : _node->prev;
         return *this;
      }	


      Iterator operator--(int) {
         Iterator tmp = *this;
         --*this;
         return tmp;
      }		

      
      const T& operator*() const { return _node->data; }

      T& operator*()  { return _node->data; }

   
      Iterator& next() {
         return ++(*this);
      }         

			
      Iterator& prev() {	
         return --(*this);
      } 
   };



 protected:
   int _length;
   Node* _bound;

 public:
   LINKLIST() {
      _bound = new Node();
      _bound->next = _bound;
      _bound->prev = _bound;
      _length = 0;
   }

   LINKLIST(const LINKLIST<T>& l) {
      *this = l;
   }

   const LINKLIST<T>& operator=(const LINKLIST<T>& right) {
      clear();
      Iterator iter = right.begin();
      while ( iter != right.end() )
         insert(*iter++);

      return *this;
   }

	 
   Iterator insert_before(Iterator position, const T& data) {
      Node* tmp = new Node();
      tmp->data = data;
      tmp->next = position._node;
      tmp->prev = (position._node)->prev;
      (tmp->prev)->next = tmp;
      (position._node)->prev = tmp;
      ++_length;
      return tmp;
   }

   Iterator insert_after(Iterator position, const T& data) { 
      Node* tmp = new Node();
      tmp->data = data;
      tmp->next = (position._node)->next;
      tmp->prev = position._node;
      (position._node)->next = tmp;
      (tmp->next)->prev = tmp;
      ++_length;
      return tmp;
   }


   Iterator insert(Iterator position, const T& data) { // equivalent to insert_before
      return insert_before(position, data);
   }


   Iterator insert(const T& data) { // equivalent to insert_before
      return insert_before(begin(), data);
   }

   /* erase automatically deletes the erasure node, but the user is responsible
      for handling the data. does not allow end node to be deleted.
      Returns an Iterator pointing to the element before erasure
      */
   
   Iterator erase(Iterator position) {
      Iterator new_position = position;

      if ( position._node != _bound ) {

         (*(*position._node).prev).next = (*position._node).next;
         (*(*position._node).next).prev = (*position._node).prev;
         Node* to_go = position._node;
         --new_position;
         delete to_go;
         --_length;
      }

      return new_position;
   }

   const T& front() const { return *(begin()); }
   T&       front()       { return *(begin()); }
   const T& back() const { return *(--end()); }
   T&       back()       { return *(--end()); }

   Iterator begin() const { return _bound->next; }

   Iterator end() const { return _bound; }
	 
   Iterator rbegin() const {
      return Iterator(_bound->prev,1); //the 1 makes it a reverse iterator
   }

	 
   // get_iterator_at calls the 1st valid element 0
   // so, to get the 4th element in a list, call get_element_at 3
   Iterator get_iterator_at(int index) const {
      Iterator iter = begin();

      if (index>=_length) return end();
      
      for (int i=0; i<index; i++) ++iter;

      return iter;
   }

	 
   Iterator rget_iterator_at(int index) const {
      Iterator iter = rbegin();
			
      if (index>=_length) return end();

      for (int i=0; i<index; i++) ++iter;

      return iter;
   }

	 
   Iterator push_front(const T& data) { return insert(begin(), data); }
	 
   Iterator push_back(const T& data)  { return insert(end(), data); }


   T pop_front() {
      T data = _bound->next->data;
      if ( _bound->next != _bound ) erase( begin() );
      return data;
   }


   T pop_back() {
      T data = _bound->prev->data;
      erase( --end() );
      return data;
   }



   void swap(Iterator pos1, Iterator pos2) {
      Node* node1 = pos1._node;
      Node* node2 = pos2._node;

      Node* prev1 = node1->prev;
      Node* next1 = node1->next;

      node1->prev = node2->prev;
      node1->next = node2->next;

      node2->prev->next = node1;
      node2->next->prev = node1;

      node2->prev = prev1;
      node2->next = next1;

      prev1->next = node2;
      next1->prev = node2;
}

   
   Iterator find(const T& data) {
      Iterator iter = begin();
      while ( iter._node != _bound ) {
         if ( *iter == data ) return iter;
         ++iter;
      }
      return _bound; 
   }

   void clear() {
      while(!empty())
         erase(--end());
   }
   
   bool empty() const { return _length == 0; }

   int size() { return _length; }
   int num() { return _length; }

   void reverse() { Node* cur = _bound; do {
      Node* temp = cur->next;
      cur->next = cur->prev;
      cur->prev = temp;
      cur = cur->next;
   } while (cur != _bound);
   }
   
   int operator ==(const LINKLIST<T> &) {
      cerr << "Warning - dummy LINKLIST<T>::operator== called" << endl;
      return 0;
   }
	 
 protected:
   struct Node {
      Node* prev;
      Node* next;
      T data;
   };

	 
};

#endif  //LINKLIST_HEADER
