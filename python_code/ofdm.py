import numpy as np
import matplotlib.pyplot as plt

fig, axs = plt.subplots(2, 2, figsize=(12, 8))  # 2x2 графика
axs = axs.flatten()  # упрощаем индексирование через axs[0], axs[1], ...

# -------- data.bin --------
try:
    arr_data = np.fromfile('data/data.bin', dtype=np.float64)
    arr_data = arr_data[::2] + 1j * arr_data[1::2]
    axs[0].plot(np.abs(arr_data))
    axs[0].set_title('Data from SDR')
except Exception as e:
    print('Error loading data/data.bin:', e)

# -------- t2_sin_corr.bin --------
try:
    corr = np.fromfile('data/t2_sin_corr.bin', dtype=np.float64)
    x_corr = np.linspace(0, arr_data.size, corr.size)
    axs[1].plot(x_corr, corr)
    axs[1].set_title('T2 Sin Correlation')
except Exception as e:
    print('Error loading data/t2_sin_corr.bin:', e)

# -------- constell.bin --------
try:
    arr_const = np.fromfile('data/constell.bin', dtype=np.float64)
    arr_const = arr_const[::2] + 1j * arr_const[1::2]
    axs[2].scatter(arr_const.real, arr_const.imag, s=1)
    axs[2].set_xlim(-1.5, 1.5)
    axs[2].set_ylim(-1.5, 1.5)
    axs[2].set_aspect('equal', 'box')
    axs[2].set_title('Constellation')
except Exception as e:
    print('Error loading data/constell.bin:', e)

# -------- phases.bin --------
try:
    arr_phase = np.fromfile('data/phases.bin', dtype=np.float64)
    arr_phase = arr_phase[::2] + 1j * arr_phase[1::2]
    axs[3].plot(np.angle(arr_phase))
    # axs[3].set_title('Phases')
    axs[3].plot(np.abs(arr_phase))
except Exception as e:
    print('Error loading data/phases.bin:', e)

# Автоматическая расстановка графиков, чтобы подписи не накладывались
plt.tight_layout()

# Отображение графиков
plt.show(block=True)
# plt.pause(3)      # окно открыто 3 секунды
# plt.close(fig)    # закрытие окна
