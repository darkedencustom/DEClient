#!/usr/bin/env python3
"""Stage single-file patches for the files inside darkeden.dpk.

    python make_patch.py info\\npcscript.en.inf [ui\\txt\\item.rpk ...]    stage changed files
    python make_patch.py info\\*.inf ui\\spk\\*.spk                         wildcards (** = any depth)
    python make_patch.py --all                                            everything that differs, then --check
    python make_patch.py --all --full                                     ... hashing every file
    python make_patch.py --remove data/ui/spk/old.spk                     drop an entry
    python make_patch.py --unstage info\\npcscript.en.inf                  take one back out
    python make_patch.py --check                                          is Archive\\ safe to upload?
    python make_patch.py --sync                                           copy git's data into Release\\Data
    python make_patch.py                                                  show what is staged

The source is ReleaseData\\Data, the loose copy of the game data kept in git: edit
and commit files there, then run --all. Paths are relative to it.

--all compares the archive files in ReleaseData\\Data with the base (size first,
then sha256) and makes the staged set the difference: changed and new files are
staged, staged files that are back to the base are unstaged. Base files missing
from ReleaseData\\Data are only reported; drop them with --remove.

--all remembers when it last ran (Release\\Package\\last_scan.txt, with the base and
the source folder it compared). The next run only hashes files modified since then,
plus every file that is already staged, so a day's work compares in seconds instead
of hashing all 1.8 GB. A file whose modification time is older than the last scan
is taken as unchanged - true for anything edited, saved, or checked out with git
(pull, checkout, reset), not for a copy made with its old timestamp kept (robocopy
/COPY:DAT, an archive extracted with timestamps). For those, or to get the exact
difference after an --unstage, add --full: it hashes everything and rewrites the
stamp. A new base (make_dpk.py), another source folder or a missing stamp also
means a full compare.

The package's Data\\darkeden.entries (written by make_dpk.py) is the base: the list
of what the published darkeden.dpk holds. A staged file lands in
Release\\Package\\Archive\\files\\ under its archive name; removals go into
Archive\\removed.txt.

Upload Release\\Package\\Archive\\ to the game folder on the server as Archive/
(replacing what is there), then run ./publish.sh patch. The server lists every
file of the archive with its sha256, marks the staged ones as hosted singly, and
players' updaters download only those and write them into their own archive.

THE UPLOAD REPLACES THE SERVER'S WHOLE Archive/, so it must hold everyone's
changes, not just yours: a patch someone else shipped that your Archive\\ lacks
goes back to the base for every player. After --all (or alone, with --check)
the Archive\\ is checked before you upload:

  * ReleaseData\\Data has nothing uncommitted, and no commits you have not pushed
    or pulled (it runs git fetch) - push before you upload, pull before you build
  * every patch the update server ships now (its files/archive/entries.txt) is in
    this Archive\\ as it is, or is a version your git history has, so yours
    supersedes it. A shipped version git has never seen is a change someone
    uploaded without pushing: uploading would take it away from players
  * nothing the server removed from the archive comes back

The server is the BaseUrl in update_server.txt beside this script (--server
overrides). A problem prints NOT READY TO UPLOAD and exits with 1.

--sync copies every file that differs from ReleaseData\\Data into Release\\Data, so
the client built there runs what git has. It leaves the settings the client
writes for itself alone (player.inf, clientconfig.inf, resolution.inf), and a
file in Release\\Data newer than git's copy - an edit made there - is reported,
not overwritten.

A file identical to the base is not staged (there is nothing to send). What
make_dpk.py would never put into the archive - Music, 2-byte files such as
version.inf, player.inf, editing leftovers - is refused here too: those ship loose.
"""
import argparse
import filecmp
import glob
import hashlib
import os
import shutil
import subprocess
import sys
import time
import urllib.request

from make_dpk import DEFAULT_OUT, DEFAULT_SRC, HERE, REPO, classify, read_entries, sha256


