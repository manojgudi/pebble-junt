from plotterTool import loadFromFile, AccelReading, plotRawAccReadings

import matplotlib.pyplot as plotObject
import numpy as np


peakPointInflectionRatioThreshold = 4
peakPointDeflectionRatioThreshold = 0.4

def peakDetected(accelReadingWindow, peakPointInflectionRatioThreshold, peakPointDeflectionRatioThreshold):
    """
    We will be working with readings of accel X only
    as we have oriented pebble in such a way that X component
    of accelerometer should show a significant jump
    """
    inflectionDetected = False
    deflectionDetected = False
    firstAccelReading  = accelReadingWindow[0].x

    # Zero Check, If zero, the window isnt formed, move the sliding
    # window further 
    if firstAccelReading == 0:
        return False

    for i in accelReadingWindow:
        # Analyze the X component only    
        i = i.x if i.x != 0 else 1

        ratio = abs(i / firstAccelReading)
        if ratio >= peakPointInflectionRatioThreshold:
            inflectionDetected = True

        if inflectionDetected and (ratio <= peakPointDeflectionRatioThreshold):
            deflectionDetected = True

        firstAccelReading = i

    return inflectionDetected and deflectionDetected

def pushInWindow(accelReadingWindow, accelReading):
    """
    append the reading and pop the first accel reading
    This function MUTATES the list itself, hence NO return value
    """
    accelReadingWindow.pop(0)
    accelReadingWindow.append(accelReading)

def initializeAccelWindow(windowSize):
    accelReadingWindow = [ AccelReading(1, 0, 0, 0) for i in range(windowSize)]
    return accelReadingWindow

def simulateRunning(accelReadings, windowSize, peakPointInflectionRatioThreshold, peakPointDeflectionRatioThreshold):
    """
    Run the algorithm on accelReadings, see if it detects peak
    by simulating incoming accelerometer data
    """
    windowsWithPeak    = []

    # Initialize reading window with empty readings 
    accelReadingWindow = [ AccelReading(1, 0, 0, 0) for i in range(windowSize)]

    # Slide the window and detect peak
    for accelReading in accelReadings:
        pushInWindow(accelReadingWindow, accelReading)
        
        if peakDetected(accelReadingWindow, peakPointInflectionRatioThreshold, peakPointDeflectionRatioThreshold):
            windowsWithPeak.append(accelReadingWindow)
            accelReadingWindow = initializeAccelWindow(windowSize)
            #print("Detected Peak")

        
    return windowsWithPeak

def plotAccelReadingWindow(accelReadingWindow, plt):
    """
    Plot the accelerometer readings (just the X Axis)
    Thats where we should see the peaks
    """
    timeSeries = np.array( [accelReading.epochTimeMs for accelReading in accelReadingWindow] )
    xs = np.array([ accelReading.x for accelReading in accelReadingWindow])
    plt.plot(timeSeries, xs, "ro", label="Peaks" )
    #plt.legend(loc="upper left")
    return plt
    
def plotWindowsWithPeak(windowsWithPeak, plt):
    """
    Plot all the Window
    """
    for accelReadingWindow in windowsWithPeak:
        plt = plotAccelReadingWindow(accelReadingWindow, plt)

    return plt

def main():
    #pebbleLogFile = "./trainingData/thirtyJumpsLegCalibrated3.txt"
    pebbleLogFile = "./trainingData/twentyJumpsLegCalibrated3.txt"
    accelReadings = loadFromFile(pebbleLogFile)
    windowSize    = 5
    print("Simulating for ", pebbleLogFile)

    # Simulate Live Reading
    #windowsWithPeak = simulateRunning(accelReadings, windowSize, peakPointInflectionRatioThreshold, peakPointDeflectionRatioThreshold)
    #print("Peaks Detected", len(windowsWithPeak))
    # Plot raw readings
    #plotObject_ = plotRawAccReadings(accelReadings, plotObject)
    #plotObject_ = plotWindowsWithPeak(windowsWithPeak, plotObject_)
    #plotObject_.show()


    
    for k in range(3, 12):
        for i in np.arange(1.5, 4.5, 0.1):
            for j in np.arange(0.3, 1.5, 0.1):
                windowsWithPeak = simulateRunning(accelReadings, k, i, j)
                jumps = len(windowsWithPeak)
                if jumps >= 25:
                    print("I ", i, "J ", j, "K", k, "Jumps ", jumps) 
    
        print("---------")

    

if __name__ == "__main__":
    main()
