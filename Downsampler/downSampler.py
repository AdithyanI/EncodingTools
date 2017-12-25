import subprocess
import os

def find_highres_source_file(subDirectoryFull):
   # https://stackoverflow.com/questions/3964681/find-all-files-in-a-directory-with-extension-txt-in-python     
    print(subDirectoryFull)
    for file in os.listdir(subDirectoryFull):
      if file.endswith(".y4m") and file.find("1080p")!=-1:
        print("Found source file = " + file)
        return file

def down_sampler():
  #https://stackoverflow.com/questions/973473/getting-a-list-of-all-subdirectories-in-the-current-directory
  #cwd = os.getcwd()
  cwd = "/home/adithyan/Innovation/RawVideo"
  subDirectories = next(os.walk(cwd))[1]
  print(subDirectories)
  downSampleResolutions = [
                                                  [320, 180],
                                                  [640, 360],
                                                  [1280, 720]
                                                ]
  
  for subDirectory in subDirectories:
    subDirectoryFull = cwd + "/" + subDirectory
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

if __name__ == "__main__":
	down_sampler()
