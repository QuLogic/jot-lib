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

#ifndef _JOT_STD_THREAD_H
#define _JOT_STD_THREAD_H
#include "std/support.H"
#include "thread_mutex.H"

// This file should contain generalized thread support, but for now
// it's just a pthread implmentation of a thread syncronization primitive.

#ifdef USE_PTHREAD
#if defined(sun) && !defined(_REENTRANT)
// Would like to use #error, but that just gives a warning
Need to compile thread stuff with -mt under Solaris
#endif
#include <pthread.h>

class Thread {
   public:
      Thread()          { _running = false; }
      virtual ~Thread() { if (_running) pthread_cancel(_thread); }  

      void start()  { 
	 pthread_attr_t attr;
         int retval;
	 if ((retval = pthread_attr_init(&attr)) != 0) 
    {
      err_msg("Thread:start - pthread_attr_init returned %d: %s", retval,strerror(errno));
      exit(1);
    }
	 if ((retval=pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM)) != 0) 
    {
      // On the SGI errno is cleared, so we have to use strerror
      err_msg("Thread:start - pthread_attr_setscope: %s", strerror(retval));
      exit(1);
    }
	 _running = !pthread_create(&_thread, &attr, kickstart, (void *)this);
	 pthread_attr_destroy(&attr);
      }
   protected:
      virtual void threadProc() = 0;

   private:
      bool      _running;
      pthread_t _thread;
      static void *kickstart(void *me) { ((Thread *)me)->threadProc(); 
                                         return 0; }
};


class ThreadSync {
private:
  pthread_mutex_t mut_;
  pthread_cond_t queue_;
  int count_;
  const int number_;
  int generation_;

public:
  ThreadSync(int number): number_(number) {
    pthread_mutex_init(&mut_, 0);
    pthread_cond_init(&queue_, 0);
    count_ = 0;
    generation_ = 0;
  }
  
  ~ThreadSync() {
    pthread_mutex_destroy(&mut_);
    pthread_cond_destroy(&queue_);
  }

#ifndef sgi 
  void wait() {
    pthread_mutex_lock(&mut_);
    if (++count_ < number_) {
      int my_generation = generation_;
      while(my_generation == generation_)
	pthread_cond_wait(&queue_, &mut_);
    } else {
      count_ = 0;
      generation_++;
      pthread_cond_broadcast(&queue_);
    }
    pthread_mutex_unlock(&mut_);
  }
#else
  void wait() {
    pthread_mutex_lock(&mut_);
    if (++count_ < number_) {
      assert((count_ < number_) && (count_ > 0));
      int my_generation = generation_;
      pthread_mutex_unlock(&mut_);
      while(my_generation == generation_)
        ;
    } else {
      count_ = 0;
      generation_++;
      pthread_mutex_unlock(&mut_);
    }
  }
#endif
};

class ThreadData {
   public:
      ThreadData()  : _key(0) { }
      // Call this only once after all threads are created
      void create() {
         if (pthread_key_create(&_key, 0)) {
            cerr << "ThreadData::create - Could not create key" << endl;
         }
      }
      // Call from threads
      void  set(void *val) { pthread_setspecific(_key, val);}
      void *get()          { return pthread_getspecific(_key);}
   protected:
      pthread_key_t _key;
};
#endif
#endif
