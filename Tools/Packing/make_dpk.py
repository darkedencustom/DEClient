#!/usr/bin/env python3
"""Pack the loose ReleaseData\\Data tree into a fresh darkeden.dpk + darkeden.dpi.

    python make_dpk.py                     ReleaseData\\Data -> Release\\Package
    python make_dpk.py --src <Data dir>    pack another loose tree
    python make_dpk.py --out <dir>         another output folder (same drive as the
                                           source, or staging falls back to copying)
    python make_dpk.py --ref <Data dir>    also diff the entry list against the
                                           darkeden.dpk/.dpi pair in that folder

ReleaseData\\Data is the loose copy of the game data kept in git - edit files
there. This folds them back into an archive for the game folder / VPS. The
output is <out>\\Data\\ holding darkeden.dpk, darkeden.dpi and the files that
must stay loose, plus <out>\\PACKAGE.txt with the counts and the sha256 of both
halves - the updater's manifest uses sha256, so the published lines can be
compared directly.

What does NOT go into the archive, and why:

  * Data\\Music\\*          Vorbis\\FileStream.cpp opens these with fopen_s, a real
                           disk read that never reaches the VFS. Not packaged at
                           all: the game folder keeps its own Music.
  * any 2-byte file        a 2-byte entry comes back with its second byte zeroed
                           (proven with dpkput build). Shipped LOOSE instead -
                           in practice Data\\Info\\version.inf.
  * Data\\Info\\player.inf  the last account and character names used on this
                           machine. Never shipped (the 2026 archive carried it).
  * Data\\Updater\\         the updater's own bookkeeping.
  * files in Data\\ itself  tools (dpkget/dpkput.exe) and the archive/lock files.
  * editing leftovers      *.bak, *.orig, *.before_*, *.py, *.log, *.tmp, ...

Every packed file is read back out of the new archive with dpkget and compared
byte for byte before the package is declared good. The .dpk must stay under
2,147,483,647 bytes (CNFS::file_lseek takes an int offset).
"""
import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
DEFAULT_SRC = os.path.join(REPO, "ReleaseData", "Data")     # in git; Release\ is build output
DEFAULT_OUT = os.path.join(REPO, "Release", "Package")
DPKPUT = os.path.join(HERE, "dpkput.exe")
DPKGET = os.path.join(HERE, "dpkget.exe")
MAX_DPK = 2147483647

SKIP_TOP_DIRS = {"music", "updater"}
SKIP_PATHS = {"info/player.inf"}
SKIP_SUFFIXES = (".bak", ".orig", ".tmp", ".py", ".pyc", ".log", ".part", ".old", ".new",
                 ".dpl", ".exe", ".pdb")
SKIP_INFIXES = (".before_", ".truncated")
# folder names as the game folder spells them (ReleaseData\Data has them lowercase)
GAME_DIR_CASE = {"info": "Info", "image": "Image", "map": "Map", "sound": "Sound", "ui": "UI", "music": "Music"}


def classify(rel, size):
    """rel: path under Data, forward slashes, lowercase. -> 'pack' | 'loose' | reason to skip"""
    parts = rel.split("/")
    if len(parts) == 1:
        return "skip: file in Data\\ itself (tool or archive)"
    if parts[0] in SKIP_TOP_DIRS:
        return "skip: Data\\%s\\ stays out of the archive" % parts[0]
    if rel in SKIP_PATHS:
        return "skip: private (last account/character names)"
    name = parts[-1]
    if name.endswith(SKIP_SUFFIXES) or any(s in name for s in SKIP_INFIXES):
        return "skip: editing leftover"
    if size == 2:
        return "loose"
    return "pack"


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def run(cmd, cwd, log):
    t = time.time()
    p = subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = p.stdout.decode("cp949", "replace")
    with open(log, "w", encoding="utf-8") as f:
        f.write(out)
    return p.returncode, out, time.time() - t


def list_archive(data_dir, log):
    rc, out, _ = run([DPKGET, "darkeden", "--list", "*"], data_dir, log)
    if rc != 0:
        sys.exit("dpkget --list failed (%d), see %s" % (rc, log))
    return {line.strip() for line in out.splitlines() if line.startswith("  ")}


