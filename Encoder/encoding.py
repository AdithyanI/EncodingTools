import subprocess
import os
import sys

def find_highres_source_file(subDirectoryFull):
   # https://stackoverflow.com/questions/3964681/find-all-files-in-a-directory-with-extension-txt-in-python     
    for file in os.listdir(subDirectoryFull):
      if file.endswith(".y4m") and file.find("1080p")!=-1:
        print("Found source file = " + file)
        return file

def encoding():
  #https://stackoverflow.com/questions/973473/getting-a-list-of-all-subdirectories-in-the-current-directory
  #cwd = str(sys.argv[1])
  cwd = "/home/adithyan/Innovation/RawVideo"
  subDirectories = next(os.walk(cwd))[1]
  print "Found subfolders :", subDirectories
  encodingInfoSet = [
                  {"name":"360p","width":640,"height":360,"reprBitRates":[500,800,1400] },
                  {"name":"720p","width":1280,"height":720,"reprBitRates":[1500,2400,4200] },
                  {"name":"1080p","width":1920,"height":1080,"reprBitRates":[3000,4800,8400] },
                  #{"name":"2160p","width":4096,"height":2160,"reprBitRate":[10000,16000,28000] },
                    ]
  
  for subDirectory in subDirectories:
    subDirectoryFull = cwd + "/" + subDirectory
    for encodingInfo in encodingInfoSet:
      for reprBitRate in encodingInfo["reprBitRates"]:
        print "Encoding",subDirectory,"-",encodingInfo["name"],"-",reprBitRate,"kpbs"
        bashCommand = "./aomenc -v --good --limit=1 --target-bitrate=",reprBitRate,"-o output.webm blue_sky_1080p25.y4m"
        p1 = subprocess.Popen(bashCommand, shell=True, cwd=subDirectoryFull)
        p1.wait()




    '''
    inputHighResFile = find_highres_source_file(subDirectoryFull)





    for downSampleResolution in downSampleResolutions:
      outputLowResFile = inputHighResFile.replace("1080p", \
                                      str(downSampleResolution[1])+"p")

      if os.path.exists(os.path.join(subDirectoryFull, outputLowResFile)):
        print(outputLowResFile + " already exists. Skipping...")
        continue
      else:
        print("Creating " + outputLowResFile + " from " + inputHighResFile)

      bashCommand = "ffmpeg -i " + inputHighResFile + " -vf scale=" \
                                      + str(downSampleResolution[0]) + ":" \
                                      + str(downSampleResolution[1])  + " " \
                                      + outputLowResFile
      p1 = subprocess.Popen(bashCommand, shell=True, cwd=subDirectoryFull)
      p1.wait()
      '''

if __name__ == "__main__":
  
  #if(len(sys.argv) < 3):
  #  print "Not enough arguments. Usuage : encoding.py </location/of/rawVideoDirectory>"
  #  sys.exit()

  #print "Downsampling all videos present in subfolder of -", \
  #str(sys.argv[1])
  encoding()
