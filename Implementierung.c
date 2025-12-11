#define _POSIX_C_SOURCE 200809L // ist nützlich für ClOCK funktionen: https://man7.org/linux/man-pages/man2/clock_gettime.2.html
#include<stdint.h>
#include<immintrin.h>
#include<emmintrin.h>
#include<stdio.h>
#include<stdlib.h>
#include<getopt.h>
#include<unistd.h>
#include<time.h>
#include<stdbool.h>
#include<string.h>

 
size_t bmp_rle_new(const uint8_t* img_in, size_t width, size_t height, uint8_t* rle_data) {
  
 size_t index_rle = 0;
uint8_t cnt=0; // cnt for encoded
uint8_t absolute_cnt=0; // cnt for absolute
     // for each line
    for (size_t i = 0; i < height*width; i= i +width) {
        // convert the bmp line to rle counterpart
        size_t j =0;
         while (j<width )  {
            
            
            cnt = 1; // find out the count and according to it choose encoded or absolute
              size_t k =j+1;
                

              //this while will be improved with simd
              while (k<width&& img_in[k+i] == img_in[k+i-1] && cnt <255) {
                  cnt++;
                  k++;
              } 
            
                 if (cnt >2) {
                // absolutely encoded mode 
                rle_data[index_rle++]= cnt;
                rle_data [index_rle++]= img_in[i+j];
                j =k; // snap out of it to work on what's next
                 } 
                 
                 else {

   bool encoded =false;
                   absolute_cnt = 0;
                  // pixels in absolute >2 ? absolute : encoded
              //this while will be improved with simd   
              // calculate absolute
              
          while (j + absolute_cnt < width - 2 && !(img_in[i+j + absolute_cnt]== img_in[i +j+ absolute_cnt+ 1 ]&&img_in[i +j+ absolute_cnt+ 1] ==img_in[i +j+ absolute_cnt+ 2]) &&  
           absolute_cnt < 255) {  absolute_cnt++;}
                  // dealing with last two pixels of a line Part 1
              if ( j+absolute_cnt> width -3 && absolute_cnt !=0) {
                  if (absolute_cnt<254)  {absolute_cnt +=2;} else 
                  {encoded = true;}
                     
              }
              // if nothing is better in Absolute => Encoded
              if (absolute_cnt ==0) {
                rle_data [index_rle++]= cnt;
                rle_data [index_rle++]= img_in[i+j]; 
                j=j+cnt;
                continue;
              }
                    // if <=2 better in encoded else absolute
                     if (absolute_cnt == 2) {
                rle_data [index_rle++]= 1;

                rle_data [index_rle++]= img_in[i+j++];
                rle_data [index_rle++]= 1;
                rle_data [index_rle++]= img_in[i+ j++];
                     } else if (absolute_cnt ==1) {
                          rle_data [index_rle++]= 1;
                           rle_data [index_rle++]= img_in[i+j++];
                     } else {
                         rle_data[index_rle++]=0;
                         rle_data[index_rle++]= absolute_cnt;
                         // this will also be improved with simd
                         size_t end =absolute_cnt+j;
                         //Schreiben von Pixeln in Absolute Modus
                        while (j <end) {  rle_data[index_rle++]= img_in[i+(j++)]; }
                        if ((absolute_cnt &1) ==1 ) {  rle_data[index_rle++]=0;}


                      // dealing with last two pixels of a line Part 1
                        if (encoded) {
                             rle_data[index_rle++]=1;
                               rle_data [index_rle++]= img_in[i+j++];
                                rle_data[index_rle++]=1;
                rle_data [index_rle++]= img_in[i+j++];
                        }
                     }

                    

                 }
            






         }
    // End of line
        rle_data[index_rle++] = 0;
        rle_data[index_rle++] = 0;
    }


    // Overwrite end of line with end of bitmap  
    rle_data[index_rle -1] = 1;

    return index_rle;
}