def hash_archive(data_dir, log):
    """{vpath: (sha256, size)} of every entry, read through the VFS (dpkget --hash)."""
    rc, out, _ = run([DPKGET, "darkeden", "--hash"], data_dir, log)
    rows, bad = {}, []
    for line in out.splitlines():
        parts = line.split(" ", 2)
        if len(parts) == 3 and len(parts[0]) == 64 and parts[1].isdigit():
            rows[parts[2]] = (parts[0], int(parts[1]))
        elif line.strip():
            bad.append(line)
    if rc != 0 or bad:
        sys.exit("dpkget --hash failed (exit %d): %s - see %s" % (rc, "; ".join(bad[:3]), log))
    return rows


# --- darkeden.entries: the base list the update server works from ------------------------
# "# dpk <sha256> <size>" / "# dpi <sha256> <size>" name the pair it describes, then
# one "<sha256> <size> <vpath>" per entry (the updater's EntryList format).
def write_entries(path, rows, dpk_hash, dpk_size, dpi_hash, dpi_size):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("# darkeden entries\n")
        f.write("# dpk %s %d\n" % (dpk_hash, dpk_size))
        f.write("# dpi %s %d\n" % (dpi_hash, dpi_size))
        for vpath in sorted(rows):
            f.write("%s %d %s\n" % (rows[vpath][0], rows[vpath][1], vpath))


