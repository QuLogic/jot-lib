// halftone-dots.fp
//
//   does halftoning based on dots laid out on a regular grid

//uniform vec2 origin;
uniform int style; // basic dot size in pixels


varying float depth;


varying vec3 normal;

//tone image uniforms

uniform sampler2D tone_map;
uniform int width;
uniform int height;



varying vec3 dU;
varying vec3 dV;
uniform mat4 P_inverse;






struct dot_style
{
   float dot_size;
   vec3  ink;
   bool  do_step;
};



float get_tone(in vec2 pix)
{
   // get tone at given pixel location via tone reference image

   return texture2D(tone_map,
                    vec2(pix.x/float(width),
                         pix.y/float(height))).r;
}



float f(in float x)
{
   const float two_pi = 2.0*3.1415927;
   const float a = 0.4;
   return 0.5 + a * cos(x*two_pi);
}




//Returns height based on LOD blending factor
//it will smoothly transition between one and two hills
//val,lod = [0:1]
float get_lod_f(in float val, in float lod)
{
	float p = lod / 2.0;
	float h;
	
	if (val > p)
	{
		h = f((val - p) / (1.0 - p));
	}
	else
	{
		
		h= f(val / p);
	}
	
	return h;  //many cards do not support conditional returns
	
}


mat3 matrix_derivative(in mat4 T,in vec4 point)
{
	//on the spot projection matrix derivative
	// code taken from mat4.C
	
	
	mat3 T_derivative;
	point.w = 1.0;
	


	vec4 pt =  T * point ;
 	float h = pt.w;
 	pt = pt /pt.w;
        
        T_derivative = mat3(((T[0].xyz - pt.xyz  * T[0][3]) / h ),
                            ((T[1].xyz - pt.xyz  * T[1][3]) / h ),
                            ((T[2].xyz - pt.xyz  * T[2][3]) / h ));
	

	     		       
	     
	return T_derivative;
 }






float
halftone(in vec2 u,in float t,in float spacing, in vec2 lod)
{  
   // Good to take still images...but LOD is messed up...
   //float t = mix(tone, 0.0, pow(1.0-clamp(nv,0.0,1.0),3.0)); 
   
   
   u.x = mod (u.x,spacing) / spacing;
   u.y = mod (u.y,spacing) / spacing;
   
   // "height" value in halftone screen:
   float h = (get_lod_f(u.x,lod.x) + get_lod_f(u.y,lod.y))/2.0;

   // do halftoning w/ a bit of antialiasing:

   float e = 0.12;

   return smoothstep(-e,e,t - h);

}



vec3
color_ub(in int r,in int g,in int b)
{
   return vec3(float(r)/255.0,
               float(g)/255.0,
               float(b)/255.0);

}




void main()
{
   dot_style my_style;
   // Lets do style
   if(style == 0)
      my_style = dot_style(10.0, color_ub(5,  25,   30), false);
   else if(style == 1)
      my_style = dot_style(13.0, color_ub(100, 117, 135), false);
   else 
      my_style = dot_style(15.0, color_ub(93,  50,  23), false);
       
   
   
   // uv coords, not yet scaled:
   float t = 0.0;
   float tone = get_tone(gl_FragCoord.xy);
 

   
   
   
	mat3 inv_proj_derivative = matrix_derivative(P_inverse, gl_FragCoord );

	//equivalents of the opengl screen space derivatives
	vec3 dUp = (dU * inv_proj_derivative);
	vec3 dVp = (dV * inv_proj_derivative);


	//scales based on u and v gradients
	float lu = 1.0 / sqrt( dUp.x*dUp.x + dUp.y * dUp.y );
	float lv = 1.0 / sqrt( dVp.x*dVp.x + dVp.y * dVp.y );




	//dynamic level of detail


	float su =  lu;
	float sv =  lv;
	const float st0 = 1.3; // transition start
	const float st1 = 1.7; // transition end

	vec2 scale_lod; 
	vec2 transition;

	//----------- U direction ------------

	float ps_u =0.0;

	while (su >= 2.0) 
	{
		ps_u+=1.0;
		su /= 2.0;
	}
	while (su < 1.0)  
	{
		ps_u-=1.0;
		su *= 2.0;
	}

	 // find interpolation parameter t between the 2 scales:

	 transition.x = (su < st0) ? 0.0 : (su > st1) ? 1.0: (su - st0)/(st1 - st0);



	 scale_lod.x = pow(2.0,ps_u); 


	//----------- V direction --------------


	float ps_v=0.0;

	while (sv >= 2.0) 
	{
		ps_v+=1.0;
		sv /= 2.0;
	}
	while (sv < 1.0)  
	{
		ps_v-=1.0;
		sv *= 2.0;
	}

		 // find interpolation parameter t between the 2 scales:

	transition.y = (sv < st0) ? 0.0 : (sv > st1) ? 1.0: (sv - st0)/(st1 - st0);



	scale_lod.y = pow(2.0,ps_v); 




	vec2 uv =  (gl_TexCoord[0].xy) * scale_lod;




	t= halftone(uv,tone, my_style.dot_size, transition);

   
      // output halftone value:
   vec3 ink = color_ub(79,135,211); 
   
   //vec3 TestColor = vec3(uv.x * 0.0001, 0.0,0.0);

   gl_FragColor = vec4(mix(my_style.ink,  gl_Color.rgb /* TestColor */, t),1.0);
   //gl_FragColor = vec4(uv.x * 0.01,0.0,uv.y * 0.01,1.0);
   
} 

// dots_ex.fp
