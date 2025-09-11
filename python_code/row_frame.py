import matplotlib.pyplot as plt
import numpy as np
import os

os.system("./run.sh")

plt.ion()  # включаем интерактивный режим
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 5))

for i in range(100):
    os.system("./main")
    
    # считываем OFDM frame
    frame = np.fromfile('data/frame.bin', dtype=np.float64)
    frame = frame[::2] + 1j * frame[1::2]

    # считываем созвездие IQ
    constell = np.fromfile('data/constell.bin', dtype=np.float64)
    constell = constell[::2] + 1j * constell[1::2]
    constell /= np.mean(np.abs(constell))

    print("\r" + ' '*20, end='')
    print(f"\r{i}", end='')

    # ---- Отображение ----
    ax1.cla()  # очищаем первый subplot
    ax1.plot(np.abs(frame))
    ax1.set_title("OFDM Frame Magnitude")

    ax2.cla()  # очищаем второй subplot
    ax2.scatter(constell.real, constell.imag, s=5, color='blue')
    ax2.set_xlabel("I")
    ax2.set_ylabel("Q")
    ax2.set_xlim(-1.5, 1.5)  
    ax2.set_ylim(-1.5, 1.5)
    ax2.set_title("Constellation IQ")
    ax2.grid(True)

    plt.pause(0.01)
else:
    plt.close()
