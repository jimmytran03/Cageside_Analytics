#!/usr/bin/env python3
"""Convert a public UFC fight dataset into this engine's CSV format.

Output columns:  date,winner,loser,method
Methods written: KO/TKO, Submission, UD, SD

Works with single-table datasets that have one row per fight, such as the
Kaggle "Ultimate UFC Dataset" (ufc-master.csv). Column names are auto-detected
and printed so you can check them; override with --date-col, --a-col, etc.

Rows that cannot be used are skipped and counted (draws, no contests,
DQs, overturned results, unreadable dates, duplicates).

Usage:
    python3 tools/convert_ufc_data.py path/to/ufc-master.csv
    python3 tools/convert_ufc_data.py raw.csv -o data/ufc_fights.csv --method-col Finish
"""
import argparse
import csv
import re
import sys
import unicodedata
from collections import Counter
from datetime import datetime
from pathlib import Path


class ConversionError(Exception):
    pass


# Header names are compared after lowercasing and removing non-alphanumerics.
CANDIDATES = {
    "date":   ["date", "eventdate", "fightdate"],
    "a":      ["redfighter", "rfighter", "redfightername", "fighter1", "fightfighter"],
    "b":      ["bluefighter", "bfighter", "bluefightername", "fighter2", "opponent"],
    "winner": ["winner", "fightresult"],
    "method": ["finish", "winby", "method", "winmethod"],
}
FIELD_LABELS = {"date": "date", "a": "fighter A (red / first)", "b": "fighter B (blue / second)",
                "winner": "winner", "method": "finish method"}

_DATE_FORMATS = ["%m/%d/%Y", "%m/%d/%y", "%B %d, %Y", "%b %d, %Y", "%d %B %Y",
                 "%d %b %Y", "%d-%b-%y", "%d-%b-%Y", "%Y/%m/%d"]
_TRANSLIT = str.maketrans({"ł": "l", "Ł": "L", "ø": "o", "Ø": "O", "đ": "d", "Đ": "D",
                           "ß": "ss", "æ": "ae", "Æ": "AE", "ð": "d", "þ": "th"})


def _key(header):
    return re.sub(r"[^a-z0-9]", "", header.lower())


def detect_columns(headers, overrides):
    by_key = {}
    for h in headers:
        by_key.setdefault(_key(h), h)

    chosen = {}
    for field, names in CANDIDATES.items():
        override = overrides.get(field)
        if override:
            if override not in headers:
                raise ConversionError(f"Column '{override}' not found in the file.\n"
                                      f"Available columns: {', '.join(headers[:60])}")
            chosen[field] = override
            continue
        for name in names:
            if name in by_key:
                chosen[field] = by_key[name]
                break

    missing = [FIELD_LABELS[f] for f in CANDIDATES if f not in chosen]
    if missing:
        raise ConversionError(
            "Could not find column(s) for: " + ", ".join(missing) + ".\n"
            f"Columns in this file: {', '.join(headers[:60])}"
            + (" ..." if len(headers) > 60 else "") + "\n"
            "Point the script at the right ones with --date-col, --a-col, --b-col, "
            "--winner-col and --method-col.")
    return chosen


def clean_name(raw, keep_accents=False):
    s = raw.strip()
    if not keep_accents:
        s = unicodedata.normalize("NFKD", s.translate(_TRANSLIT)).encode("ascii", "ignore").decode("ascii")
    s = s.replace(",", " ").replace('"', "")        # the C++ loader splits on commas
    return re.sub(r"\s+", " ", s).strip()


def parse_date(raw):
    s = raw.strip()
    if not s:
        return None
    iso = re.match(r"^(\d{4}-\d{2}-\d{2})", s)
    if iso:
        try:
            return datetime.strptime(iso.group(1), "%Y-%m-%d").date().isoformat()
        except ValueError:
            return None
    s = re.sub(r"\s+", " ", s.replace(".", "").replace("Sept", "Sep"))
    for fmt in _DATE_FORMATS:        # numeric dates are read as US month/day/year
        try:
            return datetime.strptime(s, fmt).date().isoformat()
        except ValueError:
            continue
    return None


def winner_side(winner_raw, a, b):
    """Returns 'a', 'b', or None (draw / no contest / unrecognised)."""
    v = winner_raw.strip().lower()
    if v in ("red", "r", "win", "w"):
        return "a"
    if v in ("blue", "b", "loss", "l"):
        return "b"
    if v and v == a.lower():
        return "a"
    if v and v == b.lower():
        return "b"
    return None


