// Simple halftone shader
// by: Michael Cook
// and totally messed up by Karol :P
// plan "A" LOD transition implemented


varying vec3 normal;


uniform sampler2D tone_map;
uniform sampler2D sampler2D_perlin;

uniform vec2 origin;    
uniform vec2 u_vec;     
uniform vec2 v_vec;
uniform float lod;



// width and height of the window for scaling 
uniform int     width;
uniform int	height;
uniform int	style;  



//controlls the halftone screen scale and offset
//not a part of the style

const float halftone_offset = 0.05;
const float halftone_scale  = 0.85;


//style description

struct stripes_style
{
	float stripe_spacing;
	vec3 stroke_color; 
	float noise_scale;
	float noise_amp;
	mat2 stripe_rotation;

};



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


//converts to the proxy surface uv coordinates
vec2 pixel_to_uv(in vec2 p)
{

        return vec2(dot(p,u_vec)/dot(u_vec,u_vec),
                  dot(p,v_vec)/dot(v_vec,v_vec));
}



//computes the haftone value 
float halftone(in vec2 uv,in float tone,in float spacing,in float lod)
{
	const float e = 0.1;
	// Get a floating point value between 0.0 and spacing
	float scaled_position = mod(uv.y /*+spacing/2.0*/, spacing);
	float h = halftone_scale*(get_lod_height(scaled_position/spacing,lod)+halftone_offset);
	return smoothstep(-e, e, tone-h);

	
}
	


//the thing returns a random two-vector based on the perlin noise 
vec2 get_noise(in vec2 uv,in float noise_scale)
{
	const vec2 middle = vec2(0.5,0.5);
	return (texture2D(sampler2D_perlin,uv*noise_scale).rg - middle);

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
	
	
	
	//this should be part of the style
	stripes_style style_info;
	
	
	if (style == 0)
	style_info = stripes_style(8.0,color_ub(93, 59, 36),
	                           0.01,10.0,mat2(0.707,-0.707,0.707,0.707));
	else if (style == 1)
	style_info = stripes_style(12.0,color_ub(36, 59, 93),
		                   0.01,10.0,mat2(0.707,-0.707,0.707,0.707));
	else if (style == 2)
	style_info = stripes_style(12.0,color_ub(56, 93, 36),
		                   0.01,30.0,mat2(0.707,-0.707,0.707,0.707));
	else
	style_info = stripes_style(10.0,color_ub(36, 36, 36),
		                   0.01,10.0,mat2(0.707,-0.707,0.707,0.707));
	
	
	
	
	
	// Get the texture fragment at the associated screen coordinate.
	float tone = texture2D(tone_map, gl_FragCoord.xy/vec2(float(width),float(height))).r;
	
	

	
		vec2 uv = pixel_to_uv(gl_FragCoord.xy - origin);
		uv = style_info.stripe_rotation*uv;


		//lod scales, current and next 
  		const float s0 = 1.0;	
		const float s1 = s0/2.0;

	
		 vec2  uv_lo = uv/s0;
		 vec2  uv_hi = uv/s1; 
	
		
		//compute noises at both LOD scales
		vec2 noise_hi = get_noise(uv_lo,style_info.noise_scale)*style_info.noise_amp;
		vec2 noise_lo = get_noise(uv_hi,style_info.noise_scale)*style_info.noise_amp * 0.5;
	
		
	
		//mix between the two noise scales based on transition value
		vec2 noise = mix(noise_hi,noise_lo, lod);

	
		uv_lo += noise;	



		float t = halftone(uv_lo,tone,style_info.stripe_spacing,lod);
	

		//final halftone color is a mix according to the halftone value "t"
		gl_FragColor = vec4(mix(style_info.stroke_color, gl_Color.rgb,t) ,gl_Color.a);
	

} 