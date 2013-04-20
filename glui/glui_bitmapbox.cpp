/****************************************************************************

  GLUI User Interface Toolkit
  ---------------------------

     glui_bitmapbox - GLUI_BitmapBox control class


          --------------------------------------------------

  Copyright (c) 1998 Paul Rademacher

  This program is freely distributable without licensing fees and is
  provided without guarantee or warrantee expressed or implied. This
  program is -not- in the public domain.

*****************************************************************************/

#include "glui/glui_jot.H"
#include "glui_internal_control.h"
#include <cassert>

#define GLUI_BITMAPBOX_FONT_HEIGHT					9
#define GLUI_BITMAPBOX_FONT_DROP						3
#define GLUI_BITMAPBOX_FONT_FULL_HEIGHT			(GLUI_BITMAPBOX_FONT_HEIGHT + GLUI_BITMAPBOX_FONT_DROP)
#define GLUI_BITMAPBOX_FONT_MID_HEIGHT				4

/*#define GLUI_BITMAPBOX_NAME_INDENT					6*/
#define GLUI_BITMAPBOX_NAME_INDENT					4
#define GLUI_BITMAPBOX_NAME_SIDE_BORDER			2
/*#define GLUI_BITMAPBOX_NAME_TOP_BORDER				2*/
#define GLUI_BITMAPBOX_NAME_TOP_BORDER				0
#define GLUI_BITMAPBOX_NAME_BOTTOM_BORDER			0

#define GLUI_BITMAPBOX_IMG_TOP_BORDER				1
#define GLUI_BITMAPBOX_IMG_BOTTOM_BORDER			2
#define GLUI_BITMAPBOX_IMG_SIDE_BORDER				2

/********************** GLUI_BitmapBox::mouse_down_handler() ******/

int    GLUI_BitmapBox::mouse_down_handler( int local_x, int local_y )
{
	int bitmap_x, bitmap_y;

	bitmap_x = local_x - x_abs - (w - image_w)/2;
	if (bitmap_x < 0) bitmap_x = 0;
	if (bitmap_x >= image_w) bitmap_x = image_w-1;
   bitmap_y = ((h-1) - ((border&&(margin>=2))?GLUI_BITMAPBOX_IMG_BOTTOM_BORDER:0) - margin) - (local_y - y_abs);
	if (bitmap_y < 0) bitmap_y = 0;
	if (bitmap_y >= image_h) bitmap_y = image_h-1;

	//printf("%d %d\n",bitmap_x, bitmap_y);

	event = GLUI_BITMAPBOX_EVENT_MOUSE_DOWN;
	event_x = bitmap_x;
	event_y = bitmap_y;
   event_in = true;
	event_key = -1;
	event_mod = -1;

   currently_inside = true;

   currently_pressed = true;

	execute_callback();

   draw_translated_active_area(true);

	return false;
}


/**************************** GLUI_BitmapBox::mouse_up_handler() */

int    GLUI_BitmapBox::mouse_up_handler( int local_x, int local_y, int new_inside )
{
	int bitmap_x, bitmap_y, old_inside;

	bitmap_x = local_x - x_abs - (w - image_w)/2;
	if (bitmap_x < 0) bitmap_x = 0;
	if (bitmap_x >= image_w) bitmap_x = image_w-1;
   bitmap_y = ((h-1) - ((border&&(margin>=2))?GLUI_BITMAPBOX_IMG_BOTTOM_BORDER:0) - margin) - (local_y - y_abs);
	if (bitmap_y < 0) bitmap_y = 0;
	if (bitmap_y >= image_h) bitmap_y = image_h-1;

	//printf("%d %d\n",bitmap_x, bitmap_y);

	event = GLUI_BITMAPBOX_EVENT_MOUSE_UP;
	event_x = bitmap_x;
	event_y = bitmap_y;
   event_in = new_inside;
	event_key = -1;
	event_mod = -1;

   old_inside = currently_inside;
   currently_inside = new_inside;

   currently_pressed = false;

	execute_callback();

   if ( old_inside ) draw_translated_active_area(true);

	return false;
}


