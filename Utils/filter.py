import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib import gridspec
import numpy as np
import argparse
import Utils.utils as utils

import iir
import coefficients

mpl.rcParams['figure.figsize'] = [10.0, 6.0]
mpl.rcParams['figure.dpi'] = 100
mpl.rcParams['savefig.dpi'] = 100

parser = argparse.ArgumentParser(description='Фильтры')
parser.add_argument('--filter', '-f', default=0, type=str, help='Номера фильтров')
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
filter_indexes = list(map(lambda x: int(x), args.filter.split(',')))

print(f'Используем файл {csv_file}')
try:
    axisx, axisy, axisx_temperature, axisy_temperature = utils.Load(csv_file)
except Exception as ex:
    print(f'File `{csv_file}` not found: {ex}')
    parser.exit(0)

print(f'Количество записей: {len(axisx)}, {axisx.max()} секунд')

if end_time > 2**32:
    end_time = axisx[-1]

filters = coefficients.Load()

fig = plt.figure()
gs = gridspec.GridSpec(len(filter_indexes) + 1, 1)
ax0 = plt.subplot(gs[0])
ax0.set_ylim(axisy.min(), axisy.max())
ax0.set_xlim(axisx[0], axisx[-1])
ax0.plot(axisx, axisy, label='Current')
ax0.grid()
ax0.legend()

n = 1
for index in filter_indexes:
    try:
        params = filters[index]
    except:
        print(f'Фильтр с номером {index} не найден')
        index = 0
        params = filters[index]

    if index == 0:
        print('Работаем без фильтра')
        print('Существующие фильтры: ')
        for n in range(0, len(filters)):
            print(f' {n}) {filters[n]['description']}')
        
    lpf = iir.IIR(params['acoeff'], params['bcoeff'])

    axisy_lpf = np.zeros(len(axisx), np.float64)
    for i in range(0, len(axisx)):
        axisy_lpf[i] = lpf.process(axisy[i])

    axn = plt.subplot(gs[n], sharex=ax0)
    axn.set_xlim(begin_time, end_time)
    axn.plot(axisx, axisy_lpf, label=params['description'])
    axn.grid()
    axn.legend()
    n += 1

plt.subplots_adjust(hspace=0.3)
plt.show()
