# parse and plot the data in data.txt

import matplotlib.pyplot as plt

file = open("data.txt", "r")
lines = file.readlines()
file.close()

data = []

for line in lines:
    data.append(line.split(' ')[-1])

print(data)

# scatter plot
plt.figure()
