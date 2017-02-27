#include "pixutils.h"

//private methods -> make static
static pixMap* pixMap_init(unsigned char arrayType);
static pixMap* pixMap_copy(pixMap *p);


static pixMap* pixMap_init(unsigned char arrayType){
	pixMap *p=malloc(sizeof(pixMap));
	if(!p) return 0;
 p->pixArray_arrays=0; //could use NULL
	p->pixArray_blocks=0;
	p->pixArray_overlay=0;
	p->imageHeight=0;
	p->imageWidth=0;
	p->arrayType=arrayType;
	return p;
}	

void pixMap_destroy (pixMap **p){
	if(!p || !*p) return; //check for edge case of p or *p =0 to avoid segfault
	pixMap *this_p=*p;
	if(this_p->pixArray_arrays) free(this_p->pixArray_arrays); //free one malloced block
	if(this_p->pixArray_blocks){
	 for(int i=0;i<this_p->imageHeight;i++){ //loop through row pointers
			if(this_p->pixArray_blocks[i])free(this_p->pixArray_blocks[i]); //free each row 
		}
		free(this_p->pixArray_blocks);	//free the array/block with the row pointer
	}
	if(this_p->pixArray_overlay)    
	 free(this_p->pixArray_overlay); //free the array/block with row pointers
	if(this_p->image)free(this_p->image);	//free the image in all cases
	free(this_p); //free the pixMap
	this_p=0; //set the pixMap pointer (*p) to zero <-- this is why we had to pass pixMap **p instead of pixMap *p
}
	
pixMap *pixMap_read(char *filename,unsigned char arrayType){
	pixMap *p=pixMap_init(arrayType);
 int error;
 if((error=lodepng_decode32_file(&(p->image), &(p->imageWidth), &(p->imageHeight),filename))){
  fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
  return 0;
	}
 //can also do pixel by pixel copy using memcopy or iterating through i,j and .r .g .b .a	
 //for type 0 can also just copy the entire block	
	if (arrayType ==0){
		if(p->imageWidth > MAXWIDTH)return 0;
		p->pixArray_arrays=malloc(p->imageHeight*MAXWIDTH*sizeof(rgba)); //p->imageHeight*sizeof(rgba[MAXWIDTH]) works too
		for(int i=0;i<p->imageHeight;i++){
	  memcpy(p->pixArray_arrays[i],p->image+i*p->imageWidth*sizeof(rgba),p->imageWidth*sizeof(rgba));	//copy image row to array row
		}

	}	
 else if (arrayType ==1){
		p->pixArray_blocks=malloc(p->imageHeight*sizeof(rgba*)); //malloc array/block to hold pointers to rows
		for(int i=0;i<p->imageHeight;i++){
	  p->pixArray_blocks[i]=malloc(p->imageWidth*sizeof(rgba));//malloc rows of image
	  memcpy(p->pixArray_blocks[i],p->image+i*p->imageWidth*sizeof(rgba),p->imageWidth*sizeof(rgba));	//copy image row to array row
		}
	}
	else if (arrayType ==2){ //no copying but overlay of pointers to the original image
		p->pixArray_overlay=malloc(p->imageHeight*sizeof(rgba*)); //malloc array/block to hold pointers
	 p->pixArray_overlay[0]=(rgba*) p->image; //put the address of the block into first pointer
		for(int i=1;i<p->imageHeight;i++){ //loop through the other pointers
   p->pixArray_overlay[i]=p->pixArray_overlay[i-1]+p->imageWidth; //pointer math row pointer is previous pointer + width (when talking rgba type)
		}
		//can also explicitly calculate addresses and cast them to rgba* if necessary
		//the loop through pointers however works even when the row sizes are irregular e.g. like calculating pointers to months of the year on an array of days
	}
	else{
		return 0;
	}				
	return p;
}
int pixMap_write(pixMap *p,char *filename){
	int error=0;
	//for arrayType 1 and arrayType 2 have to write out array rows to image first
	//reverse of read
	 if (p->arrayType ==0){
		 for(int i=0;i<p->imageHeight;i++){
	   memcpy(p->image+i*p->imageWidth*sizeof(rgba),p->pixArray_arrays[i],p->imageWidth*sizeof(rgba));	
		 }		
	 }	
		else if (p->arrayType ==1){
	 	for(int i=0;i<p->imageHeight;i++){
	   memcpy(p->image+i*p->imageWidth*sizeof(rgba),p->pixArray_blocks[i],p->imageWidth*sizeof(rgba));	
		 }		
	 }
 if(lodepng_encode32_file(filename, p->image, p->imageWidth, p->imageHeight)){
  fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
  return 1;
	}
	return 0;
}	 

