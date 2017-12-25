import subprocess
import os

struct

def find_highres_source_file(subDirectoryFull):
   # https://stackoverflow.com/questions/3964681/find-all-files-in-a-directory-with-extension-txt-in-python     
    for file in os.listdir(subDirectoryFull):
      if file.endswith(".y4m"):
        return file

def down_sampler():
  #https://stackoverflow.com/questions/973473/getting-a-list-of-all-subdirectories-in-the-current-directory
  cwd = os.getcwd()
  subDirectories = next(os.walk(cwd))[1]

  downSampleResolutions = [[640,360],
                                                  [1280,720]]
  
  for subDirectory in subDirectories:
    subDirectoryFull = cwd + "/" + subDirectory

    sourceHighResFile = find_highres_source_file(subDirectoryFull)
    print(os.path.join(subDirectoryFull, sourceHighResFile))
    outputLowResFile = sourceHighResFile.replace("1080p", "360p")

	 bashCommand = "ffmpeg -i " + sourceHighResFile " -vf scale=640:360 " + outputLowResFile


  #bashCommand = "ffmpeg -i blue_sky_1080p25.y4m -vf scale=640:360 blue_sky_360p25.y4m"
	#subProcess.Popen(bashCommand)

if __name__ == "__main__":
	down_sampler()
