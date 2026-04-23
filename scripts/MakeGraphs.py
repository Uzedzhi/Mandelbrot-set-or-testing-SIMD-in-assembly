import csv
import os
import pandas as pd
import numpy as np
import math
import matplotlib.pyplot as plt

FOLDER_PATH = "MeasurementsNew"
dit = dict()

if not os.path.exists(FOLDER_PATH):
    print(f"Папка {FOLDER_PATH} не найдена!")
else:
    for filename in os.listdir(FOLDER_PATH):
        frames  = []
        ms      = []

        # reading values from file
        with open(FOLDER_PATH + "/" + filename, "r") as f:
            reader = csv.reader(f, delimiter=',')

            count = 0
            ms_mean = [0] * 10
            for row in reader:
                ms_mean[count] = float(row[1]) / 1e6
                count += 1
                if count == 10:
                    count = 0
                    frames.append(int(row[0]))
                    ms.append(np.median(np.array(ms_mean)))

        frames      = np.array(frames)
        ms          = np.array(ms)

        # calculating different statistics values
        MeanVal     = np.mean(ms)
        MedianVal   = np.median(ms)      
        Dispersion  = np.var(ms, ddof=0)
        StdDiff     = np.std(ms, ddof=0)
        P95Diff     = np.percentile(ms, 95)

        # first plot is (time spend on frame)(Frame Number)
        fig, axes   = plt.subplots(1, 2, figsize=(14, 5))
        axes[0].plot(frames, ms, marker="o", linestyle="-", linewidth=1, markersize=3)
        axes[0].set_title("Время на кадр")
        axes[0].set_xlabel("Номер кадра")
        axes[0].set_ylabel("Время, мс")
        axes[0].grid(True)

        # second plot is (num of frames)(milliseconds)
        axes[1].hist(ms, bins=30, edgecolor="black")
        dit[filename] = MedianVal
        text = f"среднее количество мс на кадр:\n        "                  \
            f"t = {MedianVal:.2f}мс ± {StdDiff:.2f}мс\n  "                  \
            f"Относ. Погрешность: σ = {(StdDiff / MeanVal * 100):.2f}%\n"   \
            f"В 95% случаев t ≤ {P95Diff:.2f}мс\n        "

        fig.suptitle(f"Распределения для версии {filename}", 
                    fontsize=16, fontweight='bold', y=0.98)
        axes[1].text(0.95, 0.95, text, 
                    transform=axes[1].transAxes, 
                    fontsize=8,
                    verticalalignment='top', 
                    horizontalalignment='right',
                    multialignment='center',   
                    fontweight='bold',            
                    bbox=dict(facecolor='white', edgecolor='gray'))
        axes[1].set_title("Распределение времени")
        axes[1].set_xlabel("Время, мс")
        axes[1].set_ylabel("Количество кадров")
        axes[1].grid(True)

        plt.tight_layout()
        plt.savefig("plotsNew/" + filename.split('.')[0] + ".png")

data_groups = {
    "IOMB": zip(*filter(lambda x:   x[0].startswith('IOMB'), dit.items())),
    "DIOMB": zip(*filter(lambda x:  x[0].startswith('DIOMB'), dit.items())),
    "OMB": zip(*filter(lambda x:    x[0].startswith('OMB'), dit.items())),
    "NOMB": zip(*filter(lambda x:   x[0].startswith('NOMB'), dit.items()))
}

for label, data in data_groups.items():
    keys, values = data
    
    plt.figure(figsize=(10, 6))
    plt.bar(keys, values, color='skyblue', edgecolor='navy')
    
    plt.ylabel('Миллисекунды')
    plt.xlabel('Программы')
    plt.title(f'Результаты {label}')
    plt.xticks(rotation=45) # Поворот названий, если они длинные
    
    plt.tight_layout()
    plt.savefig(f"plotsNew/MainHist{label.lower()}.png")
    plt.close() # Закрываем текущий график, чтобы начать новый

