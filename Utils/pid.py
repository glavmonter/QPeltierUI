import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import gridspec

import argparse
import utils

mpl.rcParams['figure.figsize'] = [10.0, 6.0]
mpl.rcParams['figure.dpi'] = 100
mpl.rcParams['savefig.dpi'] = 100

parser = argparse.ArgumentParser(description='Отображатель FFT')
parser.add_argument('--frequency', '-f', default=50, type=float, help='Частота сэмплирования')
parser.add_argument('file', type=str, nargs='?', default='', help='CSV файл с записями')
parser.add_argument('begin', type=float, nargs='?', default=0, help='Начальное время отображения, секунд')
parser.add_argument('end', type=float, nargs='?', default=2**32 + 1, help='Конечное время отображения, секунд')

args = parser.parse_args()
csv_file = args.file
if csv_file == '':
    print('Файл должен быть задан')
    parser.print_usage()
    parser.exit(0)

begin_time = args.begin
end_time = args.end

print(f'Используем файл {csv_file}')
try:
    _, _, axisx, axisy = utils.LoadCSV(csv_file)
except:
    print(f'File `{csv_file}` not found')
    parser.exit(0)

print(f'Количество записей: {len(axisx)}, {axisx.max()} секунд')

begin_index = 0
end_index = len(axisx)

axisx, _bi, _ei = utils.Slice(axisx, args.begin, args.end)
axisy = axisy[_bi:_ei]
end_time = axisx[-1]

print('Temperature:')
print(f'  Mean: {axisy.mean()} C')
print(f'  Std: {axisy.std()} K')
print(f'  Var: {axisy.var()} K^2')

n = len(axisx)
frequencies = np.fft.fftfreq(n, d=1/args.frequency)
fft_values = np.fft.fft(axisy)
amplitudes = np.abs(fft_values)/n

plt.figure(figsize=(12, 6))
plt.subplot(2, 1, 1)
plt.plot(axisx, axisy)
plt.title('Original Signal')
plt.xlabel('Time (s)')
plt.ylabel('Amplitude')
plt.grid(True)

amplitudes = amplitudes[1:n // 2]
frequencies = frequencies[1:n // 2]

max_index = np.argmax(amplitudes)
freq = frequencies[max_index]
period = 1 / freq
print(f'Oscilation frequency {freq:.4f} Hz, {period:.4f} seconds')

ampl_end = args.frequency * period * 2.5
ampl_end = max(ampl_end, len(axisy))

amplitudes_sliced = axisy[0:ampl_end]
amp = amplitudes_sliced.max() - amplitudes_sliced.min()
print(f'Maximum: {amplitudes_sliced.max()}')
print(f'Minimum: {amplitudes_sliced.min()}')
print(f'Amplitude: {amp:.3f}')

# Plot the FFT results
plt.subplot(2, 1, 2)
plt.stem(frequencies, amplitudes)
plt.title('FFT of the Signal')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude')
plt.tight_layout()
plt.grid(True)
plt.show()
