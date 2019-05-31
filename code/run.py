# This script written by Mgwoo.
# 2018.02.22.
# 
# mgwoo@unist.ac.kr

import os
import sys
import subprocess as sp
from datetime import datetime

dirpos = "../bench"
binaryName = "./ispd19dr"
outpos = "../output"
logpos = "../log"
#latestDir = "../output/latest"

def ExecuteCommand( curCmd ):
    print( curCmd )
    sp.call( curCmd , shell=True)


if len(sys.argv) <=2:
    print("usage:   ./run.py <benchname or number> <# Threads>")
    print("Example: ")
    print("         ./run.py 2 1")
    print("         ./run.py example_2.input 1")
    sys.exit(1)

benchNum = -1
benchName = ""
if sys.argv[1].isdigit():
    benchNum = int(sys.argv[1])
    benchName = sorted(os.listdir(dirpos))[benchNum]
elif sys.argv[1] == "all":
    benchName = sorted(os.listdir(dirpos))
else:
    benchName = sys.argv[1]

numThreads = int(sys.argv[2])
curTime = datetime.now().strftime('%m_%d_%H_%M')

#print curTime

if type(benchName) is list:
    for curBench in benchName:
        lefPath = "%s/%s/%s.input.lef" % (dirpos, curBench, curBench)
        defPath = "%s/%s/%s.input.def" % (dirpos, curBench, curBench)
        gudPath = "%s/%s/%s.input.guide" % (dirpos, curBench, curBench)
        outPath = "%s/%s/%s.out" % (outpos, curBench, curBench)
        logPath = "./latest.log"
        logPath = "%s/%s/%s.log" % (logpos, curBench, curTime)
        
        exeStr = "%s -lef %s -def %s -guide %s -threads %s -tat 60 -output %s | tee %s" % (binaryName, lefPath, defPath, gudPath, numThreads, outPath, logPath)
        ExecuteCommand(exeStr)
else:
    lefPath = "%s/%s/%s.input.lef" % (dirpos, benchName, benchName)
    defPath = "%s/%s/%s.input.def" % (dirpos, benchName, benchName)
    gudPath = "%s/%s/%s.input.guide" % (dirpos, benchName, benchName)
    outPath = "%s/%s/%s.out" % (outpos, benchName, benchName)
    logPath = "./latest.log"
    logPath = "%s/%s/%s.log" % (logpos, benchName, curTime)
    exeStr = "%s -lef %s -def %s -guide %s -threads %s -tat 60 -output %s | tee %s" % (binaryName, lefPath, defPath, gudPath, numThreads, outPath, logPath)
    ExecuteCommand(exeStr)
