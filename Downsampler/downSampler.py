import subprocess
import os

def find_highres_source_file(subDirectoryFull):
   # https://stackoverflow.com/questions/3964681/find-all-files-in-a-directory-with-extension-txt-in-python     
    for file in os.listdir(subDirectoryFull):
      if file.endswith(".y4m"):
        return file

def down_sampler():
  #https://stackoverflow.com/questions/973473/getting-a-list-of-all-subdirectories-in-the-current-directory
  #cwd = os.getcwd()
  cwd = "/home/adithyan/Innovation/TestContent"
  subDirectories = next(os.walk(cwd))[1]
  downSampleResolutions = [
                                                  [640, 360],
                                                  [1280, 720]
                                                ]
  
  for subDirectory in subDirectories:
    subDirectoryFull = cwd + "/" + subDirectory
    sourceHighResFile = find_highres_source_file(subDirectoryFull)

    for downSampleResolution in downSampleResolutions:
      outputLowResFile = sourceHighResFile.replace("1080p", \
                                      str(downSampleResolution[1])+"p")
      #print(os.path.join(subDirectoryFull, outputLowResFile))
      #print(os.path.join(subDirectoryFull, sourceHighResFile))
      bashCommand = "ffmpeg -i " + sourceHighResFile + " -vf scale=" \
                                      + str(downSampleResolution[0]) + ":" \
                                      + str(downSampleResolution[1])  + " " \
                                      + outputLowResFile
      p1 = subprocess.Popen(bashCommand, shell=True, cwd=subDirectoryFull)
      p1.wait()

if __name__ == "__main__":
	down_sampler()
