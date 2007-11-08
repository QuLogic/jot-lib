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
#ifndef SUPPORT_H
#define SUPPORT_H

#include "platform.H"
#include "error.H"
#include "ref.H"
#include "hash.H"

//YYY - Using new iostream wrapper
#include "std/iostream.H"

#define REF_CLASS(FOO)  FOO; typedef REFptr<FOO> FOO##ptr; class FOO
#define FD_REF_CLASS(FOO)  FOO; typedef REFptr<FOO> FOO##ptr
#define CSTR const STR
#define brcase    break; case
#define brdefault break; default

// Helpful debugging statement
#define PRINT_VAR(v) cerr << # v << " = '" << v << "'" << endl

/* ------------------ class definitions --------------------- */
class STR : public REFcounter {
 protected:
   char *_s;
   static HASH *strpool;
 public:
   static STR *null;

   STR()                    { _s = 0; }
   STR(const char *s)       { if (!strpool) strpool = new HASH(1024); 
   if (!s) s = "";
   strpool->add(s, _s); }
   STR(char        s)       { char str[2]; str[0]=s; str[1] = '\0'; 
   if (!strpool) strpool = new HASH(1024); 
   strpool->add(str, _s); }
   STR(const STR  &s)       { _s = s._s; }
   STR(int         x)       { char buff[1024];sprintf(buff,"%d", x);
   if (!strpool) strpool = new HASH(1024);
   strpool->add(buff, _s); }
   STR(double      d)       { char buff[1024];sprintf(buff,"%g", d);
   if (!strpool) strpool = new HASH(1024);
   strpool->add(buff, _s); }
   ~STR()                    { /* if (strpool) strpool->del(_s); */ }
   size_t len()               const { return strlen(_s); }
   STR  &operator = (CSTR &s)       { _s = s._s; return *this;}
   char *operator * ()        const { return _s; }
   char  operator [](int   i) const { return _s[i]; }
   bool  operator ==(CSTR &s) const { return _s == s._s; }
   STR*  operator + (CSTR &s) const { char buff[1024];
   sprintf(buff,"%s%s",_s,s._s);
   return new STR(buff); }
   bool  contains   (CSTR &s) const { return strstr(_s, s._s) != 0; }
   static double load_factor() { return strpool?strpool->load_factor():0.0; }
   friend inline ostream &operator<<(ostream &os, CSTR *s) { return os<< s->_s;}
};

#define Cstr_ptr const str_ptr
#define Cstr_list const str_list
class str_ptr;
template <class T>
#if defined(sgi) || defined(WIN32) || defined(_AIX) || defined(__GNUC__) || defined(__KCC) || (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x510)
class LIST;
#else
class LIST<T>;
#endif

typedef LIST<str_ptr> str_list;
class str_ptr : public REFptr<STR> {
 public :
   static Cstr_ptr null;
   static Cstr_ptr &null_str() { if (null.p_ == 0) ((str_ptr &) null) = (STR::null = new STR("")); return null; }

   str_ptr()              : REFptr<STR>(new STR("")){ }
   str_ptr(Cstr_ptr   &p) : REFptr<STR>(p)          { }
   str_ptr(STR        *s) : REFptr<STR>(s?s:null.p_){ }
   str_ptr(const char *s) : REFptr<STR>(new STR(s)) { }
   str_ptr(char        c) : REFptr<STR>(new STR(c)) { }
   str_ptr(int         x) : REFptr<STR>(new STR(x)) { }
   str_ptr(double      d) : REFptr<STR>(new STR(d)) { }
   size_t  len        ()            const { return   p_->len(); }
   char    operator [](int i)       const { return   (*p_)[i];  }
   bool    operator ==(Cstr_ptr &s) const { return   *s.p_ == *p_;  }
   bool    operator !=(Cstr_ptr &s) const { return !(*s.p_ == *p_); }
   bool    operator !  ()           const { return (!p_ || !**p_ || !***p_) ; }
   operator STR*()          const { return (!*this) ? 0 : p_; }
   str_ptr operator + (Cstr_ptr &s) const { return str_ptr(*p_ + *s.p_); }
   str_ptr to_upper   () const;
   str_ptr to_lower    () const;
   bool    contains   (Cstr_ptr &s) const {return p_->contains(*s.p_);}
   // Does this string contain any of the strings in str_list s?
   bool    contains   (Cstr_list &s) const;
};

#ifdef JOT_AVOID_STATIC_LOCAL_INLINE_VAR
#define RET_STAT_STR(s)  return str_ptr(s)
#else
#define RET_STAT_STR(s)  static str_ptr st(s); return st
#endif

// useful string constants:
#define NULL_STR str_ptr::null_str()

extern "C" {
   typedef int (* compare_func_t) (const void *, const void *);
}


