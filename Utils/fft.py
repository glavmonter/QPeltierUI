import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import gridspec

import argparse
import csv_loader

mpl.rcParams['figure.figsize'] = [10.0, 6.0]
mpl.rcParams['figure.dpi'] = 100
mpl.rcParams['savefig.dpi'] = 100

parser = argparse.ArgumentParser(description='Отображатель FFT')
parser.add_argument('--frequency', '-f', default=1000, type=float, help='Частота сэмплирования')
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
    axisx, axisy, _, _ = csv_loader.Load(csv_file)
except:
    print(f'File `{csv_file}` not found')
    parser.exit(0)

print(f'Количество записей: {len(axisx)}, {axisx.max()} секунд')

begin_index = 0
end_index = len(axisx)

if begin_time > 0:
    begin_index = np.searchsorted(axisx, args.begin)

if end_time > 2**32:
    end_time = axisx[-1]

if end_time < 2**32:
    end_index = np.searchsorted(axisx, args.end)

axisx = axisx[begin_index:end_index]
axisy = axisy[begin_index:end_index]

print(f'Mean: {axisy.mean()}')
print(f'Std: {axisy.std()}')
print(f'var: {axisy.var()}')

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

# Plot the FFT results
plt.subplot(2, 1, 2)
plt.stem(frequencies[1:n // 2], amplitudes[1:n // 2])
plt.title('FFT of the Signal')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude')
plt.tight_layout()
plt.show()
