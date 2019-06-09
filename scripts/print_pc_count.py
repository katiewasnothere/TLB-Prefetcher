#!/usr/bin/python3

import os
champ_path = os.chdir("..") 

suite = input('Enter the benchmark suite: ') 
print('\n')

file_path='sim_lists/'+suite+'/traces.txt'

if suite == 'spec':
	file_path='sim_lists/'+suite+'/comp_traces.txt'

sim_list = open(file_path)
for bench in sim_list:
	bench_name = bench[:-1]

	if "gap" in suite:
		output= open('output/'+suite+'/pc_count/'+bench_name+'.txt', 'r')
	else:
		output= open('output/spec06/pc_count/'+bench_name+'.txt', 'r')		
	
	iterator = iter(output)
	for line in iterator:
		if line.startswith('Number unique PC'):
			print(suite+', '+bench_name + ', ' + line.split()[3] )
			
			
	

