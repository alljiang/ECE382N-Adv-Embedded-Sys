# parse and plot the data in data.txt

import matplotlib.pyplot as plt

file = open("data.txt", "r")
lines = file.readlines()
file.close()

min_dat = []
max_dat = []

for line in lines:
    split = line.split(' ')
    min_dat.append(split[0])
    max_dat.append(split[1])

# scatter plot
plt.figure()

