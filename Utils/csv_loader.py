import numpy as np
import csv


def _to_float(value: str) -> float:
    try:
        v = value.replace(',', '.')
        return float(v)
    except:
        return 0

def Load(csv_file: str):
    axisx = []
    axisy = []
    with open(csv_file) as f:
        reader = csv.reader(f, delimiter=';')
        next(reader)
        for row in reader:
            axisx.append(_to_float(row[1]))
            axisy.append(_to_float(row[2]))
    return np.array(axisx, np.float64), np.array(axisy, np.float64)