/****************************** GLUI_BitmapBox::mouse_held_down_handler() ******/

int    GLUI_BitmapBox::mouse_held_down_handler( int local_x, int local_y, int new_inside)
{

	int bitmap_x, bitmap_y, old_inside;

	bitmap_x = local_x - x_abs - (w - image_w)/2;
	if (bitmap_x < 0) bitmap_x = 0;
	if (bitmap_x >= image_w) bitmap_x = image_w-1;
   bitmap_y = ((h-1) - ((border&&(margin>=2))?GLUI_BITMAPBOX_IMG_BOTTOM_BORDER:0) - margin) - (local_y - y_abs);
	if (bitmap_y < 0) bitmap_y = 0;
	if (bitmap_y >= image_h) bitmap_y = image_h-1;

	//printf("%d %d\n",bitmap_x, bitmap_y);

	event = GLUI_BITMAPBOX_EVENT_MOUSE_MOVE;
	event_x = bitmap_x;
	event_y = bitmap_y;
   event_in = new_inside;
	event_key = -1;
	event_mod = -1;

   old_inside = currently_inside;
	currently_inside = new_inside;

   currently_pressed = true;

	execute_callback();

   if ( old_inside != new_inside ) draw_translated_active_area(true);

	return false;
}









/********************** GLUI_BitmapBox::general_mouse_down_handler() ******/

int    GLUI_BitmapBox::general_mouse_down_handler( int but, int local_x, int local_y )
{
	int bitmap_x, bitmap_y;

	bitmap_x = local_x - x_abs - (w - image_w)/2;
/*
	if (bitmap_x < 0) bitmap_x = 0;
	if (bitmap_x >= image_w) bitmap_x = image_w-1;
*/
   bitmap_y = ((h-1) - ((border&&(margin>=2))?GLUI_BITMAPBOX_IMG_BOTTOM_BORDER:0) - margin) - (local_y - y_abs);
/*
	if (bitmap_y < 0) bitmap_y = 0;
	if (bitmap_y >= image_h) bitmap_y = image_h-1;
*/
   if (but == GLUI_MOUSE_MIDDLE)
   {
	   event = GLUI_BITMAPBOX_EVENT_MIDDLE_DOWN;
   }
   else if (but == GLUI_MOUSE_RIGHT)
   {
      event = GLUI_BITMAPBOX_EVENT_RIGHT_DOWN;
   }
   else
   {
      assert(0);
   }

   event_x = bitmap_x;
	event_y = bitmap_y;
   event_in = true;
	event_key = -1;
	event_mod = -1;

   /* Maybe make this optional */
	execute_callback();

	return false;
}


/**************************** GLUI_BitmapBox::general_mouse_up_handler() */

int    GLUI_BitmapBox::general_mouse_up_handler( int but, int local_x, int local_y, int new_inside )
{
	int bitmap_x, bitmap_y;

	bitmap_x = local_x - x_abs - (w - image_w)/2;
/*
	if (bitmap_x < 0) bitmap_x = 0;
	if (bitmap_x >= image_w) bitmap_x = image_w-1;
*/
   bitmap_y = ((h-1) - ((border&&(margin>=2))?GLUI_BITMAPBOX_IMG_BOTTOM_BORDER:0) - margin) - (local_y - y_abs);
/*
	if (bitmap_y < 0) bitmap_y = 0;
	if (bitmap_y >= image_h) bitmap_y = image_h-1;
*/
   if (but == GLUI_MOUSE_MIDDLE)
   {
	   event = GLUI_BITMAPBOX_EVENT_MIDDLE_UP;
   }
   else if (but == GLUI_MOUSE_RIGHT)
   {
      event = GLUI_BITMAPBOX_EVENT_RIGHT_UP;
   }
   else
   {
      assert(0);
   }

	event_x = bitmap_x;
	event_y = bitmap_y;
   event_in = new_inside;
	event_key = -1;
	event_mod = -1;

   /* Maybe make this optional */
	execute_callback();

	return false;
}


