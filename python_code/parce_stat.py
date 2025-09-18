import numpy as np
import matplotlib.pyplot as plt
import sys

byte_prob = []
bit_prob = []

with open(sys.argv[1], 'r') as f:
    s = f.readline().split(' ')
    byte_prob.append(float(s[1]))
    bit_prob.append(float(s[4]))

byte_prob = np.array(byte_prob)
bit_prob = np.array(bit_prob)



print(byte_prob.mean(), bit_prob.mean())
    