def normalize_method(raw):
    """Returns (method, note). On failure method is None and note is the skip reason."""
    s = raw.strip().lower()
    if not s:
        return None, "missing finish method"
    tokens = set(re.findall(r"[a-z]+", s))
    if tokens & {"dq", "disqualification", "overturned", "other"} or \
            "no contest" in s or "could not continue" in s or s == "nc":
        return None, "excluded result (DQ / overturned / other)"

    if "dec" in s or s in ("ud", "sd", "md"):
        if "split" in s or s in ("s-dec", "sd"):
            return "SD", None
        if "majority" in s or s in ("m-dec", "md"):
            return "SD", "majority decision counted as split decision"
        if "unanimous" in s or s in ("u-dec", "ud"):
            return "UD", None
        return "UD", "unspecified decision counted as unanimous"
    if "ko" in s or "tko" in s:
        return "KO/TKO", None
    if "sub" in s:
        return "Submission", None
    return None, "unrecognised finish method"


def convert(input_path, output_path, overrides=None, keep_accents=False):
    overrides = overrides or {}
    skipped, notes, unknown_values = Counter(), Counter(), Counter()
    kept = []
    seen = set()
    rows_read = 0

    with open(input_path, newline="", encoding="utf-8-sig") as f:
        first_line = f.readline()
        f.seek(0)
        delimiter = ";" if first_line.count(";") > first_line.count(",") else ","
        reader = csv.DictReader(f, delimiter=delimiter)
        if not reader.fieldnames:
            raise ConversionError("The input file is empty.")
        cols = detect_columns(reader.fieldnames, overrides)

        for row in reader:
            rows_read += 1
            a = clean_name(row.get(cols["a"]) or "", keep_accents)
            b = clean_name(row.get(cols["b"]) or "", keep_accents)
            if not a or not b or a.lower() == b.lower():
                skipped["missing or invalid fighter names"] += 1
                continue
            date = parse_date(row.get(cols["date"]) or "")
            if not date:
                skipped["unreadable date"] += 1
                continue
            side = winner_side(clean_name(row.get(cols["winner"]) or "", keep_accents), a, b)
            if side is None:
                skipped["no winner (draw / no contest)"] += 1
                continue
            raw_method = row.get(cols["method"]) or ""
            method, note = normalize_method(raw_method)
            if method is None:
                skipped[note] += 1
                if note == "unrecognised finish method":
                    unknown_values[raw_method.strip()] += 1
                continue

            winner, loser = (a, b) if side == "a" else (b, a)
            key = (date, frozenset((a.lower(), b.lower())))
            if key in seen:
                skipped["duplicate fight"] += 1
                continue
            seen.add(key)
            if note:
                notes[note] += 1
            kept.append((date, winner, loser, method))

    kept.sort(key=lambda r: r[0])        # stable: same-day fights keep file order
    out = Path(output_path)
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["date", "winner", "loser", "method"])
        writer.writerows(kept)

    fighters = {r[1] for r in kept} | {r[2] for r in kept}
    return {"cols": cols, "rows_read": rows_read, "kept": len(kept), "skipped": skipped,
            "notes": notes, "unknown_values": unknown_values, "fighters": len(fighters),
            "first_date": kept[0][0] if kept else None, "last_date": kept[-1][0] if kept else None,
            "methods": Counter(r[3] for r in kept), "output": str(out)}


def main(argv=None):
    p = argparse.ArgumentParser(description="Convert a UFC fight dataset to date,winner,loser,method.")
    p.add_argument("input", help="source CSV (one row per fight)")
    p.add_argument("-o", "--output", default="data/ufc_fights.csv")
    p.add_argument("--date-col")
    p.add_argument("--a-col", help="first / red fighter name column")
    p.add_argument("--b-col", help="second / blue fighter name column")
    p.add_argument("--winner-col")
    p.add_argument("--method-col")
    p.add_argument("--keep-accents", action="store_true",
                   help="keep accented characters in names (default: convert to plain ASCII)")
    args = p.parse_args(argv)

    overrides = {"date": args.date_col, "a": args.a_col, "b": args.b_col,
                 "winner": args.winner_col, "method": args.method_col}
    try:
        r = convert(args.input, args.output, overrides, args.keep_accents)
    except (ConversionError, OSError) as e:
        print(f"Error: {e}", file=sys.stderr)
        return 2

    print("Columns used:")
    for field in CANDIDATES:
        print(f"  {FIELD_LABELS[field]:<26} <- '{r['cols'][field]}'")
    print(f"\nRead {r['rows_read']} rows, kept {r['kept']} fights ({r['fighters']} fighters)")
    if r["kept"]:
        print(f"Date range: {r['first_date']} to {r['last_date']}")
        print("Methods: " + ", ".join(f"{m} {n}" for m, n in sorted(r["methods"].items())))
    if r["skipped"]:
        print("\nSkipped:")
        for reason, n in r["skipped"].most_common():
            print(f"  {n:>6}  {reason}")
    if r["unknown_values"]:
        print("  Unrecognised finish values (most common): " +
              ", ".join(f"'{v}' x{n}" for v, n in r["unknown_values"].most_common(5)))
    if r["notes"]:
        print("\nAssumptions made:")
        for note, n in r["notes"].most_common():
            print(f"  {n:>6}  {note}")
    print(f"\nWrote {r['output']}")
    return 0 if r["kept"] else 1


if __name__ == "__main__":
    sys.exit(main())
