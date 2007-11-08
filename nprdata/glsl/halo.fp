// 1D toon shader fragment program

// the accompanying code in gtex/glsl_toon_halo.[HC] is still 
// using GL_TEXTURE2D, so we do the same here:

uniform sampler2D toon_tex; 


void main() 
{   

   const vec3 color = vec3(1.0,1.0,1.0);
   float alpha = gl_TexCoord[0].x;
   
   alpha = alpha * 1.15;
  
  
  alpha = smoothstep(0.0,1.0,alpha);
 
   
  // vec4 tex = texture2D(toon_tex, gl_TexCoord[0].xy);
   
  

   
   // mix rgb with the base color, using the texture alpha:
   //gl_FragColor = vec4(mix(gl_Color.rgb,color,alpha),alpha);
  
    gl_FragColor = vec4(gl_Color.rgb,alpha);
	
}

// halo.fp
