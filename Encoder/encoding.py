from multiprocessing import cpu_count as cpu_count
import subprocess
import os
import sys
global encodingInfoSet

encodingInfoSet = [
                {"name":"360p","width":640,"height":360,"reprBitRates":[500] },
                #{"name":"360p","width":640,"height":360,"reprBitRates":[500,800,1400] },
                #{"name":"720p","width":1280,"height":720,"reprBitRates":[1500,2400,4200] },
                #{"name":"1080p","width":1920,"height":1080,"reprBitRates":[3000,4800,8400] },
                #{"name":"2160p","width":4096,"height":2160,"reprBitRate":[10000,16000,28000] },
                  ]
processes = set()
max_processes = cpu_count

def find_highres_source_file(directory):
  # https://stackoverflow.com/questions/3964681/find-all-files-in-a-directory-with-extension-txt-in-python
  for file in os.listdir(directory):
    if file.endswith(".y4m"):
      print("Found source file = " + file)
      return file

def create_directory(directory):
  if not os.path.exists(directory):
    os.makedirs(directory)

def cleanup_directory(directory):
  for item in os.listdir(directory):
    if item.endswith(".webm") or item.endswith(".bin") or item == "log.txt":
      os.remove(os.path.join(directory,item))

def encode_all_bitrates(subDirectoryFull,video):
  global encodingInfoSet
  outputVideosDirectory = str(sys.argv[2])
  aomEncoder = str(sys.argv[3])

  for encodingInfo in encodingInfoSet:
    resolutionDirectory = os.path.join(subDirectoryFull, encodingInfo["name"])
    inputFileName = find_highres_source_file(resolutionDirectory)
    inputFile = os.path.join(resolutionDirectory, inputFileName)

    for reprBitRate in encodingInfo["reprBitRates"]:
      print "Encoding",video,"-",encodingInfo["name"],"-",reprBitRate,"kpbs"

      outputFileName = inputFileName.replace("y4m","webm")
      outputDirectory = os.path.join(outputVideosDirectory,video,encodingInfo["name"],str(reprBitRate))
      outputFile = os.path.join(outputDirectory, outputFileName)
      create_directory(outputDirectory)
      cleanup_directory(outputDirectory)
      bashCommand = aomEncoder + " --psnr --good --limit=2 --target-bitrate="+str(reprBitRate)\
                    +" -o " + outputFile + " " + inputFile + " 2> log.txt"

      # Run multiple process in parallel but limit to max_processes
      process = subprocess.Popen(bashCommand,shell=True, cwd=outputDirectory)
      processes.add(process)

      if len(processes) >= max_processes:
        os.wait()
        processes.difference_update([p for p in processes if p.poll() is not None])

def wait_for_all_to_complete():
  for p in processes:
    if p.poll() is None:
      p.wait()

def encoding():
  #https://stackoverflow.com/questions/973473/getting-a-list-of-all-subdirectories-in-the-current-directory
  inputVideosDirectory = str(sys.argv[1])
  subDirectories = next(os.walk(inputVideosDirectory))[1]
  print "Found subfolders :", subDirectories
  for video in subDirectories:
    subDirectoryFull = os.path.join(inputVideosDirectory,video)
    encode_all_bitrates(subDirectoryFull, video)
  wait_for_all_to_complete()
  print("END")

if __name__ == "__main__":
  if(len(sys.argv) < 4):
    print "Not enough arguments. Usuage : encoding.py </location/of/inputVideo> \
           </location/of/outputVideo> <encoder>"
    print "Exiting!..."
    sys.exit()
  print "Starting encoding of videos present in folder of -", str(sys.argv[1])
  encoding()
