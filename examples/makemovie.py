import sys
import os
import numpy as np
import subprocess
import matplotlib
matplotlib.use("Agg")
from matplotlib import pyplot as pl

datafile = open(sys.argv[1])
basename = os.path.splitext(sys.argv[1])[0]

events = []
maxADC = 0
maxSUM = 0
while True:
    line = datafile.readline()
    if len(line)==0:
        break
    dummy, run, event, nmodules = line.split()
    run, event = map(int, (run, event))
    for i in range(int(nmodules)):
        tmp, modulenr, cols, rows = datafile.readline().split()
        cols = int(cols)
        rows = int(rows)
        data = np.fromfile(datafile, count=cols*rows, sep=" ")
        data.shape = (rows, cols)
        data = np.ma.masked_less(data, 0)
        events.append((run, event, data))
        maxADC = max(data.max(), maxADC)
        maxSUM = max(data.sum(), maxSUM)

print len(events), "Events read, max ADC value is", maxADC,
print "with at most", maxSUM, "total in one frame"

subprocess.call(["mkdir", "-p", basename])
cmap = matplotlib.cm.get_cmap("binary")
totalframe = None
if len(events)>0:
    totalframe = np.zeros(data.shape)


def save_frame(i, data, title):
    fig = pl.figure(figsize=(15, 6))
    ax = fig.add_axes((0.05, 0.1, 1.0, 0.85))
    img = ax.imshow(data.T, interpolation="nearest", origin="lower",
                    aspect="auto", vmin=0, vmax=maxADC, cmap=cmap)
    ax.set_xlabel("column")
    ax.set_ylabel("row")
    ax.set_xlim(0, data.shape[0])
    ax.set_ylim(0, data.shape[1])
    ax.set_title(title)
    fig.colorbar(img, fraction=0.13, pad=0.01)
    fig.savefig(basename+"/%04d.png" % i, dpi=90)
    pl.close(fig)

for i, (run, event, data) in enumerate(events):
    print "saving frame ", i
    totalframe += data
    save_frame(i+1, data, "Run %d, event %d" % (run, event))

save_frame(0, totalframe, "All Frames")

subprocess.call(["ffmpeg", "-y", "-r", "10", "-qscale", "3",
                 "-i", basename+"/%04d.png", basename+".mp4"])
