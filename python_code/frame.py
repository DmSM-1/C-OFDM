import matplotlib.pyplot as plt
import numpy as np
import os

# --- Генерация данных ---
# Выполняем скрипты для генерации данных один раз
os.system("./run.sh")
os.system("./main")

# --- Чтение данных ---
# Считываем OFDM frame
frame = np.fromfile('data/frame.bin', dtype=np.float64)
frame = frame[::2] + 1j * frame[1::2]

# Считываем созвездие IQ
constell = np.fromfile('data/constell.bin', dtype=np.float64)
constell = constell[::2] + 1j * constell[1::2]
# Нормализуем созвездие
constell /= np.mean(np.abs(constell))

# Считываем данные
data = np.fromfile('data/data.bin', dtype=np.float64)
data = data[::2] + 1j * data[1::2]   

# Считываем корреляцию
cor = np.fromfile('data/cor.bin', dtype=np.float64)

# --- Отображение графиков ---
# Создаем все subplots сразу
fig, (ax3, ax4, ax1, ax2) = plt.subplots(1, 4, figsize=(15, 5))

# Plot 1: OFDM Frame Magnitude
ax1.plot(np.abs(frame))
ax1.set_title("OFDM Frame Magnitude")
ax1.set_xlabel("Sample")
ax1.set_ylabel("Magnitude")
ax1.grid(True)

# Plot 2: Constellation IQ
ax2.scatter(constell.real, constell.imag, s=5, color='blue')
ax2.set_xlabel("I")
ax2.set_ylabel("Q")
ax2.set_xlim(-1.5, 1.5)  
ax2.set_ylim(-1.5, 1.5)
ax2.set_title("Constellation IQ")
ax2.grid(True)

# Plot 3: Data Magnitude
ax3.plot(np.abs(data))
ax3.set_title("Data Magnitude")
ax3.set_xlabel("Sample")
ax3.set_ylabel("Magnitude")
ax3.grid(True)

# Plot 4: Correlation
ax4.plot(cor)
ax4.set_title("Correlation")
ax4.set_xlabel("Sample")
ax4.set_ylabel("Value")
ax4.grid(True)

# Оптимизация расположения графиков
plt.tight_layout()

# Показываем все графики в одном окне
plt.show()