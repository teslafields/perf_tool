import os
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
from matplotlib.ticker import PercentFormatter
import numpy as np
import matplotlib
import matplotlib.gridspec as gridspec
import matplotlib.style as style
from matplotlib.ticker import FuncFormatter
from pprint import pprint
matplotlib.rcParams.update({'font.size': 12})
style.use('ggplot')
matplotlib.rcParams.update({'figure.figsize': (6, 5)})
if len(os.sys.argv) < 3:
    print("""
    Usage: python3 boxplot.py <csv file/dir> <orientation>"""
    )
    os.sys.exit()

csv_file = os.sys.argv[1]
direction = os.sys.argv[2]


all_data = {}
f = open(csv_file)
data = f.readlines()
f.close()

if direction == 'x':
    events = data[0].replace('\n', '')
    events = events.split(';')
    len_events = len(events)
    print(events)
    for i in range(0, len_events):
        all_data.update({
            i: {
                'values': [],
                'name': events[i]
            }
        })

    for key in sorted(all_data.keys()):
        for k in range(1, len(data)):
            line_list = data[k].replace('\n', '')
            line_list = line_list.split(';')
            line_list = line_list[key]
            subitem = line_list.split(',')
            value = [int(subitem[0]), int(subitem[1])]
            all_data[key]['values'].append(value)

    gs = gridspec.GridSpec(len_events, 1)
    axs, k = None, 0
    for key, item in all_data.items():
        axs = plt.subplot(gs[k, 0], sharex=axs, sharey=axs)
        axs.boxplot(
            item['values'],
            widths=0.15)

        axs.set_ylabel('Time (us)')
        axs.set_title(item['name'])
        axs.legend()
        #axs.grid()
        k+=1

elif direction == 'y':
    events = data[0].replace('\n', '')
    events = events.split(',')
    len_events = len(events)
    print(events)
    for i in range(0, len_events):
        all_data.update({
            i: {
                'values': [],
                'name': events[i]
            }
        })
    for key in sorted(all_data.keys()):
        for k in range(1, len(data)):
            line_list = data[k].replace('\n', '')
            line_list = line_list.split(',')
            all_data[key]['values'].append(int(line_list[key]))

    gs = gridspec.GridSpec(1, 1)
    axs, k = None, 0
    axs = plt.subplot(gs[k, 0])
    dataset = []
    for key, item in all_data.items():
        dataset.append(item['values'])
    print(all_data[0]['values'] == all_data[1]['values'])
    axs.boxplot(dataset, widths=0.15)
    axs.set_ylabel('Tempo (us)')
    #axs.set_title(events)
    axs.legend()
    #axs.grid()
    plt.subplots_adjust(left=0.15, bottom=0.10, right=0.98, top=0.98)
axs.set_xlabel('Threads')
plt.show()