SCAN_TXT = "last_scan.txt"      # beside PACKAGE.txt, not inside Archive\\ (nothing to upload)
SCAN_MARGIN = 2                 # seconds; a save right at the scan start is checked again next time
SERVER_TXT = os.path.join(HERE, "update_server.txt")
DEFAULT_GAME_DATA = os.path.join(REPO, "Release", "Data")
# settings the client writes for itself on each machine; a copy from git would undo them
SYNC_SKIP = {"info/player.inf", "info/clientconfig.inf", "info/resolution.inf"}


def same_dir(a, b):
    return os.path.normcase(os.path.abspath(a)) == os.path.normcase(os.path.abspath(b))


def read_scan(path):
    """-> (base dpk sha256, epoch seconds when --all last walked the tree, source folder) or None.
    The folder is None in a stamp written before it was recorded (that one scanned Release\\Data)."""
    if not os.path.isfile(path):
        return None
    base_hash = when = src = None
    with open(path, encoding="utf-8") as f:
        for line in f:
            p = line.split()
            if len(p) >= 2 and p[0] == "base":
                base_hash = p[1]
            elif len(p) >= 2 and p[0] == "time":
                try:
                    when = float(p[1])
                except ValueError:
                    return None
            elif len(p) >= 2 and p[0] == "src":
                src = line.split(None, 1)[1].strip()      # may hold spaces
    if base_hash is None or when is None:
        return None
    return base_hash, when, src


def write_scan(path, base_hash, when, src):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("# make_patch.py --all last compared the source folder with this base at this time.\n"
                "# Files not modified since are taken as unchanged; --all --full hashes everything.\n")
        f.write("base %s\n" % base_hash)
        f.write("src %s\n" % os.path.abspath(src))
        f.write("time %d %s\n" % (when, time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(when))))


def vpath_of(arg):
    rel = arg.replace("\\", "/").strip("/").lower()
    if rel.startswith("data/"):
        rel = rel[5:]
    return rel, "data/" + rel


# --- --sync --------------------------------------------------------------------------------

def sync(src, dst):
    """Copy every file that differs from src into dst, so the client in Release\\ runs what git has."""
    if not os.path.isdir(dst):
        sys.exit("no folder %s to sync into" % dst)
    print("syncing %s -> %s ..." % (src, dst))
    copied = same = 0
    newer, failed = [], []
    for root, dirs, names in os.walk(src):
        for name in names:
            s = os.path.join(root, name)
            rel = os.path.relpath(s, src)
            key = rel.replace(os.sep, "/").lower()
            if "/" not in key or key in SYNC_SKIP:
                continue        # the tools beside the archive in Data\ itself, and the client's settings
            d = os.path.join(dst, rel)
            if os.path.isfile(d):
                ss, ds = os.stat(s), os.stat(d)
                if ss.st_size == ds.st_size and abs(ss.st_mtime - ds.st_mtime) < 2:
                    same += 1
                    continue
                if ss.st_size == ds.st_size and filecmp.cmp(s, d, shallow=False):
                    os.utime(d, (ds.st_atime, ss.st_mtime))     # same bytes: match the time, next sync skips the read
                    same += 1
                    continue
                if ds.st_mtime > ss.st_mtime + 2:
                    newer.append(rel)
                    continue
            try:
                os.makedirs(os.path.dirname(d), exist_ok=True)
                shutil.copy2(s, d)
            except OSError as e:
                failed.append("%s (%s)" % (rel, e.strerror or e))
                continue
            copied += 1
            print("  copied %s" % rel)
    print("  %d copied, %d already the same" % (copied, same))
    if newer:
        print("  %d files in %s are newer than git's copy - edited there? Left alone:" % (len(newer), dst))
        for rel in newer:
            print("    " + rel)
        print("  Copy the ones you mean to keep into %s and commit them;\n"
              "  delete the others from %s and --sync again to take git's." % (src, dst))
    if failed:
        print("  %d could not be written (is the client running?):" % len(failed))
        for f in failed:
            print("    " + f)


