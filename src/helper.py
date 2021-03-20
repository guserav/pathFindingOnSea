#!/usr/bin/env python
import argparse
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def plotHistogram(data, out):
    # Use non-equal bin sizes, such that they look equal on log scale.
    logbins = np.logspace(np.log10(data[0]),np.log10(data[-1]),100)
    plt.hist(data, bins=logbins)
    plt.xscale('log')
    plt.yscale('log')
    plt.savefig(out)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('input', type=str, help='the input file')
    parser.add_argument('out', type=str, help='the output file')
    args = parser.parse_args()
    data = []
    with open(args.input, 'r') as readIn:
        for line in readIn:
            try:
                data.append(int(line.strip()))
            except Exception:
                pass
    plotHistogram(data, args.out)

if __name__ == "__main__":
    main()
