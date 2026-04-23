import os
import numpy as np
import pandas as pd
import csv 

FOLDER_PATH = "MeasurementsNew"
output_file = "results/all_measurements_report.csv"
all_groups_used = dict({"NOMB": 0,
                       "OMB":   0,
                       "IOMB":  0,
                       "DIOMB": 0})

all_results = []
FirstGroupPrint = True

if not os.path.exists(FOLDER_PATH):
    print(f"Папка {FOLDER_PATH} не найдена!")
else:
    with open(output_file, 'w', newline='') as OutFile:
        OutWriter = csv.writer(OutFile)
        OutWriter.writerow([
                            "Группа",
                            "Имя замера",
                            "Медиана (мс)",
                            "Дисперсия",
                            "Станд. откл (мс)",
                            "P95 (мс)",
                            "Погрешность (%)"
                            ])
        for filename in os.listdir(FOLDER_PATH):
            file_path = FOLDER_PATH + "/" + filename
            group = filename.split('_')[0]
            IsGroupPrintedBefore = all_groups_used[group]

            if not IsGroupPrintedBefore and not FirstGroupPrint:
                OutWriter.writerow([""] * 7)
            FirstGroupPrint = False

            ms = []
            with open(file_path, newline='') as file:
                data = csv.reader(file)
                for row in data:
                    if len(row) == 2:
                        ms.append(int(row[1]) / 1e6)

            if not ms:
                print(f"Skipping empty or invalid file: {filename}")
                continue
            ms = np.array(ms)

            OutWriter.writerow([
                                group if not IsGroupPrintedBefore else "", filename,
                                round(np.median(ms), 2), round(np.var(ms, ddof=0), 2),
                                round(np.std(ms, ddof=0), 2), round(np.percentile(ms, 95), 2),
                                round(np.std(ms) / np.mean(ms) * 100, 2)
                                ])
            all_groups_used[group] = True