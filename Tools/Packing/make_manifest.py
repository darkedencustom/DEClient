#!/usr/bin/env python3
"""Turn a game folder into what the update server hosts: manifest.txt and files\\.

    python make_manifest.py <game folder> <out folder> --version 1.0.3
    python make_manifest.py <game folder> <out folder> --version 1.0.3 --server 203.0.113.7:9909

<game folder> is a finished install (make_game_folder.py builds one) with
Updater.exe in it. Upload <out folder> as it is; BaseUrl in the players'
Updater.ini points at it:

    <out>/manifest.txt       version, login server, and one line per file:
                             file <sha256> <size> <path>
                             seed <sha256> <size> <path>   (settings, see SEED)
    <out>/files/<path>       the files themselves
    <out>/news.txt           optional, yours to write; shown in the launcher

The updater downloads whatever is missing or hashes differently, and deletes
what an earlier manifest listed and this one no longer does. So a patch is: put
the changed or new loose file in the game folder, run this again with a new
version, upload.

THE ARCHIVE, FILE BY FILE. When the game folder holds Data/darkeden.entries (the
list make_dpk.py writes next to the pair it builds), darkeden.dpk is not one
2 GB manifest line any more:

    base <sha256> <size> Data/darkeden.dpk    the pair, fetched only by players
    base <sha256> <size> Data/darkeden.dpi    whose archive is missing or broken
    entries <sha256> <size> archive/entries.txt

archive/entries.txt lists every file the archive should hold. It is the base
list plus the single-file patches in <game>/Archive/files/<vpath> (make_patch.py
stages them; Archive/removed.txt drops entries): patched rows are marked P and
hosted at files/archive/<vpath>, the rest B (only inside the base archive).
Players' updaters download just the P files that differ from their archive and
write them into it. Publishing refuses to write this unless Updater.exe knows
it ("ArchivePatch/1"), because an older updater would lose the archive.

An upload replaces Archive/ whole, so an Archive/ built without someone else's
change takes that change away from every player. Publishing therefore refuses
when a patch the last publish shipped would go back to the base (or out of the
archive) without a newer patch in its place - re-staging from an up to date
checkout fixes that. To go back on purpose: --allow-revert (ALLOW_REVERT=1 for
publish.sh).

Unchanged files are not copied again (same size and time), and are hard linked
rather than copied when <out> is on the same drive.
"""
import argparse
import hashlib
import os
import shutil
import sys

# what the game and the updater write for themselves; never shipped, and since
# it is never listed the updater never deletes it either
SKIP_DIRS = {"log", "screenshot", "userset", "temp"}
# the updater's own bookkeeping (Updater.cache, Updater.state, Archive.state, tools)
SKIP_PREFIXES = ("data/updater/",)
SKIP_FILES = {"updater.ini", "updater.cache", "updater.state", "start.bat", "make_manifest.py", "publish.sh"}
# written by the game and personal: Player.inf is the account and character
# names last used on this machine. Never published.
SKIP_PATHS = {"data/info/player.inf"}
# settings the game or the player rewrites. Published as "seed": the updater
# fetches one only when it is missing and never overwrites or deletes it.
SEED_PATHS = {"data/info/clientconfig.inf", "data/info/resolution.inf"}
SKIP_SUFFIXES = (".part", ".old", ".new", ".dpl", ".pdb", ".lib", ".exp", ".ilk", ".iobj", ".ipdb")

# archive mode
ARCHIVE_DIR = "archive"         # <game>/Archive: the single-file patches, never a plain file
PAIR_FILES = {"data/darkeden.dpk", "data/darkeden.dpi", "data/darkeden.entries"}
MARKER = "ArchivePatch/1".encode("utf-16-le")
ARCHIVE_LEFTOVERS = (".bak", ".orig", ".tmp", ".py", ".pyc", ".log", ".part", ".old", ".new", ".dpl", ".exe", ".pdb")


