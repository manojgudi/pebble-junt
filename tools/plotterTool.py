import matplotlib.pyplot as plotObject
import numpy as np

import traceback

class AccelReading:
    def __init__(self, epochTimeMs, x, y, z):
        self.x = x
        self.y = y
        self.z = z
        self.epochTimeMs    = epochTimeMs
        self.isEmptyReading = False

        # Empty Reading is used later for analysis
        if x == y == z == 0:
            self.isEmptyReading = True
            


def findSubString(textLine, subString):
    if textLine.find(subString) == -1:
        return False
    return True

def extractNumber(line):
    return eval(line.split(":")[-1])

def extractReadings(textLine):
    readings = []
    textLineCSV = textLine[ textLine.find("t:") : ]
 
    readings = map( extractNumber, textLineCSV.split(",") )
    return readings


def funcParseLine(textLine):
    subStrings = ["x:", "y:", "z:", "t:"]
    # If the above subStrings are not found in a line, then ignore than line
    if not all(map(lambda x: findSubString(textLine, x), subStrings)):
        return False

    # Extract Sub String
    readings = extractReadings(textLine)
    accelReading = AccelReading(*readings)

    return accelReading

def loadFromFile(pebbleLogFile):
    """
    Takes a filePath, reads it and returns the list of accelReadings 
    objects
    """
    accelReadings = []
    try:
        with open(pebbleLogFile) as fileHandler:
            for textLine in fileHandler:
                readings = funcParseLine(textLine)
                if readings:
                    accelReadings.append(readings)
    except:
        print("Error in loading from the file:")
        traceback.print_exc()
    
    return accelReadings

def plotRawAccReadings(accelReadings, plt):
    """
    Return plt object 
    """
    timeSeries = np.array( [accelReading.epochTimeMs for accelReading in accelReadings] )
    xs = np.array( [accelReading.x for accelReading in accelReadings] )
    ys = np.array( [accelReading.y for accelReading in accelReadings] )
    zs = np.array( [accelReading.z for accelReading in accelReadings] )

    plt.plot(timeSeries, xs, label="Acc X")
    plt.plot(timeSeries, ys, label="Acc Y")
    plt.plot(timeSeries, zs, label="Acc Z")

    plt.legend(loc="upper left")    
    return plt

def main():
    pebbleLogFile = "./trainingData/thirtyJumpsLegCalibrated1.txt"
    pebbleLogFile = "./trainingData/twentyJumpsLegCalibrated1.txt"
    accelReadings = loadFromFile(pebbleLogFile)
    
    plotObject_ = plotRawAccReadings(accelReadings, plotObject)
    plotObject_.show()


if __name__ == "__main__":
    main()
