// solid fragment program
uniform vec2 origin;    
uniform vec2 u_vec;     
uniform vec2 v_vec; 

uniform sampler2D paper_tex; 
uniform sampler2D texture; 

uniform float tex_width;
uniform float tex_height;

uniform float st;   // interpolation parameter from s1 to s0

uniform float contrast;

vec3 color_ub(int r, int g, int b) {
  return vec3(float(r)/255.0, float(g)/255.0, float(b)/255.0);
}

vec3 paper_color = vec3(0.0);

vec2 pix_to_uv(vec2 p, vec2 u, vec2 v)
{
   // return components of vector p wrt u and v vectors
   return vec2(dot(p,u)/dot(u,u),dot(p,v)/dot(v,v));
}


//This version of paper effect only uses paper texture, not "texture" texture
float paper_effect()
{   
   float pressure = 0.3;
	 
   vec2 u_vec_p = u_vec*tex_width;
   vec2 v_vec_p = v_vec*tex_height;

   vec2 u = pix_to_uv(gl_FragCoord.xy - origin, u_vec_p, v_vec_p);

   float s1 = 1.0;
   float s0 = s1/2.0;
   vec4 tex = vec4(0.0);
   if (st <= 0.0) {
      tex = texture2D(paper_tex, u/s1);
   } else if (st >= 1.0) {
      tex = texture2D(paper_tex, u/s0);
   } else {
     // blend between the two results
     tex = mix(texture2D(paper_tex, u/s1),texture2D(paper_tex, u/s0),st);
   }
		 
  
   float h = tex.a; 
   h = clamp(contrast*(h-0.5)+0.5,0.0,1.0);  
   float p = clamp(2.0 * pressure,      0.0, 1.0);
   float v = clamp(2.0 * pressure -1.0, 0.0, 1.0);
   float new_alpha = mix(v,p, h); 
   
   return new_alpha;                 
}

void main() 
{   
   //Fragment coords in PIXEL space
   float n_alpha = paper_effect();
   
   
   gl_FragColor = vec4(mix(gl_Color.rgb, paper_color, n_alpha), 1.0); //(gl_Color.rgb,n_alpha);
     
}
// solid.fp
