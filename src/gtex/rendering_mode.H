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
#ifndef RENDERING_MODE_H_IS_INCLUDED
#define RENDERING_MODE_H_IS_INCLUDED

/*!
 *  \file rendering_mode.H
 *  \brief Contains the definitions of classes that make it easier to create
 *  gTextures that have multiple rendering modes.  That is, they have multiple
 *  ways to achieve the same effect.  The mode that is used usually depends on
 *  the capabilities of the graphics card that is available.
 *
 */

#include "mesh/patch.H"

#include "gtex/basic_texture.H"

/*!
 *  \brief The abstract interface for a rendering mode.
 *
 *  Rendering modes determine how a gTexture is rendered (for example, using
 *  just shaders, using shaders and textures, or using no shaders).
 *
 *  Concrete rendering modes should subclass this class and override one or more
 *  of the setup_for_drawing or after_drawing methods.  Subclasses should also
 *  create their own subclass of RenderingModeStripCB to act as the GLStripCB for
 *  their gTexture.  The get_new_strip_cb() method should be overriden to return
 *  a newly allocated one of these RenderingModeStripCB subclasses.
 *
 *  \note Each concrete rendering mode is responsible for managing all the OpenGL
 *  resources it needs for rendering.  These resources will most likely be
 *  allocated in the rendering mode's constructor and deallocated in the
 *  destructor.
 *
 */
class RenderingMode {
   
   public:
   
      class RenderingModeStripCB : public GLStripCB {
         
      };
      
      virtual ~RenderingMode() { };
      
      //! \brief Setup the OpenGL state for rendering in this mode.  This
      //! method will be called each time the style is drawn.
      //!
      //! \note Code that uses values that vary from redraw to redraw should
      //! be put in this method so that the values are not captured once in a
      //! display list.
      virtual void setup_for_drawing_outside_dl(const Patch *patch) { }
   
      //! \brief Setup the OpenGL state for rendering in this mode.  This
      //! method will be called once inside a display list.
      virtual void setup_for_drawing_inside_dl(const Patch *patch) { }
      
      //! \brief Perform any operations that should happen after drawing is
      //! complete.  This method will be called each time the style is drawn.
      //! 
      //! \note Code that uses values that vary from redraw to redraw should
      //! be put in this method so that the values are not captured once in a
      //! display list.
      virtual void after_drawing_outside_dl(const Patch *patch) { }
      
      //! \brief Perform any operations that should happen after drawing is
      //! complete.  This method will be called once inside a display list.
      virtual void after_drawing_inside_dl(const Patch *patch) { }
      
      //! \brief Get a newly allocated object of the GLStripCB class used by this
      //! mode.
      //! \note The returned GLStripCB pointer should be deleted by the caller
      //! when it is done being used.
      virtual GLStripCB *get_new_strip_cb() const = 0;
   
};

/*!
 *  \brief A singleton class that keeps track of the rendering mode to be used
 *  by all instances of a particular gTexture.
 *
 *  This class determines which rendering modes are available for use and picks
 *  the best one based on the RenderingModeSelectionPolicy.  This policy should
 *  be a class with a public static class method called SelectRenderingMode that
 *  takes no arguments and returns a pointer to a newly allocated rendering mode
 *  (or 0 if none of the available rendering modes can be used).
 *
 */
template <typename RenderingModeSelectionPolicy>
class RenderingModeSingleton {
   
   public:
   
      inline static RenderingModeSingleton<RenderingModeSelectionPolicy> &Instance();
      
      //! \brief Indicates whether any of the available rendering modes are
      //! supported on the current system.
      bool supported()
         { return mode != 0; }
      
      //! \copydoc RenderingMode::setup_for_drawing_outside_dl(const Patch*)
      void setup_for_drawing_outside_dl(const Patch *patch)
         { if(mode) mode->setup_for_drawing_outside_dl(patch); }
      
      //! \copydoc RenderingMode::setup_for_drawing_inside_dl(const Patch*)
      void setup_for_drawing_inside_dl(const Patch *patch)
         { if(mode) mode->setup_for_drawing_inside_dl(patch); }
      
      //! \copydoc RenderingMode::after_drawing_outside_dl(const Patch*)
      void after_drawing_outside_dl(const Patch *patch)
         { if(mode) mode->after_drawing_outside_dl(patch); }
      
      //! \copydoc RenderingMode::after_drawing_inside_dl(const Patch*)
      void after_drawing_inside_dl(const Patch *patch)
         { if(mode) mode->after_drawing_inside_dl(patch); }
      
      //! \copydoc RenderingMode::get_new_strip_cb()
      GLStripCB *get_new_strip_cb()
         { return mode ? mode->get_new_strip_cb() : 0; }
   
   private:
   
      RenderingMode *mode;
   
      inline RenderingModeSingleton();
      RenderingModeSingleton(const RenderingModeSingleton &other);
      
      inline ~RenderingModeSingleton();
      
      RenderingModeSingleton &operator=(const RenderingModeSingleton &rhs);
   
};

template <typename RenderingModeSelectionPolicy>
inline
RenderingModeSingleton<RenderingModeSelectionPolicy>&
RenderingModeSingleton<RenderingModeSelectionPolicy>::Instance()
{
   
   static RenderingModeSingleton<RenderingModeSelectionPolicy> instance;
   return instance;
   
}

template <typename RenderingModeSelectionPolicy>
inline
RenderingModeSingleton<RenderingModeSelectionPolicy>::RenderingModeSingleton()
   : mode(RenderingModeSelectionPolicy::SelectRenderingMode())
{
   
}

template <typename RenderingModeSelectionPolicy>
inline
RenderingModeSingleton<RenderingModeSelectionPolicy>::~RenderingModeSingleton()
{
   
   delete mode;
   
}

#endif // RENDERING_MODE_H_IS_INCLUDED
