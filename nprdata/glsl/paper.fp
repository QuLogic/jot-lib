// paper shader fragment program
uniform sampler2D paper_tex; 
uniform sampler2D texture; 

uniform vec2 origin;    
uniform vec2 u_vec;     
uniform vec2 v_vec; 

uniform float st;   // interpolation parameter from s1 to s0

uniform float tex_width;
uniform float tex_height;

uniform float contrast; 

vec2 pix_to_uv(vec2 p, vec2 u, vec2 v)
{
   // return components of vector p wrt u and v vectors
   return vec2(dot(p,u)/dot(u,u),dot(p,v)/dot(v,v));
}

float paper_effect()
{
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
     
   vec4 texture_pix = texture2D(texture, gl_TexCoord[0].xy); // Paper coords 
    
   float h = tex.a;   
   float press = 0.5;
   h = clamp(contrast*(h-0.5)+0.5,0.0,1.0);
   float p = clamp(2.0 * press, 0.0, 1.0);
   float v = clamp(2.0 * press-1.0, 0.0, 1.0);
   float new_alpha = mix(v,p, h); 
   new_alpha = min(new_alpha, texture_pix.a);
   return new_alpha;                 
}

void main() 
{   
   gl_FragColor = vec4(gl_Color.rgb, paper_effect());     
}

// paper.fp
