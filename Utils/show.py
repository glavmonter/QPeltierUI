import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import argparse
import utils
from matplotlib import gridspec

def print_statistic(header: str, data: np.array, units: str) -> None:
    print(f'{header}')
    print(f'  Mean: {data.mean()} {units}')
    print(f'  Std: {data.std()} {units}')
    print(f'  Var: {data.var()} {units}')

def show_graph(axes: mpl.axes.Axes, title: str, data: tuple[np.array, np.array], limits: tuple[float, float]) -> None:
    axes.set_ylim(data[1].min(), data[1].max())
    axes.set_xlim(limits[0], limits[1])
    axes.set_ylabel(title)
    axes.plot(data[0], data[1])
    axes.grid()

mpl.rcParams['figure.figsize'] = [10.0, 6.0]
mpl.rcParams['figure.dpi'] = 100
mpl.rcParams['savefig.dpi'] = 100

parser = argparse.ArgumentParser(description='Отображатель логов')
parser.add_argument('-g', '--graph', type=str, nargs='?', default='a', help='Выводимый график: все (a), ток (c), температура (t)')
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

graph = args.graph
if graph not in ['a', 'c', 't']:
    print('Неизвестный параметр аргумента -g/--graph, используем "a"')
    graph = 'a'

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

grid_count = 2 if graph == 'a' else 1

fig = plt.figure()
gs = gridspec.GridSpec(grid_count, 1)

if graph == 'c':
    print_statistic('Ток:', axisy, 'A')
    show_graph(plt.subplot(gs[0]), 'Current, A', (axisx, axisy), (begin_time, end_time))
elif graph == 't':
    print_statistic('Температура:', axisy_temperature, 'C')
    show_graph(plt.subplot(gs[0]), 'Temperature, C', (axisx_temperature, axisy_temperature), (begin_time, end_time))
else:
    print_statistic('Ток:', axisy, 'A')
    print_statistic('Температура:', axisy_temperature, 'C')

    show_graph(plt.subplot(gs[0]), 'Current, A', (axisx, axisy), (begin_time, end_time))
    show_graph(plt.subplot(gs[1]), 'Temperature, C', (axisx_temperature, axisy_temperature), (begin_time, end_time))
    plt.subplots_adjust(hspace=0.15)

plt.xlabel('Time, seconds')
plt.show()
