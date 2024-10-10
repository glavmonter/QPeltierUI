import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib import gridspec

import argparse
import csv_loader

mpl.rcParams['figure.figsize'] = [10.0, 6.0]
mpl.rcParams['figure.dpi'] = 100
mpl.rcParams['savefig.dpi'] = 100

parser = argparse.ArgumentParser(description='Отображатель логов')
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
    axisx, axisy = csv_loader.Load(csv_file)
except:
    print(f'File `{csv_file}` not found')
    parser.exit(0)

print(f'Количество записей: {len(axisx)}, {axisx.max()} секунд')

if end_time > 2**32:
    end_time = axisx[-1]

print(f'Mean: {axisy.mean()}')
print(f'Std: {axisy.std()}')
print(f'var: {axisy.var()}')

fig = plt.figure()
gs = gridspec.GridSpec(2, 1)
ax0 = plt.subplot(gs[0])
ax0.set_ylim(axisy.min(), axisy.max())
ax0.set_xlim(axisx[0], axisx[-1])
ax0.plot(axisx, axisy, label='Current')
ax0.grid()
ax0.legend()

ax1 = plt.subplot(gs[1])
ax1.set_xlim(begin_time, end_time)
ax1.plot(axisx, axisy, label='Zoomed')
ax1.grid()
ax1.legend()

plt.subplots_adjust(hspace=0.1)
plt.show()
