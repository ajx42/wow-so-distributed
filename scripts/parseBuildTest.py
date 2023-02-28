import sys
import pandas as pd
import matplotlib.pyplot as plt

def RunTimeGraph(df):
	font = {'family' : 'normal',
        'weight' : 'normal',
        'size'   : 14}
	plt.rc('font', **font)
	plt.bar('jobcount', 'time', data=df, align='center', alpha=0.5)
	plt.xlabel("Thread Count")
	plt.ylabel("Build Time (s)")
	plt.title("leveldb: Parallel Build Time")
	plt.show()

def get_sec(time_str):
    """Get seconds from time."""
    m, s = time_str.split(':')
    return int(m) * 60 + float(s)

def extractData(f):
	jobCountList = []
	timeList = []
	
	state = 0

	for l in f.readlines():

		if "Command being timed" in l:
			JobCount = int(l.split()[-1].replace('"', ''))
			jobCountList.append(JobCount)
		
		if "wall clock" in l:
			timeList.append(get_sec(l.split()[-1]))

	return pd.DataFrame(
		dict(
			jobcount = jobCountList,
			time = timeList
		)
	)

def main(inFile):

	data = {}

	#Fill dict with test results
	with open(inFile) as f:
		data = extractData(f)

	print(data)

	RunTimeGraph(data)

if __name__ == "__main__":
	main(sys.argv[1])