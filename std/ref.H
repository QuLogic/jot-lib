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
#ifndef REF_H_HAS_BEEN_INCLUDED
#define REF_H_HAS_BEEN_INCLUDED

#include <cassert>

#include "std/iostream.H"

#include "platform.H"
#include "thread_mutex.H"

#ifdef _AIX
#include <sys/atomic_op.h>
#endif

#ifdef sgi
#include <climits>
#include <mutex.h>
#endif

// AIX compiler seems to create a macro called Free(p), so this is necessary
#ifdef Free
#undef Free
#endif

#define CREFptr const REFptr
#define REF_ME(A)   ((REFcounter *)A)

/*****************************************************************
 * REFcounter:
 *
 *   Reference-counting base or mix-in class.
 *
 *   I.e., A class that wants to be ref-counted derives from this.
 *****************************************************************/
class REFcounter {
 private:

   union  {
      struct {
// Endian differences strike again
// XXX - assumes WIN32 only on i386 boxes
#if defined(WIN32) || defined(i386)
         unsigned int _ref:31;
         unsigned int _lock:1;
#else
         unsigned int _lock:1;
         unsigned int _ref:31;
#endif
      } _a;
      unsigned int _all;
   } _u;

   ThreadMutex _mutex;
 
 public:
   REFcounter() { _u._all = 0; }
   virtual ~REFcounter() {}

   void     Own() const {
#ifdef _AIX
      fetch_and_add((int *) &REF_ME(this)->_u._all, 1);
#elif defined(sgi)
      test_then_add((unsigned long *) &REF_ME(this)->_u._all, 1);
#else
#ifdef USE_PTHREAD
      CriticalSection cs((ThreadMutex*)&_mutex);
#endif
      REF_ME(this)->_u._a._ref++;
#endif
   }

   // Need to lock ourself before calling delete since the
   // destructor may make a local REFptr to itself.  When the
   // REFptr goes out of scope, Free() gets called again leading
   // to an infinite loop.
   void     Free()         const     {
	//This chunk of code is just the conditional if the IF statement
      // If new value is 0...
#if defined(_AIX)
      if ( fetch_and_add((int *) &REF_ME(this)->_u._all, -1) == 1 )
#elif defined(sgi)
      // hack to do atomic decrement
      if ( test_then_add((unsigned long *) &REF_ME(this)->_u._all, UINT_MAX)==1 )
#else
#ifdef USE_PTHREAD
      CriticalSection cs((ThreadMutex*)&_mutex);
#endif
      if (--REF_ME(this)->_u._all == 0)
#endif
	  // THEN
         {
#if !defined(_AIX) && !defined(sgi)
            ((ThreadMutex*)&_mutex)->unlock();
#endif
            REF_ME(this)->Lock();
#if !defined(_AIX) && !defined(sgi)
            ((ThreadMutex*)&_mutex)->lock();
#endif
            delete REF_ME(this); 
         }
	 // end crazy IF statement
   }


   int Lock() {
      CriticalSection cs(&_mutex);
      int old = _u._a._lock;
      _u._a._lock = 1; 
      return old;
   }
   void Unlock() {
      CriticalSection cs(&_mutex);
      _u._a._lock = 0;
   }
   int Unique() const {
      CriticalSection cs((ThreadMutex*)&_mutex);
      return _u._a._ref == 1;
   }
};

/*****************************************************************
 * REFlock:
 *
 *   Temporarily locks a REFcounter so it won't delete its
 *   pointer when the number of owners hits zero.
 *****************************************************************/
class REFlock {
   REFcounter*  _ref;
   int          _old;
 public :
   REFlock(REFcounter *r):_ref(r)  { _old = _ref->Lock(); }
   ~REFlock()                      { if (!_old) _ref->Unlock(); }
};

/*****************************************************************
 * REFptr:
 *
 *   Reference-counted shared pointer. Wraps around the "real"
 *   pointer, which points to an object derived from REFcounter.
 *   I.e., the REFcounter object has the count of its owners
 *   in its own data.
 *****************************************************************/
template <class T>
class REFptr {
 protected:
   T*    p_;    // The real pointer