def wanted(rel):
    parts = rel.lower().split("/")
    if parts[0] in SKIP_DIRS and len(parts) > 1:
        return False
    if rel.lower() in SKIP_PATHS or rel.lower().startswith(SKIP_PREFIXES):
        return False
    return parts[-1] not in SKIP_FILES and not parts[-1].endswith(SKIP_SUFFIXES)


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def place(src, dst):
    """Hard link when it can, copy when it cannot; skip what is already there."""
    if os.path.exists(dst):
        s, d = os.stat(src), os.stat(dst)
        if s.st_size == d.st_size and abs(s.st_mtime - d.st_mtime) < 2:
            return "kept"
        os.remove(dst)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    try:
        os.link(src, dst)
        return "linked"
    except OSError:
        shutil.copy2(src, dst)
        return "copied"


def find_ci(root, rel):
    """The real path of rel under root, whatever its casing (the VPS is Linux)."""
    cur = root
    for part in rel.split("/"):
        try:
            cur = os.path.join(cur, next(e for e in os.listdir(cur) if e.lower() == part.lower()))
        except (StopIteration, FileNotFoundError, NotADirectoryError):
            return None
    return cur


def archive_refusal(vpath, size):
    """Why a patch file cannot go into the archive (same rules as make_dpk.py), or None."""
    if not vpath.startswith("data/") or vpath.count("/") < 2:
        return "not under data/<folder>/"
    if vpath.startswith("data/music/"):
        return "music is read from disk, never from the archive"
    if vpath == "data/info/player.inf":
        return "private"
    if size == 2:
        return "a 2-byte file cannot be stored in the archive (ship it loose)"
    name = vpath.rsplit("/", 1)[1]
    if name.endswith(ARCHIVE_LEFTOVERS) or ".before_" in name:
        return "editing leftover"
    return None


def read_entries(path):
    header, rows = {}, {}
    with open(path, encoding="utf-8") as f:
        for n, line in enumerate(f, 1):
            line = line.rstrip("\n").rstrip("\r")
            if line.startswith("#"):
                p = line[1:].split()
                if len(p) == 3 and p[0] in ("dpk", "dpi"):
                    header[p[0]] = (p[1], int(p[2]))
                continue
            if not line.strip():
                continue
            parts = line.split(" ", 2)
            if len(parts) != 3 or len(parts[0]) != 64 or not parts[1].isdigit():
                sys.exit("darkeden.entries line %d is not an entry: %s" % (n, line))
            rows[parts[2]] = (parts[0], int(parts[1]))
    if "dpk" not in header or "dpi" not in header:
        sys.exit("darkeden.entries does not say which dpk/dpi it describes (no '# dpk' / '# dpi' lines)")
    return header, rows


def lost_patches(game, out, entries_path):
    """Patches the last publish shipped that this Archive/ takes back: back to the base version,
    or out of the archive, with no newer patch in their place. Read before anything is written."""
    listing = os.path.join(out, "files", "archive", "entries.txt")
    if not os.path.isfile(listing):
        return []
    shipped = {}
    with open(listing, encoding="utf-8") as f:
        for line in f:
            p = line.rstrip("\n").split(" ", 3)
            if len(p) == 4 and p[0] == "P":
                shipped[p[3]] = p[1]
    if not shipped:
        return []
    _, base = read_entries(entries_path)
    arch = find_ci(game, "Archive")
    files = find_ci(arch, "files") if arch else None
    removed = set()
    removed_txt = find_ci(arch, "removed.txt") if arch else None
    if removed_txt:
        removed = {l.strip().lower() for l in open(removed_txt, encoding="utf-8") if l.strip()}
    lost = []
    for v, h in sorted(shipped.items()):
        src = find_ci(files, v) if files else None
        if src is not None and base.get(v) != (sha256(src), os.path.getsize(src)):
            continue        # still patched, perhaps with a newer version
        if v not in removed and base.get(v, (None,))[0] == h:
            continue        # a new base already holds what was shipped
        lost.append(v)
    return lost


