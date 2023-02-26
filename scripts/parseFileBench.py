import sys
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

#Class to hold per operation statistics
class OperationData:
	ops = []
	opRate = []
	bandwidth = []
	avgLatency = []
	minLatency = []
	maxLatency = []

	def __init__(self):
		self.ops = np.array([])
		self.opRate = []
		self.bandwidth = []
		self.avgLatency = []
		self.minLatency = []
		self.maxLatency = []

#Class to hold summary results printed at end of each test
class TestSummaryData:
	ops = []
	opRate = []
	readOps = []
	writeOps = []
	bandwidth = []
	avgLatency = []

	def __init__(self):
		self.ops = np.array([])
		self.opRate = []
		self.readOps = []
		self.writeOps = []
		self.bandwidth = []
		self.avgLatency = []


#Key value to use in results dict
SummaryLabel = "OPERATION_SUMMARY"

#Retrieve test results from given file
def extractData(f):
	data = {}

	testName = ""
	state = 0

	for l in f.readlines():

		splt = l.split()

		#At start of new entry
		if splt[0] == "NEXT_ENTRY":
			state = 1
			continue

		#Record config file name
		if state == 1:
			if splt[0] not in data.keys():
				data[splt[0]] = {}
			testName = splt[0]
			state = 2
			continue

		#Ignore lines until we hit start of operation breakdown
		elif state == 2:
			if len(splt) > 1 and splt[1] == "Per-Operation":
				state = 3
			continue

		#Parse operation details
		elif state == 3:
			if splt[1] == "IO":
				state = 0

				if SummaryLabel not in data[testName].keys():
					data[testName][SummaryLabel] = TestSummaryData()

				data[testName][SummaryLabel].ops = np.append(
					data[testName][SummaryLabel].ops, (int(splt[3])))
				data[testName][SummaryLabel].opRate = np.append(
					data[testName][SummaryLabel].opRate, (float(splt[5])))
				data[testName][SummaryLabel].readOps = np.append(
					data[testName][SummaryLabel].readOps, (int(splt[7].split('/')[0])))
				data[testName][SummaryLabel].writeOps = np.append(
					data[testName][SummaryLabel].writeOps, (int(splt[7].split('/')[1])))
				data[testName][SummaryLabel].bandwidth = np.append(
					data[testName][SummaryLabel].bandwidth, (float(splt[9].replace("mb/s", ""))))
				data[testName][SummaryLabel].avgLatency = np.append(
					data[testName][SummaryLabel].avgLatency, (float(splt[10].replace("ms/op", ""))))

				continue

			#Allocate list for OperationData if not present
			if splt[0] not in data[testName].keys():
				data[testName][splt[0]] = OperationData()

			#Extract operation details
			data[testName][splt[0]].ops = np.append(
				data[testName][splt[0]].ops, (int(splt[1].replace("ops", ""))))
			data[testName][splt[0]].opRate = np.append(
				data[testName][splt[0]].opRate, (int(splt[2].replace("ops/s", ""))))
			data[testName][splt[0]].bandwidth = np.append(
				data[testName][splt[0]].bandwidth, (float(splt[3].replace("mb/s", ""))))
			data[testName][splt[0]].avgLatency = np.append(
				data[testName][splt[0]].avgLatency, (float(splt[4].replace("ms/op", ""))))
			data[testName][splt[0]].minLatency = np.append(
				data[testName][splt[0]].minLatency, (float((splt[5].replace("[", "")).replace("ms", ""))))
			data[testName][splt[0]].maxLatency = np.append(
				data[testName][splt[0]].maxLatency, (float(splt[7].replace("ms]", ""))))

	return data

def LatencyGraph(df):
	font = {'family' : 'normal',
        'weight' : 'normal',
        'size'   : 14}
	plt.rc('font', **font)

	df = df.sort_values('avgLatency')
	plt.barh('labels', 'avgLatency', data=df, align='center', alpha=0.5)
	plt.ylabel("Workload")
	plt.xlabel("Average Operation Latency (ms)")
	plt.title("FileBench: Average Operation Latency")
	plt.show()

def BandwidthGraph(df):
	font = {'family' : 'normal',
        'weight' : 'normal',
        'size'   : 14}
	plt.rc('font', **font)

	df = df.sort_values('bandwidth')
	plt.barh('labels', 'bandwidth', data=df, align='center', alpha=0.5)
	plt.ylabel("Workload")
	plt.xlabel("Bandwidth (B/s)")
	plt.title("FileBench: Bandwidth")
	plt.xscale('log', base=2)
	plt.show()


def main(inFile):

	data = {}

	#Fill dict with test results
	with open(inFile) as f:
		data = extractData(f)

	SummaryAvgLatency = []
	SummaryAvgBW = []

	#Formatting output table
	gridFormat = "{:30}{:20}{:20}{:20}{:20}{:20}{:20}"
	gridFormatEntry = "{:30}{:<20.2f}{:<20.2f}{:<20.2f}{:<20.2f}{:<20.2f}{:<20.2f}"
	labels = gridFormat.format(
		"Operation", "ops", "ops/s", "mb/s", "ms/op", "Min ms", "Max ms")

	#Per operation results
	print("==========Operation BreakDown==========")
	for k1 in data.keys():
		print(k1, "----------------------")
		print(labels)
		for k2 in data[k1].keys():
			entry = data[k1][k2]
			if k2 != SummaryLabel:
				print(gridFormatEntry.format(k2, entry.ops.mean(), entry.opRate.mean(), entry.bandwidth.mean(
				), entry.avgLatency.mean(), entry.minLatency.min(), entry.maxLatency.max()))

	#Test summary results
	print("==========Test Summary==========")
	summaryColLabels = gridFormat.format(
		"Test", "ops", "ops/s", "rd", "wr", "mb/s", "ms/op")
	print(summaryColLabels)
	for k1 in data.keys():
		for k2 in data[k1].keys():
			if k2 != SummaryLabel:
				continue
			entry = data[k1][k2]
			print(gridFormatEntry.format(k1, entry.ops.mean(), entry.opRate.mean(), entry.readOps.mean(
			), entry.writeOps.mean(), entry.bandwidth.mean(), entry.avgLatency.mean()))
			SummaryAvgLatency.append(entry.avgLatency.mean())
			SummaryAvgBW.append(entry.bandwidth.mean() * (2**20))

	y_pos = np.arange(len(data.keys()))
	print(y_pos)
	print(SummaryAvgLatency)

	df = pd.DataFrame(
		dict(
			labels=data.keys(),
			avgLatency=SummaryAvgLatency,
			bandwidth=SummaryAvgBW
		)
	)

	LatencyGraph(df)
	BandwidthGraph(df)



if __name__ == "__main__":
	main(sys.argv[1])
