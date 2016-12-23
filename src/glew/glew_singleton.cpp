/*!
 *  \file glew_singleton.cpp
 *  \brief Contains the implementation of the GLEWSingleton class.
 *
 *  \sa glew_singleton.hpp
 *
 */

#include <iostream>

using namespace std;

#include "glew/glew_singleton.hpp"
#include <GL/glew.h>

GLEWSingleton::GLEWSingleton()
   : initialized(false)
{
   
   GLenum err = glewInit();
   
   if(err == GLEW_OK){
      
      initialized = true;
      
   } else {
      
      initialized = false;
      
      cerr << "GLEWSingleton::GLEWSingleton() - Error while initializing GLEW:  "
           << glewGetErrorString(err) << endl;
      
   }
   
}

GLEWSingleton::~GLEWSingleton()
{
   
   // Doesn't do anything...
   
}

bool
GLEWSingleton::is_supported(const char *extension_names)
{
   
   return initialized ? (glewIsSupported(extension_names) != 0) : false;
   
}