 public:
   //******** MANAGERS ********
   REFptr(             ): p_(0)             { }
   REFptr(CREFptr<T> &p): p_(p.p_)          { if (p_) REF_ME(p_)->Own (); }
   REFptr(T *      pObj): p_(pObj)          { if (p_) REF_ME(p_)->Own (); }

   // The following should be virtual, but if so, Sun CC
   // gives warnings about str_ptr::operator== hiding
   // REFptr<STR>::operator==(const REFptr<STR>&)
#if !defined(sun)
   virtual
#endif
   ~REFptr()    { if (p_) REF_ME(p_)->Free(); }

   void Init()  { p_ = 0; }
   void Clear() { if (p_) REF_ME(p_)->Free(); p_=0; }

   //******** ASSIGNMENT ********
   REFptr<T> &operator=(T* o) {
      if (o != p_) {
         if (o)
            REF_ME(o)->Own(); 
         Clear();  
         p_ = o; 
      }    
      return *this;
   }
   REFptr<T> &operator=(CREFptr<T>& p) {
      if (p.p_ != p_) {
         if (p.p_)
            REF_ME(p.p_)->Own();
         Clear();  
         p_ = p.p_; 
      } 
      return *this;
   }

   //******** EQUALITY ********
   bool operator == (CREFptr<T>& p)     const   { return  p.p_ == p_; }
   bool operator == (T* p)              const   { return     p == p_; }
   bool operator != (CREFptr<T>& p)     const   { return  p.p_ != p_; }
   bool operator != (T* p)              const   { return     p != p_; }
   bool operator!()                     const   { return !p_; }

   //******** CASTING ********
   const T& operator*()                 const   { assert(p_);  return *p_; }
   T&       operator*()                         { assert(p_);  return *p_; }
   const T* operator->()                const   { assert(p_);  return  p_; }
   T*       operator->()                        { assert(p_);  return  p_; }

   operator T* ()                       const   { return  p_; }

   REFptr<T> &Cast_from_void(void *V) { *this = (T*) V; return *this;}

   //******** I/0 ********
   friend inline ostream& operator<<(ostream& os,CREFptr<T>& p) {
      return os<<p.p_;
   }
};

#undef REF_ME

// This macro makes a REFptr baseclass that other
// REFptr templates can subclass from.  The key to 
// allowing subclassing is the definition of the subc
// class which defines a virtual cast function.  The
// derived REFptr classes will have to define a subclass
// of the subc class and fill in the cast function with
// something that can return the subclass type.
//
#define MAKE_PTR_BASEC(A)             \
class A;                              \
class A##subc {                       \
   public :                           \
      virtual ~A##subc() {}           \
      virtual A *A##cast() const = 0; \
};                                    \
class A##ptr : public REFptr<A>, public A##subc {  \
   public :                                        \
     A##ptr()                                   { }\
     A##ptr(A             *g): REFptr<A>(g)     { }\
     A##ptr(const A##ptr  &p): REFptr<A>(p.p_)  { }\
     A##ptr(const A##subc &p): REFptr<A>(p.A##cast()) { } \
                                                   \
    virtual A *A##cast() const { return (A *)p_; } \
}


// This macro is used to define a REFptr subclass from another
// templated REFptr.  We can't use normal inheritance because
// we want the subclassed pointer to return different template
// types than the parent pointer for all of the derefrencing
// functions.  Thus we define the subclass pointer to multiply
// inherit from its template defintion and from a "hack" subc
// class for the parent.  In addition, this macro defines a
// "hack" subc class for the subclassed pointer so that other
// ptr classes can in turn subclass from it.
//
#define MAKE_PTR_SUBC(A,B) \
class A; \
class A##subc: public B##subc  {  \
   public :  \
      virtual A *A##cast() const = 0; \
      virtual B *B##cast() const { return (B *)A##cast(); } \
}; \
class A##ptr : public REFptr<A>, public A##subc {  \
   public :                                        \
     A##ptr()                                   { }\
     A##ptr(A             *g): REFptr<A>(g)     { }\
     A##ptr(const A##ptr  &p): REFptr<A>(p.p_)  { }\
     A##ptr(const A##subc &p): REFptr<A>(p.A##cast()) { } \
                                                   \
    virtual A *A##cast() const { return (A *)p_; } \
}

#endif // REF_H_HAS_BEEN_INCLUDED

/* end of file ref.H */
