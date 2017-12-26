import subprocess
import os
import sys

def find_highres_source_file(subDirectoryFull):
   # https://stackoverflow.com/questions/3964681/find-all-files-in-a-directory-with-extension-txt-in-python     
    for file in os.listdir(subDirectoryFull):
      if file.endswith(".y4m") and file.find("1080p")!=-1:
        print "Found source file = ", file
        return file

def down_sampler():
  #https://stackoverflow.com/questions/973473/getting-a-list-of-all-subdirectories-in-the-current-directory
  cwd = str(sys.argv[1])
  subDirectories = next(os.walk(cwd))[1]
  print "Found subfolders :", subDirectories
  downSampleResolutions = [
                            [640, 360],
                            [1280,720]
                          ]
  
  for subDirectory in subDirectories:
    inputHighResDirectory = os.path.join(cwd,subDirectory,"1080p")
    inputHighResFile = find_highres_source_file(inputHighResDirectory)

    for downSampleResolution in downSampleResolutions:
      outputLowResFile = inputHighResFile.replace("1080p", \
                                      str(downSampleResolution[1])+"p")
      outputLowResDirectory = os.path.join(cwd,subDirectory,(str(downSampleResolution[1])+"p"))

      if os.path.exists(os.path.join(outputLowResDirectory, outputLowResFile)):
        print outputLowResFile,"already exists. Skipping..."
        continue
      else:
        if not os.path.exists(outputLowResDirectory):
          os.makedirs(outputLowResDirectory)
        print "Creating",outputLowResFile,"from",inputHighResFile

      bashCommand = "ffmpeg -i " + os.path.join(inputHighResDirectory,inputHighResFile) + " -vf scale=" \
                    + str(downSampleResolution[0]) + ":" + str(downSampleResolution[1])  + " " \
                    + os.path.join(outputLowResDirectory,outputLowResFile)

      p1 = subprocess.Popen(bashCommand, shell=True)
      p1.wait()

if __name__ == "__main__":
  if(len(sys.argv) < 2):
    print "Not enough arguments. Usuage : downsampler.py </location/of/rawVideoDirectory>"
    sys.exit()
  print "Downsampling all videos present in subfolder of -", \
  str(sys.argv[1])
  down_sampler()