def archive_mode(game, out, entries_path, lines, listed):
    """Adds the base/entries lines and fills files/archive/. Returns a summary line."""
    updater = find_ci(game, "Updater.exe")
    if updater is None or MARKER not in open(updater, "rb").read():
        sys.exit("Updater.exe in %s does not know how to patch the archive file by file.\n"
                 "Publishing the archive as base/entries lines would make it LOSE the archive.\n"
                 "Upload the new Updater.exe first (or remove Data/darkeden.entries to publish the old way)." % game)

    header, base = read_entries(entries_path)

    # the pair must be the one the list describes - uploaded together, or not at all
    for key, rel in (("dpk", "Data/darkeden.dpk"), ("dpi", "Data/darkeden.dpi")):
        path = find_ci(game, rel)
        if path is None:
            sys.exit("%s is missing but darkeden.entries is there" % rel)
        want_hash, want_size = header[key]
        if os.path.getsize(path) != want_size or sha256(path) != want_hash:
            sys.exit("%s is not the one darkeden.entries describes.\n"
                     "Upload darkeden.dpk, darkeden.dpi and darkeden.entries together." % rel)
        rel_real = os.path.relpath(path, game).replace(os.sep, "/")
        place(path, os.path.join(out, "files", rel_real))
        lines.append("base %s %d %s" % (want_hash, want_size, "Data/darkeden." + key))
        listed.add(rel_real.lower())

    # the single-file patches
    current = {v: ("B", h, s) for v, (h, s) in base.items()}
    arch = find_ci(game, "Archive")
    hosted, same, dropped = [], [], []
    if arch is not None:
        base_txt = find_ci(arch, "base.txt")
        if base_txt is not None:
            staged_for = open(base_txt, encoding="utf-8").read().strip()
            if staged_for and staged_for != header["dpk"][0]:
                sys.exit("Archive/ was staged for another base archive (%s...), not this one (%s...).\n"
                         "Its patches would be wrong here. Stage them again against this base."
                         % (staged_for[:16], header["dpk"][0][:16]))

        files = find_ci(arch, "files")
        if files is not None:
            for root, dirs, names in os.walk(files):
                for name in names:
                    src = os.path.join(root, name)
                    vpath = os.path.relpath(src, files).replace(os.sep, "/").lower()
                    size = os.path.getsize(src)
                    why = archive_refusal(vpath, size)
                    if why:
                        sys.exit("Archive/files/%s cannot be patched into the archive: %s" % (vpath, why))
                    h = sha256(src)
                    if base.get(vpath) == (h, size):
                        same.append(vpath)
                        continue
                    current[vpath] = ("P", h, size)
                    place(src, os.path.join(out, "files", "archive", *vpath.split("/")))
                    listed.add(("archive/" + vpath).lower())
                    hosted.append((vpath, size))

        removed_txt = find_ci(arch, "removed.txt")
        if removed_txt is not None:
            for v in open(removed_txt, encoding="utf-8"):
                v = v.strip().lower()
                if v and current.pop(v, None) is not None:
                    dropped.append(v)

    listing = os.path.join(out, "files", "archive", "entries.txt")
    os.makedirs(os.path.dirname(listing), exist_ok=True)
    with open(listing, "w", encoding="utf-8", newline="\n") as f:
        f.write("# darkeden entries\n")
        for v in sorted(current):
            flag, h, s = current[v]
            f.write("%s %s %d %s\n" % (flag, h, s, v))
    lines.append("entries %s %d archive/entries.txt" % (sha256(listing), os.path.getsize(listing)))
    listed.add("archive/entries.txt")

    for v, s in sorted(hosted):
        print("  patch   %12d  %s%s" % (s, v, "   (new)" if v not in base else ""))
    for v in same:
        print("  same as the base, not sent: %s" % v)
    for v in dropped:
        print("  removed from the archive: %s" % v)
    return current, "archive: %d files, %d patched singly (%.1f KB), %d removed" % (
        len(current), len(hosted), sum(s for _, s in hosted) / 1024.0, len(dropped))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("game")
    ap.add_argument("out")
    ap.add_argument("--version", required=True, help="digits and dots, e.g. 1.0.3 (the launcher draws it)")
    ap.add_argument("--server", default="", help="login server ip:port for players whose Updater.ini names none")
    ap.add_argument("--launch", default="DarkEden.exe")
    ap.add_argument("--allow-revert", action="store_true",
                    help="publish even when a patch the last publish shipped goes back to the base")
    args = ap.parse_args()

    game, out = os.path.abspath(args.game), os.path.abspath(args.out)
    if not os.path.isfile(os.path.join(game, args.launch)):
        sys.exit("%s is not in %s" % (args.launch, game))
    if out == game or out.startswith(game + os.sep):
        sys.exit("the out folder must not be inside the game folder")

    entries_path = find_ci(game, "Data/darkeden.entries")

    # Archive/ belongs beside Data/, not inside it: in Data/ it would go out as plain
    # loose files and its patches would never reach the archive
    misplaced = find_ci(game, "Data/Archive")
    if misplaced is not None and os.path.isdir(misplaced):
        sys.exit("%s is in the wrong place: the Archive folder goes in the game folder itself,\n"
                 "beside Data/ (%s), not inside Data/. Move it:\n"
                 "    mv \"%s\" \"%s\"" % (misplaced, os.path.join(game, "Archive"), misplaced, os.path.join(game, "Archive")))

    # before anything in <out> changes, so a refusal leaves the live publish as it was
    if entries_path and not args.allow_revert:
        lost = lost_patches(game, out, entries_path)
        if lost:
            sys.exit("This Archive/ takes back %d patch(es) players already have - back to the base\n"
                     "archive's version, or out of the archive:\n    %s\n"
                     "An upload replaces Archive/ whole, so this is usually someone else's change missing\n"
                     "from the one uploaded. Whoever built it: pull, make_patch.py --all, upload again.\n"
                     "If going back is intended:  ALLOW_REVERT=1 ./publish.sh patch"
                     % (len(lost), "\n    ".join(lost)))

    rels = []
    for root, dirs, files in os.walk(game):
        for name in files:
            rel = os.path.relpath(os.path.join(root, name), game).replace(os.sep, "/")
            if not wanted(rel):
                continue
            low = rel.lower()
            # in archive mode the pair and Archive/ are published as base/entries, not as files
            if entries_path and (low in PAIR_FILES or low.split("/")[0] == ARCHIVE_DIR):
                continue
            if not entries_path and low == "data/darkeden.entries":
                continue
            rels.append(rel)
    rels.sort(key=str.lower)

    lines = ["# DarkEden update manifest", "version " + args.version]
    if args.server:
        lines.append("server " + args.server)
    lines.append("launch " + args.launch)

    total = 0
    for rel in rels:
        src = os.path.join(game, rel)
        size = os.path.getsize(src)
        how = place(src, os.path.join(out, "files", rel))
        kind = "seed" if rel.lower() in SEED_PATHS else "file"
        lines.append("%s %s %d %s" % (kind, sha256(src), size, rel))
        total += size
        print("  %-7s %12d  %s%s" % (how, size, rel, "   (seed)" if kind == "seed" else ""))

    # files/ holds exactly what the manifest lists
    listed = {rel.lower() for rel in rels}
    summary = ""
    if entries_path:
        current, summary = archive_mode(game, out, entries_path, lines, listed)
        # a loose copy of an archive file wins over the archive on every player's disk
        for rel in rels:
            low = rel.lower()
            if low in current and low not in SEED_PATHS:
                print("WARNING: %s is published loose AND is in the archive - the loose copy overrides the"
                      " archive for every player. Remove it from the game folder unless that is intended." % rel)

    for root, dirs, files in os.walk(os.path.join(out, "files")):
        for name in files:
            full = os.path.join(root, name)
            rel = os.path.relpath(full, os.path.join(out, "files")).replace(os.sep, "/")
            if rel.lower() not in listed:
                os.remove(full)
                print("  dropped               %s" % rel)

    with open(os.path.join(out, "manifest.txt"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")

    if not any(r.lower() == "updater.exe" for r in rels):
        print("note: no Updater.exe in the game folder, so players' updaters will not update themselves")
    print("%d files, %.1f MB, version %s -> %s" % (len(rels), total / 1048576.0, args.version, out))
    if summary:
        print(summary)


if __name__ == "__main__":
    main()
