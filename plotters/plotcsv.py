import os

if len(os.sys.argv) < 4:
    print("""
    Usage: python3 sample_window.py <type> <dir> <log yaxis> <legend> <indexes>
    """
    )
    os.sys.exit()

import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
from matplotlib.ticker import PercentFormatter
import numpy as np
import matplotlib
import matplotlib.gridspec as gridspec
import matplotlib.style as style
from matplotlib.ticker import FuncFormatter
from pprint import pprint
style.use('ggplot')
matplotlib.rcParams.update({'font.size': 12})
matplotlib.rcParams.update({'figure.figsize': (8, 9)})

# gtitle = "SIZE=65536 & mlockall + stacksize"
gtitle = "thread com mlockall          thread sem mlockall"

indexes = None
if len(os.sys.argv) == 6:
    indexes = os.sys.argv[5].split(',')
    indexes = [int(i) for i in indexes]
    print('Indexes: {}'.format(indexes))

bin_offset = 200
outlier_offset = 1000
graph_type = os.sys.argv[1]
csv_file = os.sys.argv[2]
logax = int(os.sys.argv[3])

graph_legend = os.sys.argv[4]
graph_legend = graph_legend.split(',')

events = []
len_events = 0
all_data = {}
axis_arr = []


def get_dataset(afile=None):
    global events, len_events, all_data, csv_file
    ofile = afile if afile else csv_file
    f = open(ofile)
    data = f.readlines()
    f.close()
    events = data[0].replace(',\n', '')
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
            line_list = data[k].replace(',\n', '')
            line_list = line_list.split(',')
            value = int(line_list[key])
            all_data[key]['values'].append(value)
        all_data[key]['values'] = np.array(all_data[key]['values'])

    return all_data.copy()

def set_graph_len(data):
    global indexes
    n = len(data) if not indexes else len(indexes)
    print('subplots: %d' % n)
    gs = gridspec.GridSpec(n, 1)
    return gs

def create_plot(plot_data, gs, **kwargs):
    global events, logax, axis_arr
    i, c, gid = 0, 0, -1
    if kwargs.get('column'):
        c = kwargs.get('column')
    if kwargs.get('line'):
        i = kwargs.get('line')
    axis = None
    for key, item in plot_data.items():
        gid += 1
        if indexes and gid not in indexes:
            continue
        dataset = item
        name = dataset['name']
        if name == 'KBytes':
            dataset['values'] = dataset['values']/1024
        # if 'end' in name or 'complete' in name or 'done' in name:
        #     continue
        vmin = int(dataset['values'].min())
        vmax = int(dataset['values'].max())

        print("{}\n{} {}".format("-"*50, dataset['name'], dataset['values'].size))
        print("median:", np.median(dataset['values']))
        print(vmin, vmax)

        xlabel = name
        ylabel = 'Amostras'
        try:
            comment = graph_legend[i]
        except IndexError:
            comment = ''
        legend = "%s min: %d max: %d" % (comment, vmin, vmax)
        if 'window' == graph_type:
            if c > 0:
                axis = plt.subplot(gs[i, c], sharey=axis_arr[i], sharex=axis)
            else:
                axis = plt.subplot(gs[i, c], sharex=axis)
                axis_arr.append(axis)
            xlabel = ylabel
            ylabel = name
            ylabel = 'Tempo (us)' if '0' == name else name
            x = np.arange(dataset['values'].size)
            x = x + 1
            axis.vlines(x, [0], dataset['values'], label=legend)
        elif 'histogram' == graph_type:
            axis = plt.subplot(gs[i, c]) #, sharex=axis, sharey=axis)
            axis.hist(dataset['values'], range(vmin, vmax+bin_offset, bin_offset), log=bool(logax), label=legend, color='black')
        elif 'outlier' == graph_type:
            ylabel = 'Outliers'
            xlabel = 'Espaçamento'

            outliers_arr = [item for item in dataset['values'] if item >= outlier_offset]
            outlier_gap = []
            bef = 0
            for m in range(dataset['values'].size):
                if dataset['values'][m] >= outlier_offset:
                    outlier_gap.append(m - bef)
                    bef = m
            outlier_gap.pop(outlier_gap.index(0))
            legend = "espaçamento min: %d max: %d" % (min(outlier_gap), max(outlier_gap))
            print(legend)
            axis = plt.subplot(gs[i, c], sharex=axis)
            axis.hist(outlier_gap, 200, log=bool(logax), label=legend, color='black')

        if c == 0:
            axis.set_ylabel(ylabel)
            if i == 0:
                pass # axis.annotate("thread com mlockall", xy=(0.5, 1.), xytext=(50, 200), va="bottom", ha="center")
                # plt.title(gtitle, loc='right', pad=10, x=2.0)
        else:
            if i == 0:
                pass # axis.annotate("thread sem mlockall", xy=(0.5, 1.), xytext=(50, 200), va="bottom", ha="center")
        glegend = axis.legend(loc='upper right')
        glegend.get_frame().set_facecolor('snow')
        #axis.set_ylim(top=1000-1)
        i += 1
    #plt.tight_layout()
    axis.set_xlabel(xlabel)

def create_dir_plot(table):
    n = len(table[0]) if not indexes else len(indexes)
    m = len(table)
    print('subplots: %d %d' % (n, m))
    gs = gridspec.GridSpec(n, m)
    for i in range(m):
        create_plot(table[i], gs, column=i)

if __name__ == "__main__":
    if '.csv' in csv_file:
        data = get_dataset()
        gs = set_graph_len(data)
        create_plot(plot_data=data, gs=gs)
    elif os.path.isdir(csv_file):
        files = np.sort(os.listdir(csv_file))
        tables_csv = {}
        for i in range(files.size):
            if '.csv' not in files[i]:
                print(files[i],"not csv file")
                continue
            cur = csv_file + '/' + files[i]
            tables_csv[i] = get_dataset(cur)
        create_dir_plot(tables_csv)
    else:
        print(csv_file, "not csv file")
        sys.exit()

    plt.subplots_adjust(left=0.10, bottom=0.08, right=0.98, top=0.95)
    plt.savefig(csv_file.replace('.csv', '.png'))
    plt.show()
