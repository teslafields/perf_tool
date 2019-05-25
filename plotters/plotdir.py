import os
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
from matplotlib.ticker import PercentFormatter
import numpy as np
import matplotlib
import matplotlib.gridspec as gridspec
from matplotlib.ticker import FuncFormatter
from pprint import pprint
matplotlib.rcParams.update({'font.size': 16})


if len(os.sys.argv) < 4:
    print("""
    Usage: python3 sample_window.py <type> <dir> <log yaxis> <legend>
    """
    )
    os.sys.exit()


graph_type = os.sys.argv[1]
csv_dir = os.sys.argv[2]
logax = int(os.sys.argv[3])

graph_legend = os.sys.argv[4]
graph_legend = graph_legend.split(',')

events = []
len_events = 0
tables_csv = {}


def get_dataset(afile):
    global events, len_events
    all_data = {}
    f = open(afile)
    data = f.readlines()
    f.close()
    events = data[0].replace(',\n', '')
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
            line_list = data[k].replace(',\n', '')
            line_list = line_list.split(',')
            value = int(line_list[key])
            all_data[key]['values'].append(value)
        all_data[key]['values'] = np.array(all_data[key]['values'])

    return all_data


def formatter(x, pos):
    return '%d' % int(x)


def create_plot(**kwargs):
    global tables_csv, events, logax
    n = len(tables_csv)
    # form = FuncFormatter(formatter)
    gs = gridspec.GridSpec(n, 1)
    i = 0
    axis = None
    for key, item in tables_csv.items():
        dataset = item[0]

        vmin = int(dataset['values'].min())
        vmax = int(dataset['values'].max())

        print("{}\n{}".format("-"*50, dataset['name']))
        print("median:", np.median(dataset['values']))
        print(vmin, vmax)

        xlabel = dataset['name']
        ylabel = 'Samples'
        legend = " %s min: %d max: %d" % (graph_legend[i], vmin, vmax)
        if 'window' == graph_type:
            axis = plt.subplot(gs[i, 0], sharey=axis)
            aux = ylabel
            ylabel = xlabel
            xlabel = aux
            x = np.arange(dataset['values'].size)
            x = x + 1
            axis.vlines(x, [0], dataset['values'], label=legend)
        elif 'histogram' == graph_type:
            axis = plt.subplot(gs[i, 0], sharex=axis)
            axis.hist(dataset['values'], 200, log=bool(logax), label=legend, color='black')

        axis.set_ylabel(ylabel)
        axis.set_xlabel(xlabel)
        axis.legend()
        axis.grid()
        i += 1
    #plt.tight_layout()
    # plt.subplots_adjust(left=0.07, bottom=0.07, right=0.95, top=0.95, wspace=0.20, hspace=0.20)
    plt.show()


if __name__ == "__main__":
    if not os.path.isdir(csv_dir):
        print("invalid path")
    else:
        files = np.sort(os.listdir(csv_dir))
        print(files)
        for i in range(files.size):
            if '.csv' not in files[i]:
                print(files[i],"not csv file")
                continue
            cur = csv_dir + '/' + files[i]
            tables_csv[i] = get_dataset(cur)
        create_plot()
