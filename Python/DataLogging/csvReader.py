import csv

def read_csv(filename, linechar):
	datas = []
	with open(filename, newline=linechar) as csvfile:
		spamreader = csv.reader(csvfile, delimiter=',')
		for row in spamreader:
			datas.append(row)
	return datas

