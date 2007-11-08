
//************************
//
//	Separable 
//	Blur
//	
//	by KS
//
//*************************



uniform sampler2D input_map;
uniform float x_size;
uniform float y_size;
uniform int   direction; //separable filter pass direction [row,column]
uniform int   kernel_size; //column, row size

vec4 average_neighbors(vec2 pix)
{
   int filter_step;
   float sample_pos;
   vec4 avg = vec4(0.0,0.0,0.0,0.0);
   

   if (direction==0) //horizontal of the separable filter
   {
   	for(filter_step = -kernel_size; filter_step <= kernel_size; filter_step++)
   	{
		 sample_pos = pix.x/x_size + float(filter_step)/x_size;
		 avg += texture2D(input_map, vec2(sample_pos, pix.y / y_size)).rgba;
   	}
   }
   
   if (direction==1) //vertical of the separable filter
   {
   	for(filter_step = -kernel_size; filter_step <= kernel_size; filter_step++)
   	{
		 sample_pos = pix.y/y_size + float(filter_step)/y_size;
   		 avg += texture2D(input_map, vec2(pix.x / x_size,sample_pos)).rgba;
   	}
   }


   avg = (avg/float(kernel_size*2+1));

   return avg;

}

void main() 
{
   gl_FragColor = average_neighbors(gl_FragCoord.xy);
}
