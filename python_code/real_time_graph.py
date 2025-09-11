import matplotlib.pyplot as plt
import numpy as np
import os

PIPE_PATH = '/tmp/row_input'

if not os.path.exists(PIPE_PATH):
    raise FileNotFoundError(f"{PIPE_PATH} не найден")

# Настраиваем график
plt.ion()  # интерактивный режим
fig, ax = plt.subplots()
line, = ax.plot([], [])
ax.set_xlabel('Sample Index')
ax.set_ylabel('Amplitude')
ax.set_title('Received Signal')

# Фиксированные лимиты
# ax.set_xlim(0, 18000)   # например, 2048 сэмплов на кадр
# ax.set_ylim(0, 2048)  # диапазон амплитуд (подстрой под свои данные)

# Открываем pipe для чтения
with open(PIPE_PATH, 'rb') as f:
    for j in range(10000):
        raw = f.read()  # читаем доступные данные
        if not raw:
            continue  # ждём данные
        
        data = np.frombuffer(raw, dtype=np.int16)
        if len(data) < 2:
            continue
        
        complex_data = data[::2] + 1j * data[1::2]

        # Обновляем график
        line.set_data(np.arange(complex_data.size), np.abs(complex_data))

        plt.pause(0.01)
