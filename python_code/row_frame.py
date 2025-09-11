import matplotlib.pyplot as plt
import numpy as np
import os;

os.system("./run.sh")


for i in range(100):
    os.system("./main")
    frame = np.fromfile('data/frame.bin', dtype=np.float64)
    frame = frame[::2] + 1j * frame[1::2]

    print("\r"+' '*20, end='')
    print(f"\r{i}", end='')


    plt.clf()
    plt.plot(np.abs(frame))

    plt.pause(0.01)
else:
    plt.close()

    