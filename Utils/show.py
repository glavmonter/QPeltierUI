import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import argparse
import utils
from matplotlib import gridspec

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
    axisx, axisy, axisx_temperature, axisy_temperature = utils.LoadCSV(csv_file)
except:
    print(f'File `{csv_file}` not found')
    parser.exit(0)

print(f'Количество записей: {len(axisx)}, {axisx.max()} секунд')

axisx, _bi, _ei = utils.Slice(axisx, args.begin, args.end)
axisy = axisy[_bi:_ei]
axisx_temperature, _bi, _ei = utils.Slice(axisx_temperature, args.begin, args.end)
axisy_temperature = axisy_temperature[_bi:_ei]
end_time = axisx[-1]

print('Current:')
print(f'  Mean: {axisy.mean()} A')
print(f'  Std: {axisy.std()} A')
print(f'  Var: {axisy.var()} A')

print('Temperature:')
print(f'  Mean: {axisy_temperature.mean()} K')
print(f'  Std: {axisy_temperature.std()} K')
print(f'  Var: {axisy_temperature.var()} K')

fig = plt.figure()
gs = gridspec.GridSpec(2, 1)
ax0 = plt.subplot(gs[0])
ax0.set_ylim(axisy.min(), axisy.max())
ax0.set_xlim(begin_time, end_time)
ax0.plot(axisx, axisy, label='Current')
ax0.grid()
ax0.legend()

ax1 = plt.subplot(gs[1])
ax1.set_xlim(begin_time, end_time)
ax1.plot(axisx_temperature, axisy_temperature, label='Temperature')
ax1.grid()
ax1.legend()

plt.subplots_adjust(hspace=0.1)
plt.show()