#define BAD_IND -1
//----------------------------------------------
//
//  ARRAY:
//      Templated array-based list. It resizes
//  itself as necessary. Constructor lets you
//  specify the expected size.
//
//----------------------------------------------
#define CARRAY const ARRAY
template <class T>
class ARRAY {
 protected:
   T*      _array;      // pointer to the data
   int     _num;        // number of elements in the array
   int     _max;        // max elements for currently allocated array
   bool    _unique;     // true means elements always added uniquely
   bool    _do_index;   // true means element indices are stored

   //******** PROTECTED METHODS ********

   //******** ARRAY INDEXING ********

   //   Derived ARRAY classes may override get_index(),
   //   set_index() and clear_index() so that removing an
   //   element from the array becomes an O(1) operation.
   //
   // For derived classes that manage element indices:
   virtual void   set_index(const T&, int /* index */)  const {}
   virtual void clear_index(const T&)                   const {}

   //******** REFptr METHODS ********

   // Needed by LIST (below -- ARRAY of REFptrs):
   // Clear element i:
   virtual void clear_ele  (int /* i */)  {}

   // Clear elements i thru j-1:
   virtual void clear_range(int i, int j) {
      if (_do_index)
         for ( ; i < j; i++)
            clear_index(_array[i]);
   }

   //******** ADDING ********

   virtual void  append_ele(const T& el) {
      // append element
      if (_do_index)
         set_index(el, _num);
      if (_num < _max) {
         _array[_num++] = el;
      } else {
         // In case the element referenced is a current item of
         // the array, we copy it before reallocating:
         T tmp = el;
         realloc();
         _array[_num++] = tmp;
      }
   }

 public:

   //******** MANAGERS ********

   ARRAY(int m=0) :
      _array(0), _num(0), _max(max(m,0)), _unique(0), _do_index(0) {
      if (_max) _array = new T[_max];
   }
   ARRAY(CARRAY<T>& l) :
      _array(0), _num(0), _max(0), _unique(0), _do_index(0) { *this = l; }

   virtual ~ARRAY() { clear(); delete [] _array;}

   //******** ACCESSORS/CONVENIENCE ********

   int  num()                   const   { return _num; }
   bool empty()                 const   { return (_num<=0); }
   bool valid_index(int k)      const   { return (k>=0 && k<_num); }
   void set_unique()                    { _unique = 1; }

   T* array()                           { return _array; }
   T& operator[](int j)         const   { assert(_array); return _array[j]; }

   // Convenience: return last or first element in array.
   // Don't call these on an empty array. (It would be bad):
   T& last() const { 
      assert(!empty());
      return _array[_num-1];
   }
   T& first() const { 
      assert(!empty());
      return _array[0];
   }

   //******** ARRAY INDEXING ********

   // Turn on array indexing and set the index of each element
   void begin_index() {
      _do_index = true;
      for (int i=0; i<_num; i++)
         set_index(_array[i], i);
   }

   // Turn off array indexing and clear indices
   void end_index() {
      for (int i=0; i<_num; i++)
         clear_index(_array[i]);
      _do_index = false;
   }

   bool is_indexing() const { return _do_index; }

   //******** MEMORY MANAGEMENT ********

   // Clear the array (i.e. make it empty):
   virtual void clear() {
      // If doing array indexing, clear all indices:
      if (_do_index)
         end_index();
      // For LISTs, assign 0 to REFptrs:
      clear_range(0,_num);
      _num=0;
   }

   // Truncate the array, leaving just the first n elements:
   virtual void truncate(int n) {
      if (valid_index(n)) {
         clear_range(n,_num);
         _num = n;
      } else err_msg("ARRAY::truncate: Error: bad index %d/%d", n, _num);
   }

   // The following is called automatically as necessary when the
   // array runs out of room, or explicitly when it's known that
   // the array will shortly need to contain a given number of
   // elements. The parameter 'new_max' tells how large the array
   // should be -- if the array is already that large nothing
   // happens. Otherwise, the array is reallocated and its
   // elements are copied to the new array. If new_max == 0, this
   // is interpreted as a request that _max should be
   // doubled. The exception is when _max is also 0 (meaning the
   // array itself is null), in which case _max is set to 1 and
   // the array is allocated to hold a single element.
   virtual void realloc(int new_max=0) {
      // If already big enough do nothing:
      if (new_max && new_max <= _max)
         return;
      // Decide on the new max value:
      _max = (new_max == 0) ? (_max ? _max*2 : 1) : new_max;
      // Allocate the new array:
      T *tmp = new T [_max]; assert(tmp);

      // Copy over existing elements:
      for (int i=0; i<_num; i++) {
         tmp[i] = _array[i];
         clear_ele(i);
      }
      // Replace old array with new one:
      delete [] _array;
      _array = tmp;
   }

