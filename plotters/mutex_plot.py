import os
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
from matplotlib.ticker import PercentFormatter
import numpy as np
import matplotlib
import matplotlib.gridspec as gridspec
from matplotlib.ticker import FuncFormatter
from pprint import pprint
matplotlib.rcParams.update({'font.size': 14})


if len(os.sys.argv) < 4:
    print("""
    Usage: python3 sample_window.py <type> <csv file/dir> <log yaxis> <event index>\
    """
    )
    os.sys.exit()

graph_type = os.sys.argv[1]
csv_file = os.sys.argv[2]
logax = int(os.sys.argv[3])

try:
    ev_index = int(os.sys.argv[4])
except IndexError:
    ev_index = None

bin_offset = 20
events = []
len_events = 0


def get_dataset(afile):
    global events, len_events
    all_data = {}
    f = open(afile)
    data = f.readlines()
    f.close()
    events = data[0].replace(',\n', '').replace('\n', '')
    events = events.split(',')
    len_events = len(events)

    print('dataset with string events')
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

def histogram(dataset):
    global logax, bin_offset, events, len_events

    if ev_index is not None:
        new_dset = {}
        new_dset[0] = dataset[ev_index]
        dataset = new_dset

    k = 0
    gs = gridspec.GridSpec(len(dataset), 1)
    axs = None
    for key, item in dataset.items():
        axs = plt.subplot(gs[k, 0], sharex=axs, sharey=axs)
        vmin = int(item['values'].min())
        vmax = int(item['values'].max())
        outliers_len = int(item['values'].size*0.1)
        outliers_arr = sorted(item['values'])[-outliers_len:]
        print('%d dataset len %s min %d max %d\noutliers: %s' % (item['values'].size, item['name'], vmin, vmax, outliers_arr))
        axs.hist(
            item['values'],
            range(vmin, vmax+bin_offset, bin_offset),
            log=False,
            label='%s: min %d max %d' % (item['name'], vmin, vmax),
            color='black')

        if logax: axs.set_yscale('log')
        axs.set_xlabel('Tempo (us)')
        axs.set_ylabel('Amostras')
        axs.legend()
        axs.grid()
        k+=1

def lines(dataset):
    global logax, events, len_events

    if ev_index is not None:
        new_dset = {}
        new_dset[0] = dataset[ev_index]
        dataset = new_dset

    k = 0
    gs = gridspec.GridSpec(len(dataset), 1)
    axs = None
    for key, item in dataset.items():
        axs = plt.subplot(gs[k, 0], sharex=axs)
        vmin = int(item['values'].min())
        vmax = int(item['values'].max())
        print('%s min %d max %d' % (item['name'], vmin, vmax))
        axs.vlines(
            np.arange(1, item['values'].size+1),
            [0],
            item['values'],
            label='%s: min %d max %d' % (item['name'], vmin, vmax),
            color='black')

        if logax: axs.set_yscale('log')
        axs.set_xlabel('Samples')
        axs.set_ylabel('Time (us)')
        axs.legend()
        axs.grid()
        k+=1

def normal():
    global tables_csv, M, N, graph_legend
    gs = gridspec.GridSpec(M, N)
    i = 0
    k = 1
    aux = None
    for key, item in tables_csv.items():
        if i == 1:
            main_axs = plt.subplot(gs[i, 0], sharey=aux)
        else:
            main_axs = plt.subplot(gs[i, 0])
        item['x'] = item['x']/1000000
        print(item)

        vmin = int(item['x'].min())
        vmax = int(item['x'].max())

        print("median:", np.median(item['x']))
        print(vmin, vmax)

        main_axs.plot(item['y'], item['x'],
            label="%s" % (graph_legend[k]))

        if int(logax):
            print('seting log sacle')
            main_axs.set_yscale('log')
        k -= 1

    ylabel = 'Time (s)'
    xlabel = 'Nr Processes'
    main_axs.set_ylabel(ylabel)
    main_axs.set_xlabel(xlabel)
    main_axs.legend()
    main_axs.grid()


if __name__ == "__main__":

    obj = os.sys.modules[__name__]
    if graph_type not in dir(obj):
        print("Graph {} not supported".format(graph_type))
        raise SystemExit

    dataset = get_dataset(csv_file)
    getattr(obj, graph_type)(dataset)
    # plt.subplots_adjust(left=0.07, bottom=0.07, right=0.95, top=0.95, wspace=0.20, hspace=0.20)
    plt.show()
