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

#ifndef _JOT_STD_THREAD_MUTEX_H
#define _JOT_STD_THREAD_MUTEX_H
// This file should contain generalized thread support, but for now
// it's just a pthread implmentation of a thread syncronization primitive.

#ifdef USE_PTHREAD
#include <pthread.h>
class ThreadMutex {
   public:
      ThreadMutex() { pthread_mutex_init(&_mtx, 0); }
     ~ThreadMutex() { pthread_mutex_destroy(&_mtx); }
      void lock()   { pthread_mutex_lock(&_mtx);    }
      void unlock() { pthread_mutex_unlock(&_mtx);  }
   protected:
      pthread_mutex_t _mtx;
};
#else
class ThreadMutex {
   public:
      ThreadMutex() { }
     ~ThreadMutex() { }
      void lock()   { }
      void unlock() { }
};
#endif

class CriticalSection {
   public:
      CriticalSection(ThreadMutex *mutex) : _mutex(mutex) { _mutex->lock(); }
      ~CriticalSection() { _mutex->unlock(); }
   protected:
      ThreadMutex *_mutex;
};
#endif