def read_entries(path):
    """-> (header {"dpk": (hash, size), "dpi": (hash, size)}, {vpath: (hash, size)})"""
    header, rows = {}, {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("#"):
                p = line[1:].split()
                if len(p) == 3 and p[0] in ("dpk", "dpi"):
                    header[p[0]] = (p[1], int(p[2]))
                continue
            if line.strip():
                h, size, vpath = line.split(" ", 2)
                rows[vpath] = (h, int(size))
    return header, rows


def entries_only(data_dir, out_path):
    """Write darkeden.entries for an existing pair (e.g. the one already published)."""
    for f in ("darkeden.dpk", "darkeden.dpi"):
        if not os.path.isfile(os.path.join(data_dir, f)):
            sys.exit("no %s in %s" % (f, data_dir))
    print("reading every entry of %s ..." % data_dir)
    rows = hash_archive(data_dir, out_path + ".log")
    dpk, dpi = os.path.join(data_dir, "darkeden.dpk"), os.path.join(data_dir, "darkeden.dpi")
    print("hashing the pair ...")
    dpk_hash, dpi_hash = sha256(dpk), sha256(dpi)
    write_entries(out_path, rows, dpk_hash, os.path.getsize(dpk), dpi_hash, os.path.getsize(dpi))
    os.remove(out_path + ".log")
    print("%s: %d entries\n  dpk %s\n  dpi %s" % (out_path, len(rows), dpk_hash, dpi_hash))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--src", default=DEFAULT_SRC)
    ap.add_argument("--out", default=DEFAULT_OUT)
    ap.add_argument("--ref", help="a Data folder holding the previous darkeden.dpk/.dpi, to diff entries")
    ap.add_argument("--keep-temp", action="store_true", help="keep the staging folder")
    ap.add_argument("--entries-only", metavar="DATA_DIR",
                    help="only write darkeden.entries for the pair already in DATA_DIR (e.g. the published one)")
    ap.add_argument("--entries-out", help="with --entries-only: where to write it (default: DATA_DIR\\darkeden.entries)")
    a = ap.parse_args()

    for tool in (DPKPUT, DPKGET):
        if not os.path.isfile(tool):
            sys.exit("missing %s - run Tools\\Packing\\build.bat first" % tool)
    if a.entries_only:
        return entries_only(a.entries_only, a.entries_out or os.path.join(a.entries_only, "darkeden.entries"))
    if not os.path.isdir(a.src):
        sys.exit("no source folder %s" % a.src)

    out = os.path.abspath(a.out)
    if os.path.exists(out):
        if not os.path.isfile(os.path.join(out, "PACKAGE.txt")):
            sys.exit("%s exists and is not a package folder (no PACKAGE.txt) - refusing to clear it" % out)
        print("clearing the previous package in %s" % out)
        shutil.rmtree(out)
    data_out = os.path.join(out, "Data")
    stage = os.path.join(out, "_stage")
    logs = os.path.join(out, "logs")
    archive = os.path.join(out, "Archive")
    for d in (data_out, stage, logs, os.path.join(archive, "files")):
        os.makedirs(d)
    # marks the folder as ours from the start, so a failed run can be cleared next time
    open(os.path.join(out, "PACKAGE.txt"), "w").write("incomplete - the packing run did not finish\n")

    # --- classify ----------------------------------------------------------------------
    packs, looses, skips = [], [], []
    for dirpath, dirnames, files in os.walk(a.src):
        for f in files:
            disk = os.path.join(dirpath, f)
            rel = os.path.relpath(disk, a.src).replace("\\", "/").lower()
            size = os.path.getsize(disk)
            kind = classify(rel, size)
            if kind == "pack":
                packs.append((rel, disk, size))
            elif kind == "loose":
                looses.append((rel, disk, size))
            else:
                skips.append((rel, kind))
    print("source %s\n  %d to pack (%.1f MB), %d loose, %d skipped"
          % (a.src, len(packs), sum(p[2] for p in packs) / 1e6, len(looses), len(skips)))

    # --- stage: Data\... under one root, hard links where possible ----------------------
    linked = copied = 0
    for rel, disk, size in packs:
        dst = os.path.join(stage, "Data", *rel.split("/"))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        try:
            os.link(disk, dst)
            linked += 1
        except OSError:
            shutil.copy2(disk, dst)
            copied += 1
    print("staged: %d hard links, %d copies" % (linked, copied))

    # --- build ---------------------------------------------------------------------------
    print("building darkeden.dpk ... (several minutes)")
    rc, outp, secs = run([DPKPUT, "darkeden", "build", stage], data_out, os.path.join(logs, "dpkput.txt"))
    stray = os.path.join(data_out, "dpkput.log")
    if os.path.exists(stray):
        shutil.move(stray, os.path.join(logs, "dpkput.log"))
    tail = [l for l in outp.splitlines() if "written" in l or "FAIL" in l or "ERROR" in l]
    print("  dpkput exit %d after %.0fs: %s" % (rc, secs, "; ".join(tail[-3:])))
    if rc != 0:
        sys.exit("dpkput build failed - see %s" % logs)

    dpk = os.path.join(data_out, "darkeden.dpk")
    dpi = os.path.join(data_out, "darkeden.dpi")
    dpk_size, dpi_size = os.path.getsize(dpk), os.path.getsize(dpi)
    problems = []
    if dpk_size > MAX_DPK:
        problems.append("darkeden.dpk is %d bytes - past the 2 GB limit the client can seek" % dpk_size)

    # --- read everything back: the archive must hold exactly the source files -------------
    # dpkget --hash reads every entry through the VFS, as the client would
    print("reading every entry back ...")
    t0 = time.time()
    got = hash_archive(data_out, os.path.join(logs, "readback.txt"))
    entries = set(got)
    expected = {"data/" + rel for rel, _, _ in packs}
    missing, extra = sorted(expected - entries), sorted(entries - expected)
    if missing or extra:
        problems.append("entry list differs: %d missing, %d unexpected" % (len(missing), len(extra)))

    base_rows = {}
    bad = []
    for rel, disk, size in packs:
        vpath = "data/" + rel
        h = sha256(disk)
        base_rows[vpath] = (h, size)
        if vpath not in got:
            continue    # already counted as missing
        if got[vpath][1] != size:
            bad.append((rel, "size %d, expected %d" % (got[vpath][1], size)))
        elif got[vpath][0] != h:
            bad.append((rel, "content differs"))
    print("  read back %d entries in %.0fs: %d bad" % (len(got), time.time() - t0, len(bad)))
    if bad:
        problems.append("%d entries read back wrong" % len(bad))

    # --- loose files ------------------------------------------------------------------------------
    # written with the game folder's casing (Data\Info\...): the VPS is Linux, where a
    # lowercase "info" would sit beside "Info" and publish the same file twice
    for rel, disk, size in looses:
        parts = rel.split("/")
        parts[0] = GAME_DIR_CASE.get(parts[0], parts[0])
        dst = os.path.join(data_out, *parts)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copy2(disk, dst)

    # --- optional diff against the previous archive ----------------------------------------------
    ref_note = []
    if a.ref:
        old = list_archive(a.ref, os.path.join(logs, "list_ref.txt"))
        gone, new = sorted(old - entries), sorted(entries - old)
        ref_note = ["", "Against %s (%d entries):" % (a.ref, len(old)),
                    "  %d entries are no longer in the archive:" % len(gone)]
        ref_note += ["    " + g for g in gone]
        ref_note += ["  %d entries are new:" % len(new)] + ["    " + n for n in new]

    if not a.keep_temp:
        shutil.rmtree(stage)
    # the VFS lock file; the client makes its own, so it is not shipped
    lock = os.path.join(data_out, "darkeden.dpl")
    if os.path.exists(lock):
        os.remove(lock)

    # --- report ---------------------------------------------------------------------------------------
    print("hashing the archive ...")
    dpk_hash, dpi_hash = sha256(dpk), sha256(dpi)

    # the base list the update server patches from; the delta starts empty
    if not problems:
        write_entries(os.path.join(data_out, "darkeden.entries"), base_rows, dpk_hash, dpk_size, dpi_hash, dpi_size)
        with open(os.path.join(archive, "base.txt"), "w", encoding="utf-8", newline="\n") as f:
            f.write(dpk_hash + "\n")
    lines = ["darkeden.dpk / .dpi package - %s" % time.strftime("%Y-%m-%d %H:%M:%S"),
             "source   %s" % a.src,
             "",
             "RESULT   %s" % ("GOOD" if not problems else "PROBLEMS - do not ship"),
             ]
    lines += ["  ! " + p for p in problems]
    lines += ["",
              "Data\\darkeden.dpk  %13d  sha256 %s" % (dpk_size, dpk_hash),
              "Data\\darkeden.dpi  %13d  sha256 %s" % (dpi_size, dpi_hash),
              "(%d bytes below the 2 GB limit)" % (MAX_DPK - dpk_size),
              "",
              "%d files packed, every one read back byte for byte" % len(packs) if not bad else
              "%d files packed, %d read back wrong" % (len(packs), len(bad)),
              "",
              "Loose files in this package (keep them beside the archive):"]
    lines += ["  Data\\%s  (%d bytes)" % (rel.replace("/", "\\"), size) for rel, _, size in looses]
    lines += ["", "Left out (%d):" % len(skips)]
    by_reason = {}
    for rel, why in skips:
        by_reason.setdefault(why, []).append(rel)
    for why, rels in sorted(by_reason.items()):
        shown = rels if len(rels) <= 8 else rels[:8] + ["... %d more" % (len(rels) - 8)]
        lines.append("  %s (%d): %s" % (why, len(rels), ", ".join(shown)))
    if missing or extra:
        lines += ["", "Entry list problems:"] + ["  missing " + m for m in missing[:50]] + \
                 ["  unexpected " + e for e in extra[:50]]
    if bad:
        lines += ["", "Read-back failures:"] + ["  %s: %s" % b for b in bad[:100]]
    lines += ref_note
    lines += ["",
              "Data\\darkeden.entries lists every file in this archive: it goes up WITH the",
              "pair (the three belong together) and is what the update server patches from.",
              "Archive\\ holds later single-file patches (make_patch.py); it starts empty.",
              "",
              "Upload darkeden.dpk, darkeden.dpi AND darkeden.entries together, then",
              "compare the sha256 values on the server. Any other loose file left in the",
              "game folder's Data\\ (outside Music and the loose list above) overrides",
              "the archive's copy."]
    report = "\n".join(lines) + "\n"
    with open(os.path.join(out, "PACKAGE.txt"), "w", encoding="utf-8") as f:
        f.write(report)
    print("\n" + report)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