/****************************** GLUI_BitmapBox::general_mouse_held_down_handler() ******/

int    GLUI_BitmapBox::general_mouse_held_down_handler( int but, int local_x, int local_y, int new_inside)
{
	int bitmap_x, bitmap_y;

	bitmap_x = local_x - x_abs - (w - image_w)/2;
/*
	if (bitmap_x < 0) bitmap_x = 0;
	if (bitmap_x >= image_w) bitmap_x = image_w-1;
*/
   bitmap_y = ((h-1) - ((border&&(margin>=2))?GLUI_BITMAPBOX_IMG_BOTTOM_BORDER:0) - margin) - (local_y - y_abs);
/*
   if (bitmap_y < 0) bitmap_y = 0;
	if (bitmap_y >= image_h) bitmap_y = image_h-1;
*/
   if (but == GLUI_MOUSE_MIDDLE)
   {
	   event = GLUI_BITMAPBOX_EVENT_MIDDLE_MOVE;
   }
   else if (but == GLUI_MOUSE_RIGHT)
   {
      event = GLUI_BITMAPBOX_EVENT_RIGHT_MOVE;
   }
   else
   {
      assert(0);
   }
	event_x = bitmap_x;
	event_y = bitmap_y;
   event_in = new_inside;
	event_key = -1;
	event_mod = -1;

   /* Maybe make this optional */
	execute_callback();

	return false;
}

/****************************** GLUI_BitmapBox::special_handler() **********/

int    GLUI_BitmapBox::special_handler( int key,int mods )
{

	//printf("%d %d\n",key,mods);

	event = GLUI_BITMAPBOX_EVENT_KEY;
	event_x = -1;
	event_y = -1;
   event_in = -1;
	event_key = key;
	event_mod = mods;

	execute_callback();

	return false;
}


/****************************** GLUI_BitmapBox::draw() **********/

void    GLUI_BitmapBox::draw( int x, int y )
{
   GLUI_DRAWINGSENTINAL_IDIOM;

   if (border && (margin>=2))
   {
      if (string_width(name) > 0)
      {
	      draw_emboss_box(	margin - 2,
							      (w-1) - margin + 2,
							      margin +
                           GLUI_BITMAPBOX_NAME_TOP_BORDER +
							      GLUI_BITMAPBOX_FONT_HEIGHT - 1 -
							      GLUI_BITMAPBOX_FONT_MID_HEIGHT,
							      (h-1) - margin + 2);

	      draw_bkgd_box( margin +
                        GLUI_BITMAPBOX_NAME_INDENT-1,
						      margin +
                        GLUI_BITMAPBOX_NAME_INDENT +
						      string_width(name) +
						      2*GLUI_BITMAPBOX_NAME_SIDE_BORDER - 1,
						      margin, /* -2 negative?! */
						      margin +
						      GLUI_BITMAPBOX_NAME_TOP_BORDER +
						      GLUI_BITMAPBOX_FONT_FULL_HEIGHT +
						      GLUI_BITMAPBOX_NAME_BOTTOM_BORDER);

	      draw_name(	margin +
                     GLUI_BITMAPBOX_NAME_INDENT +
					      GLUI_BITMAPBOX_NAME_SIDE_BORDER,
					      margin +
                     GLUI_BITMAPBOX_FONT_HEIGHT-1 +
					      GLUI_BITMAPBOX_NAME_TOP_BORDER);
      }
      else
      {
	      draw_emboss_box(	margin - 2,
							      (w-1) - margin + 2,
                           margin - 2,
							      (h-1) - margin + 2);
      }
   }

	draw_active_area();

   if (border && (margin >= 2))
   {
      if (string_width(name) > 0)
      {
	      draw_active_box(	margin +
                           GLUI_BITMAPBOX_NAME_INDENT,
							      margin +
                           GLUI_BITMAPBOX_NAME_INDENT +
							      string_width(name) +
							      2*GLUI_BITMAPBOX_NAME_SIDE_BORDER - 1,
							      margin - 2,
							      margin +
							      GLUI_BITMAPBOX_NAME_TOP_BORDER +
                           GLUI_BITMAPBOX_FONT_FULL_HEIGHT +
							      GLUI_BITMAPBOX_NAME_BOTTOM_BORDER - 1);
      }
      else
      {
	      draw_active_box(	margin + 1,
							      (w-1) - margin - 1,
                           margin + 1,
                           (h-1) - margin - 1);
      }

   }
   else if (margin >= 2)
   {
	   draw_active_box(	margin - 2,
							   (w-1) - margin + 2,
                        margin - 2,
                        (h-1) - margin + 2);
   }


}


