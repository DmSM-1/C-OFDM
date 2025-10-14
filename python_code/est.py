import numpy as np
import sys
import os
import subprocess
import matplotlib.pyplot as plt


files = subprocess.run(["ls", "frames"], capture_output=True, text=True).stdout
files = files.split('\n')
files.remove('')

tx = []
rx = []

for i in files:
    if 'tx' in i:
        tx.append(i)
    else:
        rx.append(i)


pairs = []

loss = 0

for tx_frame in tx:
    rx_frame = tx_frame.replace('tx', 'rx')
    if rx_frame not in rx: 
        loss += 1
    else:
        pairs.append([tx_frame, rx_frame])
    

BER = []
mean_err_dist = []
total_len = 0

for i in pairs:
    with open("frames/"+i[0], 'rb') as tx_file:
        tx_file_data = tx_file.read()
    with open("frames/"+i[1], 'rb') as rx_file:
        rx_file_data = rx_file.read()
    
    # err = 0
    err = []
    for i in range(len(rx_file_data)):
        for j in range(8):
            if ((tx_file_data[i]^rx_file_data[i])>>j)&1:
                err.append(8*i+j)
            # err.append(((tx_file_data[i]^rx_file_data[i])>>j)&1)
    err = np.array(err)
    err_dist = err[1:]-err[:-1] 
    mean_err_dist.extend(err_dist.tolist())
    mean_err_dist.extend(np.zeros(len(err), np.int32).tolist())

    BER.append(len(err)/len(rx_file_data*8))

mean_err_dist = np.array(mean_err_dist)
# print(mean_err_dist)
mean_BER = np.mean(BER)
norm_coef = mean_BER/(len(mean_err_dist)-np.count_nonzero(mean_err_dist))

unique_vals, counts = np.unique(mean_err_dist, return_counts=True)
norm_counts = counts * norm_coef

# Берем первые 100 уникальных значений
x_limit = min(100, len(unique_vals))
x_vals = unique_vals[:x_limit]
y_vals = norm_counts[:x_limit]

plt.figure(figsize=(14, 6))
plt.bar(x_vals, y_vals, width=0.8, edgecolor='black', alpha=0.7, color='skyblue')
plt.title('Вероятность двух ошибок последовательных от расстоянии')
plt.xlabel('Расстояние между ошибками')
plt.ylabel(f'Вероятность')
plt.grid(True, alpha=0.3)
plt.xticks(x_vals[::max(1, x_limit//20)])  # Разреженные метки для читаемости
plt.show()


print(mean_BER)
print()
    