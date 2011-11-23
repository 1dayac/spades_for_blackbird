#!/usr/bin/python -O

#Calculate coverage from raw file

import sys

if len(sys.argv) != 5:
	print("Usage: <coverage file> <output> <genome length> <bar width>");
	exit(0)

inFile = open(sys.argv[1])
outFile = open(sys.argv[2], 'w')
maxLen = int(sys.argv[3])
bar = int(sys.argv[4])

hist = [0 for i in range(maxLen	+ 1)]

for line in inFile:
	stpos = int(line.split()[0])
	for i in range(0,int(line.split()[1])):
		cpos = stpos + i
		if cpos <= maxLen:
			hist[cpos] += 1

covered = 0.0
for i in range(0,maxLen + 1):
	if (hist[i] > 0):
		covered += 1.0

print("Coverage: " + str(covered/maxLen) + "\n")

newHist = [0 for i in range((maxLen + 1) / bar + 2)]

for i in range(maxLen + 1):
	newHist[int(i/bar)] += hist[i]

for i in range((maxLen + 1) / bar + 1):
	outFile.write(str(i) + ' ' + str(newHist[i] / bar) + '\n')


inFile.close()
outFile.close()
