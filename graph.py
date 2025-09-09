import matplotlib.pyplot as plt
import numpy as np
import os;

# data = np.fromfile('data.bin', dtype=np.float64)
# complex_data = data[::2] + 1j * data[1::2] 

# sin = np.fromfile('sin.bin', dtype=np.float64)
# sin_data = sin[::2] + 1j * sin[1::2] 



# plt.plot(np.arange(complex_data.size),np.abs(complex_data))
# # plt.plot(np.linspace(0,complex_data.size, sin_data.size),np.abs(sin_data)*100000, color='red')
# # plt.scatter(np.arange(complex_data.size),np.abs(complex_data), s = 9, color='red')
# # plt.plot(np.abs(np.fft.fftshift(np.fft.fft(complex_data))))
# plt.show()

# plt.plot(np.abs(np.fft.fftshift(np.fft.fft(complex_data))))
# plt.show()


data = np.fromfile('data.bin', dtype=np.float64)
complex_data = (data[::2] + 1j * data[1::2])

sin = np.fromfile('sin.bin', dtype=np.float64) 
sin *= max(np.abs(complex_data))/max(sin)

plt.plot(np.arange(complex_data.size),np.abs(complex_data))

step = 512
plt.plot(np.arange(0, sin.size*step, step),sin)
plt.scatter(np.arange(0, sin.size*step, step),sin)
    
plt.show()

start = int(np.argmin(sin)*complex_data.size/sin.size)
start = 7200
print(start)
spec = (np.fft.fft(complex_data[start:start+512]))
plt.plot(np.abs(spec))
plt.show()