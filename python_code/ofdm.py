import matplotlib.pyplot as plt
import numpy as np

# --- data.bin ---
arr = np.fromfile('data/data.bin', dtype=np.float64)
arr = arr[::2] + 1j * arr[1::2]
# arr /= np.max(np.abs(arr))
plt.plot(np.abs(arr))
plt.title('data from SDR')
plt.show()

# --- pr_corr.bin ---
corr = np.fromfile('data/pr_corr.bin', dtype=np.float64)
# corr /= np.max(corr)
plt.plot(np.arange(0, arr.size, arr.size / corr.size), corr)
plt.title('pr corr')
plt.show()

# --- constell.bin ---
arr = np.fromfile('data/constell.bin', dtype=np.float64)
arr = arr[::2] + 1j * arr[1::2]
plt.scatter(arr.real, arr.imag)
plt.xlim(-1.1, 1.1)
plt.ylim(-1.1, 1.1)
plt.axis('equal')
plt.show()

# --- phases.bin ---
arr = np.fromfile('data/phases.bin', dtype=np.float64)
arr = arr[::2] + 1j * arr[1::2]
plt.plot(np.angle(arr))
plt.show()

plt.plot(np.abs(arr))
plt.show()
