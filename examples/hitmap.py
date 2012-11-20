import sys
import numpy as np
import matplotlib
matplotlib.use("Agg")
from matplotlib import pyplot as pl
from matplotlib.backends.backend_pdf import PdfPages

def save_all(basename,pdf=True,png=True,close=True):
    """Save all figures"""
    if pdf: pp = PdfPages(basename+".pdf")
    for i in pl.get_fignums():
        fig = pl.figure(i)
        if pdf: pp.savefig(fig)
        if png:
            fig.patch.set_alpha(0.0)
            fig.savefig(basename+"-%02d.png" % i)
    if pdf: pp.close()
    if close: pl.close("all")

for filename in sys.argv[1:]:
    datafile = open(filename)
    line = datafile.readline()
    cols,rows = map(int,line.split())
    data = np.fromfile(datafile, count=cols*rows, sep=" ")
    data.shape = (cols,rows)
    data = np.ma.masked_less(data,0)

    fig = pl.figure()
    ax = fig.add_subplot(111)
    img = ax.imshow(data.T,interpolation="nearest",origin="lower", aspect="auto", vmin=0)
    ax.set_title(filename)
    ax.set_xlabel("column")
    ax.set_ylabel("row")
    print "Average ADC count:", filename, data.mean()
    print "Total ADC sum:", filename, data.sum()
    fig.colorbar(img)

save_all("hitmap",png=False)
#pl.show()
