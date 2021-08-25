# Copyright (C) 2021 Niklas Jacob
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import re
import math

import matplotlib.pyplot as plt
import numpy as np

plot_colors = {
    'running' : 'b',
    'broken' : 'r',
    'success' : 'g',
}

def plot_histograms(title, x_label, category_dict, bin_count=None, colors=plot_colors, alpha=.5, yscale=None):

    total = sum(len(vs) for vs in category_dict.values())

    # calculate bins
    value_max = max(max(vs + [0]) for vs in category_dict.values())
    value_min = min(min(vs + [0xffffffff]) for vs in category_dict.values())

    bin_start = value_min
    bin_end = value_max
    bin_delta = 1

    if bin_count:
        bin_delta = math.ceil(float(value_max - value_min) / bin_count)

    bins = list(i - .5 for i in range(bin_start, bin_end+bin_delta, bin_delta))

    # plot histogramms
    fig = plt.figure()

    plt.title(title)
    plt.ylabel(f'categorial frequency (of {total} in total)')
    if yscale:
        plt.yscale(yscale)
    plt.xlabel(x_label)

    ax = fig.gca()

    for (name, values) in category_dict.items():
        color = plot_colors.get(name) # returns None if not present
        ax.hist(values, bins=bins, label=name, color=color, alpha=alpha)
    
    fig.legend()
    fig.show()


def plot_stacked_bars(title, x_label, names, values, bin_count=None, colors=plot_colors, alpha=.5, yscale=None):

    total = sum(len(vs) for vs in values)

    # calculate bins
    value_max = max(max(vs + [0]) for vs in values)
    value_min = min(min(vs + [0xffffffff]) for vs in values)

    bin_start = value_min
    bin_end = value_max
    bin_delta = 1

    if bin_count:
        bin_delta = max(1, math.ceil(float(value_max - value_min) / bin_count))

    locs = list(i + bin_delta/2.0 for i in range(bin_start, bin_end, bin_delta))
    bins = list(i for i in range(bin_start, bin_end+1, bin_delta))

    # get histogramms
    hists = [np.histogram(vs, bins=bins)[0] for vs in values]

    # plot bars
    fig = plt.figure()

    plt.title(title)
    plt.ylabel(f'categorial shares (of {total} in total)')
    if yscale:
        plt.yscale(yscale)
    plt.xlabel(x_label)

    ax = fig.gca()

    bottom = np.array([0.0]*len(locs))
    for (name, hs) in zip(names, hists):
        color = plot_colors.get(name) # returns None if not present
        ax.bar(locs, hs, width=bin_delta, bottom=bottom, label=name, color=color)
        bottom += hs
    
    fig.legend()
    fig.show()

def results_to_durations(results):
    return [r.duration for r in results]

def plot_durations_hist(result_dict, title='Durations', **kwargs):

    duration_dict = {}
    for (name, results) in result_dict.items():
        duration_dict[name] = results_to_durations(results)

    plot_histograms(title, 'glitch duration (60 ~ 1 us)', duration_dict, **kwargs)

def plot_durations_bar(names, results, title='Durations', **kwargs):

    values = [results_to_durations(results[name]) for name in names]

    plot_stacked_bars(title, 'glitch duration (60 ~ 1 us)', names, values, **kwargs)

def results_to_delays(results):
    return [r.delay for r in results]

def plot_delays_hist(result_dict, title='Delays', **kwargs):

    delay_dict = {}
    for (name, results) in result_dict.items():
        delay_dict[name] = results_to_delays(results)

    plot_histograms(title, 'glitch delay (60 ~ 1 us)', delay_dict, **kwargs)

def plot_delays_bar(names, results, title='Delays', **kwargs):

    values = [results_to_delays(results[name]) for name in names]

    plot_stacked_bars(title, 'glitch delay (60 ~ 1 us)', names, values, **kwargs)


