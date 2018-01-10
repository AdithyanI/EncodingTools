#include <stdio.h>
#include "dbg.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_DEP_REPR 10
#define SB_SIZE 128
#define MI_SIZE 4
#define FRAME_SIZE 15

typedef unsigned char UChar;

typedef struct resolutionInformation_t{
  char name[6];
  int height;
  int width;
  int miRows;
  int miCols;
  int nrSBsInFrame;
  int nrDepthInfoInFrame;
} resolutionInformation;

typedef struct depthComparisonInfo_t{
  float greaterDepth;
  float equalDepth;
  float lesserDepth;
} depthComparisonInfo;

typedef struct  refAndDepEncoding_t{
  int totalCount;
  int refBitRate;
  int depBitRate[MAX_DEP_REPR];
  char* refBitRateDirectory;
  char* depBitRateDirectory[MAX_DEP_REPR];
  resolutionInformation* resInformation;
  depthComparisonInfo depthComparisonInfoFrame[FRAME_SIZE];
} refAndDepEncoding;


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
    sprintf(tmpLocationStr, "%s/%d/%s", path, bitRates[i],"analysisData.bin");
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


void set_resolution_information_helper(resolutionInformation* p_resolutionInformation)
{
    p_resolutionInformation->miRows = (p_resolutionInformation->height)/MI_SIZE;
    p_resolutionInformation->miCols = (p_resolutionInformation->width)/MI_SIZE;
    p_resolutionInformation->nrSBsInFrame =
    ceil(p_resolutionInformation->height/(double)SB_SIZE) * ceil(p_resolutionInformation->width/(double)SB_SIZE);
    p_resolutionInformation->nrDepthInfoInFrame = p_resolutionInformation->nrSBsInFrame * (SB_SIZE/MI_SIZE) * (SB_SIZE/MI_SIZE);
}

void set_resolution_information(resolutionInformation* p_resolutionInformation, int height)
{
  switch(height){
    case 360:
      sprintf(p_resolutionInformation->name, "%dp", height);
      p_resolutionInformation->height = 360;
      p_resolutionInformation->width = 640;
      set_resolution_information_helper(p_resolutionInformation);
      break;

    case 720:
      sprintf(p_resolutionInformation->name, "%dp", height);
      p_resolutionInformation->height = 720;
      p_resolutionInformation->width = 1280;
      set_resolution_information_helper(p_resolutionInformation);
      break;

    case 1080:
      sprintf(p_resolutionInformation->name, "%dp", height);
      p_resolutionInformation->height = 1080;
      p_resolutionInformation->width = 1920;
      set_resolution_information_helper(p_resolutionInformation);
      break;

    default:
      log_warn("Unknown resolution given.");
      break;
    }
  }

depthComparisonInfo compare_depth_info(const UChar* refDepthInfo, const UChar* depDepthInfo,
                                                                                const int bytesToRead)
{
  const unsigned char depth;
  depthComparisonInfo returnValue;
  float greaterDepth = 0;
  float equalDepth = 0;
  float lesserDepth = 0;

  for (int i=0; i<bytesToRead; i++){
    if(refDepthInfo[i] < depDepthInfo[i]){
      greaterDepth++;
    }
    else if (refDepthInfo[i] == depDepthInfo[i]){
      equalDepth++;
    }
  }

  returnValue.greaterDepth = greaterDepth/bytesToRead;
  returnValue.equalDepth = equalDepth/bytesToRead;
  returnValue.lesserDepth = 1 - (greaterDepth+equalDepth);
  return returnValue;
}

void print_depth_info_frame(const depthComparisonInfo* depthComparisonInfoFrame)
{
  float greaterDepth;
  float lessThanGreaterDepth;
  for( int i=0; i<FRAME_SIZE; i++){
    greaterDepth = depthComparisonInfoFrame[i].greaterDepth;
    log_info("Equal or lesser = %f, Greater = %f", 1-greaterDepth, greaterDepth);
  }
}

int analyse_depth_information(refAndDepEncoding* p_RefAndDepEncoding)
{
  int bytesToRead;
  bytesToRead = p_RefAndDepEncoding->resInformation->nrDepthInfoInFrame;

  FILE* mDataFileRef;
  FILE* mDataFileDep;

  mDataFileRef = fopen(p_RefAndDepEncoding->refBitRateDirectory, "rb"); // READ mode (read, binary)
  mDataFileDep = fopen(p_RefAndDepEncoding->depBitRateDirectory[0], "rb"); // READ mode (read, binary)

  check(mDataFileRef != NULL, "Error opening file - %s",  p_RefAndDepEncoding->refBitRateDirectory);
  check(mDataFileDep != NULL, "Error opening file - %s",  p_RefAndDepEncoding->depBitRateDirectory[0]);

  UChar* refDepthInfo = (UChar* )malloc(bytesToRead);
  UChar* depDepthInfo = (UChar* )malloc(bytesToRead);

  check_mem(refDepthInfo);
  check_mem(depDepthInfo);

  for (int i=0; i<FRAME_SIZE; i++){
    int sizeReadRef = fread(refDepthInfo, sizeof(UChar), bytesToRead, mDataFileRef);
    int sizeReadDep = fread(depDepthInfo, sizeof(UChar), bytesToRead, mDataFileDep);
    p_RefAndDepEncoding->depthComparisonInfoFrame[i] = compare_depth_info(refDepthInfo, depDepthInfo, bytesToRead);
  }

  print_depth_info_frame(p_RefAndDepEncoding->depthComparisonInfoFrame);
  free(refDepthInfo);
  free(depDepthInfo);
  fclose(mDataFileRef);
  fclose(mDataFileDep);
  return 0;

  error:
    if(refDepthInfo) free(refDepthInfo);
    if(depDepthInfo) free(depDepthInfo);
    if(mDataFileRef) fclose(mDataFileRef);
    if(mDataFileDep) fclose(mDataFileDep);
    return -1;
}

int main(int argc, char* argv[])
{
  int status;
  resolutionInformation resInfo360p;
  resolutionInformation resInfo720p;
  resolutionInformation resInfo1080p;

  set_resolution_information(&resInfo360p, 360);
  set_resolution_information(&resInfo720p, 720);
  set_resolution_information(&resInfo1080p, 1080);

	char* videoPath = "/home/adithyan/Innovation/MultiRate/PartitionReuse/BlueSky/";
  char* resolutionPath = "360p";

  char* path;
  path = malloc(1 + strlen(videoPath) + strlen(resolutionPath));
  strcpy(path, videoPath);
  strcat(path, resolutionPath);
  log_info("Analysing folder location - %s", path);

  refAndDepEncoding a_RefAndDepEncoding;
  status = extract_encoding_info(path,  &a_RefAndDepEncoding);
  check(status == 0, "Extracting encoding failed for %s.", path);
  a_RefAndDepEncoding.resInformation = &resInfo360p;

  status = analyse_depth_information(&a_RefAndDepEncoding);
  check(status == 0, "Analysing depth information failed for %s.", path)

  clean_encoding_info(&a_RefAndDepEncoding);

  if(path) free(path);
  return 0;

  error:
    if(path) free(path);
    return -1;
}

