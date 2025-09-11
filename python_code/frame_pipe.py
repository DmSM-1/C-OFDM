import numpy as np
import matplotlib.pyplot as plt
import os
import errno

plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 6))

fifo_frame = "/tmp/fifo_frame"
fifo_constell = "/tmp/fifo_constell"

# создаём FIFO только если не существует
for fifo in [fifo_frame, fifo_constell]:
    try:
        os.mkfifo(fifo)
    except FileExistsError:
        pass

# запускаем C++ скрипт, который пишет в pipe
os.system("./run.sh &")  # запускаем в фоне

for i in range(100):
    frame = None
    constell = None

    # читаем бинарные данные из pipe frame
    try:
        with open(fifo_frame, "rb", buffering=0) as f:
            data = f.read()
            if data:
                frame = np.frombuffer(data, dtype=np.float64)
                frame = frame[::2] + 1j*frame[1::2]
    except OSError as e:
        if e.errno != errno.EAGAIN:
            raise

    # читаем бинарные данные из pipe constell
    try:
        with open(fifo_constell, "rb", buffering=0) as f:
            data = f.read()
            if data:
                constell = np.frombuffer(data, dtype=np.float64)
                constell = constell[::2] + 1j*constell[1::2]
    except OSError as e:
        if e.errno != errno.EAGAIN:
            raise

    # вывод прогресса
    print(f"\r{i}", end='')

    # ---- визуализация ----
    if frame is not None:
        ax1.cla()
        ax1.plot(np.abs(frame))
        ax1.set_title("OFDM Frame Magnitude")

    if constell is not None:
        ax2.cla()
        ax2.scatter(constell.real, constell.imag, s=5, color='blue')
        ax2.set_xlabel("I")
        ax2.set_ylabel("Q")
        ax2.set_title("Constellation IQ")
        ax2.grid(True)

    plt.pause(0.01)

plt.close()

# удаляем FIFO после работы
os.remove(fifo_frame)
os.remove(fifo_constell)
