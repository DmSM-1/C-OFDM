import matplotlib.pyplot as plt
import numpy as np
import os

PIPE_PATH = '/tmp/row_input'

if not os.path.exists(PIPE_PATH):
    raise FileNotFoundError(f"{PIPE_PATH} не найден")

# Настройка графика с заданным размером (12x8 дюймов)
plt.ion()
fig, ax = plt.subplots(figsize=(8, 8))
scatter = ax.scatter([], [], s=10, color='blue')
ax.set_xlabel('I (Real)')
ax.set_ylabel('Q (Imag)')
ax.set_title('Received Signal (Constellation)')
ax.set_xlim(-1.5, 1.5)
ax.set_ylim(-1.5, 1.5)
ax.grid(True)

CHUNK_SIZE = 1024 * 64  # читаем блоками по 64КБ

with open(PIPE_PATH, 'rb') as f:
    for i in range(1):  # цикл только 10 итераций
        raw = f.read(CHUNK_SIZE)
        if not raw:
            plt.pause(0.01)
            continue
        
        data = np.frombuffer(raw, dtype=np.float64)
        if len(data) < 2:
            continue
        # делаем длину чётной
        data = data[:len(data)//2*2]
        complex_data = data[::2] + 1j * data[1::2]

        # обновляем точки без очистки
        scatter.set_offsets(np.c_[complex_data.real, complex_data.imag])
        plt.pause(0.01)

# чтобы график не закрылся сразу после 10 итераций
plt.ioff()
plt.show()