int pixMap_rotate(pixMap *p,float theta){
	pixMap *oldPixMap=pixMap_copy(p);
	if(!oldPixMap)return 1;
 const float ox=p->imageWidth/2.0f;
 const float oy=p->imageHeight/2.0f;
 const float s=sin(degreesToRadians(-theta));
	const float c=cos(degreesToRadians(-theta));
	for(int y=0;y<p->imageHeight;y++){
		for(int x=0;x<p->imageWidth;x++){ 
   float rotx = c*(x-ox) - s * (oy-y) + ox;
   float roty = -(s*(x-ox) + c * (oy-y) - oy);
		 int rotj=rotx+.5;
		 int roti=roty+.5; 
		 if(roti >=0 && roti < oldPixMap->imageHeight && rotj >=0 && rotj < oldPixMap->imageWidth){
			 if(p->arrayType==0) memcpy(p->pixArray_arrays[y]+x,oldPixMap->pixArray_arrays[roti]+rotj,sizeof(rgba));
			 else if(p->arrayType==1) memcpy(p->pixArray_blocks[y]+x,oldPixMap->pixArray_blocks[roti]+rotj,sizeof(rgba));
			 else if(p->arrayType==2) memcpy(p->pixArray_overlay[y]+x,oldPixMap->pixArray_overlay[roti]+rotj,sizeof(rgba));			 
			}
			else{
				if(p->arrayType==0) memset(p->pixArray_arrays[y]+x,0,sizeof(rgba));
			 else if(p->arrayType==1) memset(p->pixArray_blocks[y]+x,0,sizeof(rgba));
			 else if(p->arrayType==2) memset(p->pixArray_overlay[y]+x,0,sizeof(rgba));		
			}		
		}	
	}
 pixMap_destroy(&oldPixMap);
 return 0;
}

pixMap *pixMap_copy(pixMap *p){
	pixMap *new=pixMap_init(p->arrayType); //allocate memory for new arrayType
	*new=*p; //shallow copy of old struct to new (could just copy imageWidth and imageHeight)
	new->image=malloc(new->imageHeight*new->imageWidth*sizeof(rgba)); //malloc space for new image - can't use lodepng to read it in
	memcpy(new->image,p->image,p->imageHeight*p->imageWidth*sizeof(rgba));	//memcopy the old image to the new images
	//same steps as before with read function
	if (new->arrayType ==0){
		new->pixArray_arrays=malloc(new->imageHeight*MAXWIDTH*sizeof(rgba));
		for(int i=0;i<p->imageHeight;i++){
	  memcpy(new->pixArray_arrays[i],p->image+i*p->imageWidth*sizeof(rgba),p->imageWidth*sizeof(rgba));	
		}		
	}	
 else if (new->arrayType ==1){
		new->pixArray_blocks=malloc(p->imageHeight*sizeof(rgba*));
		for(int i=0;i<p->imageHeight;i++){
	  new->pixArray_blocks[i]=malloc(p->imageWidth*sizeof(rgba));
	  memcpy(new->pixArray_blocks[i],p->image+i*p->imageWidth*sizeof(rgba),p->imageWidth*sizeof(rgba));	
		}
	}
	else if (new->arrayType ==2){
		new->pixArray_overlay=malloc(new->imageHeight*sizeof(rgba*));
	 new->pixArray_overlay[0]=(rgba*) new->image;
		for(int i=1;i<new->imageHeight;i++){
   new->pixArray_overlay[i]=new->pixArray_overlay[i-1]+new->imageWidth;
		}
	}
	return new;
}	
