#!/usr/bin/python -O

import sys

inFile = open(sys.argv[1])
outFile = open(sys.argv[2], 'w')

delim = '/'

newRead = ""

line = inFile.readline()
while line:
	if (line.strip() == ""):
		line = inFile.readline()
		continue


	l = line.split()[0]
	strand = int(l.split(delim, 1)[1])
	id1 = line.split(delim, 1)[0]
	if strand == 1 or strand == 2:
		ids1 = id1.split(':')
		if len(ids1) < 5:
			for i in range(0,5 - len(ids1)):
				ids1.append('1')

		newRead = ids1[len(ids1) - 5]
		for i in range(len(ids1) - 4, len(ids1)):
			newRead += ':' + ids1[i]
		
		newRead += delim + str(strand)
				
		newRead += ':' + inFile.readline().strip();
		inFile.readline()
		newRead += ':' + inFile.readline().strip();
		
		outFile.write(newRead)
		outFile.write('\n')
	else:
		print("Wrong strand\n")
		print(line)
		break

	line = inFile.readline()


inFile.close()
outFile.close()
