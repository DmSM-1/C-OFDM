import matplotlib.pyplot as plt
import numpy as np

data = np.fromfile('data.bin', dtype=np.float64)
complex_data = data[::2] + 1j * data[1::2] 


# plt.scatter(np.arange(complex_data.size),np.abs(complex_data))
plt.plot(np.abs(complex_data))
# plt.scatter(complex_data.real, complex_data.imag)
plt.show()