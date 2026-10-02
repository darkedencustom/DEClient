# Packing tools — reading `darkeden.dpk`

| File | Purpose |
|---|---|
| `dpkget.cpp` | Extract entries from a `.dpk` (decompresses like the game does) |
| `dpkput.cpp` | Write entries into a `.dpk` |
| `dpkraw.cpp` | Dump an entry's **raw stored bytes**, bypassing the compress/XOR layer |
| `dpkunmask.py` | Repair raw dumps from newer archives that `dpkget` gets wrong |
| `build.bat` | Builds all three against the client's own `VirtualFileSystem.lib` |

`build.bat` needs `Release\VirtualFileSystem.lib` and `Release\MZLib.lib`, so build
the `VirtualFileSystem` project first. Output is x86, matching the client.

## Shipping a data change

`ReleaseData\Data` is the loose game data kept in git: edit and commit files there,
not in `Release\Data` (build output, gitignored). Then

```
python make_patch.py --all
```

stages everything that differs from the published base into `Release\Package\Archive\`;
upload that and run `./publish.sh patch` on the server (details in `make_patch.py`).
`make_dpk.py` and `dpk_replace.py` read the same folder; `--src` points any of them
elsewhere. The client in `Release\` reads `Release\Data`: `make_patch.py --sync`
copies git's changes there to try them in game.

**An upload replaces the server's whole `Archive/`**, so it has to hold everyone's
changes. Push before you upload, pull before you build. `--all` then checks the
result and prints `NOT READY TO UPLOAD` when ReleaseData has uncommitted, unpushed
or unpulled changes, or when the server ships a patch this Archive would lose
(read from the BaseUrl in `update_server.txt`). `publish.sh` refuses the same loss
on the server side; `ALLOW_REVERT=1 ./publish.sh patch` when it is intended.

## Newer archives extract silently corrupt

`dpkget` is correct for our own v664 archive (202 of 205 `data/ui/spk/*.spk` validate).
Against a 2019-era archive such as `D:\GitHub\sandbox_v9` **every one of its 220 UI
sprite packs is corrupt** — and it never reports an error.

The cause is in `VirtualFileSystem/VFS_Macro.h`. `encrypt_data()` XORs a masking
string over the compressed payload at `key % (size - masksize)`, `key` being a djb2
hash of the file path:

* v664 archives use a **16-byte** mask, `s2/234ESsQ^$SFT@`
* 2019 archives use a **32-byte** mask, `HGHYPWXB5BRelease01u-n:s8hrecpin`, at a
  *different* key derivation (not reproducible by djb2 over the path — still unknown)

Applying the wrong mask damages exactly 32 bytes mid-stream. zlib therefore inflates
correctly right up to those bytes and only then produces nonsense, and the client's
`uncompress()` leaves the remainder of its buffer as uninitialized heap. The result
looks like half-finished artwork rather than a failed read, which is why this went
unnoticed. Sprite dimensions in the first frames are perfectly valid.

### Recovering a newer archive

We do not know the 2019 key derivation and do not need it: zlib's Adler-32 makes a
candidate offset self-verifying, so `dpkunmask.py` locates the mask empirically.

```
dpkraw.exe darkeden rawdir data/ui/spk/new201412hpbarslayer.spk
python dpkunmask.py rawdir outdir
```

`dpkraw` accepts many paths in one run, which matters because opening a 2 GB archive
per file is slow. `dpkunmask.py` also rebuilds each `.spki` from its `.spk` — 14 of
sandbox_v9's 233 index files contain unrelated UTF-16 path fragments. That is
harmless in itself (only `CTypePack::LoadFromFileRunning` reads `.spki`; the normal
`LoadFromFile` path takes the sprite count from the `.spk` and walks it), but
regenerating keeps the pair consistent.

Multi-megabyte entries are slow — every probe costs a full inflate.

### Verifying a recovery

A recovered `.spk` should walk exactly to its own end, and the regenerated `.spki`
should match the recovered one byte for byte. Both held for all 12 UI packs recovered
so far, which is a stronger check than "it decodes without crashing".

See `Tools/Sprites/README.md` for turning the resulting `.spk` files into PNGs.
