// Experimental halftone shader
// by Karol Szerszen


//rigged up to use model's texture coordinates
//lod is non functional at the moment

varying vec3 normal;
varying vec4 position;


varying vec3 dU;
varying vec3 dV;
uniform mat4 P_inverse;

uniform sampler2D tone_map;
uniform sampler2D sampler2D_perlin;



//uniform int style;

//varying float lod;



// width and height of the window for scaling 
uniform int     width;
uniform int	height;



//controlls the halftone screen scale and offset
//not a part of the style

const float halftone_offset = 0.125;
const float halftone_scale  = 0.75;




// Returns the height of the hat function at 'val'	
float get_height(in float val)
{
	return (val < 0.5) ? (val/0.5) : (1.0 - val)/0.5;
}



//Returns height based on LOD blending factor
//it will smoothly transition between one and two hills
//val,lod = [0:1]
float get_lod_height(in float val, in float lod)
{
	float p = lod / 2.0;
	float h;
	
	if (val > p)
	{
		h = get_height((val - p) / (1.0 - p));
	}
	else
	{
		
		h= get_height(val / p);
	}
	
	return h;  //many cards do not support conditional returns
	
}




//computes the haftone value 
float halftone(in vec2 uv,in float tone,in float spacing,in float lod)
{
	const float e = 0.15;
	// Get a floating point value between 0.0 and spacing
	float scaled_position = mod(uv.y /*+spacing/2.0*/, spacing);
	float h = halftone_scale*(get_lod_height(scaled_position/spacing,lod)+halftone_offset);
	return smoothstep(-e, e, tone-h);

	
}
	


//the thing returns a random two-vector based on the perlin noise 
vec2 get_noise(in vec2 uv)
{
	const vec2 middle = vec2(0.5,0.5);
	return (texture2D(sampler2D_perlin,uv).rg - middle);

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
                                   

	            

void main()
{
	
	
	
	//this should be part of the style
	const float spacing = 9.0 ; //0.05;
	const vec3 stroke_color = vec3(0.0,0.0,0.0); //vec3(93./255., 59./255., 36./255.);
	const float noise_scale = 0.05; //coordinate scale
	const float noise_amp = 4.0; //0.05; //1.0;   //value scale
	const mat2 rotation = mat2(0.707,-0.707,0.707,0.707);
	
	
	
	
	
	
	// Get the texture fragment at the associated screen coordinate.
	float tone = texture2D(tone_map, gl_FragCoord.xy/vec2(float(width),float(height))).r;
	
		

	

		
				
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
		
		
		
	
		vec2  uv_lo_u_lo_v = uv;
		vec2  uv_hi_u_lo_v = vec2(uv.x * 2.0, uv.y); 
		vec2  uv_lo_u_hi_v = vec2(uv.x, uv.y * 2.0); 
		vec2  uv_hi_u_hi_v = vec2(uv.x * 2.0, uv.y * 2.0); 
		
		
		
		
		//compute noises at all LOD scales
		vec2 noise_lo_u_lo_v = get_noise(uv_lo_u_lo_v * noise_scale) * noise_amp;
		vec2 noise_hi_u_lo_v = get_noise(uv_hi_u_lo_v * noise_scale) * noise_amp;
		
		vec2 noise_lo_u_hi_v = get_noise(uv_lo_u_hi_v * noise_scale) * noise_amp;
		vec2 noise_hi_u_hi_v = get_noise(uv_hi_u_hi_v * noise_scale) * noise_amp;
		

		//mix between the two noise scales based on transition values
		
		
		vec2 noise_lo_v = mix(noise_lo_u_lo_v,noise_hi_u_lo_v,transition.x);
		vec2 noise_hi_v = mix(noise_lo_u_hi_v,noise_hi_u_hi_v,transition.x) * 0.5;
		
		vec2 noise = mix(noise_lo_v,noise_hi_v,transition.y);

	
		uv += noise;	//noise is turned off for scale compensation fine tuning

		

		float t = halftone(uv,tone,spacing,transition.y);
		//t=1.0;
		


		//final halftone color is a mix according to the halftone value "t"
		 
		
		//vec3 color_t = vec3(0.0,  0.0  , uv_lo_u_lo_v.x * 0.001) ;
		
		
		
		gl_FragColor = vec4(  mix(stroke_color,  gl_Color.rgb /* color_t */ ,t) ,gl_Color.a);
		//gl_FragColor = vec4(  color_t   ,gl_Color.a);
	

} 