# --- the upload check ------------------------------------------------------------------------

def git(*args):
    """stdout of a git command run in the repository, or None when it fails."""
    env = dict(os.environ, GIT_TERMINAL_PROMPT="0")     # never sit waiting on a password prompt
    try:
        p = subprocess.run(["git", "-C", REPO, "-c", "core.quotePath=false"] + list(args),
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env, timeout=300)
    except (OSError, subprocess.TimeoutExpired):
        return None
    return p.stdout if p.returncode == 0 else None


def git_versions(gsrc, vpath):
    """sha256 of every committed version of vpath reachable from HEAD, as a checkout writes it."""
    out = git("log", "--full-history", "-m", "--format=commit %H", "--name-only", "--",
              ":(icase)%s/%s" % (gsrc, vpath[5:]))
    found, seen, commit = set(), set(), None
    for line in (out or b"").decode("utf-8", "replace").splitlines():
        if line.startswith("commit "):
            commit = line[7:].strip()
        elif line.strip() and commit and (commit, line.strip()) not in seen:
            seen.add((commit, line.strip()))
            blob = git("cat-file", "--filters", "%s:%s" % (commit, line.strip()))
            if blob is not None:        # None where that commit deleted it
                found.add(hashlib.sha256(blob).hexdigest())
    return found