size_t bmp_rle_newSIMD(const uint8_t* img_in, size_t width, size_t height, uint8_t* rle_data) {
      

 size_t index_rle = 0;
uint8_t cnt=0; // cnt for encoded
uint8_t absolute_cnt=0; // cnt for absolute
     // for each line

    for (size_t i = 0; i < height*width; i= i +width) {
        // convert the bmp line to rle counterpart
        size_t j =0;
         while (j<width )  {
            
            
            cnt = 1; // find out the count and according to it choose encoded or absolute
              size_t k =j+1;  


                 __m128i pixelVector = _mm_set1_epi8(img_in[i+j]);
               while (k<width-16) {
                   __m128i imgVector = _mm_loadu_si128((__m128i*) (img_in+ (k+ i) ));
                __m128i equal_vector = _mm_cmpeq_epi8(imgVector, pixelVector);
                                   int mask = _mm_movemask_epi8(equal_vector);
                       int toAdd;
                      // IF a == 0
	// dst is undefined ( Vorsicht=> this line is important) in  _bit_scan_forward
                       if (mask ==0xffff) {toAdd =16;} else {
                       toAdd =  _bit_scan_forward(mask ^ (-1)  ) ;}

                        int overFlow = toAdd +cnt ;
                        if (overFlow >255 ) {
                            k += 255-cnt;
                             cnt =255;
                          
                            break;

                        } else {

                            cnt = overFlow;
                            k=k+toAdd;
                        }
                      
                      if (toAdd !=16) {
                        break;
                      }
                     
                   
              }
                     


            // there is still something to look for (SIMD couldnt find everything)
            //usually because we do not have more than 128 from which we can work on __m128
              while (k<width&& img_in[k+i] == img_in[k+i-1] && cnt <255) {
                  cnt++;
                  k++;
              

              }
                 if (cnt >2) {
                // absolutely encoded mode 
                rle_data[index_rle++]= cnt;
                rle_data [index_rle++]= img_in[i+j];
                j =k; // snap out of it to work on what's next
                 } 
                 
                 else {

   bool encoded =false;
                   absolute_cnt = 0;
                  // pixels in absolute >2 ? absolute : encoded
              //this while will be improved with simd   
              // calculate absolute
              //with simd look for the first 3 equla bytes nacheinander = > it's encoded o'clock
                    size_t toAdd=16;
                    size_t overFlow=0;
               while (j+2 + overFlow <width-16 &&toAdd ==16&& overFlow<255) {
                // using three vectors to compare three following pixels at to time (indicator of Encoded mode)
                __m128i plusOne = _mm_loadu_si128((__m128i*) (img_in+ (j+ i +1 +overFlow) ));
 
             
                 int mask = _mm_movemask_epi8(
                 _mm_and_si128 (
                    _mm_cmpeq_epi8( _mm_loadu_si128((__m128i*) (img_in+ (j+ i+overFlow) )), 
                    plusOne),
                    _mm_cmpeq_epi8(_mm_loadu_si128((__m128i*) (img_in+ (j+ i+2+overFlow) )),
                     plusOne)
                 ));
                       toAdd = mask ==0x0000?16: _bit_scan_forward(mask) ;
                    

                         overFlow += toAdd  ;
                           
                      
              }    
              // Using an Overflow temp variable is helpful to find out when to stop looking for absolute_count
              absolute_cnt = overFlow>=255?255:overFlow;
          // continue with no SIMD in case SIMD is uncapable of finding em all
          while (j + absolute_cnt < width - 2 && !(img_in[i+j + absolute_cnt]== img_in[i +j+ absolute_cnt+ 1 ]&&img_in[i +j+ absolute_cnt+ 1] ==img_in[i +j+ absolute_cnt+ 2]) &&  
           absolute_cnt < 255) {  absolute_cnt++;}

              if ( j+absolute_cnt> width -3 && absolute_cnt !=0) {
                  if (absolute_cnt<254)  {absolute_cnt +=2;} else 
                  {encoded = true;}
                     
              }

              if (absolute_cnt ==0) {
                rle_data [index_rle++]= cnt;
                rle_data [index_rle++]= img_in[i+j]; 
                j=j+cnt;
                continue;
              }

                     if (absolute_cnt == 2) {
                rle_data [index_rle++]= 1;

                rle_data [index_rle++]= img_in[i+j++];
                rle_data [index_rle++]= 1;
                rle_data [index_rle++]= img_in[i+j++];
                     } else if (absolute_cnt ==1) {
                          rle_data [index_rle++]= 1;
                           rle_data [index_rle++]= img_in[i+j++];
                     } else {


                         rle_data[index_rle++]=0;
                         rle_data[index_rle++]= absolute_cnt;
                         
                       
                       
                         size_t end =absolute_cnt+j;
                         // writing with SIMD and if not enough using no SIMD afterwards
                        while (j+16 <end) {
                       
                        _mm_storeu_si128((__m128i *)(rle_data+index_rle), _mm_loadu_si128((__m128i*)(img_in+i+j)));
                          j=j+16;
                          index_rle +=16;
                         } 
                        while (j <end) {  rle_data[index_rle++]= img_in[i+(j++)]; }
                        if ( ( (absolute_cnt) & (1) )==1 ) {  rle_data[index_rle++]=0; /** padding**/}

                        if (encoded) {
                             rle_data[index_rle++]=1;
                               rle_data [index_rle++]= img_in[i+j++];
                                rle_data[index_rle++]=1;
                rle_data [index_rle++]= img_in[i+j++];
                        }



                     }

                    

                 }
            






         }
    // End of line
        rle_data[index_rle++] = 0;
        rle_data[index_rle++] = 0;
    }


    // Overwrite end of line with end of bitmap  
    rle_data[index_rle -1] = 1;

    return index_rle;
}


