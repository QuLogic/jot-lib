#include "glui.h"
#include "stdinc.h"

/****************************** GLUI_ActiveText::mouse_down_handler() **********/

int    GLUI_ActiveText::mouse_down_handler( int local_x, int local_y )
{
   int old_inside;

   set_int_val( 1 );   

   old_inside  = currently_inside;
   currently_inside = true;

   currently_pressed = true;
  
   draw_translated_active_area();

   return false;
}


/****************************** GLUI_ActiveText::mouse_held_down_handler() ******/

int    GLUI_ActiveText::mouse_held_down_handler( int local_x, int local_y,
					     int new_inside)
{
   int old_inside;

   old_inside  = currently_inside;
   currently_inside = new_inside;

   currently_pressed = true;

   if ( new_inside != old_inside ) 
   {
      draw_translated_active_area();
   } 
  
   return false;
}


/****************************** GLUI_ActiveText::mouse_up_handler() **********/

int    GLUI_ActiveText::mouse_up_handler( int local_x, int local_y, int new_inside )
{
   int old_inside;

   set_int_val( 0 );   

   old_inside  = currently_inside;
   currently_inside = new_inside;

   currently_pressed = false;
  
   if ( old_inside )
   {
      draw_translated_active_area();
   }

   if ( currently_inside ) 
   {
      execute_callback();
   }

   return false;
}



/****************************** GLUI_ActiveText::key_handler() **********/

int    GLUI_ActiveText::key_handler( unsigned char key,int modifiers )
{
   /*execute_callback();*/

   return false;
}


/****************************** GLUI_ActiveText::draw_translated_active_area() **********/

void    GLUI_ActiveText::draw_translated_active_area( )
{
   int state, orig;

   if ( NOT can_draw() ) return;

   orig  = set_to_glut_window();
   state = glui->set_front_draw_buffer();

   glPushMatrix();
      translate_to_origin();
      draw_active_area();
   glPopMatrix();

   glui->restore_draw_buffer(state);
   restore_window(orig);

}

/****************************** GLUI_ActiveText::draw_active_area() **********/

void    GLUI_ActiveText::draw_active_area( void )
{

  if (currently_highlighted)
  {
     glColor3ub( glui->bkgd_color.r+25, glui->bkgd_color.g+25, glui->bkgd_color.b+25 );
  }
  else
  {
     glColor3ub( glui->bkgd_color.r, glui->bkgd_color.g, glui->bkgd_color.b );
  }
    
  glBegin( GL_QUADS );
  glVertex2f( -0.5 + 2  , -0.5 + 2   );     glVertex2f( -0.5 + w-2, -0.5 + 2   );
  glVertex2f( -0.5 + w-2, -0.5 + h-1 );     glVertex2f( -0.5 + 2  , -0.5 + h-1 );
  glEnd();

  if ( currently_pressed && currently_inside ) 
  {
    draw_name( 3 + 1, 13 + 1);
  }
  else 
  {
    draw_name( 3 , 13 );
  }


}


/********************************************** GLUI_ActiveText::draw() **********/

void    GLUI_ActiveText::draw( int x, int y )
{
  if ( NOT can_draw() ) return;

  draw_active_area();

  draw_active_box(0, w-1, 0, h);

}

/********************************* GLUI_Listbox::check_fit() **************/

int	GLUI_ActiveText::check_fit(char *s)
{

  if ( w < string_width(s) + 6 )
    return 0;
  else 
    return 1;
}

/************************************** GLUI_ActiveText::update_size() **********/

void   GLUI_ActiveText::update_size( void )
{
  int text_size;

  if ( NOT glui ) return;

  text_size = string_width( name );

  if ( w < text_size + 6 )  w = text_size + 6 ;
}

/************************************** GLUI_ActiveText::set_text() **********/

void    GLUI_ActiveText::set_text( char *text )
{
  if (strcmp(text,name) == 0) return;

  int old_w = w;

  strncpy((char*)name,text,sizeof(GLUI_String)); 
  
  update_size();
  
  if ((w != old_w)&&(glui)) glui->refresh();

  draw_translated_active_area();
}




/*
void    GLUI_ActiveText::draw( int x, int y )
{
  int orig;

  if ( NOT can_draw() )
    return;

  orig = set_to_glut_window();

  draw_text();

  restore_window( orig );
}


void    GLUI_ActiveText::set_text( char *text )
{
  int orig;

  if (strcmp(text,name) == 0) return;

  if ( can_draw() )
  {
      orig = set_to_glut_window();

     glMatrixMode( GL_MODELVIEW );
     glPushMatrix();
     translate_to_origin();
     erase_text();
     glPopMatrix();
  }
  
  int old_w = w;
  strncpy((char*)name,text,sizeof(GLUI_String)); 
  update_size();
  if ((w != old_w)&&(glui)) glui->refresh();

  if ( can_draw() )
  {

     glMatrixMode( GL_MODELVIEW );
     glPushMatrix();
     translate_to_origin();
     draw_text();
     glPopMatrix();

     restore_window( orig );
  }
}

void   GLUI_ActiveText::update_size( void )
{
  int text_size;

  if ( NOT glui )
    return;

  text_size = string_width( name );

  if ( w < text_size )
    w = text_size;    
}


void    GLUI_ActiveText::draw_text( void )
{
  if ( NOT can_draw() )
    return;

  erase_text();
  draw_name( 0, 13 );
}


void    GLUI_ActiveText::erase_text( void )
{
  if ( NOT can_draw() )
    return;

  set_to_bkgd_color();
  glDisable( GL_CULL_FACE );
  glBegin( GL_TRIANGLES );
  glVertex2i( 0,0 );   glVertex2i( w, 0 );  glVertex2i( w, h );  
  glVertex2i( 0, 0 );  glVertex2i( w, h );  glVertex2i( 0, h );   
  glEnd();
}




void    GLUI_ActiveText::draw_translated_active_area( )
{
	int orig,  state;
	int win_h, win_w;
   float x, y;
	
   if ( !glui ) return;

	win_h = glutGet(GLUT_WINDOW_HEIGHT);
	win_w = glutGet(GLUT_WINDOW_WIDTH);
   
   x = this->x_abs + .5f;
   y = this->y_abs + .5f;

	orig = set_to_glut_window();

	state = glui->set_front_draw_buffer();

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	glTranslatef( (float) win_w/2.0, (float) win_h/2.0, 0.0 );
	glRotatef( 180.0, 0.0, 1.0, 0.0 );	glRotatef( 180.0, 0.0, 0.0, 1.0 );
	glTranslatef( (float) -win_w/2.0, (float) -win_h/2.0, 0.0 );
   glTranslatef( x, y, 0.0 );

	   draw_active_area();

	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

	glui->restore_draw_buffer(state);  

	restore_window(orig);

}
*/
