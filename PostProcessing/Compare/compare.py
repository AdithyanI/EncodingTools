from multiprocessing import cpu_count as cpu_count
import numpy as np
import Bjontegaard_metric.bjontegaard_metric as bjontegaard
import subprocess
import os
import sys
global encodingInfoSet
import math
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib2tikz import save as tikz_save
import csv

#python encoding.py /home/adithyan/Innovation/RawVideo/ /home/adithyan/Innovation/MultiRate/PartitionReuse/ /home/adithyan/Innovation/aomenc

encodingInfoSet = [
                {"name":"360p","width":640,"height":360,"reprBitRates":[500,650,800,1100] },
                #{"name":"360p","width":640,"height":360,"reprBitRates":[500,650,800,1100,1400] },
                {"name":"720p","width":1280,"height":720,"reprBitRates":[1500,1950,2400,3300,4200] },
                #{"name":"1080p","width":1920,"height":1080,"reprBitRates":[3000,3900,4800,6600,8400] }
                #{"name":"2160p","width":4096,"height":2160,"reprBitRate":[10000,16000,28000] },
                  ]
processes = set()
max_processes = cpu_count
max_processes = 4

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

def extract_statistic(fileName):
  lastTwoLine = subprocess.check_output(['tail', '-n', '2', fileName])
  lastLine = lastTwoLine.split('\x1b[K')[3]
  lastButOneLine = lastTwoLine.split('\x1b[K')[2]
  return {'PSNR':lastLine.split()[4], 'bitRate':lastButOneLine.split()[6][:-3],'encodingTime':lastButOneLine.split()[7]}

def analyse_statistic(unmodifiedStatistic, modifiedStatistic, videoDirectoryModified):
  R1 = np.array(map(float,unmodifiedStatistic["bitRate"]))/1000
  PSNR1 = np.array(map(float,unmodifiedStatistic["PSNR"]))
  R2 = np.array(map(float,modifiedStatistic["bitRate"]))/1000
  PSNR2 = np.array(map(float,modifiedStatistic["PSNR"]))

  bdpsnr = bjontegaard.BD_PSNR(R1, PSNR1, R2, PSNR2)
  bdrate =  bjontegaard.BD_RATE(R1, PSNR1, R2, PSNR2)
  print bdpsnr
  print bdrate

  bdpsnr = int(bdpsnr * 1000)/ 1000.0
  bdrate = int(bdrate * 1000)/1000.0

  latexTable = [["BD-PSNR (dB)", bdpsnr],["BD-Rate (percent)", bdrate]]
  print latexTable
  with open(os.path.join(videoDirectoryModified,'bd.csv'), 'wb') as myfile:
      wr = csv.writer(myfile, quoting=csv.QUOTE_ALL)
      wr.writerows(latexTable)

  unmodifiedPlot, = plt.plot(R1, PSNR1, 'o-')
  modifiedPlot, = plt.plot(R2, PSNR2, 'o-')
  plt.xlabel('Bitrate (kbits/s)')
  plt.ylabel('PSNR (dB)')
  plt.title('RD performance')
  plt.legend([unmodifiedPlot, modifiedPlot], ['Reference encoder','Multirate encoder'])
  plt.grid(True)
  tikz_save(os.path.join(videoDirectoryModified,'RD.tex'))
  plt.clf()

def compare(subDirectoryFullUnmodified,  subDirectoryFullModified, video):
  global encodingInfoSet

  for encodingInfo in encodingInfoSet:
    videoDirectoryUnmodified = os.path.join(subDirectoryFullUnmodified, encodingInfo["name"])
    videoDirectoryModified = os.path.join(subDirectoryFullModified, encodingInfo["name"])

    unmodifiedStatistic = {}
    modifiedStatistic = {}
    errorFlag = 0

    for reprBitRate in encodingInfo["reprBitRates"]:
      print video,"-",encodingInfo["name"],"-",reprBitRate,"kpbs"

      unmodifiedFile = os.path.join(videoDirectoryUnmodified,str(reprBitRate),"log.txt")
      if not os.path.exists(unmodifiedFile):
        print "Error: Unmodified file does not exist", unmodifiedFile
        errorFlag = 1
        continue
      extractedStatistic = extract_statistic(unmodifiedFile)
      unmodifiedStatistic.setdefault("PSNR", []).append(extractedStatistic["PSNR"])
      unmodifiedStatistic.setdefault("bitRate", []).append(extractedStatistic["bitRate"])
      unmodifiedStatistic.setdefault("encodingTime", []).append(extractedStatistic["encodingTime"])

      modifiedFile = os.path.join(videoDirectoryModified,str(reprBitRate),"log.txt")
      if not os.path.exists(modifiedFile):
        print "Error: Modified file does not exist", modifiedFile
        errorFlag = 1
        continue
      extractedStatistic = extract_statistic(modifiedFile)
      modifiedStatistic.setdefault("PSNR", []).append(extractedStatistic["PSNR"])
      modifiedStatistic.setdefault("bitRate", []).append(extractedStatistic["bitRate"])
      modifiedStatistic.setdefault("encodingTime", []).append(extractedStatistic["encodingTime"])

    print unmodifiedStatistic
    print modifiedStatistic
    if not errorFlag:
      print "Analysing statistic."
      analyse_statistic(unmodifiedStatistic, modifiedStatistic, videoDirectoryModified)
 
def wait_for_all_to_complete():
  for p in processes:
    if p.poll() is None:
      p.wait()

def main():
  #https://stackoverflow.com/questions/973473/getting-a-list-of-all-subdirectories-in-the-current-directory
  unmodifiedVideosDirectory = str(sys.argv[1])
  modifiedVideosDirectory = str(sys.argv[2])
  subDirectories = next(os.walk(unmodifiedVideosDirectory))[1]
  print "Found subfolders in unmodified :", subDirectories
  for video in subDirectories:
    subDirectoryFullUnmodified = os.path.join(unmodifiedVideosDirectory, video)
    subDirectoryFullModified = os.path.join(modifiedVideosDirectory, video)
    if not os.path.exists(subDirectoryFullModified):
      print "Modifed directory not present", subDirectoryFullModified
    compare(subDirectoryFullUnmodified,  subDirectoryFullModified, video)
  wait_for_all_to_complete()
  print("END")

if __name__ == "__main__":
  if(len(sys.argv) < 3):
    print "Not enough arguments. Usuage : encoding.py </location/of/unmodifiedFiles> </location/of/modifiedFiles> <encoder>"
    print "Exiting!..."
    sys.exit()
  print "Comparing video present in ", str(sys.argv[1]),"and", str(sys.argv[2])
  main()