//test of the output comparing with the input
bool tests(uint8_t* uncmp , uint8_t* cmp, unsigned int width, unsigned int height) {

    uint8_t nbr = 0;
    bool endoffile = false;
    unsigned int w = 0;
    unsigned int h = 0;

    uint8_t* testarray = (uint8_t*) malloc(width * height * sizeof(uint8_t));
    uint8_t* ForSavingThePointerOfTestarray = testarray;
    if(testarray == NULL) {
        printf("Unable to allocate memory for the test array\n");
        return false;
    }

            int counter = 0;
            int max = width * height;

    while (endoffile == false || counter > max) {
        
        if (*cmp == 0) { //not encoded mode
       
           cmp++;  //go to the second byte to check if it is end of file or end of line or absolute mode
         
            if (*cmp == (uint8_t) 1) { //end of file detected
                h++;
                endoffile = true;
            } else if (*cmp == (uint8_t) 0) { //end of line
                h++;  //it is the end of line, then the height should be incrimented
                cmp++;

            if (w != width) {
                    printf("Test failed. The width is false!\n"); 
                    return false;
                } 
                w = 0;
            } else { //it is absolute mode
            
                nbr = *cmp;
                cmp++;
                for (uint8_t i = 0; i < nbr; i++) { //decode and copy to the test array
                    *testarray = *cmp;
                    counter++;  //counting the decoded pixels
                    testarray++;
                     cmp++;
                    w++;
                }
                if (nbr % 2 == 1){
                     cmp++;
                }
            }
        } else { //it is encoded mode
       
            nbr = *cmp;
           
             cmp++;
             
            for (uint8_t i = 0; i < nbr; i++) {
                *testarray = *cmp;
                counter++;  //counting the decoded pixels
                testarray++;
                w++;
            }
             cmp++; //don't forget to go the next byte, because you did not do it in the for loop
           
        }

    }

    if (counter > max) {
        printf("Test failed. End of file can't be reached!\n");
        return false;
    }

   if (h != height) {
        printf("Test failed. The height is false!\n");
        return false;
    }


    unsigned int sizeofarray = height * width;
    testarray = ForSavingThePointerOfTestarray;

    for (unsigned int i = 0; i < sizeofarray; i++) {
        if (testarray[i] != uncmp[i]) {
            printf("Test Failed. The two images differ at the pixel number %u\n", i);
            return false;
        }
    }
    
    printf("Test succeeded. The two images are the same!\n");
    free(testarray);

    return true;
}








//Rahmenprogramm

//Help page
void print_usage() {
    printf("    You're now in our help page of .....\n \n");
    printf("Usage : \n [-B Benchmark ] [-V Version ] [-o output ] [-h ] [--help ] \n \n ") ;
    printf("Options and their functionalities :\n");
    printf("-o :\t print output of our function \n");
    printf("-V : v0 or v1 \n");
    printf("-B : \t Benchmark ---> Time measurement \n ");
    printf("-h : \t print this help page \n");
    printf("--help : \t print this help page \n");
    printf("-t : \t test the accurency of the chosen method\n");
    printf("   :\t path of image (input must be BMP form ) \n ");



}

/*
According to LINUX man page (https://linux.die.net/man/3/getopt_long)
The getopt_long() function works like getopt() except that it also accepts long options,
 started with two dashes. (If the program accepts only long options,
  then optstring should be specified as an empty string (""), not NULL.)
 Long option names may be abbreviated if the abbreviation is unique or is an exact match for some defined option.
  A long option may take a parameter, of the form --arg=param or --arg param.
*/

typedef struct {
    uint8_t* pixels_img;
    uint8_t* header_img;
    unsigned int dataOffset;
    unsigned int width;
    unsigned int height;
    unsigned int size;

} ImageData;

//Input

