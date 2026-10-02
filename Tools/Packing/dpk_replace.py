#!/usr/bin/env python3
"""Replace (or add) a few files inside an existing package's darkeden.dpk, in place.

NOT FOR PUBLISHING since the updater patches players' archives file by file: the
package's pair is the published BASE and must stay equal to its darkeden.entries.
To ship a changed file use make_patch.py (it stages Archive\\ for the server). This
script is for patching a local pair for testing.

    python dpk_replace.py info\\npcscript.en.inf [ui\\txt\\item.rpk ...]
    python dpk_replace.py --package <dir> <path under Data> ...

Paths are relative to ReleaseData\\Data. Much faster than make_dpk.py when only a
handful of files changed: dpkput add frees the old entry's blocks and the new
copy reuses them (npcscript.en.inf went in without the .dpk growing a byte).

Steps: dpkput add, then check that every replaced entry reads back byte for byte,
that no entry was lost, and that a sample of the others (all info files, the
neighbours of each replaced entry, 400 random) still match ReleaseData\\Data. Finally
the new sha256s go into PACKAGE.txt. If dpkput fails, rebuild the package with
make_dpk.py. The same rules as make_dpk.py apply: nothing make_dpk.py
would leave out or ship loose (Music, 2-byte files, player.inf, ...) is accepted.
"""
import argparse
import hashlib
import os
import random
import re
import shutil
import subprocess
import sys
import time

from make_dpk import DEFAULT_OUT, DEFAULT_SRC, DPKGET, DPKPUT, MAX_DPK, classify, sha256


def dpkget_list(data_dir):
    out = subprocess.run([DPKGET, "darkeden", "--list", "*"], cwd=data_dir,
                         capture_output=True).stdout.decode("cp949", "replace")
    return sorted(l.strip() for l in out.splitlines() if l.startswith("  "))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("paths", nargs="+", help="files under ReleaseData\\Data, e.g. info\\npcscript.en.inf")
    ap.add_argument("--src", default=DEFAULT_SRC)
    ap.add_argument("--package", default=DEFAULT_OUT)
    a = ap.parse_args()

    data_dir = os.path.join(a.package, "Data")
    report = os.path.join(a.package, "PACKAGE.txt")
    for p in (report, os.path.join(data_dir, "darkeden.dpk"), os.path.join(data_dir, "darkeden.dpi")):
        if not os.path.isfile(p):
            sys.exit("no package at %s (missing %s) - build one with make_dpk.py" % (a.package, p))

    items = []
    for p in a.paths:
        rel = p.replace("\\", "/").strip("/").lower()
        if rel.startswith("data/"):
            rel = rel[5:]
        disk = os.path.join(a.src, *rel.split("/"))
        if not os.path.isfile(disk):
            sys.exit("no such source file: %s" % disk)
        kind = classify(rel, os.path.getsize(disk))
        if kind != "pack":
            sys.exit("%s cannot go into the archive (%s)" % (rel, kind))
        items.append((rel, disk))

    before = dpkget_list(data_dir)
    size_before = os.path.getsize(os.path.join(data_dir, "darkeden.dpk"))

    args = [DPKPUT, "darkeden", "add"]
    for rel, disk in items:
        args += [disk, "data/" + rel]
    p = subprocess.run(args, cwd=data_dir, capture_output=True)
    print(p.stdout.decode("cp949", "replace").strip())
    if p.returncode != 0:
        sys.exit("dpkput add failed (%d). Rebuild the package with make_dpk.py" % p.returncode)
    lock = os.path.join(data_dir, "darkeden.dpl")
    if os.path.exists(lock):
        os.remove(lock)

    # --- verify --------------------------------------------------------------------------
    after = dpkget_list(data_dir)
    problems = []
    lost = sorted(set(before) - set(after))
    if lost:
        problems.append("%d entries disappeared: %s" % (len(lost), ", ".join(lost[:10])))
    replaced = ["data/" + rel for rel, _ in items]
    sample = set(replaced) | {e for e in after if e.startswith("data/info/")}
    for r in replaced:
        i = after.index(r) if r in after else 0
        sample |= set(after[max(0, i - 20):i + 20])
    random.seed(time.time())
    sample |= set(random.sample(after, min(400, len(after))))
    sample = sorted(sample)

    tmp = os.path.join(a.package, "_verify")
    if os.path.exists(tmp):
        shutil.rmtree(tmp)
    os.makedirs(tmp)
    for k in range(0, len(sample), 100):
        subprocess.run([DPKGET, "darkeden", tmp] + sample[k:k + 100], cwd=data_dir, capture_output=True)
    bad = []
    for e in sample:
        got = os.path.join(tmp, *e.split("/"))
        src = os.path.join(a.src, *e.split("/")[1:])
        if not os.path.isfile(got):
            bad.append(e + " (not extracted)")
        elif os.path.isfile(src) and sha256(got) != sha256(src):
            bad.append(e + (" (REPLACED ENTRY differs)" if e in replaced else " (differs from the source folder)"))
    shutil.rmtree(tmp)
    if bad:
        problems.append("%d sampled entries read back wrong: %s" % (len(bad), "; ".join(bad[:10])))

    dpk = os.path.join(data_dir, "darkeden.dpk")
    dpi = os.path.join(data_dir, "darkeden.dpi")
    dpk_size, dpi_size = os.path.getsize(dpk), os.path.getsize(dpi)
    if dpk_size > MAX_DPK:
        problems.append("darkeden.dpk is past the 2 GB limit")
    print("entries %d -> %d, dpk %d -> %d bytes, %d sampled entries checked, %d bad"
          % (len(before), len(after), size_before, dpk_size, len(sample), len(bad)))

    # --- report ------------------------------------------------------------------------------
    print("hashing ...")
    dpk_hash, dpi_hash = sha256(dpk), sha256(dpi)
    text = open(report, encoding="utf-8").read()
    text = re.sub(r"(?m)^Data\\darkeden\.dpk .*$",
                  "Data\\\\darkeden.dpk  %13d  sha256 %s" % (dpk_size, dpk_hash), text)
    text = re.sub(r"(?m)^Data\\darkeden\.dpi .*$",
                  "Data\\\\darkeden.dpi  %13d  sha256 %s" % (dpi_size, dpi_hash), text)
    text = re.sub(r"(?m)^RESULT .*$", "RESULT   %s" % ("GOOD" if not problems else "PROBLEMS - do not ship"), text)
    note = ["UPDATED %s - replaced in place (dpk_replace.py): %s"
            % (time.strftime("%Y-%m-%d %H:%M"), ", ".join(replaced)),
            "  %d entries, %d sampled entries read back byte for byte%s"
            % (len(after), len(sample) - len(bad), "" if not problems else " - PROBLEMS: " + " / ".join(problems))]
    lines = text.split("\n")
    lines[3:3] = note + [""]        # after the title, the source line and the blank line
    open(report, "w", encoding="utf-8").write("\n".join(lines))

    print("\ndarkeden.dpk  %d  sha256 %s\ndarkeden.dpi  %d  sha256 %s" % (dpk_size, dpk_hash, dpi_size, dpi_hash))
    print("RESULT %s" % ("GOOD" if not problems else "PROBLEMS:\n  " + "\n  ".join(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
