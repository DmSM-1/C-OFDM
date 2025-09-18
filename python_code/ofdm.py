import numpy as np
import matplotlib.pyplot as plt

def main():
    # -------- data.bin --------
    # try:
    #     arr = np.fromfile('data/data.bin', dtype=np.float64)
    #     arr = arr[::2] + 1j * arr[1::2]
    #     plt.plot(np.abs(arr))
    #     plt.title('Data from SDR')
    #     plt.show()
    # except Exception as e:
    #     print('Error loading data/data.bin:', e)

    # # -------- t2_sin_corr.bin --------
    # try:
    #     corr = np.fromfile('data/t2_sin_corr.bin', dtype=np.float64)
    #     plt.plot(np.arange(0, arr.size, arr.size / corr.size), corr)
    #     plt.title('T2 Sin Correlation')
    #     plt.show()
    # except Exception as e:
    #     print('Error loading data/t2_sin_corr.bin:', e)

    # -------- constell.bin --------
    try:
        arr = np.fromfile('data/constell.bin', dtype=np.float64)
        arr = arr[::2] + 1j * arr[1::2]
        plt.scatter(arr.real, arr.imag)
        plt.xlim(-1.1, 1.1)
        plt.ylim(-1.1, 1.1)
        plt.axis('equal')
        plt.title('Constellation')
        plt.show()
    except Exception as e:
        print('Error loading data/constell.bin:', e)

#     # -------- phases.bin --------
#     try:
#         arr = np.fromfile('data/phases.bin', dtype=np.float64)
#         arr = arr[::2] + 1j * arr[1::2]
#         plt.plot(np.angle(arr))
#         plt.title('Phases')
#         plt.show()
#     except Exception as e:
#         print('Error loading data/phases.bin:', e)

if __name__ == '__main__':
    main()
