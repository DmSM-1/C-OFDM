import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import glob

fig, axs = plt.subplots(2, 1, figsize=(12, 8))

with open("data/stat.txt", 'r') as f:
    stat = f.readlines()

print(stat[0])

for i in stat:
    j = i.split(' ')
    for k in range(len(j)):
        j.
    # print(i.split(' '))