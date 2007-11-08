#ifndef GLEW_SINGLETON_H_IS_INCLUDED
#define GLEW_SINGLETON_H_IS_INCLUDED

/*!
 *  \file glew_singleton.H
 *  \brief Contains the definition of the GLEWSingleton class.
 *
 *  \sa glew_singleton.C
 *
 */

/*!
 *  \brief A singleton class that provides a uniform access point for GLEW
 *  functionality and makes sure that GLEW is initialized before it is used.
 *
 */
class GLEWSingleton {
   
   public:
   
      inline static GLEWSingleton &Instance();
      
      //! \brief Has GLEW been initialized successfully.
      bool is_initialized() { return initialized; }
      
      //! \brief Are all the extensions listed in \p extension_names supported?
      bool is_supported(const char *extension_names);
   
   private:
   
      bool initialized;
   
      GLEWSingleton();
      GLEWSingleton(const GLEWSingleton &);
      
      ~GLEWSingleton();
      
      GLEWSingleton &operator=(const GLEWSingleton &);
   
};

inline GLEWSingleton&
GLEWSingleton::Instance()
{
   
   static GLEWSingleton instance;
   return instance;
   
}

#endif // GLEW_SINGLETON_H_IS_INCLUDED
