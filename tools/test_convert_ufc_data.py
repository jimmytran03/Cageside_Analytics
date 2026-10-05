"""Run with:  python3 -m unittest discover -s tools -p "test_*.py" -v"""
import csv
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(__file__))
import convert_ufc_data as c


def write_csv(path, header, rows):
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


def read_rows(path):
    with open(path, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


class NormalizeMethodTests(unittest.TestCase):
    def test_common_values(self):
        cases = {
            "KO/TKO": "KO/TKO", "TKO - Doctor's Stoppage": "KO/TKO", "SUB": "Submission",
            "Submission": "Submission", "U-DEC": "UD", "Decision - Unanimous": "UD",
            "S-DEC": "SD", "Decision - Split": "SD", "M-DEC": "SD", "Decision - Majority": "SD",
        }
        for raw, expected in cases.items():
            self.assertEqual(c.normalize_method(raw)[0], expected, raw)

    def test_excluded_and_unknown(self):
        for raw in ["DQ", "Overturned", "Other", "No Contest", "Could Not Continue", ""]:
            self.assertIsNone(c.normalize_method(raw)[0], raw)
        self.assertIsNone(c.normalize_method("Mystery")[0])

    def test_assumptions_are_flagged(self):
        self.assertIsNotNone(c.normalize_method("M-DEC")[1])
        self.assertIsNotNone(c.normalize_method("Decision")[1])
        self.assertIsNone(c.normalize_method("KO/TKO")[1])


class HelperTests(unittest.TestCase):
    def test_parse_date_formats(self):
        for raw in ["2022-07-30", "2022-07-30 00:00:00", "07/30/2022", "Jul. 30, 2022",
                    "July 30, 2022", "Jul 30, 2022"]:
            self.assertEqual(c.parse_date(raw), "2022-07-30", raw)
        self.assertEqual(c.parse_date("Sept. 3, 2022"), "2022-09-03")
        self.assertIsNone(c.parse_date("not a date"))
        self.assertIsNone(c.parse_date(""))

    def test_clean_name(self):
        self.assertEqual(c.clean_name("Jiří  Procházka"), "Jiri Prochazka")
        self.assertEqual(c.clean_name("Jan Błachowicz"), "Jan Blachowicz")
        self.assertEqual(c.clean_name('Smith, "Bob"'), "Smith Bob")
        self.assertEqual(c.clean_name("Jiří", keep_accents=True), "Jiří")

    def test_winner_side(self):
        self.assertEqual(c.winner_side("Red", "A", "B"), "a")
        self.assertEqual(c.winner_side("blue", "A", "B"), "b")
        self.assertEqual(c.winner_side("b", "A", "B"), "b")
        self.assertEqual(c.winner_side("Win", "A", "B"), "a")
        self.assertEqual(c.winner_side("Loss", "A", "B"), "b")
        self.assertEqual(c.winner_side("a", "A", "B"), "a")       # name match, case-insensitive
        self.assertIsNone(c.winner_side("Draw", "A", "B"))
        self.assertIsNone(c.winner_side("", "A", "B"))


class ConvertTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.src = os.path.join(self.tmp.name, "in.csv")
        self.out = os.path.join(self.tmp.name, "out.csv")

    def tearDown(self):
        self.tmp.cleanup()

    def test_red_blue_layout(self):
        write_csv(self.src, ["RedFighter", "BlueFighter", "Date", "Winner", "Finish", "Extra"], [
            ["Jiří Procházka", "Jan Błachowicz", "2022-02-01", "Red", "KO/TKO", "x"],
            ["A", "B", "2021-01-01", "Blue", "U-DEC", "x"],        # earlier: must sort first
            ["C", "D", "2021-03-01", "Draw", "U-DEC", "x"],        # draw
            ["E", "F", "2021-04-01", "Red", "DQ", "x"],            # excluded
            ["G", "H", "not-a-date", "Red", "SUB", "x"],           # bad date
            ["I", "J", "2021-05-01", "Red", "M-DEC", "x"],         # majority -> SD
        ])
        r = c.convert(self.src, self.out)
        rows = read_rows(self.out)
        self.assertEqual([x["date"] for x in rows], ["2021-01-01", "2021-05-01", "2022-02-01"])
        self.assertEqual((rows[0]["winner"], rows[0]["loser"], rows[0]["method"]), ("B", "A", "UD"))
        self.assertEqual(rows[1]["method"], "SD")
        self.assertEqual(rows[2]["winner"], "Jiri Prochazka")
        self.assertEqual(r["rows_read"], 6)
        self.assertEqual(r["kept"], 3)
        self.assertEqual(sum(r["skipped"].values()), 3)

    def test_winner_given_as_name(self):
        write_csv(self.src, ["Fighter_1", "Fighter_2", "date", "Winner", "Method"],
                  [["Ann", "Bea", "2020-01-01", "Bea", "Submission"],
                   ["Cat", "Dee", "2020-02-01", "Draw/NC", "Decision - Unanimous"]])
        c.convert(self.src, self.out)
        rows = read_rows(self.out)
        self.assertEqual(len(rows), 1)
        self.assertEqual((rows[0]["winner"], rows[0]["loser"]), ("Bea", "Ann"))

    def test_per_fighter_rows_are_deduplicated(self):
        write_csv(self.src, ["fight_fighter", "opponent", "fight_result", "event_date", "method"],
                  [["Ann", "Bea", "Win", "Jul. 30, 2022", "KO/TKO"],
                   ["Bea", "Ann", "Loss", "Jul. 30, 2022", "KO/TKO"]])
        r = c.convert(self.src, self.out)
        self.assertEqual(r["kept"], 1)
        self.assertEqual(r["skipped"]["duplicate fight"], 1)
        row = read_rows(self.out)[0]
        self.assertEqual((row["winner"], row["loser"], row["date"]), ("Ann", "Bea", "2022-07-30"))

    def test_missing_columns_gives_helpful_error(self):
        write_csv(self.src, ["foo", "bar"], [["1", "2"]])
        with self.assertRaises(c.ConversionError) as ctx:
            c.convert(self.src, self.out)
        self.assertIn("--method-col", str(ctx.exception))

    def test_column_override(self):
        write_csv(self.src, ["R", "B", "when", "who", "how"],
                  [["Ann", "Bea", "2020-01-01", "Red", "SUB"]])
        r = c.convert(self.src, self.out, {"a": "R", "b": "B", "date": "when",
                                           "winner": "who", "method": "how"})
        self.assertEqual(r["kept"], 1)

    def test_semicolon_delimited_file(self):
        with open(self.src, "w", encoding="utf-8") as f:
            f.write("R_fighter;B_fighter;date;Winner;win_by\nAnn;Bea;2020-01-01;Red;KO/TKO\n")
        self.assertEqual(c.convert(self.src, self.out)["kept"], 1)

    def test_output_loads_with_expected_header(self):
        write_csv(self.src, ["RedFighter", "BlueFighter", "Date", "Winner", "Finish"],
                  [["A", "B", "2020-01-01", "Red", "SUB"]])
        c.convert(self.src, self.out)
        with open(self.out, encoding="utf-8") as f:
            self.assertEqual(f.readline().strip(), "date,winner,loser,method")


if __name__ == "__main__":
    unittest.main()