def server_url(given):
    url = given or ""
    if not url and os.path.isfile(SERVER_TXT):
        with open(SERVER_TXT, encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#"):
                    url = line
                    break
    if url and not url.endswith("/"):
        url += "/"
    return url


def fetch(url):
    req = urllib.request.Request(url, headers={"Cache-Control": "no-cache"})
    with urllib.request.urlopen(req, timeout=30) as r:
        return r.read()


def deploy_check(a, header, base, files, removed):
    """Is Archive\\ as staged now safe to upload? Prints why not. -> True when it is."""
    print("\nChecking Archive\\ against git and the update server ...")
    problems, notes, oks = [], [], []

    # --- git: the Archive must be built from what everyone can pull
    try:
        gsrc = os.path.relpath(os.path.abspath(a.src), REPO).replace(os.sep, "/")
    except ValueError:              # another drive
        gsrc = None
    if gsrc is None or gsrc == ".." or gsrc.startswith("../") or git("rev-parse", "HEAD") is None:
        notes.append("%s is not in this git repository - git not checked" % a.src)
        gsrc = None
    else:
        dirty = git("status", "--porcelain", "--", gsrc)
        if dirty is None:
            notes.append("git status failed - uncommitted changes not checked")
        elif dirty.strip():
            problems.append("%d uncommitted change(s) in %s - commit and push them first, or the next\n"
                            "      person's upload will not have them (git status %s)"
                            % (len(dirty.splitlines()), gsrc, gsrc))
        upstream = git("rev-parse", "--abbrev-ref", "@{u}")
        if upstream is None:
            notes.append("this branch has no upstream - not checked whether you are behind or unpushed")
        elif git("fetch", "--quiet") is None:
            notes.append("git fetch failed - not checked whether someone pushed data you do not have")
        else:
            upstream = upstream.decode().strip()
            behind = int(git("rev-list", "--count", "HEAD..@{u}", "--", gsrc) or 0)
            ahead = int(git("rev-list", "--count", "@{u}..HEAD", "--", gsrc) or 0)
            if behind:
                problems.append("%s has %d commit(s) changing %s that you do not have - pull, then --all again"
                                % (upstream, behind, gsrc))
            if ahead:
                problems.append("%d commit(s) changing %s are not pushed to %s - push before you upload"
                                % (ahead, gsrc, upstream))
            if not behind and not ahead:
                oks.append("%s is in step with %s" % (gsrc, upstream))

    # --- the server: nothing it ships now may be lost
    url = server_url(a.server)
    deployed = None
    if not url:
        notes.append("no update server in %s - what the server ships was NOT checked" % SERVER_TXT)
    else:
        try:
            manifest = fetch(url + "manifest.txt").decode("utf-8", "replace")
            rows = [l.split(" ", 3) for l in manifest.splitlines()]
            pair = {p[3]: p[1] for p in rows if len(p) == 4 and p[0] == "base"}
            listing = next((p[3] for p in rows if len(p) == 4 and p[0] == "entries"), None)
            if listing is None:
                oks.append("the server does not patch the archive file by file yet - nothing shipped to lose")
            elif pair.get("Data/darkeden.dpk") != header["dpk"][0]:
                problems.append("the server's base archive (%s...) is not this package's base (%s...) - get its\n"
                                "      list with make_dpk.py --entries-only" % (pair.get("Data/darkeden.dpk", "?")[:16],
                                                                              header["dpk"][0][:16]))
            else:
                deployed = {}
                for line in fetch(url + "files/" + listing).decode("utf-8", "replace").splitlines():
                    p = line.split(" ", 3)
                    if len(p) == 4 and p[0] in ("P", "B"):
                        deployed[p[3]] = (p[0], p[1])
        except (OSError, ValueError) as e:
            notes.append("could not read the update server %s (%s) - what it ships was NOT checked" % (url, e))

    if deployed is not None:
        removed = set(removed)
        ours = {v: row[0] for v, row in base.items() if v not in removed}
        for root, dirs, names in os.walk(files):
            for n in names:
                p = os.path.join(root, n)
                ours[os.path.relpath(p, files).replace(os.sep, "/")] = sha256(p)
        shipped = sum(1 for flag, _ in deployed.values() if flag == "P")
        superseded, lost = [], 0
        for v in sorted(set(deployed) | set(ours)):
            flag, theirs = deployed.get(v, (None, None))
            mine = ours.get(v)
            if theirs == mine:
                continue
            if flag == "P":
                # shipped as a patch, and this upload would change it
                if mine is None:
                    what = "removes it"
                elif v in base and mine == base[v][0]:
                    what = "puts it back to the base version"
                else:
                    what = "replaces it with yours"
                if gsrc is not None and theirs in git_versions(gsrc, v):
                    superseded.append((v, what))        # git has it, and a later commit changed it again
                    continue
                lost += 1
                problems.append("%s: the server ships a version git has never seen, and this upload %s.\n"
                                "      Someone uploaded it without pushing - get it into git, pull, then --all again"
                                % (v, what))
            elif flag is None and v in base and mine is not None:
                lost += 1
                problems.append("%s was removed from the archive on the server; this upload brings it back\n"
                                "      (--remove it too, if that removal stands)" % v)
        if not lost:
            oks.append("all %d patches the server ships are kept or superseded by a newer version in git" % shipped)
        for v, what in superseded:
            print("  shipped patch %s: git has it and later commits changed it - this upload %s" % (v, what))

    for s in oks:
        print("  ok  " + s)
    for s in notes:
        print("  ??  " + s)
    for s in problems:
        print("  !!  " + s)
    return not problems


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("paths", nargs="*", help="files under ReleaseData\\Data (wildcards allowed)")
    ap.add_argument("--all", action="store_true", help="stage what differs from the base, then --check")
    ap.add_argument("--full", action="store_true",
                    help="with --all: hash every file instead of only those modified since the last scan")
    ap.add_argument("--remove", nargs="+", metavar="VPATH", help="archive entries to drop")
    ap.add_argument("--unstage", nargs="+", metavar="PATH", help="staged files to take back out")
    ap.add_argument("--check", action="store_true",
                    help="check the staged Archive\\ against git and what the update server ships")
    ap.add_argument("--server", metavar="URL", help="the update server's BaseUrl (default: update_server.txt)")
    ap.add_argument("--sync", action="store_true",
                    help="copy what differs in the source into Release\\Data, for the local client")
    ap.add_argument("--sync-to", default=DEFAULT_GAME_DATA, metavar="DIR", help="where --sync copies to")
    ap.add_argument("--src", default=DEFAULT_SRC, help="the loose Data tree (default: ReleaseData\\Data)")
    ap.add_argument("--package", default=DEFAULT_OUT)
    a = ap.parse_args()
    if not os.path.isdir(a.src):
        sys.exit("no source folder %s" % a.src)

    if a.sync:
        sync(a.src, a.sync_to)
        if not (a.paths or a.all or a.remove or a.unstage or a.check):
            return 0
        print()

    entries = os.path.join(a.package, "Data", "darkeden.entries")
    if not os.path.isfile(entries):
        sys.exit("no base list at %s\n  build the base with make_dpk.py, or list the published pair with\n"
                 "  make_dpk.py --entries-only <its Data folder> --entries-out %s" % (entries, entries))
    header, base = read_entries(entries)
    archive = os.path.join(a.package, "Archive")
    files = os.path.join(archive, "files")
    removed_txt = os.path.join(archive, "removed.txt")
    os.makedirs(files, exist_ok=True)

    # the delta belongs to this base; the server refuses a delta made for another
    base_txt = os.path.join(archive, "base.txt")
    with open(base_txt, "w", encoding="utf-8", newline="\n") as f:
        f.write(header["dpk"][0] + "\n")

    removed = []
    if os.path.isfile(removed_txt):
        removed = [l.strip() for l in open(removed_txt, encoding="utf-8") if l.strip()]

    # wildcards, expanded against the source folder (cmd and PowerShell leave them to us)
    paths = []
    for arg in a.paths:
        if "*" in arg or "?" in arg:
            hits = [h for h in glob.glob(os.path.join(a.src, arg), recursive=True) if os.path.isfile(h)]
            if not hits:
                sys.exit("nothing in %s matches %s" % (a.src, arg))
            paths += [os.path.relpath(h, a.src) for h in sorted(hits)]
        else:
            paths.append(arg)

    if a.all:
        # files modified before the last scan (of this same base) are taken as unchanged
        scan_txt = os.path.join(a.package, SCAN_TXT)
        since = None
        if a.full:
            print("comparing every archive file in %s with the base (--full) ..." % a.src)
        else:
            prev = read_scan(scan_txt)
            if prev is None:
                print("no scan recorded yet - comparing every archive file in %s with the base ..." % a.src)
            elif prev[0] != header["dpk"][0]:
                print("the base changed since the last scan - comparing every archive file in %s ..." % a.src)
            elif prev[2] is None or not same_dir(prev[2], a.src):
                print("the last scan was of %s - comparing every archive file in %s ..."
                      % (prev[2] or "Release\\Data", a.src))
            else:
                since = prev[1] - SCAN_MARGIN
                print("comparing files in %s modified since %s with the base (--full checks all) ..."
                      % (a.src, time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(prev[1]))))
        scan_start = time.time()
        hashed = untouched = 0
        in_src = set()
        for root, dirs, names in os.walk(a.src):
            for name in names:
                disk = os.path.join(root, name)
                rel = os.path.relpath(disk, a.src).replace("\\", "/").lower()
                st = os.stat(disk)
                size = st.st_size
                if classify(rel, size) != "pack":
                    continue
                vpath = "data/" + rel
                in_src.add(vpath)
                row = base.get(vpath)
                staged = os.path.join(files, *vpath.split("/"))
                if row is not None and row[1] == size:
                    # same size as the base: only a hash tells. Skip it when it was not touched
                    # since the last scan - unless it is staged, which must always be re-checked
                    if since is not None and st.st_mtime < since and not os.path.exists(staged):
                        untouched += 1
                        continue
                    hashed += 1
                    if row[0] == sha256(disk):
                        if os.path.exists(staged):
                            os.remove(staged)
                            print("  back to the base, unstaged: %s" % vpath)
                        continue
                if os.path.exists(staged) and os.path.getsize(staged) == size and sha256(staged) == sha256(disk):
                    continue        # already staged as it is now
                paths.append(rel)
        if since is not None:
            print("  %d files hashed, %d untouched since the last scan" % (hashed, untouched))
        missing = sorted(v for v in base if v not in in_src and v not in removed)
        if missing:
            print("  %d base files are no longer in %s (not removed - use --remove if meant):" % (len(missing), a.src))
            for v in missing[:20]:
                print("    " + v)

    for arg in paths:
        rel, vpath = vpath_of(arg)
        disk = os.path.join(a.src, *rel.split("/"))
        if not os.path.isfile(disk):
            sys.exit("no such file: %s" % disk)
        kind = classify(rel, os.path.getsize(disk))
        if kind != "pack":
            sys.exit("%s does not go into the archive (%s)" % (rel, kind))

        staged = os.path.join(files, *vpath.split("/"))
        h, size = sha256(disk), os.path.getsize(disk)
        if base.get(vpath) == (h, size):
            if os.path.exists(staged):
                os.remove(staged)
            print("  same as the base archive, nothing to send: %s" % vpath)
            continue
        os.makedirs(os.path.dirname(staged), exist_ok=True)
        shutil.copy2(disk, staged)
        if vpath in removed:
            removed.remove(vpath)
        print("  %-8s %s (%d bytes)" % ("changed" if vpath in base else "new", vpath, size))

    if a.all:
        write_scan(scan_txt, header["dpk"][0], scan_start, a.src)

    for arg in a.unstage or []:
        rel, vpath = vpath_of(arg)
        staged = os.path.join(files, *vpath.split("/"))
        if os.path.exists(staged):
            os.remove(staged)
            print("  unstaged %s" % vpath)

    for v in a.remove or []:
        rel, vpath = vpath_of(v)
        if vpath not in base:
            sys.exit("%s is not in the base archive" % vpath)
        if vpath not in removed:
            removed.append(vpath)
        staged = os.path.join(files, *vpath.split("/"))
        if os.path.exists(staged):
            os.remove(staged)
        print("  remove   %s" % vpath)

    with open(removed_txt, "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(v + "\n" for v in sorted(set(removed))))

    # prune empty folders, then show the whole delta
    for root, dirs, fs in os.walk(files, topdown=False):
        if root != files and not os.listdir(root):
            os.rmdir(root)
    staged = []
    for root, dirs, fs in os.walk(files):
        for n in fs:
            p = os.path.join(root, n)
            staged.append((os.path.relpath(p, files).replace(os.sep, "/"), os.path.getsize(p)))
    staged.sort()

    print("\nStaged against base %s (%d files):" % (header["dpk"][0][:16], len(base)))
    for v, s in staged:
        print("  %s  %s (%d bytes)" % ("changed" if v in base else "new    ", v, s))
    for v in sorted(set(removed)):
        print("  remove   %s" % v)
    if not staged and not removed:
        print("  nothing - players would download nothing")

    ready = deploy_check(a, header, base, files, removed) if (a.all or a.check) else None
    if ready is False:
        print("\nNOT READY TO UPLOAD. An upload replaces the server's whole Archive/, so whatever is\n"
              "missing from this one is taken away from every player. Fix the !! lines first.")
        return 1
    if staged or removed:
        print("\nplayers download %.1f KB. Upload %s to the server's game folder as Archive/\n"
              "(replace its contents), then run ./publish.sh patch there."
              % (sum(s for _, s in staged) / 1024.0, archive))
        if ready is None:
            print("Not checked against git and the update server - run --check before you upload.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