/************************************ GLUI_BitmapBox::update_size() **********/

void   GLUI_BitmapBox::update_size( void )
{
	int name_w, img_w;

	if ( !glui )
		return;


   if (border && (margin >=2))
   {
      if (string_width(name) > 0)
      {
	      h =	margin +
               GLUI_BITMAPBOX_NAME_TOP_BORDER +
			      GLUI_BITMAPBOX_FONT_FULL_HEIGHT +
			      GLUI_BITMAPBOX_NAME_BOTTOM_BORDER +
               GLUI_BITMAPBOX_IMG_TOP_BORDER +
			      image_h +
               GLUI_BITMAPBOX_IMG_BOTTOM_BORDER +
			      margin;

	      name_w =	margin +
                  GLUI_BITMAPBOX_NAME_INDENT +
				      string_width(name) +
				      2*GLUI_BITMAPBOX_NAME_SIDE_BORDER +
				      GLUI_BITMAPBOX_NAME_INDENT +
                  margin;

         img_w = margin + 2*GLUI_BITMAPBOX_IMG_SIDE_BORDER + image_w + margin;

	      if (img_w >= name_w)
         {
		      w = img_w;
         }
	      else
	      {
		      w = img_w + 2*((name_w - img_w + 1)/2);
	      }
      }
      else
      {
	      h =	margin +
               GLUI_BITMAPBOX_IMG_TOP_BORDER +
			      image_h +
               GLUI_BITMAPBOX_IMG_BOTTOM_BORDER +
			      margin;

         w = margin +
            GLUI_BITMAPBOX_IMG_SIDE_BORDER +
            image_w +
            GLUI_BITMAPBOX_IMG_SIDE_BORDER +
            margin;
      }
   }
   else
   {
	   h =   margin +
			   image_h +
			   margin;

      w = margin +
          image_w +
          margin;
   }
}


/****************************** GLUI_BitmapBox::draw_translated_active_area() **********/

void    GLUI_BitmapBox::draw_translated_active_area( int front )
{
   float x,y;

	int win_h;
	int win_w;

	GLUI_DRAWINGSENTINAL_IDIOM;

	win_h = glutGet(GLUT_WINDOW_HEIGHT);
	win_w = glutGet(GLUT_WINDOW_WIDTH);

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	glTranslatef( (float) win_w/2.0, (float) win_h/2.0, 0.0 );
	glRotatef( 180.0, 0.0, 1.0, 0.0 );
	glRotatef( 180.0, 0.0, 0.0, 1.0 );
	glTranslatef( (float) -win_w/2.0, (float) -win_h/2.0, 0.0 );
	//XXX - Why did I put the .5's in?! This cause ATI cards to
   //      'blend' the bitmap between adjacent pixels since
   //      we we're half a pixel out of step... leading to artifacts...

   x = this->x_abs + .5f;
   y = this->y_abs + .5f;
	//printf("Translate to: %f %f\n",x,y);

   glTranslatef( x, y, 0.0 );

	draw_active_area();

	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}

/****************************** GLUI_BitmapBox::draw_active_area() **********/

