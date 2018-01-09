#include <stdio.h>
#include "dbg.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#define MAX_DEP_REPR 10
#define SB_SIZE 128
#define MI_SIZE 4

typedef struct  refAndDepEncoding_t{
  int totalCount;
  int refBitRate;
  int depBitRate[MAX_DEP_REPR];
  char* refBitRateDirectory;
  char* depBitRateDirectory[MAX_DEP_REPR];
  } refAndDepEncoding;

typedef struct resolutionInformation_t{
  char name[6];
  int height;
  int width;
  int miRows;
  int miCols;
  int nrSBsInFrame;
  } resolutionInformation;


int find_maximum(int a[], int n) {
  int c, max, index; 
  max = a[0];
  index = 0; 
  for (c = 1; c < n; c++) {
    if (a[c] > max) {
       index = c;
       max = a[c];
    }
  }
   return index;
}

void set_ref_dep_encoding(const char* path, const int bitRates[], 
          const int subDirCount, const int maxIndex, refAndDepEncoding* p_RefAndDepEncoding)
{
   p_RefAndDepEncoding->totalCount = subDirCount;
  int tmpDepCount = 0;
  char tmpLocationStr[300]; 

  for (int i = 0; i < subDirCount; i++) {
    sprintf(tmpLocationStr, "%s/%d", path, bitRates[i]);
    if ( i == maxIndex){
        p_RefAndDepEncoding->refBitRate = bitRates[i];
        p_RefAndDepEncoding->refBitRateDirectory  = strdup(tmpLocationStr) ;         
    }
    else{
        p_RefAndDepEncoding->depBitRate[tmpDepCount] = bitRates[i];
        p_RefAndDepEncoding->depBitRateDirectory[tmpDepCount]  =  strdup(tmpLocationStr);
        tmpDepCount++;         
      }
  }
}

void print_ref_dep_encoding(const refAndDepEncoding* p_RefAndDepEncoding)
{
    debug("Reference: Bitrate - %d, Directory - %s", p_RefAndDepEncoding->refBitRate,
                  p_RefAndDepEncoding->refBitRateDirectory);         

    for (int i = 0; i < p_RefAndDepEncoding->totalCount - 1; i++){
      debug("Depedent: Bitrate - %d, Directory - %s", p_RefAndDepEncoding->depBitRate[i] ,
                    p_RefAndDepEncoding->depBitRateDirectory[i]);         
    }    
}

int extract_encoding_info(const char* path, refAndDepEncoding* p_RefAndDepEncoding)
{
	DIR *d;
  struct dirent *dir;
  d = opendir(path);
  check(d, "Failed to open directory %s.", path); // Print fail message and jump to error.
  
  int bitRates[MAX_DEP_REPR] = {0};
  int subDirCount  = 0;
  while ( (dir = readdir(d)) != NULL ) {
    if (dir->d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0 ){
      debug("Found Bitrate folder - %d", atoi(dir->d_name));
      bitRates[subDirCount] = atoi(dir->d_name);
      subDirCount++;
    }
  }
  check(subDirCount, "No subdirectories found in %s.", path); // Print fail message and jump to error.

  int maxIndex = find_maximum(bitRates, subDirCount);
  debug("Reference value - %d, Total count - %d", bitRates[maxIndex], subDirCount);
  set_ref_dep_encoding(path, bitRates, subDirCount, maxIndex, p_RefAndDepEncoding);
  print_ref_dep_encoding(p_RefAndDepEncoding);
  closedir(d);
  return 0;

  error:
    return -1;
}

void clean_encoding_info(refAndDepEncoding* p_RefAndDepEncoding)
{
    if (p_RefAndDepEncoding->refBitRateDirectory) free (p_RefAndDepEncoding->refBitRateDirectory);
    for (int i = 0; i < p_RefAndDepEncoding->totalCount - 1; i++){
      if (p_RefAndDepEncoding->depBitRateDirectory[i]) free(p_RefAndDepEncoding->depBitRateDirectory[i]);
    }
}



int main(int argc, char* argv[])
{
	log_info("Invoking main function with argument count");

	char* videoPath = "/home/adithyan/Innovation/MultiRate/PartitionReuse/BlueSky";
  char* resolutionPath = "360p";
  char* path;
  path = malloc(1 + strlen(videoPath) + strlen(resolutionPath));
  sprintf(path, "%s/%s", videoPath, resolutionPath);
  log_info("Entering path - %s", path);

  refAndDepEncoding a_RefAndDepEncoding;
  int status = extract_encoding_info(path,  &a_RefAndDepEncoding);

  check(status == 0, "Reading failed.");
  clean_encoding_info(&a_RefAndDepEncoding);

  return 0;

  error:
    if(path) free(path);
    return 1;
}

