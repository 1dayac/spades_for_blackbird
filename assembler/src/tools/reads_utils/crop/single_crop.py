#!/usr/bin/python -O

import sys
import os

bowtie = sys.argv[1]

if (bowtie == '-h'):
	print("Usage: <bowtie_path/> <genome_reference> <file with list of files> <ouput_dir/> [crop_len = 100000] [offset = 0] [format(-f/-q) = -q]\n")
	sys.exit(0)

genome = sys.argv[2]
flist = open(sys.argv[3])
outdir = sys.argv[4]

crop_len = 100000
if len(sys.argv) > 5:
	crop_len = int(sys.argv[5])

offset = 0
if len(sys.argv) > 6:
	offset = int(sys.argv[6])

format = "-q"
if len(sys.argv) > 7:
	format = sys.argv[7]


output_genome = outdir + genome + "_" + str(crop_len) + "_" + str(offset) + ".fa"

# crop genome to first 'crop_len' basepairs
gf = open(genome)
ogf = open(output_genome, 'w')

for line in gf:
	ogf.write(line)
	break

cnt = -offset
if offset > 0:
	for line in gf:
		if cnt >= 0:
			break
		cnt+=70

cnt = 0
for line in gf:
	if (crop_len - cnt < 70):
		line = line[0:crop_len-cnt]
	ogf.write(line);
	cnt += 70
	if cnt >= max:
		break

gf.close()
ogf.close()

print('Cropped\n')

file_suff = str(crop_len) + '_' + str(offset) 

# build bowtie index
os.system('mkdir -p index')
os.system(bowtie +'bowtie-build ' + output_genome + ' ' + outdir + 'index/ref' + file_suff)
print('Index built\n')

# align reads using bowtie
for line in flist:
	fName, ext = os.path.splitext(line)
<<<<<<< HEAD
	os.system(bowtie + 'bowtie -p 8 -c ' + format + ' ' + outdir + 'index/ref' + file_suff + ' ' + line.strip() + ' --al ' + fName + file_suff + ext.strip() + ' > /dev/null 2> errlog_' + fName)
=======
	os.system(bowtie + 'bowtie -p 8 -c ' + format + ' ' + outdir + 'index/ref' + file_suff + ' ' + line.strip() + ' --al ' + outdir + fName + file_suff + ext.strip() + ' > /dev/null 2>  errlog' + fName)
>>>>>>> bd2bc9b8955f31b87a3398d43f26e84fe201eb78

print('Alligned\n')
# use -X for maxins length