   //******** CONTAINMENT ********

   // Exhaustive search to find index of element.
   // Returns BAD_IND on failure.
   virtual int get_index(const T &el) const {
      // If _do_index is set, must over-ride this method:
      for (int k = _num-1; k >= 0; k--)
         if (_array[k] == el)
            return k;
      return BAD_IND;
   }

   bool contains(const T &el) const { return get_index(el) >= 0; }

   //******** ADDING ********

   // Add the element uniquely.
   // (Do nothing if it's there already):
   bool add_uniquely(const T& el) {
      if (get_index(el) < 0) {
         append_ele(el);
         return true;
      }
      return false;
   }

   // Append element to end of list:
   void operator += (const T& el) {
      if (_unique)
         add_uniquely(el);
      else
         append_ele(el);
   }

   // add (same as +=):
   void add (const T& p) { *this += p; }     

   // Insert element at front of list:
   void push(const T& p) {
      insert(0,1);      // Make a space in 1st slot
      _array[0] = p;    // Copy element in there
      if (_do_index)
         set_index(p, 0);
   } 

   // Open up a gap of uninitialized elements in the array,
   // starting at index 'ind', extending for 'num' elements.
   // Presumably these elements then get assigned directly:
   void insert(int ind, int num) {
      if (_num+num > _max) 
         realloc(_num+num);
      _num += num;
      for (int i=_num-1; i>=ind+num; i--) {
         _array[i] = _array[i-num];
         if (_do_index)
            set_index(_array[i], i);
      }
   }

   //******** REMOVING ********

   // Search for given element, remove it:
   bool remove(int k) {
      // remove element k
      // return 1 on success, 0 on failure:
      if (valid_index(k)) {
         // replace element k with last element and shorten list:
         if (_do_index) {
            set_index(last(), k);
            clear_index(_array[k]);
         }
         _array[k] = _array[--_num];
         clear_ele(_num);
         return 1;
      } else if (k != BAD_IND) {
         err_msg("ARRAY::remove: invalid index %d", k);
         return 0;
      } else return 0; // assume the call to get_index() failed
   }

   // Remove the given element:
   bool operator -= (const T &el)         { return remove(get_index(el)); }
   bool          rem(const T &p)          { return (*this -= p); }

   // Remove and return the last element.
   // Don't call this on an empty array. (It would be bad):
   T pop() {
      // delete last element in list and return it:
      T tmp = last();
      remove(_num-1);
      return tmp;
   }

   // Remove element k, keeping remaining elements in order.
   // Returns true on success.
   bool pull_index(int k) {
      if (valid_index(k)) {
         if (_do_index)
            clear_index(_array[k]);
         for (int i=k; i<_num-1; i++) {
            _array[i] = _array[i+1];
            if (_do_index)
               set_index(_array[i], i);
         }
         clear_ele(--_num);
         return true;
      }
      if (k != BAD_IND)
         err_msg("ARRAY::pull: invalid index %d", k);
      return 0;
   }
   // Pull the given element:
   bool pull_element(const T& p) { return pull_index(get_index(p)); }

   //******** CYCLING ********

   // Cyclically shift elements p steps to the right.
   // If p < 0 it means shift left.
   virtual void shift(int p) {
      // Can't shift the empty list
      if (empty())
         return;

      int n = _num;

      // Make sure 0 <= p < n:
      if (p >= n)   p %= n;
      else if (p<0) p = div(p,n).rem + n;

      assert(p >=0 && p<n);

      if (p == 0)
         return;

      // Copy elements to temporary array, then copy them
      // back shifted:
      ARRAY<T> tmp = *this;      
      for (int k=0; k<n; k++)
         _array[(k + p)%n] = tmp[k];

      // Reset element indices if needed:
      if (_do_index)
         begin_index();
   }

   // Extract a section of an array
   ARRAY<T> extract(int start, int n) const {
      if (!(valid_index(start) && valid_index(start+n-1)))
         return ARRAY<T>();
      ARRAY<T> ret(n);
      for (int i=0; i<n; i++)
         ret += _array[i+start];
      return ret;
   }

   //******** ARRAY OPERATORS ********

   // See more operators below (outside ARRAY class definition)

   // Array concatenation
   ARRAY<T>& operator+=(CARRAY<T>& b) {
      if(!b.empty()) {
         realloc(num() + b.num());
         for (int i=0; i<b.num(); i++)
            *this += b[i];
      }
      return *this;
   }

   // Array assignment
   ARRAY<T>& operator=(CARRAY<T>& b) {
      // don't do anything if rhs already is lhs
      if (this == &b)
         return *this;

      clear();
      return *this += b;
   }

