import numpy as np
import adi
import subprocess
import numpy as np
import matplotlib.pyplot as plt


def tx_connect():
    s = subprocess.run(["iio_attr", "-S"], capture_output=True, text=True).stdout.split('\n')
    sdr_usb = []
    for i in s:
        if "PlutoSDR" in i and "usb" in i:
            sdr_usb.append(i.split(' ')[-1][1:-1])

    return sdr_usb[0]


def rx_connect():
    s = subprocess.run(["iio_attr", "-S"], capture_output=True, text=True).stdout.split('\n')
    sdr_usb = []
    for i in s:
        if "PlutoSDR" in i and "usb" in i:
            sdr_usb.append(i.split(' ')[-1][1:-1])

    return sdr_usb[1]


def get_tx(Fc, Fs, bandwidth, buffer_size = 65536,  tx_hardwaregain_chan0 = 0, cycle = False):
    tx_sdr = adi.Pluto(tx_connect())
    tx_sdr.sample_rate = int(Fs)
    tx_sdr.tx_lo = int(Fc)
    tx_sdr.tx_rf_bandwidth = int(bandwidth)
    tx_sdr.tx_hardwaregain_chan0 = tx_hardwaregain_chan0
    tx_sdr.tx_cyclic_buffer = cycle

    return tx_sdr


def get_rx(Fc, Fs, bandwidth, buffer_size = 65536, gain_control_mode_chan0 = "manual", rx_hardwaregain_chan0 = 50.0):
    rx_sdr = adi.Pluto(rx_connect())
    rx_sdr.sample_rate = int(Fs)
    rx_sdr.rx_lo = int(Fc)
    rx_sdr.rx_rf_bandwidth = int(bandwidth)
    rx_sdr.rx_buffer_size = int(buffer_size)
    rx_sdr.gain_control_mode_chan0 = gain_control_mode_chan0
    rx_sdr.rx_hardwaregain_chan0 = rx_hardwaregain_chan0

    return rx_sdr



def read_complex_from_file(filename):
    raw = np.fromfile(filename, dtype=np.int16)
    
    if len(raw) % 2 != 0:
        raise ValueError("Файл повреждён: количество чисел не делится на 2")
    
    complex_data = raw[0::2] + 1j * raw[1::2]
    return complex_data




input_data = read_complex_from_file("data/tx.bin")
input_data = np.hstack((np.zeros_like(input_data), input_data, np.zeros_like(input_data)))
output_data = np.zeros(input_data.size*10, dtype = input_data.dtype)


tx_sdr = get_tx(2.8e9, 5e6, 20e6, input_data.size, cycle=True)
rx_sdr = get_rx(2.8e9, 5e6, 20e6, output_data.size, rx_hardwaregain_chan0=60)


tx_sdr.tx(input_data)
output_data = rx_sdr.rx()

np.save("data/rx.bin" ,output_data)






