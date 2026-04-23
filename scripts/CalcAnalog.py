import os
import numpy as np
import pandas as pd
import csv 

Temp = []
Chast = []
Throttle = []
with open("results/analog.csv", "r", encoding="utf-8", newline='') as anal:
    CsvRead = list(csv.reader(anal));

    IndexTemp       = CsvRead[0].index("Температуры ядер (avg) [°C]")
    IndexChast      = CsvRead[0].index("Частоты ядер (avg) [МГц]")
    IndexThrottle1  = CsvRead[0].index("Тепловой троттлинг (HTC) [Yes/No]")
    IndexThrottle2  = CsvRead[0].index("Тепловой троттлинг (PROCHOT ЦП) [Yes/No]")
    IndexThrottle3  = CsvRead[0].index("Тепловой троттлинг (PROCHOT EXT) [Yes/No]")
    for row in CsvRead[1:]:
        Temp.append(float(row[IndexTemp])), Chast.append(float(row[IndexChast])), Throttle.append(any([row[IndexThrottle1] == 'Yes', row[IndexThrottle2] == 'Yes', row[IndexThrottle3] == 'Yes']))
Temp = np.array(Temp)
Chast = np.array(Chast)
print(f" среднее значение температуры: {np.median(Temp)},\n" +
      f" минимальное: {np.min(Temp)},\n" + 
      f" максимальное: {np.max(Temp)},\n" + 
      f" троттлинг: {any(Throttle)},\n" + 
      f" среднее значение частоты: {np.median(Chast)},\n" + 
      f" минимальное: {np.min(Chast)},\n" + 
      f" максимальное: {np.max(Chast)}")
