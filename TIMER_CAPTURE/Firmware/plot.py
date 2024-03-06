# parse and plot the data in data.txt

import numpy as np
import matplotlib.pyplot as plt

file = open("data.txt", "r")
lines = file.readlines()
file.close()

min_dat = []
max_dat = []
x = np.arange(0, len(lines), 1)

for line in lines:
    split = line.split(' ')
    min_dat.append(float(split[0]))
    max_dat.append(float(split[1].split('\n')[0]))

    print("{}".format(max_dat[-1]))

# scatter plot
plt.figure()
plt.scatter(x, min_dat, label='min')
plt.scatter(x, max_dat, label='max')
plt.yscale('log')
plt.xlabel('Sample Number')
plt.ylabel('Latency (us)')
plt.title('Latency Measurements')
plt.legend()
plt.show()