#include <stdio.h>
#include "dbg.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_DEP_REPR 4
#define SB_SIZE 128
#define MI_SIZE 4
#define FRAME_SIZE 10

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
  depthComparisonInfo depthComparisonInfoFrame[MAX_DEP_REPR][FRAME_SIZE];
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

int set_ref_dep_encoding(const char* path, const int bitRates[],
          const int subDirCount, const int maxIndex, refAndDepEncoding* p_RefAndDepEncoding)
{
  p_RefAndDepEncoding->totalCount = subDirCount;
  int tmpDepCount = 0;
  char* tmpLocationStr;

  for (int i = 0; i < subDirCount; i++) {
    tmpLocationStr = malloc(1 + strlen(path) + 10 +16); // 1 + strlen(path) + 140000 + analysisData.bin
    check_mem(tmpLocationStr);
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

    free(tmpLocationStr);
  }

  return 0;

  error:
    if(tmpLocationStr) free(tmpLocationStr);
    return -1;
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

void print_depth_info_frame(refAndDepEncoding* p_RefAndDepEncoding)
{
  int nrDepRepr = p_RefAndDepEncoding->totalCount - 1;

  float greaterDepth;
  float lessThanGreaterDepth;
  for(int j=0; j<nrDepRepr; j++ ){
    debug("Print depth compraison for %s.", p_RefAndDepEncoding->depBitRateDirectory[j]);
    for( int i=0; i<FRAME_SIZE; i++){
      greaterDepth = p_RefAndDepEncoding->depthComparisonInfoFrame[j][i].greaterDepth;
      log_info("Frame number = %d, Equal or lesser = %f, Greater = %f", i, 1-greaterDepth, greaterDepth);
    }
  }
}

int save_avg_depth_info_frame(refAndDepEncoding* p_RefAndDepEncoding)
{
  int nrDepRepr = p_RefAndDepEncoding->totalCount - 1;

  float greaterDepth = 0;
  float equalDepth = 0;
  float avgGreaterDepth = 0;
  float avgEqualDepth = 0;
  float avgLesserDepth = 0;
  FILE* mDepthInfoFile;

  char* tmpLocationStr;
  int tmpCutOffLength;

  for(int j=0; j<nrDepRepr; j++ ){

    greaterDepth = 0;
    equalDepth = 0;

    // Open file for writing.
    tmpCutOffLength = strlen(p_RefAndDepEncoding->depBitRateDirectory[j])-16;

    tmpLocationStr = malloc(1 + tmpCutOffLength + 19); // 1 + strlen(path) + analysisData.bin
    check_mem(tmpLocationStr);

    strncpy(tmpLocationStr, p_RefAndDepEncoding->depBitRateDirectory[j], tmpCutOffLength);
    tmpLocationStr[tmpCutOffLength] = '\0';
    strcat(tmpLocationStr, "depthComparison.csv");

    for( int i=0; i<FRAME_SIZE; i++){
      greaterDepth += p_RefAndDepEncoding->depthComparisonInfoFrame[j][i].greaterDepth;
      equalDepth += p_RefAndDepEncoding->depthComparisonInfoFrame[j][i].greaterDepth;
    }

    avgGreaterDepth = greaterDepth/FRAME_SIZE;
    avgEqualDepth = equalDepth/FRAME_SIZE;
    avgLesserDepth = 1 - (avgGreaterDepth + avgEqualDepth);
    debug("%s-%d, Greater=%f, Equal=%f, Lesser=%f", p_RefAndDepEncoding->resInformation->name, p_RefAndDepEncoding->depBitRate[j], avgGreaterDepth, avgEqualDepth, avgLesserDepth);

    mDepthInfoFile = fopen(tmpLocationStr, "w"); // Write mode (delete pre existing)
    check(mDepthInfoFile != NULL, "Error opening file - %s",  tmpLocationStr);
    debug("Saving info in %s", tmpLocationStr);
    fprintf(mDepthInfoFile, "%s,%s,%s\n", "greater", "equal", "lesser");
    fprintf(mDepthInfoFile, "%f,%f,%f\n", avgGreaterDepth, avgEqualDepth,avgLesserDepth);

    if(mDepthInfoFile) fclose(mDepthInfoFile);
    if(tmpLocationStr) free(tmpLocationStr);
  }

  return 0;

  error:
    if(tmpLocationStr) free(tmpLocationStr);
    if(mDepthInfoFile) fclose(mDepthInfoFile);
    return -1;
}



int analyse_depth_information(refAndDepEncoding* p_RefAndDepEncoding)
{
  int nrDepRepr = p_RefAndDepEncoding->totalCount - 1;
  int bytesToRead;
  bytesToRead = p_RefAndDepEncoding->resInformation->nrDepthInfoInFrame;

  FILE* mDataFileRef;
  mDataFileRef = fopen(p_RefAndDepEncoding->refBitRateDirectory, "rb"); // READ mode (read, binary)
  check(mDataFileRef != NULL, "Error opening file - %s",  p_RefAndDepEncoding->refBitRateDirectory);
  UChar* refDepthInfoFrame = (UChar* )malloc(bytesToRead);
  check_mem(refDepthInfoFrame);

  FILE** mDataFileDep = malloc(sizeof(FILE*) * nrDepRepr);
  for(int j=0; j<nrDepRepr; j++ ){
    mDataFileDep[j] = fopen(p_RefAndDepEncoding->depBitRateDirectory[j], "rb"); // READ mode (read, binary)
    check(mDataFileDep[j] != NULL, "Error opening file - %s",  p_RefAndDepEncoding->depBitRateDirectory[j]);
  }
  UChar* depDepthInfoFrame = (UChar* )malloc(bytesToRead);
  check_mem(depDepthInfoFrame);

  for (int i=0; i<FRAME_SIZE; i++){
    int sizeReadRef = fread(refDepthInfoFrame, sizeof(UChar), bytesToRead, mDataFileRef);
    for(int j=0; j<nrDepRepr; j++ ){
      int sizeReadDep = fread(depDepthInfoFrame, sizeof(UChar), bytesToRead, mDataFileDep[j]);
      p_RefAndDepEncoding->depthComparisonInfoFrame[j][i] = compare_depth_info(refDepthInfoFrame, depDepthInfoFrame, bytesToRead);
    }
  }

  //print_depth_info_frame(p_RefAndDepEncoding);
  save_avg_depth_info_frame(p_RefAndDepEncoding);
  free(refDepthInfoFrame);
  fclose(mDataFileRef);
  free(depDepthInfoFrame);

  for(int j=0; j<nrDepRepr; j++ ){
    fclose(mDataFileDep[j]);
  }
  return 0;

  error:
    if(refDepthInfoFrame) free(refDepthInfoFrame);
    if(depDepthInfoFrame) free(depDepthInfoFrame);
    if(mDataFileRef) fclose(mDataFileRef);
    for(int j=0; j<nrDepRepr; j++ ){
      if(mDataFileDep[j]) fclose(mDataFileDep[j]);
    }
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

  char* videoPathArray[] = {"/home/adithyan/Innovation/MultiRate/PartitionReuse/Modified/BlueSky/"};
  char* resolutionPathArray[] = {"360p", "720p"};


  int videoLength = sizeof(videoPathArray)/sizeof(videoPathArray[0]);
  int resolutionLength = sizeof(resolutionPathArray)/sizeof(resolutionPathArray[0]);
  char* path;

  debug("Video length - %d", videoLength);
  debug("Resolution length - %d", resolutionLength);

  for (int j=0; j<videoLength; j++){
    char* videoPath = videoPathArray[j];
    for (int i=0; i<resolutionLength; i++){
      char* resolutionPath = resolutionPathArray[i];
      path = malloc(1 + strlen(videoPath) + strlen(resolutionPath));
      check_mem(path);
      strcpy(path, videoPath);
      strcat(path, resolutionPath);
      log_info("Analysing folder location - %s", path);

      refAndDepEncoding a_RefAndDepEncoding;
      status = extract_encoding_info(path,  &a_RefAndDepEncoding);
      check(status == 0, "Extracting encoding failed for %s.", path);

      if (strcmp(resolutionPath, "360p") == 0){
        log_info("360p successuul. %s.", resolutionPath);
        a_RefAndDepEncoding.resInformation = &resInfo360p;
      }
      else if (strcmp(resolutionPath, "720p") == 0){
        log_info("720p successuul. %s.", resolutionPath);
        a_RefAndDepEncoding.resInformation = &resInfo720p;
      }
      else if (strcmp(resolutionPath, "1080p") == 0){
        log_info("1080p successuul. %s.", resolutionPath);
        a_RefAndDepEncoding.resInformation = &resInfo1080p;
      }
      else{
        log_err("Unknow resolution path - %s", resolutionPath);
      }

      status = analyse_depth_information(&a_RefAndDepEncoding);
      check(status == 0, "Analysing depth information failed for %s.", path);
      clean_encoding_info(&a_RefAndDepEncoding);
      if(path) free(path);
    }
  }
  return 0;
  error:
    if(path) free(path);
    return -1;
}

