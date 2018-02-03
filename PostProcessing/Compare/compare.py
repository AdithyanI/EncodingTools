from multiprocessing import cpu_count as cpu_count
import numpy as np
import Bjontegaard_metric.bjontegaard_metric as bjontegaard
import subprocess
import os
import sys
global encodingInfoSet


#python encoding.py /home/adithyan/Innovation/RawVideo/ /home/adithyan/Innovation/MultiRate/PartitionReuse/ /home/adithyan/Innovation/aomenc

encodingInfoSet = [
                {"name":"360p","width":640,"height":360,"reprBitRates":[500,650,800,1100] },
                #{"name":"720p","width":1280,"height":720,"reprBitRates":[1500,2400,4200] },
                #{"name":"360p","width":640,"height":360,"reprBitRates":[500,650,800,1100,1400] },
                #{"name":"720p","width":1280,"height":720,"reprBitRates":[1500,1950,2400,3300,4200] },
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


def bdsnr(unmodifiedStatistic, modifiedStatistic):
  """
  BJONTEGAARD    Bjontegaard metric calculation
  Bjontegaard's metric allows to compute the average gain in psnr between two
  rate-distortion curves [1].
  rate1,psnr1 - RD points for curve 1
  rate2,psnr2 - RD points for curve 2

  returns the calculated Bjontegaard metric 'dsnr'

  code adapted from code written by : (c) 2010 Giuseppe Valenzise
  http://www.mathworks.com/matlabcentral/fileexchange/27798-bjontegaard-metric/content/bjontegaard.m
  """
  # pylint: disable=too-many-locals
  # numpy seems to do tricks with its exports.
  # pylint: disable=no-member
  # map() is recommended against.
  # pylint: disable=bad-builtin
  rate1 = np.array(map(float,unmodifiedStatistic["bitRate"]))
  psnr1 = np.array(map(float,unmodifiedStatistic["PSNR"]))
  rate2 = np.array(map(float,modifiedStatistic["bitRate"]))
  psnr2 = np.array(map(float,modifiedStatistic["PSNR"]))

  log_rate1 = map(math.log, rate1)
  log_rate2 = map(math.log, rate2)

  # Best cubic poly fit for graph represented by log_ratex, psrn_x.
  poly1 = numpy.polyfit(log_rate1, psnr1, 3)
  poly2 = numpy.polyfit(log_rate2, psnr2, 3)

  # Integration interval.
  min_int = max([min(log_rate1), min(log_rate2)])
  max_int = min([max(log_rate1), max(log_rate2)])

  # Integrate poly1, and poly2.
  p_int1 = numpy.polyint(poly1)
  p_int2 = numpy.polyint(poly2)

  # Calculate the integrated value over the interval we care about.
  int1 = numpy.polyval(p_int1, max_int) - numpy.polyval(p_int1, min_int)
  int2 = numpy.polyval(p_int2, max_int) - numpy.polyval(p_int2, min_int)

  # Calculate the average improvement.
  if max_int != min_int:
    avg_diff = (int2 - int1) / (max_int - min_int)
  else:
    avg_diff = 0.0
  return avg_diff


def bdrate(unmodifiedStatistic, modifiedStatistic):
  """
  BJONTEGAARD    Bjontegaard metric calculation
  Bjontegaard's metric allows to compute the average % saving in bitrate
  between two rate-distortion curves [1].

  rate1,psnr1 - RD points for curve 1
  rate2,psnr2 - RD points for curve 2

  adapted from code from: (c) 2010 Giuseppe Valenzise

  """
  # numpy plays games with its exported functions.
  # pylint: disable=no-member
  # pylint: disable=too-many-locals
  # pylint: disable=bad-builtin
  rate1 = np.array(map(float,unmodifiedStatistic["bitRate"]))
  psnr1 = np.array(map(float,unmodifiedStatistic["PSNR"]))
  rate2 = np.array(map(float,modifiedStatistic["bitRate"]))
  psnr2 = np.array(map(float,modifiedStatistic["PSNR"]))

  log_rate1 = map(math.log, rate1)
  log_rate2 = map(math.log, rate2)

  # Best cubic poly fit for graph represented by log_ratex, psrn_x.
  poly1 = numpy.polyfit(psnr1, log_rate1, 3)
  poly2 = numpy.polyfit(psnr2, log_rate2, 3)

  # Integration interval.
  min_int = max([min(psnr1), min(psnr2)])
  max_int = min([max(psnr1), max(psnr2)])

  # find integral
  p_int1 = numpy.polyint(poly1)
  p_int2 = numpy.polyint(poly2)

  # Calculate the integrated value over the interval we care about.
  int1 = numpy.polyval(p_int1, max_int) - numpy.polyval(p_int1, min_int)
  int2 = numpy.polyval(p_int2, max_int) - numpy.polyval(p_int2, min_int)

  # Calculate the average improvement.
  avg_exp_diff = (int2 - int1) / (max_int - min_int)

  # In really bad formed data the exponent can grow too large.
  # clamp it.
  if avg_exp_diff > 200:
    avg_exp_diff = 200

  # Convert to a percentage.
  avg_diff = (math.exp(avg_exp_diff) - 1) * 100

  return avg_diff


def analyse_statistic(unmodifiedStatistic, modifiedStatistic):
  R1 = np.array(map(float,unmodifiedStatistic["bitRate"]))
  PSNR1 = np.array(map(float,unmodifiedStatistic["PSNR"]))
  R2 = np.array(map(float,modifiedStatistic["bitRate"]))
  PSNR2 = np.array(map(float,modifiedStatistic["PSNR"]))

  print 'BD-PSNR: ', bjontegaard.BD_PSNR(R1, PSNR1, R2, PSNR2)
  print 'BD-RATE: ', bjontegaard.BD_RATE(R1, PSNR1, R2, PSNR2)

def compare(subDirectoryFullUnmodified,  subDirectoryFullModified, video):
  global encodingInfoSet

  for encodingInfo in encodingInfoSet:
    videoDirectoryUnmodified = os.path.join(subDirectoryFullUnmodified, encodingInfo["name"])
    videoDirectoryModified = os.path.join(subDirectoryFullModified, encodingInfo["name"])

    unmodifiedStatistic = {}
    modifiedStatistic = {}

    for reprBitRate in encodingInfo["reprBitRates"]:
      print video,"-",encodingInfo["name"],"-",reprBitRate,"kpbs"

      unmodifiedFile = os.path.join(videoDirectoryUnmodified,str(reprBitRate),"log.txt")
      if not os.path.exists(unmodifiedFile):
        print "Error: Unmodified file does not exist", unmodifiedFile
        continue
      extractedStatistic = extract_statistic(unmodifiedFile)
      unmodifiedStatistic.setdefault("PSNR", []).append(extractedStatistic["PSNR"])
      unmodifiedStatistic.setdefault("bitRate", []).append(extractedStatistic["bitRate"])
      unmodifiedStatistic.setdefault("encodingTime", []).append(extractedStatistic["encodingTime"])

      modifiedFile = os.path.join(videoDirectoryModified,str(reprBitRate),"log.txt")
      if not os.path.exists(modifiedFile):
        print "Error: Modified file does not exist", modifiedFile
        continue
      extractedStatistic = extract_statistic(modifiedFile)
      modifiedStatistic.setdefault("PSNR", []).append(extractedStatistic["PSNR"])
      modifiedStatistic.setdefault("bitRate", []).append(extractedStatistic["bitRate"])
      modifiedStatistic.setdefault("encodingTime", []).append(extractedStatistic["encodingTime"])

    print unmodifiedStatistic
    print modifiedStatistic
    analyse_statistic(unmodifiedStatistic, modifiedStatistic)
    print "SNR", bdsnr(unmodifiedStatistic, modifiedStatistic)
    print "RATE", bdrate(unmodifiedStatistic, modifiedStatistic)

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
