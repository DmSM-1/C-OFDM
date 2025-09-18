import numpy as np
import matplotlib.pyplot as plt
import sys
import math

byte_prob = []
bit_prob = []

with open(sys.argv[1], 'r') as f:
    for i in f:
        s = i.split(' ')
        byte_prob.append(float(s[1]))
        bit_prob.append(float(s[4]))

byte_prob = np.array(byte_prob)
bit_prob = np.array(bit_prob)


bit_prob = np.sort(bit_prob)


print(byte_prob.mean(), bit_prob.mean())

plt.plot(np.linspace(0, 1, bit_prob.size), bit_prob)
plt.show()

p_err = 1 - bit_prob[int(bit_prob.size*0.2)]

code_len = 5
max_error = code_len//2
p_suc = (1- p_err)**code_len
for i in range(1, max_error+1):
    p_suc += math.factorial(code_len)/math.factorial(code_len-i)/math.factorial(i)*p_err**i*(1-p_err)**(code_len-i)

num_symbols = 1024

print(p_suc**num_symbols)