uint8_t* readInput(char * filename, ImageData* imageData) {
    

    FILE* file = fopen((const char *) filename, (const char *) "rb");
    if(file == NULL) {
        fprintf(stderr,"Unable to open file %s \n",filename);
        return NULL;
    }

    uint8_t bmpHeader[2];
    if(fread((void*) bmpHeader,1,2,file) != 2) {
        fprintf(stderr,"Failed to read the header of the image %s \n",filename);
        fclose(file);
        return NULL;
    }

    if(bmpHeader[0] != 'B' || bmpHeader[1] != 'M') {
        fprintf(stderr,"No BMP file %s \n",filename);
        fclose(file);
        return NULL;
    }
    //width 0x0012
    //height 0x0016
    //DataOffset 0x000A : Here you find the total size of the header before coming to the pixels

   if  ( fseek(file,0L,SEEK_END) ) {
     printf("Seeking problem\n");
        fclose(file);
        return NULL;
    }
    (*imageData).size = ftell(file);

   if  (fseek(file,18L,SEEK_SET)) {
    printf("Seeking problem\n");
        fclose(file);
        return NULL;
   }
if  (  fread(&(*imageData).width,4,1,file)!=1) {
 printf("Reading problem\n");
        fclose(file);
        return NULL;

    }

   if  (fseek(file,22L,SEEK_SET)) {
         printf("Seeking problem\n");
        fclose(file);
        return NULL;
   }

    
  if ( fread(&(*imageData).height,4,1,file) !=1 ){
 printf("Reading problem\n");
        fclose(file);
        return NULL;

    }
    
  
  if ( fseek(file,10L,SEEK_SET) ) {
     printf("Seeking problem\n");
        fclose(file);
        return NULL;
   }
  
  if ( fread(&(*imageData).dataOffset,4,1,file)!=1) {
    printf("Seeking problem\n");
        fclose(file);
        return NULL;
  }

    unsigned int pixelsinbytes = (*imageData).size - (*imageData).dataOffset;
    (*imageData).pixels_img =(uint8_t *) malloc(pixelsinbytes);
    if((*imageData).pixels_img == NULL){
        printf("Unable to allocate memory for pixels \n");
        fclose(file);
        return NULL;
    }

    //reading the pixel data from the file
    rewind(file);
   if (fseek(file, (*imageData).dataOffset, SEEK_SET) ) {
     printf("Seeking problem\n");
        fclose(file);
        return NULL;
   }
    unsigned long pixelsread = fread((*imageData).pixels_img,1,pixelsinbytes,file);
    if( pixelsread != pixelsinbytes) {
        printf("Can not read pixel data \n");
        fclose(file);
        return NULL;
    }

    fclose(file);
    
    return (*imageData).pixels_img ;
}
//Output

bool writeOutput(const char* filename, char* inputfilename , ImageData* imageData , uint8_t* rle_data, size_t rle_data_size){

        FILE* oldfile =NULL;
    FILE* newfile =NULL;
    uint8_t* headerData = NULL;
    bool status = false ;

    headerData=(uint8_t*) malloc(imageData->dataOffset);
    if(headerData == NULL) {
        printf("Unable to allocate memory for the headerdata file");
        return status;
    }

    oldfile=fopen(inputfilename,"rb");
    if(oldfile == NULL) {
        printf("Unable to open the old file");
        free(headerData);
        return status;
    }

    newfile= fopen(filename,"wb");
    if(newfile == NULL) {
        printf("Unable to open thenew file");
        fclose(oldfile);
        free(headerData);
        return status ;
    }

    size_t bytesread = fread(headerData,1,imageData->dataOffset,oldfile);
    if(bytesread != imageData->dataOffset) {
        printf("\n Failed to read header from old file");
        fclose(oldfile);
        fclose(newfile);
        free(headerData);
        return status;
    }

    unsigned int filesize = rle_data_size + imageData->dataOffset;
    memcpy(headerData + 2, &filesize, 4);

    unsigned int compressiontype = 1;
    memcpy(headerData + 30,&compressiontype, 4);

    unsigned imagesize = rle_data_size;
    memcpy(headerData + 34, &imagesize, 4);

    size_t byteswritten = fwrite(headerData,1,bytesread,newfile);
    if(byteswritten != bytesread){
        printf("\n A proble occured when copying header");
        fclose(oldfile);
        fclose(newfile);
        free(headerData);
        return status;
    }

    fseek(newfile, (long) imageData->dataOffset , SEEK_SET);
    size_t rle_index = 0;
    while(rle_index < rle_data_size){
        uint8_t value = rle_data[rle_index++];
        int a = fwrite(&value,1,1,newfile);
           if (a != 1) {
                printf("an error accured");
                exit(EXIT_FAILURE);
            }
    }
    
    status=true;
    fclose(oldfile);
    fclose(newfile);
    free(headerData);

    return status;

}


