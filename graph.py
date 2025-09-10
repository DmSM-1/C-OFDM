import matplotlib.pyplot as plt
import numpy as np
import os;

c_data = np.fromfile('c_data.bin', dtype=np.float64)
c_data = c_data[::2] + 1j * c_data[1::2] 

sin = np.fromfile('sin.bin', dtype=np.float64) 
sin *= max(np.abs(c_data))/max(sin)


c_data = np.fromfile('c_data.bin', dtype=np.float64)
c_data = c_data[::2] + 1j * c_data[1::2] 
plt.plot(np.arange(c_data.size),np.abs(c_data))


# step = 256
# t2sin = np.fromfile('sin.bin', dtype=np.float64)
# c_data *= max(t2sin)/np.max(np.abs(c_data))
# plt.plot(np.arange(0, t2sin.size*step, step),np.abs(t2sin))
plt.show()