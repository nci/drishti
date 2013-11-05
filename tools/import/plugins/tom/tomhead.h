struct thead
  {
    short xsize,ysize,zsize,lmarg,rmarg,tmarg,bmarg,tzmarg,bzmarg,\
     num_samples,num_proj,num_blocks,num_slices,bin,gain,speed,pepper,spare_int[15];
    float scale,offset,voltage,current,thickness,pixel_size,distance,exposure,\
     mag_factor,filterb,correction_factor,spare_float[2];
	long z_shift,z,theta;
    char time[26],duration[12],owner[21],user[5],specimen[32],scan[32],\
     comment[64],spare_char[192];
  };