void    GLUI_BitmapBox::draw_active_area( void )
{
   float x,y;
	if (image && (image_w>0) && (image_h>0))
	{
      x = ((int)((w - image_w)/2)) - 0.5;
      y = ((int)((h-1) - ((border&&(margin>=2))?GLUI_BITMAPBOX_IMG_BOTTOM_BORDER:0) - margin) + 1) - 0.5;

      //printf("Drawing at: %f %f\n",x,y);

      glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

      if (depressable && (margin>0))
      {
         if (currently_inside && currently_pressed)
         {
            glRasterPos2f( x+1, y+1 );
         }
         else
         {
            glRasterPos2f( x, y );
         }
      }
      else
      {
         glRasterPos2f( x, y );
      }

      if (enabled AND (!glui OR !glui->get_blocked()))
         glDrawPixels( image_w, image_h, GL_RGB, GL_UNSIGNED_BYTE, image );
      else
         glDrawPixels( image_w, image_h, GL_RGB, GL_UNSIGNED_BYTE, image_disabled );

      if (depressable && (margin>0))
      {
         unsigned char *foo = (unsigned char *)calloc( 3*(image_w+image_h), sizeof(unsigned char)); assert(foo);

         for (int i=0; i<(image_w+image_h); i++)
         {
            foo[3*i  ] = glui->bkgd_color[0];
            foo[3*i+1] = glui->bkgd_color[1];
            foo[3*i+2] = glui->bkgd_color[2];

         }

         if (currently_inside && currently_pressed)
         {
            glRasterPos2f( x , y );
            glDrawPixels( 1, image_h, GL_RGB, GL_UNSIGNED_BYTE, foo );

            glRasterPos2f( x , y - (image_h-1));
            glDrawPixels( image_w, 1, GL_RGB, GL_UNSIGNED_BYTE, foo );
         }
         else
         {
            glRasterPos2f( (x+1) , (y+1) );
            glDrawPixels( image_w, 1, GL_RGB, GL_UNSIGNED_BYTE, foo );

            glRasterPos2f( (x+1) + (image_w-1), (y+1) );
            glDrawPixels( 1, image_h, GL_RGB, GL_UNSIGNED_BYTE, foo );
         }
      }
   }

}

/****************************** GLUI_BitmapBox::get_event() **********/

int    GLUI_BitmapBox::get_event( void )
{
   return event;
}

/****************************** GLUI_BitmapBox::get_event_key() **********/

int    GLUI_BitmapBox::get_event_key( void )
{
   return event_key;
}

/****************************** GLUI_BitmapBox::get_event_mod() **********/

int    GLUI_BitmapBox::get_event_mod( void )
{
   return event_mod;
}

/****************************** GLUI_BitmapBox::get_event_in() **********/

int    GLUI_BitmapBox::get_event_in( void )
{
   return event_in;
}

/****************************** GLUI_BitmapBox::get_event_x() **********/

int    GLUI_BitmapBox::get_event_x( void )
{
   return event_x;
}

/****************************** GLUI_BitmapBox::get_event_y() **********/

int    GLUI_BitmapBox::get_event_y( void )
{
   return event_y;
}

/****************************** GLUI_BitmapBox::get_image_w() **********/

int    GLUI_BitmapBox::get_image_w( void )
{
   return image_w;
}

/****************************** GLUI_BitmapBox::get_image_h() **********/

int    GLUI_BitmapBox::get_image_h( void )
{
   return image_h;
}

/****************************** GLUI_BitmapBox::copy_img() **********/

