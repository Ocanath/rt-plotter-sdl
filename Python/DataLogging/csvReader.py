import csv
import numpy as np

def read_csv(filename, linechar):
	datas = []
	with open(filename, newline=linechar) as csvfile:
		spamreader = csv.reader(csvfile, delimiter=',')
		for row in spamreader:
			datas.append(row)
	return datas


def format_csv(csv_data):
	names = csv_data[0]
	num_rows = np.size(csv_data[1])	#first data entry,number of elements, pre-transpose
	csv_data = np.transpose(csv_data)	
	datas = []
	for row in range(0,num_rows):
		datas.append([])
		for col in range(1, len(csv_data[row])):
			floatval = float(csv_data[row][col])
			datas[row].append(floatval)
	return datas, names