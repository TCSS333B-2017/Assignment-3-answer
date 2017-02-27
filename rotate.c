#include <stdio.h>
#include <string.h>
#include "pixutils.h"

int main(int argc, char *argv[]){
  char *inputfile=0,*outputfile=0;
  unsigned char arrayType=0;
  float theta=0;
  int i=1;
  while (i<argc){
			if(! strcmp(argv[i],"-i")){
				inputfile=argv[i+1];
				i+=2;
			}	
	  else if(strcmp(argv[i],"-o")==0){
				outputfile=argv[i+1];
			 i+=2;
			}
			else if(strcmp(argv[i],"-t")==0){
				arrayType=atoi(argv[i+1]);
			 i+=2;
			}
			else if(strcmp(argv[i],"-r")==0){
				theta=atof(argv[i+1]);
			 i+=2;
			}
			else{
				fprintf(stderr,"usage: rotate -i inputfile -o outputfile -r <theta> -t <0-2>");
				exit(0);
			}	
		}		
  pixMap *p=pixMap_read(inputfile,arrayType);
  if(!p){
			fprintf(stderr,"unable to allocate memory for pixMap\n");
			return 1;
		}	
  pixMap_rotate(p,theta);
  pixMap_write(p,outputfile);  
  pixMap_destroy(&p);
  return 0;
}

