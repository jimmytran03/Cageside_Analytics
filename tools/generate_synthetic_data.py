"""Generates data/synthetic_fights.csv: a SIMULATED league (not real UFC data).

Each fighter gets a hidden skill; winners are drawn from the standard Elo
win-probability formula. Because the true skills are known, this is a useful
sanity check that the rating engine and backtester recover real signal.

Usage: python3 tools/generate_synthetic_data.py
"""
import datetime
import random

random.seed(42)
N_FIGHTERS, N_FIGHTS = 40, 800

names = [f"Fighter {i:02d}" for i in range(1, N_FIGHTERS + 1)]
skill = {n: random.gauss(0, 120) for n in names}   # Elo points relative to 1500
start = datetime.date(2015, 1, 1)

rows = ["date,winner,loser,method"]
for i in range(N_FIGHTS):
    a, b = random.sample(names, 2)
    p_a = 1 / (1 + 10 ** ((skill[b] - skill[a]) / 400))
    winner, loser = (a, b) if random.random() < p_a else (b, a)
    method = random.choices(["KO/TKO", "Submission", "UD", "SD"], weights=[30, 15, 40, 15])[0]
    date = start + datetime.timedelta(days=i * 2)
    rows.append(f"{date.isoformat()},{winner},{loser},{method}")

with open("data/synthetic_fights.csv", "w") as f:
    f.write("\n".join(rows) + "\n")
print(f"Wrote {N_FIGHTS} fights, {N_FIGHTERS} fighters")