void    GLUI_BitmapBox::copy_img(unsigned char *i_data,int i_w, int i_h, int i_bpp)
{

	if (!i_data)
	{
		fprintf(stderr,"GLUI_BitmapBox::copy_img - Supplied image buffer is NULL.\n");
		return;
	}

	if (image)
	{
		if ((i_w != image_w) || (i_h != image_h))
		{
			fprintf(stderr,"GLUI_BitmapBox::copy_img - Supplied image doesn't match internal image's dimensions.\n");
			return;
		}
		if (i_bpp != 3)
		{
			fprintf(stderr,"GLUI_BitmapBox::copy_img - Supplied image must have 3 BPP.\n");
			return;
		}
		for (int y=0;y<image_h;y++)
      {
		   for (int x=0;x<image_w;x++)
         {
            int i = 3*(image_w*y + x);

            image[i  ] = i_data[i  ];
            image[i+1] = i_data[i+1];
            image[i+2] = i_data[i+2];

            if (((x+y)%2)==0)
            {
               image_disabled[i  ] = i_data[i  ];
               image_disabled[i+1] = i_data[i+1];
               image_disabled[i+2] = i_data[i+2];
            }
            else
            {
               image_disabled[i  ] = glui->bkgd_color[0];
               image_disabled[i+1] = glui->bkgd_color[1];
               image_disabled[i+2] = glui->bkgd_color[2];
            }
         }
      }

		draw_translated_active_area(true);
	}
	else
	{
		fprintf(stderr,"GLUI_BitmapBox::copy_img - Internal image buffer is not alloced.\n");
	}

}

/****************************** GLUI_BitmapBox::set_img_size() **********/

void    GLUI_BitmapBox::set_img_size( int i_w, int i_h )
{
	unsigned char *new_image, *new_image_disabled;


	if ((i_w>0) && (i_h>0))
	{

		new_image =          (unsigned char *)calloc( i_w*i_h*3, sizeof(unsigned char));
      new_image_disabled = (unsigned char *)calloc( i_w*i_h*3, sizeof(unsigned char));

		if (new_image && new_image_disabled)
		{
			if (image)           free(image);
			if (image_disabled)  free(image_disabled);
			image = new_image;
         image_disabled = new_image_disabled;
			image_w = i_w;
			image_h = i_h;

			update_size();
			if( glui ) glui->refresh();

		}
		else
		{
			fprintf(stderr,"GLUI_BitmapBox::set_img_size - Failed to alloc new image.\n");
		}

	}
	else
	{
		fprintf(stderr,"GLUI_BitmapBox::set_img_size - Bad dimensions.\n");
	}

}

/****************************** GLUI_BitmapBox::get_border() **********/

int     GLUI_BitmapBox::get_border( void )
{
   return border;
}

/****************************** GLUI_BitmapBox::set_border() **********/

void    GLUI_BitmapBox::set_border( int b )
{
   border = b;
   update_size();
   if ( glui )
      glui->refresh();
}

/****************************** GLUI_BitmapBox::get_margin() **********/

int     GLUI_BitmapBox::get_margin( void )
{
   return margin;
}

/****************************** GLUI_BitmapBox::set_margin() **********/

void    GLUI_BitmapBox::set_margin( int m )
{
   margin = m;
   update_size();
   if ( glui )
      glui->refresh();
}

/****************************** GLUI_BitmapBox::get_depressable() **********/

int     GLUI_BitmapBox::get_depressable( void )
{
   return depressable;
}

/****************************** GLUI_BitmapBox::set_depressable() **********/

void    GLUI_BitmapBox::set_depressable( int d )
{
   depressable = d;
}

/****************************** GLUI_BitmapBox::execute_callback() **********/

void    GLUI_BitmapBox::execute_callback( void )
{

	GLUI_Control::execute_callback();

	event							= GLUI_BITMAPBOX_EVENT_NONE;
	event_key					= -1;
	event_mod					= -1;
   event_in						= -1;
	event_x						= -1;
	event_y						= -1;

}

/************** GLUI_BitmapBox::GLUI_BitmapBox() ********************/

GLUI_BitmapBox::GLUI_BitmapBox(GLUI_Node *parent, const char *name,
                               int id, GLUI_CB cb, bool active)
{
   common_init();
   user_id = id;
   callback = cb;
   set_name( name );
   can_activate = active;

   parent->add_control( this );
}

/************** GLUI_BitmapBox::GLUI_~BitmapBox() ********************/

GLUI_BitmapBox::~GLUI_BitmapBox( void )
{

	if (image)
		free(image);
	if (image_disabled)
		free(image_disabled);

	image = NULL;
	image_disabled = NULL;
	image_w = 0;
	image_h = 0;

}
