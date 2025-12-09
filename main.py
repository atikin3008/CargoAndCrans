import matplotlib.pyplot as plt
from collections import Counter

filename = "deviations.txt"

values = []

# читаем только второе число в строке (само значение)
with open(filename, "r") as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith("#"):
            continue

        parts = line.split()
        if len(parts) >= 2:
            values.append(float(parts[1]))

# считаем количество повторений
counter = Counter(values[:100])

# сортируем по значению, чтобы график был красивый
xs = sorted(counter.keys())
ys = [counter[x] for x in xs]

# строим график
plt.figure(figsize=(10,5))
plt.bar(xs, ys)

plt.xlabel("Значение")
plt.ylabel("Количество встреч")
plt.title("Частота появления значений Deviations")
plt.grid(True)

plt.show()