int main (int argc , char *argv[]) {
    bool t = false;
    int option ;
    int v_value = 3 ;
    int b_value = 0 ;
    char* output_filename = NULL ;
    

    char* end_ptr;
  
  int option_index=0; 
    if(argc <= 1){
        printf("Failure : not enough arguments counts -h\n");
        exit(EXIT_FAILURE);
    }
struct option long_options[] = {
        {"help",0,0,'h'},
        {0,0,0,0}
         };
         option = getopt_long(argc,argv , "V:B::o:ht",long_options,&option_index);
    while(option != -1){ 
        
        switch (option) {  
            case 'V' :
                v_value = strtoul(optarg,&end_ptr,10);
                if(*end_ptr != '\0') {
                    fprintf(stderr,"Invalid value for -V : %s \n",optarg);
                    return -1;
                }
                break ;
            case 'B'  :
            if (optarg == NULL) {
                b_value = 3;
            } else {
                b_value = strtoul(optarg,&end_ptr,10);
                
                if (*end_ptr != '\0'){
                   fprintf(stderr,"Invlid value for -B: %s \n",optarg);
                   return -1;
               }
                
            }
                break ;
            case 'o' :
                output_filename = optarg ;
                break ;
            case 'h' :
                print_usage();
                return 0;
            
            case 't' :
                t = true;
            break;

            default:
                fprintf(stderr,"Invalid option: %c. Use [-B Benchmark ] [-V Version ] [-o output ] [-h ] [--help ] [-t] \n ", option);
                return -1 ;

        }
        option = getopt_long(argc,argv , "V:B::o:ht",long_options,&option_index);
    }
   
    

    if (optind >= argc ){
        fprintf(stderr,"Provide an input file here : \n");
        print_usage();
        return -1;
    }
        size_t(*rle_func_t)(const uint8_t*,size_t,size_t,uint8_t*);
        switch(v_value) {
            case 0:
                rle_func_t = bmp_rle_newSIMD;
                printf("rle1 with SIMD\n");
                break;
            case 1:
                rle_func_t = bmp_rle_new;
                printf("rle \n");
                break;
            default:
                printf("There are only two versions 'Version 0' without SIMD and 'Version 1' with SIMD\n");
                printf("This is the help page for more Informations:\n");
                print_usage();
                exit(EXIT_FAILURE);
                break;

        }

    if(argv[optind] != NULL) {
        ImageData imageData;
        uint8_t* img_in = readInput(argv[optind],&imageData);
       
    if(img_in == NULL){
        fprintf(stderr,"Error when reading the input .\n");
        exit(EXIT_FAILURE);
    }
   
        unsigned int width_img = imageData.width;
        unsigned int height_img = imageData.height;
                                                        
        unsigned long maxsize;

        if (width_img <= 255) {
            if (width_img % 2 == 0) {
                maxsize = (width_img + 2) * height_img;
            } else {
                maxsize = (width_img + 3) * height_img;
            }
        } else {
            maxsize = 258L * (long) width_img * (long) height_img / 255L;
            //== (((width_img * 3) / 255) + width_img) * height_img : divison at the end to avoid unprcision
        }

        uint8_t * memory_allocation = (uint8_t*) malloc(maxsize);

        if (memory_allocation == NULL) {
            printf("Allocation problem for the result array of pixels \n");
            free(img_in);
            return  1;
        }

        /*typedef*/ 
        
    struct timespec start ,end;
    size_t newsize;
   if (b_value <= 0) {
     newsize = rle_func_t(img_in,width_img,height_img,memory_allocation);
   } else {
    
    double time_spent = 0 ;
      
    for (int i = 0 ; i < b_value ; i++) {
    clock_gettime(CLOCK_MONOTONIC,&start);
   newsize = rle_func_t(img_in,width_img,height_img,memory_allocation);
    clock_gettime(CLOCK_MONOTONIC,&end);

    time_spent += ((end.tv_sec -start.tv_sec) + 1e-9 *(end.tv_nsec -start.tv_nsec))*1000;
}
    time_spent = time_spent / b_value;
    
    printf("The function was executed in  %f ms \n",time_spent);
    
   }

    if(output_filename != NULL) {

        if(!writeOutput(output_filename,argv[optind],&imageData,memory_allocation, newsize)){
            printf("Couldn't provide output file \n");
            free(img_in);
            free(memory_allocation);
           exit(EXIT_FAILURE);
        }

    }

        if (t == true) {
        tests(img_in,memory_allocation,width_img, height_img);
        }
        free(img_in);
        free(memory_allocation);
        return 0;
       
   

    }else {
        printf("You didn't provide any input file \n");
        //return 1;
        exit(EXIT_FAILURE);
    }
}

