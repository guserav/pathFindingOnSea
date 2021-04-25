import matplotlib.pyplot as plt
import numpy as np
import statistics as st

def append(l, v):
    l.append(v)
    return l

dataSets = 4

file = open("stats/complexResultDetail.txt", "r")

width = 0.4
#colors = [(1,0,0), (0,1,0), (0,0,1), (0,1,1), (1,0,1), (1,1,0)]
colors = ['r', 'g', 'b', 'c', 'm', 'y']
phases = ["preparation", "point in polygon"]

names = {}
columns = {}
columnsAggr = {}
plt.figure(dpi=300)
for line in file:
    line = line.strip("\n ")
    lineSplit = line.split(";")
    stuff = lineSplit.pop(0).split("$")
    name = stuff[0]
    nodes = stuff[1]
    param = stuff[2]

    data = np.reshape(list(map(lambda x:float(x), lineSplit)), (len(phases),-1), order='F')

    column = list(map(lambda x:st.mean(x), data))
    columnAggr = list(sum(column[0:x:]) for x in range(len(column)))
    errorbar = list(map(lambda x: 0, data))

    total = sum(column)
    columnPercent = list((x/total) for x in column)
    columnPercentAggr = list(sum(columnPercent[0:x:]) for x in range(len(columnPercent)))

    columns[name] = append(columns.get(name, []), column)
    columnsAggr[name] = append(columnsAggr.get(name, []), columnAggr)
    names[name] = append(names.get(name, []), param)
#plt.show()
#print(str(columns))
rows = {k: np.swapaxes(v, 0, 1) for k, v in columns.items()}
rowsAggr = {k: np.swapaxes(v, 0, 1) for k, v in columnsAggr.items()}
#rowsErr = np.swapaxes(columnsError, 0, 1)
#rowsPercent = np.swapaxes(columnsPercent, 0, 1)
#rowsPercentAggr = np.swapaxes(columnsPercentAggr, 0, 1)

bars = []
phaseNames = []
indexes = {k: np.arange(len(names[k])) for k, v in names.items()}
c = 0
for k in rows:
    for i in range(len(rows[k])):
        bars.append(plt.bar(indexes[k] + width * c, rows[k][i], width, bottom=rowsAggr[k][i], color=colors[i + c * len(phases)]))
        phaseNames.append("{} - {}".format(k, phases[i]))
    c += 1
plt.ylabel('Execution Time in μs')
plt.xlabel('Break count')
m = 0
for k in names:
    if len(names[k]) > m:
        m = len(names[k])
        plt.xticks(indexes[k], names[k])
plt.legend(bars[::-1], phaseNames[::-1])
plt.savefig("test.pdf", dpi=600)
plt.show()

#bars = []
#indexes = np.arange(len(names))
#for i in range(len(rowsPercent)):
    #bars.append(plt.bar(indexes, rowsPercent[i], width, bottom=rowsPercentAggr[i], color=colors[i]))
#plt.xticks(indexes, names)
#plt.legend(bars[::-1], phases[::-1])
#plt.show()
