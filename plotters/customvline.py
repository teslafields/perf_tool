import datasetgetter as dg
import matplotlib.pyplot as plt
import numpy as np


data1 = dg.get_dataset('../results/comparison/fragmentation/random_calc.csv')
data2 = dg.get_dataset('../results/comparison/fragmentation/random_calc_lock.csv')

tmin1, tmax1 = data1[1]['npvalues'].min(), data1[1]['npvalues'].max()
tmin2, tmax2 = data2[1]['npvalues'].min(), data2[1]['npvalues'].max()

pmin1, pmax1 = data1[0]['npvalues'].min(), data1[0]['npvalues'].max()
pmin2, pmax2 = data2[0]['npvalues'].min(), data2[0]['npvalues'].max()

x = np.arange(1, data2[0]['npvalues'].size + 1)

fig, axs = plt.subplots(2, 1)
axs[1].vlines(x, [0], data1[0]['npvalues'], label='no lock min: %d max: %d' % (pmin1, pmax1))
axs[1].vlines(x, [0], data2[0]['npvalues'], label='mlock min: %d max: %d' % (pmin2, pmax2), color='r')

axs[0].vlines(x, [0], data1[1]['npvalues'], label='no lock min: %d max: %d' % (tmin1, tmax1))
axs[0].vlines(x, [0], data2[1]['npvalues'], label='mlock min: %d max: %d' % (tmin2, tmax2), color='r')

axs[0].grid()
axs[1].grid()

axs[0].legend()
axs[1].legend()

axs[0].set_ylabel('Tempo (us)')
axs[1].set_ylabel('Page Alloc')

plt.show()