   // Pull elements of given array out of this one:
   void operator-=(CARRAY<T> &l) {
      for (int i=0; i < l.num(); i++)
         *this -= l[i];
   }

   //******** REORDERING ********
   virtual void reverse() {
      for (int i=0, j=_num-1; i<j; ++i, --j) {
         swap(_array[i],_array[j]);
         if (_do_index) {
            set_index(_array[i], i);
            set_index(_array[j], j);
         }
      }
   }

   //******** SORTING ********
   virtual void sort(compare_func_t compare) {
      qsort(_array, _num, sizeof(T), compare);

      // Reset element indices if needed:
      if (_do_index)
         begin_index();
   }
};

//----------------------------------------------
// ARRAY OPERATORS
//
// The following are convenience operators for
// pairs of arrays of different types, where
// elements of the 2nd array can be cast to the
// type of element in the first array.
//----------------------------------------------

// equality
template <class T, class S>
inline bool
operator ==(CARRAY<T>& a, CARRAY<S> &b)
{
   if (a.num() != b.num())
      return 0;
   for (int i = 0; i < b.num(); i++) {
      // Use !(x == y) because == should be available and != may not be
      if (!(a[i] == (T)b[i]))
         return 0;
   }
   return 1;
}

// concatenation
template <class T, class S>
inline ARRAY<T>&
operator +=(ARRAY<T>& a, CARRAY<S>& b)
{
   if(!b.empty()) {
      a.realloc(a.num() + b.num());
      for (int i=0; i<b.num(); i++)
         a += (T)b[i];
   }
   return a;
}

// concatenation
template <class T, class S>
inline ARRAY<T>
operator +(CARRAY<T>& a, CARRAY<S>& b)
{
   ARRAY<T> ret = a;
   return ret += b;
}

// XXX - hack for backward compatibility -- should just remove
// occurrences of ARRAYptrs:
#define ARRAYptrs ARRAY

//----------------------------------------------
//
//  LIST:
//
//      same as ARRAY, but assumes the templated
//      type T is derived from a REFptr, and calls
//      Clear() on ref pointers as needed.
//
//----------------------------------------------
#define CLIST const LIST
template <class T>
class LIST : public ARRAY<T> {
 protected:
   virtual void clear_ele  (int i)          { (*this)[i].Clear(); }
   virtual void clear_range(int i, int j) {
      ARRAY<T>::clear_range(i,j);
      for (int k=i; k < j; k++)
         clear_ele(k);
   }
 public:
   LIST(int m=0)     : ARRAY<T>(m) {}
   LIST(CLIST<T>& l) : ARRAY<T>(l) {}
};

template<class T>
ostream &
operator<< (ostream &os, const ARRAY<T> &array)
{
   os << array.num() << endl;
   for (int i = 0; i < array.num(); i++) {
      os << array[i] << endl;
   }
   os << endl;
   return os;
}

template<class T>
istream &
operator>> (istream &is, ARRAY<T> &array)
{
   int num; is >> num; array.clear();
   for (int i = 0; i < num; i++) {
      T x; is >> x; array += x;
   }
   return is;
}

str_list tokenize(Cstr_ptr &str, char delim=' ');
str_list dir_list(Cstr_ptr &directory);

// None of jot's code checks if select() was interrupted by a
// signal (for example, SIGALRM used by IBM's MPI
// implementation).  This should probably be fixed, but for now
// we have a drop in replacement that we call instead.  If jot
// will be running in a signal unsafe environment then you should
// call the magic voodoo "signalSafeSelect();" as one of the
// first things in main() before calling any jot stuff.

#ifndef WIN32
extern "C" int (*SELECT)(int, fd_set *, fd_set *, fd_set *, timeval *);

extern "C" int Select(int maxfds, fd_set *reads, fd_set *writes, 
                      fd_set *errors, struct timeval *timeout);
#endif

inline void
signalSafeSelect(void) 
{
#ifndef WIN32
   SELECT = Select;
#endif
}

inline void
fsleep(double dur)
{
#ifdef WIN32
   Sleep(int(dur * 1e3));
#else
   struct timeval timeout;
   double         sleep_time = dur * 1e6;
   timeout.tv_sec  = 0;
   // XXX - there should be a better way to avoid warnings
#if defined(macosx) || defined(sgi) || defined(__CYGWIN__)
   timeout.tv_usec = (long) sleep_time;
#else
#  ifdef linux
   timeout.tv_usec = (time_t) sleep_time;
#else
   timeout.tv_usec = (suseconds_t) sleep_time;
#  endif
#endif
   SELECT(FD_SETSIZE, 0, 0, 0, &timeout);
#endif
}

#endif  // SUPPORT_H

/* end of file support.H */
