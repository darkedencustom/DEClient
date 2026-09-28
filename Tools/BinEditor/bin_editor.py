#!/usr/bin/env python3
"""DarkEden treasure (.bin) editor: view and edit the gameserver's data/<Monster>.<race>.bin loot files.

    bin_editor.py                         open the editor window
    bin_editor.py FILE_OR_FOLDER          open the window on that .bin file or data folder
    bin_editor.py dump FILE...            print a file as a readable tree (names + per-kill chances)
    bin_editor.py check FILE_OR_FOLDER... parse every file and list the problems the server would hit
    bin_editor.py export FILE OUT.json    write a file as JSON (edit it by hand, then import)
    bin_editor.py import IN.json OUT.bin  write a .bin from JSON
    bin_editor.py roundtrip FOLDER        prove read+write is byte-exact for every .bin in the folder

Layout of a .bin (all little-endian int32, no padding; Treasure.cpp loadFromFile):
    [entry count]
      entry x N:   [ItemRatio][OptionRatio][PremiumItemRatio][PremiumOptionRatio][class count]
        class x N:   [ItemClass][Ratio][type count]
          type x N:    [ItemType][Ratio][option count]
            option x N:  [OptionType][Ratio]

How a kill rolls (MonsterManager.cpp ~2853, Treasure.cpp getRandomItem):
  * every entry rolls on its own. ItemRatio 999999 always passes; the server turns any other value into
    40000, and the pass check is rand()%100000 < 40000 * ITEM_PROBE_RATIO/100 (AttrInfo 3, 500 live),
    so 40000 also always passes at the live setting. OptionRatio is NOT scaled: 40000 = 40% of drops get
    an option, 999999 = all of them.
  * class pick: r = rand()%(sum of class ratios); class i wins when prev_sum < r < sum_i (both ends
    miss, so a class with ratio R wins (R-1)/total of the time and a roll of exactly 0 drops nothing).
  * type pick: r = rand()%100000 against the running sum of type ratios, first sum above r wins. Types
    whose running sum starts at or past 100000 can never drop; if the ratios sum below 100000 the rest of
    the picks drop nothing.
  * unique items (UniqueItemInfo) need one more roll against UNIQUE_ITEM_RATIO/10000 (AttrInfo 18).
  * option pick: r = rand()%(sum of option ratios), share = ratio/total.
  * a chief re-rolls a successful entry up to CHIEF_ITEM_BONUS_NUM (AttrInfo 55) more times.
Names come from names.json next to this program (made by tools/export_treasure_names.py in the server
repo); without it everything still works, it just shows numbers.
"""
import copy
import json
import os
import struct
import sys

RACES = ('slayer', 'vampire', 'ousters')
RATIO_MODULUS = 100000
ALWAYS = 999999
FORCED = 40000
APP_TITLE = 'DarkEden Treasure Editor'

DEFAULT_DATA_DIRS = [
    r'\\wsl.localhost\ubuntu\home\andrew\DEServer_v664\data',
    r'\\wsl$\ubuntu\home\andrew\DEServer_v664\data',
    r'D:\GitHub\DEServer_v664\data',
    os.path.expanduser('~/DEServer_v664/data'),
]


# ----------------------------------------------------------------------------------------------------
# model
# ----------------------------------------------------------------------------------------------------
class OptionEntry:
    __slots__ = ('option_type', 'ratio')

    def __init__(self, option_type=0, ratio=0):
        self.option_type = int(option_type)
        self.ratio = int(ratio)


class TypeEntry:
    __slots__ = ('item_type', 'ratio', 'options')

    def __init__(self, item_type=0, ratio=0, options=None):
        self.item_type = int(item_type)
        self.ratio = int(ratio)
        self.options = options if options is not None else []


class ClassEntry:
    __slots__ = ('item_class', 'ratio', 'types')

    def __init__(self, item_class=0, ratio=0, types=None):
        self.item_class = int(item_class)
        self.ratio = int(ratio)
        self.types = types if types is not None else []


class Treasure:
    __slots__ = ('ratios', 'classes')

    def __init__(self, ratios=None, classes=None):
        self.ratios = list(ratios) if ratios is not None else [ALWAYS] * 4
        self.classes = classes if classes is not None else []


class BinFormatError(Exception):
    pass


def parse_bin(data, name='<data>'):
    o = 0
    n = len(data)

    def i32():
        nonlocal o
        if o + 4 > n:
            raise BinFormatError('%s: file ends inside a field at byte %d of %d' % (name, o, n))
        v = struct.unpack_from('<i', data, o)[0]
        o += 4
        return v

    def count(what):
        c = i32()
        if c < 0 or c > 100000:
            raise BinFormatError('%s: bad %s count %d at byte %d' % (name, what, c, o - 4))
        return c

    treasures = []
    for _ in range(count('entry')):
        t = Treasure([i32(), i32(), i32(), i32()])
        for _ in range(count('class')):
            c = ClassEntry(i32(), i32())
            for _ in range(count('type')):
                ty = TypeEntry(i32(), i32())
                for _ in range(count('option')):
                    ty.options.append(OptionEntry(i32(), i32()))
                c.types.append(ty)
            t.classes.append(c)
        treasures.append(t)
    if o != n:
        raise BinFormatError('%s: %d trailing bytes after the last entry (parsed %d of %d)' % (name, n - o, o, n))
    return treasures


def pack_bin(treasures):
    out = [struct.pack('<i', len(treasures))]
    for t in treasures:
        out.append(struct.pack('<5i', *t.ratios, len(t.classes)))
        for c in t.classes:
            out.append(struct.pack('<3i', c.item_class, c.ratio, len(c.types)))
            for ty in c.types:
                out.append(struct.pack('<3i', ty.item_type, ty.ratio, len(ty.options)))
                for op in ty.options:
                    out.append(struct.pack('<2i', op.option_type, op.ratio))
    return b''.join(out)


def read_bin(path):
    with open(path, 'rb') as f:
        return parse_bin(f.read(), os.path.basename(path))


def write_bin(path, treasures):
    data = pack_bin(treasures)
    with open(path, 'wb') as f:
        f.write(data)
    return len(data)


def to_jsonable(treasures):
    return [{'ratios': list(t.ratios),
             'classes': [{'itemClass': c.item_class, 'ratio': c.ratio,
                          'types': [{'itemType': ty.item_type, 'ratio': ty.ratio,
                                     'options': [{'optionType': op.option_type, 'ratio': op.ratio} for op in ty.options]}
                                    for ty in c.types]}
                         for c in t.classes]}
            for t in treasures]


def from_jsonable(doc):
    treasures = []
    for t in doc:
        ratios = t.get('ratios', [ALWAYS] * 4)
        if len(ratios) != 4:
            raise BinFormatError('an entry needs exactly 4 ratios, got %r' % (ratios,))
        tr = Treasure(ratios)
        for c in t.get('classes', []):
            ce = ClassEntry(c['itemClass'], c.get('ratio', 0))
            for ty in c.get('types', []):
                te = TypeEntry(ty['itemType'], ty.get('ratio', 0))
                for op in ty.get('options', []):
                    te.options.append(OptionEntry(op['optionType'], op.get('ratio', 0)))
                ce.types.append(te)
            tr.classes.append(ce)
        treasures.append(tr)
    return treasures


# ----------------------------------------------------------------------------------------------------
# names (optional names.json)
# ----------------------------------------------------------------------------------------------------
def app_dir():
    if getattr(sys, 'frozen', False):
        return os.path.dirname(os.path.abspath(sys.executable))
    return os.path.dirname(os.path.abspath(__file__))


class Names:
    def __init__(self, path=None):
        self.path = None
        self.doc = {'item_classes': {}, 'items': {}, 'options': {}, 'monsters': [], 'server_vars': {}}
        candidates = [path] if path else []
        candidates += [os.path.join(app_dir(), 'names.json'), os.path.join(os.getcwd(), 'names.json')]
        for cand in candidates:
            if cand and os.path.isfile(cand):
                with open(cand, encoding='utf-8') as f:
                    self.doc = json.load(f)
                self.path = cand
                break
        self.item_classes = {int(k): v for k, v in self.doc.get('item_classes', {}).items()}
        self.items = {}
        self.max_type = {}
        for cid, rec in self.doc.get('items', {}).items():
            self.items[int(cid)] = {int(t): v for t, v in rec.get('types', {}).items()}
            self.max_type[int(cid)] = rec.get('max_type')
        self.options = {int(k): v for k, v in self.doc.get('options', {}).items()}
        self.monsters = self.doc.get('monsters', [])
        self.by_bin = {}
        for m in self.monsters:
            self.by_bin.setdefault(m['bin'], []).append(m)
        sv = self.doc.get('server_vars', {})
        self.probe_percent = sv.get('ITEM_PROBE_RATIO', {}).get('value', 500)
        self.premium_percent = sv.get('PREMIUM_ITEM_PROBE_PERCENT', {}).get('value', 150)
        self.unique_ratio = sv.get('UNIQUE_ITEM_RATIO', {}).get('value', 0)
        self.chief_bonus = sv.get('CHIEF_ITEM_BONUS_NUM', {}).get('value', 4)

    @property
    def loaded(self):
        return self.path is not None

    def class_name(self, cid):
        return self.item_classes.get(cid, '?')

    def class_label(self, cid):
        return '%d %s' % (cid, self.class_name(cid))

    def item(self, cid, t):
        return self.items.get(cid, {}).get(t)

    def item_name(self, cid, t):
        rec = self.item(cid, t)
        if not rec:
            return ''
        name = rec.get('e') or rec.get('n') or ''
        if rec.get('req'):
            name += ' [%s]' % rec['req']       # equip requirement from ReqAbility, e.g. [STR 120] or [M, stat sum 30]
        return name

    def item_detail(self, cid, t):
        """Longer description for the editor pane: requirement, tier, unique flag."""
        rec = self.item(cid, t)
        if not rec:
            return 'not in names.json'
        bits = []
        if rec.get('n') and rec.get('n') != rec.get('e'):
            bits.append(rec['n'])
        bits.append('requires: ' + rec['req'] if rec.get('req') else 'no requirement')
        if rec.get('tier') is not None:
            bits.append('ItemLevel %d (the server\'s tier index for level-band picks, not a player level)' % rec['tier'])
        if rec.get('unique'):
            bits.append('unique item')
        return '; '.join(bits)

    def item_status(self, cid, t):
        """'ok', 'unknown' (server logs Invalid Item Type, ratio -> 0), 'gap' (passes the check, crashes on
        create), or '' when we have no table for the class."""
        if cid not in self.items:
            return ''
        mx = self.max_type.get(cid)
        if mx is None:
            return 'unknown'
        if t > mx or t < 0:
            return 'unknown'
        return 'ok' if t in self.items[cid] else 'gap'

    def is_unique(self, cid, t):
        rec = self.item(cid, t)
        return bool(rec and rec.get('unique'))

    def option_name(self, o):
        rec = self.options.get(o)
        if not rec:
            return ''
        s = rec.get('e') or rec.get('n') or ''
        if rec.get('nick'):
            s += ' (%s)' % rec['nick']
        return s

    def item_choices(self, cid):
        return ['%d  %s' % (t, self.item_name(cid, t)) for t in sorted(self.items.get(cid, {}))]

    def class_choices(self):
        return ['%d  %s' % (cid, self.item_classes[cid]) for cid in sorted(self.item_classes)]

    def option_choices(self):
        return ['%d  %s' % (o, self.option_name(o)) for o in sorted(self.options)]

    def bin_info(self, base):
        """Short description of which monsters load <base>.<race>.bin."""
        ms = self.by_bin.get(base)
        if not ms:
            return ''
        # the level range of the regular field monsters; chiefs and event-only variants of a tier can be far above
        regular = [m for m in ms if not m['chief'] and m['regen']] or ms
        levels = [m['lvl'] for m in regular if m['lvl']]
        names = []
        seen = set()
        for m in regular + [m for m in ms if m not in regular]:
            n = m['e'].split('[')[0].strip()
            if n not in seen:
                seen.add(n)
                names.append(n)
        s = ''
        if levels:
            lo, hi = min(levels), max(levels)
            s = 'lv %d' % lo if lo == hi else 'lv %d-%d' % (lo, hi)
        if base.startswith('Class'):
            others = len(ms) - len(regular)
            s += ' · %d monsters' % len(regular) + (' (+%d chiefs/variants)' % others if others else '') + ': ' + \
                 ', '.join(names[:8]) + (', ...' if len(names) > 8 else '')
        else:
            s += ' · type ' + ', '.join(str(m['t']) for m in ms[:6])
            if any(m['chief'] for m in ms):
                s += ' · chief'
        return s.strip(' ·')


# ----------------------------------------------------------------------------------------------------
# roll math
# ----------------------------------------------------------------------------------------------------
def entry_pass(item_ratio, probe_percent):
    """Chance the entry passes its own roll on the normal (non-premium) path."""
    if item_ratio == ALWAYS:
        return 1.0
    return min(1.0, FORCED * probe_percent / 100.0 / RATIO_MODULUS)


def option_chance(option_ratio):
    if option_ratio == ALWAYS:
        return 1.0
    return min(1.0, FORCED / float(RATIO_MODULUS))


def class_probs(treasure):
    total = sum(c.ratio for c in treasure.classes)
    if total <= 0:
        return [0.0] * len(treasure.classes), total
    return [max(0, c.ratio - 1) / float(total) for c in treasure.classes], total


def type_probs(cls):
    probs = []
    cum = 0
    for ty in cls.types:
        lo = cum
        cum += ty.ratio
        hi = min(cum, RATIO_MODULUS)
        probs.append(max(0, hi - lo) / float(RATIO_MODULUS) if lo < RATIO_MODULUS else 0.0)
    return probs, cum


def option_probs(ty):
    total = sum(op.ratio for op in ty.options)
    if total <= 0:
        return [0.0] * len(ty.options), total
    return [op.ratio / float(total) for op in ty.options], total


def fmt_chance(p):
    if p <= 0:
        return 'never'
    if p >= 0.9999:
        return 'always'
    if p >= 0.01:
        s = '%.2f%%' % (p * 100)
    elif p >= 0.0001:
        s = '%.3f%%' % (p * 100)
    else:
        s = '%.5f%%' % (p * 100)
    return '%s (1 in %s)' % (s, '{:,}'.format(int(round(1 / p))))


def analyse(treasures, names):
    """Per-node chances, warnings (things the server will choke on or that silently never drop) and notes
    (facts worth seeing but usually by design). Returns dict path-tuple -> {'chance', 'share', 'warn', 'note', ...}."""
    info = {}
    for ei, t in enumerate(treasures):
        pe = entry_pass(t.ratios[0], names.probe_percent)
        warns, notes = [], []
        if t.ratios[0] not in (ALWAYS, FORCED):
            notes.append('ItemRatio %d is stored as %d by the server' % (t.ratios[0], FORCED))
        if not t.classes:
            notes.append('entry has no classes: drops nothing')
        cps, ctotal = class_probs(t)
        if t.classes and ctotal <= 0:
            notes.append('class ratios sum to 0: drops nothing')
        info[(ei,)] = {'chance': pe, 'warn': warns, 'note': notes, 'share': pe,
                       'option': option_chance(t.ratios[1])}
        for ci, c in enumerate(t.classes):
            cw, cn = [], []
            tps, tcum = type_probs(c)
            if c.ratio == 0:
                cn.append('ratio 0: disabled')
            elif c.ratio == 1 and ctotal > 0:
                cw.append('ratio 1 never wins the class pick (needs 2 or more)')
            if not c.types:
                (cw if c.ratio > 1 else cn).append('class has no types: a win drops nothing')
            elif tcum > RATIO_MODULUS:
                cw.append('type ratios sum to %s (> 100000): the last types can never drop' % '{:,}'.format(tcum))
            elif tcum < RATIO_MODULUS:
                cn.append('type ratios sum to %s: %.1f%% of class wins drop nothing' % ('{:,}'.format(tcum), (1 - tcum / float(RATIO_MODULUS)) * 100))
            if names.loaded and c.item_class not in names.item_classes:
                cw.append('unknown item class')
            info[(ei, ci)] = {'chance': pe * cps[ci], 'share': cps[ci], 'warn': cw, 'note': cn, 'total': ctotal, 'tcum': tcum}
            for ti, ty in enumerate(c.types):
                tw, tn = [], []
                p = pe * cps[ci] * tps[ti]
                st = names.item_status(c.item_class, ty.item_type)
                if st == 'unknown':
                    tw.append('unknown item type: the server logs "Invalid Item Type" and sets its ratio to 0')
                elif st == 'gap':
                    tw.append('no row for this type in %s: creating it would crash the server' % (names.doc['items'].get(str(c.item_class), {}).get('table') or 'the info table'))
                if tps[ti] == 0 and ty.ratio > 0 and c.ratio > 1:
                    tw.append('unreachable: earlier types already fill the 100000 range')
                if names.is_unique(c.item_class, ty.item_type):
                    p *= names.unique_ratio / 10000.0
                    tn.append('unique item: extra roll UNIQUE_ITEM_RATIO %d/10000' % names.unique_ratio)
                ops, ototal = option_probs(ty)
                if ty.options and ototal <= 0:
                    tw.append('option ratios sum to 0: options never apply')
                info[(ei, ci, ti)] = {'chance': p, 'share': tps[ti], 'warn': tw, 'note': tn, 'ototal': ototal}
                for oi, op in enumerate(ty.options):
                    ow = []
                    if names.loaded and op.option_type not in names.options:
                        ow.append('unknown option type: the server sets its ratio to 0')
                    info[(ei, ci, ti, oi)] = {'chance': ops[oi], 'share': ops[oi], 'warn': ow, 'note': []}
    return info


def remarks(i):
    """Warnings and notes of one node, warnings first."""
    return list(i.get('warn', [])) + list(i.get('note', []))


# ----------------------------------------------------------------------------------------------------
# text output (CLI + copy)
# ----------------------------------------------------------------------------------------------------
def dump_text(treasures, names, title=''):
    info = analyse(treasures, names)
    lines = []
    if title:
        lines.append(title)
    lines.append('%d entries (ITEM_PROBE_RATIO %d%%)' % (len(treasures), names.probe_percent))
    for ei, t in enumerate(treasures):
        i = info[(ei,)]
        lines.append('Entry %d  ratios %s  pass %s  option chance %s%s' % (
            ei + 1, '/'.join(str(r) for r in t.ratios), fmt_chance(i['chance']), fmt_chance(i['option']), marks(i)))
        for ci, c in enumerate(t.classes):
            i = info[(ei, ci)]
            lines.append('  class %-4d %-24s ratio %-7d share %-22s%s' % (
                c.item_class, names.class_name(c.item_class), c.ratio, fmt_chance(i['share']), marks(i)))
            for ti, ty in enumerate(c.types):
                i = info[(ei, ci, ti)]
                lines.append('    type %-5d %-32s ratio %-7d per kill %-24s%s' % (
                    ty.item_type, names.item_name(c.item_class, ty.item_type)[:32], ty.ratio, fmt_chance(i['chance']), marks(i)))
                for oi, op in enumerate(ty.options):
                    i = info[(ei, ci, ti, oi)]
                    lines.append('      option %-4d %-28s ratio %-7d share %s%s' % (
                        op.option_type, names.option_name(op.option_type)[:28], op.ratio, fmt_chance(i['share']), marks(i)))
    return '\n'.join(lines)


def marks(i):
    return ''.join('  !! ' + w for w in i['warn']) + ''.join('  -- ' + n for n in i['note'])


def collect_bins(paths):
    files = []
    for p in paths:
        if os.path.isdir(p):
            for n in sorted(os.listdir(p)):
                if n.lower().endswith('.bin'):
                    files.append(os.path.join(p, n))
        else:
            files.append(p)
    return files


def cli(argv):
    cmd = argv[0]
    names = Names()
    out = sys.stdout
    if cmd == 'dump':
        for f in collect_bins(argv[1:]):
            out.write(dump_text(read_bin(f), names, '== ' + f) + '\n\n')
        return 0
    if cmd == 'export':
        treasures = read_bin(argv[1])
        with open(argv[2], 'w', encoding='utf-8') as f:
            json.dump(to_jsonable(treasures), f, indent=1)
        out.write('%s: %d entries -> %s\n' % (argv[1], len(treasures), argv[2]))
        return 0
    if cmd == 'import':
        with open(argv[1], encoding='utf-8') as f:
            treasures = from_jsonable(json.load(f))
        n = write_bin(argv[2], treasures)
        out.write('%s: %d entries, %d bytes -> %s\n' % (argv[1], len(treasures), n, argv[2]))
        return 0
    if cmd == 'check':
        bad = 0
        for f in collect_bins(argv[1:]):
            try:
                treasures = read_bin(f)
            except (BinFormatError, OSError) as e:
                out.write('%s: ERROR %s\n' % (f, e))
                bad += 1
                continue
            info = analyse(treasures, names)
            base = os.path.basename(f).split('.')[0]
            if names.loaded and base not in names.by_bin:
                out.write('%s: no monster in names.json loads this file\n' % os.path.basename(f))
            for path, i in sorted(info.items()):
                for w in i['warn']:
                    out.write('%s: %s: %s\n' % (os.path.basename(f), '/'.join(str(x + 1) for x in path), w))
                    bad += 1
        out.write('%d warning(s) (notes are not listed; the window shows them in grey)\n' % bad)
        return 1 if bad else 0
    if cmd == 'roundtrip':
        n = ok = 0
        for f in collect_bins(argv[1:]):
            n += 1
            data = open(f, 'rb').read()
            try:
                if pack_bin(parse_bin(data, f)) == data:
                    ok += 1
                else:
                    out.write('%s: repacked bytes differ\n' % f)
            except BinFormatError as e:
                out.write('%s\n' % e)
        out.write('%d of %d files round-trip byte-exact\n' % (ok, n))
        return 0 if ok == n else 1
    out.write(__doc__)
    return 2


# ----------------------------------------------------------------------------------------------------
# GUI
# ----------------------------------------------------------------------------------------------------
def settings_path():
    return os.path.join(app_dir(), 'bin_editor.settings.json')


def load_settings():
    try:
        with open(settings_path(), encoding='utf-8') as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def save_settings(s):
    try:
        with open(settings_path(), 'w', encoding='utf-8') as f:
            json.dump(s, f, indent=1)
    except OSError:
        pass


def find_data_dir(settings):
    cands = ([settings['data_dir']] if settings.get('data_dir') else []) + DEFAULT_DATA_DIRS
    for c in cands:
        if os.path.isdir(c):
            return c
    return ''


def run_gui(start=None):
    import tkinter as tk
    from tkinter import ttk, filedialog, messagebox, simpledialog

    class RaceView(ttk.Frame):
        """One race's treasure list: tree on top, editor for the selected node underneath."""

        def __init__(self, master, app, race):
            super().__init__(master)
            self.app = app
            self.race = race
            self.path = None
            self.treasures = []
            self.original = b''
            self.undo = []
            self.info = {}
            self.opened = set()     # type rows the user expanded
            self.closed = set()     # entry/class rows the user collapsed
            self.selected = None

            top = ttk.Frame(self)
            top.pack(fill='x', padx=4, pady=(4, 0))
            self.file_label = ttk.Label(top, text='(no file)', anchor='w')
            self.file_label.pack(side='left', fill='x', expand=True)

            body = ttk.Panedwindow(self, orient='vertical')
            body.pack(fill='both', expand=True, padx=4, pady=4)

            tf = ttk.Frame(body)
            cols = ('id', 'name', 'ratio', 'chance', 'note')
            self.tree = ttk.Treeview(tf, columns=cols, selectmode='browse')
            self.tree.heading('#0', text='Node')
            self.tree.heading('id', text='ID')
            self.tree.heading('name', text='Name')
            self.tree.heading('ratio', text='Ratio')
            self.tree.heading('chance', text='Per kill / share')
            self.tree.heading('note', text='Notes')
            self.tree.column('#0', width=150, stretch=False)
            self.tree.column('id', width=55, anchor='e', stretch=False)
            self.tree.column('name', width=230)
            self.tree.column('ratio', width=80, anchor='e', stretch=False)
            self.tree.column('chance', width=170, stretch=False)
            self.tree.column('note', width=320)
            self.tree.tag_configure('warn', foreground='#b00020')
            self.tree.tag_configure('note', foreground='#6a6a6a')
            self.tree.tag_configure('entry', font=('TkDefaultFont', 9, 'bold'))
            self.tree.tag_configure('dead', foreground='#9a9a9a')
            sb = ttk.Scrollbar(tf, orient='vertical', command=self.tree.yview)
            self.tree.configure(yscrollcommand=sb.set)
            self.tree.pack(side='left', fill='both', expand=True)
            sb.pack(side='right', fill='y')
            self.tree.bind('<<TreeviewSelect>>', self.on_select)
            self.tree.bind('<<TreeviewOpen>>', lambda e: self.remember_open(True))
            self.tree.bind('<<TreeviewClose>>', lambda e: self.remember_open(False))
            self.tree.bind('<Delete>', lambda e: self.delete_node())
            self.tree.bind('<Double-1>', lambda e: self.focus_editor())
            body.add(tf, weight=3)

            self.editor = ttk.LabelFrame(body, text='Selected node')
            body.add(self.editor, weight=1)
            self.build_editor()

            btns = ttk.Frame(self)
            btns.pack(fill='x', padx=4, pady=(0, 4))
            for text, cmd in (('+ Entry', self.add_entry), ('+ Class', self.add_class), ('+ Type', self.add_type),
                              ('+ Option', self.add_option), ('Duplicate', self.duplicate), ('Delete', self.delete_node),
                              ('Up', lambda: self.move(-1)), ('Down', lambda: self.move(1)), ('Undo', self.do_undo)):
                ttk.Button(btns, text=text, command=cmd, width=9).pack(side='left', padx=1)
            self.summary = ttk.Label(btns, text='', anchor='e')
            self.summary.pack(side='right', fill='x', expand=True)

        # ---- editor pane -------------------------------------------------------------------------
        def build_editor(self):
            e = self.editor
            self.kind_label = ttk.Label(e, text='Nothing selected', font=('TkDefaultFont', 9, 'bold'))
            self.kind_label.grid(row=0, column=0, columnspan=4, sticky='w', padx=6, pady=(4, 2))

            # entry fields
            self.ratio_vars = [tk.StringVar() for _ in range(4)]
            self.ratio_widgets = []
            for i, lab in enumerate(('ItemRatio', 'OptionRatio', 'PremiumItemRatio', 'PremiumOptionRatio')):
                l = ttk.Label(e, text=lab)
                en = ttk.Entry(e, textvariable=self.ratio_vars[i], width=10)
                en.bind('<Return>', lambda ev: self.apply())
                en.bind('<FocusOut>', lambda ev: self.apply(quiet=True))
                self.ratio_widgets.append((l, en))
            self.always_btn = ttk.Button(e, text='Set all 999999 (always)', command=self.set_always)
            self.forced_btn = ttk.Button(e, text='Set all 40000', command=self.set_forced)

            # class / type / option fields
            self.id_var = tk.StringVar()
            self.id_label = ttk.Label(e, text='Item class')
            self.id_combo = ttk.Combobox(e, textvariable=self.id_var, width=48)
            self.id_combo.bind('<<ComboboxSelected>>', lambda ev: self.apply())
            self.id_combo.bind('<Return>', lambda ev: self.apply())
            self.id_combo.bind('<FocusOut>', lambda ev: self.apply(quiet=True))
            self.one_ratio_var = tk.StringVar()
            self.one_ratio_label = ttk.Label(e, text='Ratio')
            self.one_ratio = ttk.Entry(e, textvariable=self.one_ratio_var, width=10)
            self.one_ratio.bind('<Return>', lambda ev: self.apply())
            self.one_ratio.bind('<FocusOut>', lambda ev: self.apply(quiet=True))
            self.apply_btn = ttk.Button(e, text='Apply', command=self.apply)

            self.help = ttk.Label(e, text='', wraplength=700, justify='left', foreground='#444444')
            self.help.grid(row=5, column=0, columnspan=4, sticky='w', padx=6, pady=(4, 6))
            e.columnconfigure(3, weight=1)

        def show_editor(self, kind):
            for w in self.editor.grid_slaves():
                if w not in (self.kind_label, self.help):
                    w.grid_forget()
            if kind == 'entry':
                for i, (l, en) in enumerate(self.ratio_widgets):
                    l.grid(row=1 + i // 2, column=(i % 2) * 2, sticky='w', padx=6)
                    en.grid(row=1 + i // 2, column=(i % 2) * 2 + 1, sticky='w', padx=2, pady=1)
                self.always_btn.grid(row=3, column=0, columnspan=2, sticky='w', padx=6, pady=2)
                self.forced_btn.grid(row=3, column=2, columnspan=2, sticky='w', padx=6, pady=2)
                self.apply_btn.grid(row=4, column=0, sticky='w', padx=6, pady=2)
            elif kind in ('class', 'type', 'option'):
                self.id_label.config(text={'class': 'Item class', 'type': 'Item type', 'option': 'Option type'}[kind])
                self.id_label.grid(row=1, column=0, sticky='w', padx=6)
                self.id_combo.grid(row=1, column=1, columnspan=3, sticky='w', padx=2, pady=1)
                self.one_ratio_label.grid(row=2, column=0, sticky='w', padx=6)
                self.one_ratio.grid(row=2, column=1, sticky='w', padx=2, pady=1)
                self.apply_btn.grid(row=2, column=2, sticky='w', padx=6, pady=2)

        def focus_editor(self):
            node = self.node_at(self.selected)
            if node is None:
                return
            if isinstance(node, Treasure):
                self.ratio_widgets[0][1].focus_set()
            else:
                self.one_ratio.focus_set()
                self.one_ratio.select_range(0, 'end')

        def on_select(self, event=None):
            sel = self.tree.selection()
            self.selected = tuple(int(x) for x in sel[0].split('/')) if sel else None
            self.load_editor()

        def load_editor(self):
            names = self.app.names
            node = self.node_at(self.selected)
            i = self.info.get(self.selected, {})
            if node is None:
                self.kind_label.config(text='Nothing selected')
                self.show_editor(None)
                self.help.config(text='Select a row to edit it. Buttons add a child under the selected row.')
                return
            if isinstance(node, Treasure):
                self.kind_label.config(text='Entry %d  (each entry rolls independently on every kill)' % (self.selected[0] + 1))
                for v, r in zip(self.ratio_vars, node.ratios):
                    v.set(str(r))
                self.show_editor('entry')
                self.help.config(text='Pass chance %s, option chance %s. 999999 = always. Anything else is stored as 40000 by the '
                                      'server, which passes rand()%%100000 < 40000*ITEM_PROBE_RATIO(%d%%)/100 = %s.'
                                      % (fmt_chance(i.get('chance', 0)), fmt_chance(i.get('option', 0)), names.probe_percent,
                                         fmt_chance(entry_pass(FORCED, names.probe_percent))))
            elif isinstance(node, ClassEntry):
                self.kind_label.config(text='Item class in entry %d' % (self.selected[0] + 1))
                self.id_combo['values'] = names.class_choices()
                self.id_var.set('%d  %s' % (node.item_class, names.class_name(node.item_class)))
                self.one_ratio_var.set(str(node.ratio))
                self.show_editor('class')
                self.help.config(text='Share of this entry: %s (ratio %d of %s; a class wins when prev_sum < rand < sum, so ratio-1 '
                                      'of total). Type ratios below sum to %s of 100000.'
                                      % (fmt_chance(i.get('share', 0)), node.ratio, '{:,}'.format(i.get('total', 0)),
                                         '{:,}'.format(i.get('tcum', 0))))
            elif isinstance(node, TypeEntry):
                cls = self.node_at(self.selected[:2])
                self.kind_label.config(text='Item type in class %s' % names.class_label(cls.item_class))
                self.id_combo['values'] = names.item_choices(cls.item_class)
                self.id_var.set('%d  %s' % (node.item_type, names.item_name(cls.item_class, node.item_type)))
                self.one_ratio_var.set(str(node.ratio))
                self.show_editor('type')
                self.help.config(text='%s\nPer kill: %s. Within the class: %s (ratio %d out of a fixed 100000).%s'
                                 % (names.item_detail(cls.item_class, node.item_type),
                                    fmt_chance(i.get('chance', 0)), fmt_chance(i.get('share', 0)), node.ratio,
                                    ''.join('\n! ' + w for w in remarks(i))))
            else:
                self.kind_label.config(text='Option on the item type above')
                self.id_combo['values'] = names.option_choices()
                self.id_var.set('%d  %s' % (node.option_type, names.option_name(node.option_type)))
                self.one_ratio_var.set(str(node.ratio))
                self.show_editor('option')
                self.help.config(text='Share among this type\'s options: %s (ratio %d of %s). Options only apply when the entry\'s '
                                      'OptionRatio roll passes.' % (fmt_chance(i.get('share', 0)), node.ratio,
                                                                     '{:,}'.format(sum(o.ratio for o in self.node_at(self.selected[:3]).options))))

        @staticmethod
        def parse_id(s):
            s = s.strip()
            head = s.split()[0] if s else ''
            try:
                return int(head)
            except ValueError:
                return None

        def apply(self, quiet=False):
            node = self.node_at(self.selected)
            if node is None:
                return
            try:
                if isinstance(node, Treasure):
                    new = [int(v.get().strip()) for v in self.ratio_vars]
                    if new != node.ratios:
                        self.snapshot()
                        node.ratios = new
                        self.changed()
                    return
                ident = self.parse_id(self.id_var.get())
                ratio = int(self.one_ratio_var.get().strip())
                if ident is None:
                    raise ValueError('pick an id from the list or type a number')
            except ValueError as e:
                if not quiet:
                    messagebox.showerror(APP_TITLE, 'Not a number: %s' % e, parent=self.app)
                return
            if isinstance(node, ClassEntry):
                cur = (node.item_class, node.ratio)
                if cur != (ident, ratio):
                    self.snapshot()
                    node.item_class, node.ratio = ident, ratio
                    self.changed()
            elif isinstance(node, TypeEntry):
                if (node.item_type, node.ratio) != (ident, ratio):
                    self.snapshot()
                    node.item_type, node.ratio = ident, ratio
                    self.changed()
            else:
                if (node.option_type, node.ratio) != (ident, ratio):
                    self.snapshot()
                    node.option_type, node.ratio = ident, ratio
                    self.changed()

        def set_always(self):
            for v in self.ratio_vars:
                v.set(str(ALWAYS))
            self.apply()

        def set_forced(self):
            for v in self.ratio_vars:
                v.set(str(FORCED))
            self.apply()

        # ---- model helpers ------------------------------------------------------------------------
        def node_at(self, path):
            if not path:
                return None
            try:
                node = self.treasures[path[0]]
                if len(path) > 1:
                    node = node.classes[path[1]]
                if len(path) > 2:
                    node = node.types[path[2]]
                if len(path) > 3:
                    node = node.options[path[3]]
                return node
            except IndexError:
                return None

        def container(self, path):
            """The list that holds the node at path."""
            if len(path) == 1:
                return self.treasures
            parent = self.node_at(path[:-1])
            return getattr(parent, {Treasure: 'classes', ClassEntry: 'types', TypeEntry: 'options'}[type(parent)])

        def snapshot(self):
            self.undo.append(pack_bin(self.treasures))
            if len(self.undo) > 200:
                del self.undo[0]

        def do_undo(self):
            if not self.undo:
                return
            self.treasures = parse_bin(self.undo.pop())
            self.changed()

        @property
        def dirty(self):
            if self.path is None:
                return False
            if self.original is None:          # file does not exist yet
                return bool(self.treasures)
            return pack_bin(self.treasures) != self.original

        def changed(self):
            self.refresh()
            self.app.update_title()

        def load(self, path):
            self.path = path
            self.undo = []
            self.selected = None
            self.opened = set()
            self.closed = set()
            if path and os.path.isfile(path):
                try:
                    self.treasures = read_bin(path)
                    self.original = pack_bin(self.treasures)
                    self.file_label.config(text=path, foreground='')
                except BinFormatError as e:
                    self.treasures = []
                    self.original = b''
                    self.file_label.config(text='%s  -- %s' % (path, e), foreground='#b00020')
            else:
                self.treasures = []
                self.original = None
                self.file_label.config(text='%s  (missing: will be created on save)' % path if path else '(no file)',
                                       foreground='#b06000')
            self.refresh()
            self.load_editor()

        def save(self):
            if self.path is None:
                return False
            n = write_bin(self.path, self.treasures)
            self.original = pack_bin(self.treasures)
            self.file_label.config(text=self.path, foreground='')
            self.app.status('saved %s (%d bytes)' % (self.path, n))
            return True

        def revert(self):
            if self.path:
                self.load(self.path)

        # ---- tree -----------------------------------------------------------------------------------
        def remember_open(self, is_open):
            iid = self.tree.focus()
            if not iid:
                return
            if is_open:
                self.opened.add(iid)
                self.closed.discard(iid)
            else:
                self.closed.add(iid)
                self.opened.discard(iid)

        def refresh(self):
            names = self.app.names
            self.info = analyse(self.treasures, names)
            tree = self.tree
            tree.delete(*tree.get_children(''))
            def tags_for(i, *extra):
                if i['warn']:
                    return ['warn'] + list(extra)
                if i['note']:
                    return ['note'] + list(extra)
                return list(extra)

            for ei, t in enumerate(self.treasures):
                i = self.info[(ei,)]
                iid = str(ei)
                tree.insert('', 'end', iid=iid, text='Entry %d' % (ei + 1), open=(iid not in self.closed),
                            values=('', '%d classes' % len(t.classes), '/'.join(str(r) for r in t.ratios),
                                    'pass ' + fmt_chance(i['chance']),
                                    '; '.join(remarks(i)) or 'option chance ' + fmt_chance(i['option'])),
                            tags=['entry'] + tags_for(i))
                for ci, c in enumerate(t.classes):
                    i = self.info[(ei, ci)]
                    cid = '%d/%d' % (ei, ci)
                    tree.insert(iid, 'end', iid=cid, text='class', open=(cid not in self.closed),
                                values=(c.item_class, names.class_name(c.item_class), c.ratio,
                                        'share ' + fmt_chance(i['share']),
                                        '; '.join(remarks(i)) or '%d types, ratios sum %s' % (len(c.types), '{:,}'.format(i['tcum']))),
                                tags=tags_for(i))
                    for ti, ty in enumerate(c.types):
                        i = self.info[(ei, ci, ti)]
                        tid = '%d/%d/%d' % (ei, ci, ti)
                        tags = tags_for(i)
                        if i['chance'] <= 0 and not tags:
                            tags.append('dead')
                        tree.insert(cid, 'end', iid=tid, text='type', open=(tid in self.opened),
                                    values=(ty.item_type, names.item_name(c.item_class, ty.item_type), ty.ratio,
                                            fmt_chance(i['chance']),
                                            '; '.join(remarks(i)) or ('%d options' % len(ty.options) if ty.options else '')),
                                    tags=tags)
                        for oi, op in enumerate(ty.options):
                            i = self.info[(ei, ci, ti, oi)]
                            oid = '%d/%d/%d/%d' % (ei, ci, ti, oi)
                            tree.insert(tid, 'end', iid=oid, text='option',
                                        values=(op.option_type, names.option_name(op.option_type), op.ratio,
                                                'share ' + fmt_chance(i['share']), '; '.join(remarks(i))),
                                        tags=tags_for(i))
            if self.selected is not None:
                iid = '/'.join(str(x) for x in self.selected)
                if tree.exists(iid):
                    tree.selection_set(iid)
                    tree.focus(iid)
                    tree.see(iid)
                else:
                    self.selected = None
            nwarn = sum(len(i['warn']) for i in self.info.values())
            nnote = sum(len(i['note']) for i in self.info.values())
            ntypes = sum(len(c.types) for t in self.treasures for c in t.classes)
            self.summary.config(text='%d entries, %d item types, %d warning(s), %d note(s)%s' % (
                len(self.treasures), ntypes, nwarn, nnote, '   [modified]' if self.dirty else ''),
                foreground='#b00020' if nwarn else '')

        def select_path(self, path):
            self.selected = path
            for k in range(1, len(path or ())):
                iid = '/'.join(str(x) for x in path[:k])
                self.opened.add(iid)
                self.closed.discard(iid)
            self.refresh()
            self.load_editor()

        # ---- editing commands -----------------------------------------------------------------------
        def add_entry(self):
            self.snapshot()
            self.treasures.append(Treasure())
            self.select_path((len(self.treasures) - 1,))
            self.app.update_title()

        def add_class(self):
            if not self.selected:
                if not self.treasures:
                    self.add_entry()
                else:
                    self.select_path((len(self.treasures) - 1,))
            t = self.node_at(self.selected[:1])
            self.snapshot()
            t.classes.append(ClassEntry(0, 0))
            self.select_path((self.selected[0], len(t.classes) - 1))
            self.app.update_title()

        def add_type(self):
            if not self.selected or len(self.selected) < 2:
                messagebox.showinfo(APP_TITLE, 'Select a class (or something inside one) first.', parent=self.app)
                return
            c = self.node_at(self.selected[:2])
            self.snapshot()
            c.types.append(TypeEntry(0, 0))
            self.select_path((self.selected[0], self.selected[1], len(c.types) - 1))
            self.app.update_title()

        def add_option(self):
            if not self.selected or len(self.selected) < 3:
                messagebox.showinfo(APP_TITLE, 'Select an item type (or an option under one) first.', parent=self.app)
                return
            ty = self.node_at(self.selected[:3])
            self.snapshot()
            ty.options.append(OptionEntry(0, 0))
            self.select_path(self.selected[:3] + (len(ty.options) - 1,))
            self.app.update_title()

        def duplicate(self):
            if not self.selected:
                return
            lst = self.container(self.selected)
            self.snapshot()
            lst.insert(self.selected[-1] + 1, copy.deepcopy(lst[self.selected[-1]]))
            self.select_path(self.selected[:-1] + (self.selected[-1] + 1,))
            self.app.update_title()

        def delete_node(self):
            if not self.selected:
                return
            lst = self.container(self.selected)
            self.snapshot()
            del lst[self.selected[-1]]
            idx = min(self.selected[-1], len(lst) - 1)
            if idx >= 0:
                self.select_path(self.selected[:-1] + (idx,))
            else:
                self.select_path(self.selected[:-1] or None)
            self.app.update_title()

        def move(self, delta):
            if not self.selected:
                return
            lst = self.container(self.selected)
            i = self.selected[-1]
            j = i + delta
            if j < 0 or j >= len(lst):
                return
            self.snapshot()
            lst[i], lst[j] = lst[j], lst[i]
            self.select_path(self.selected[:-1] + (j,))
            self.app.update_title()

    class App(tk.Tk):
        def __init__(self):
            super().__init__()
            self.settings = load_settings()
            self.names = Names(self.settings.get('names'))
            if self.settings.get('probe_percent'):
                self.names.probe_percent = int(self.settings['probe_percent'])
            self.data_dir = ''
            self.base = None
            self.bases = []
            self.title(APP_TITLE)
            self.geometry(self.settings.get('geometry', '1250x780'))
            self.build()
            self.protocol('WM_DELETE_WINDOW', self.on_close)
            self.bind_all('<Control-s>', lambda e: self.save())
            self.bind_all('<Control-z>', lambda e: self.current_view().do_undo())
            self.bind_all('<Control-f>', lambda e: (self.filter_entry.focus_set(), self.filter_entry.select_range(0, 'end')))
            d = find_data_dir(self.settings)
            if d:
                self.set_data_dir(d)
            self.status('names: %s' % (self.names.path or 'names.json not found (numbers only)'))

        def build(self):
            m = tk.Menu(self)
            fm = tk.Menu(m, tearoff=0)
            fm.add_command(label='Open data folder...', command=self.choose_data_dir)
            fm.add_command(label='Open .bin file...', command=self.open_file)
            fm.add_separator()
            fm.add_command(label='Save (all three races)', accelerator='Ctrl+S', command=self.save)
            fm.add_command(label='Revert', command=self.revert)
            fm.add_separator()
            fm.add_command(label='New monster loot...', command=self.new_base)
            fm.add_separator()
            fm.add_command(label='Exit', command=self.on_close)
            m.add_cascade(label='File', menu=fm)
            em = tk.Menu(m, tearoff=0)
            em.add_command(label='Undo', accelerator='Ctrl+Z', command=lambda: self.current_view().do_undo())
            em.add_separator()
            em.add_command(label='Copy this race to the other two', command=self.copy_race)
            em.add_command(label='Copy this race to...', command=self.copy_race_to)
            em.add_separator()
            em.add_command(label='Export this race as JSON...', command=self.export_json)
            em.add_command(label='Import JSON into this race...', command=self.import_json)
            em.add_command(label='Copy this race as text', command=self.copy_text)
            m.add_cascade(label='Edit', menu=em)
            tm = tk.Menu(m, tearoff=0)
            tm.add_command(label='Check every file in the data folder', command=self.check_all)
            tm.add_command(label='Set ITEM_PROBE_RATIO used for the chances...', command=self.set_probe)
            tm.add_command(label='Reload names.json', command=self.reload_names)
            tm.add_command(label='Pick names.json...', command=self.pick_names)
            m.add_cascade(label='Tools', menu=tm)
            hm = tk.Menu(m, tearoff=0)
            hm.add_command(label='How the rolls work', command=lambda: self.show_text('How the rolls work', __doc__))
            m.add_cascade(label='Help', menu=hm)
            self.config(menu=m)

            bar = ttk.Frame(self)
            bar.pack(fill='x', padx=6, pady=4)
            ttk.Label(bar, text='Data folder:').pack(side='left')
            self.dir_label = ttk.Label(bar, text='(none)', foreground='#444444')
            self.dir_label.pack(side='left', padx=4)
            ttk.Button(bar, text='Browse...', command=self.choose_data_dir).pack(side='left')
            ttk.Label(bar, text='   Filter:').pack(side='left')
            self.filter_var = tk.StringVar()
            self.filter_var.trace_add('write', lambda *a: self.fill_list())
            self.filter_entry = ttk.Entry(bar, textvariable=self.filter_var, width=28)
            self.filter_entry.pack(side='left', padx=4)
            self.show_missing = tk.BooleanVar(value=False)
            ttk.Checkbutton(bar, text='show monsters without a .bin', variable=self.show_missing,
                            command=self.fill_list).pack(side='left', padx=8)

            pane = ttk.Panedwindow(self, orient='horizontal')
            pane.pack(fill='both', expand=True, padx=6, pady=(0, 4))
            lf = ttk.Frame(pane)
            self.list = ttk.Treeview(lf, columns=('info',), selectmode='browse', show='tree headings')
            self.list.heading('#0', text='Loot file')
            self.list.heading('info', text='Monsters')
            self.list.column('#0', width=150, stretch=False)
            self.list.column('info', width=250)
            self.list.tag_configure('missing', foreground='#888888')
            self.list.tag_configure('dirty', foreground='#0044aa')
            lsb = ttk.Scrollbar(lf, orient='vertical', command=self.list.yview)
            self.list.configure(yscrollcommand=lsb.set)
            self.list.pack(side='left', fill='both', expand=True)
            lsb.pack(side='right', fill='y')
            self.list.bind('<<TreeviewSelect>>', self.on_pick_base)
            pane.add(lf, weight=1)

            rf = ttk.Panedwindow(pane, orient='vertical')
            mf = ttk.LabelFrame(rf, text='Monsters that load this file')
            mcols = ('type', 'name', 'native', 'level', 'flags')
            self.monsters = ttk.Treeview(mf, columns=mcols, show='headings', selectmode='browse', height=6)
            for col, head, width, anchor in (('type', 'Type', 60, 'e'), ('name', 'EName', 220, 'w'),
                                             ('native', 'Name', 160, 'w'), ('level', 'Level', 60, 'e'),
                                             ('flags', 'Flags', 300, 'w')):
                self.monsters.heading(col, text=head)
                self.monsters.column(col, width=width, anchor=anchor, stretch=(col == 'flags'))
            self.monsters.tag_configure('chief', foreground='#8a4b00')
            self.monsters.tag_configure('off', foreground='#888888')
            msb = ttk.Scrollbar(mf, orient='vertical', command=self.monsters.yview)
            self.monsters.configure(yscrollcommand=msb.set)
            self.monsters.pack(side='left', fill='both', expand=True, padx=(4, 0), pady=(0, 4))
            msb.pack(side='right', fill='y', pady=(0, 4))
            rf.add(mf, weight=1)

            nf = ttk.Frame(rf)
            self.nb = ttk.Notebook(nf)
            self.nb.pack(fill='both', expand=True)
            rf.add(nf, weight=4)
            self.views = {}
            for race in RACES:
                v = RaceView(self.nb, self, race)
                self.views[race] = v
                self.nb.add(v, text=race.capitalize())
            pane.add(rf, weight=4)

            self.status_var = tk.StringVar()
            ttk.Label(self, textvariable=self.status_var, anchor='w', relief='sunken').pack(fill='x', side='bottom')

        # ---- data folder / list -----------------------------------------------------------------
        def status(self, s):
            self.status_var.set(s)

        def current_view(self):
            return self.views[RACES[self.nb.index(self.nb.select())]]

        def set_data_dir(self, d):
            self.data_dir = d
            self.settings['data_dir'] = d
            self.dir_label.config(text=d)
            self.scan()
            self.fill_list()

        def scan(self):
            bases = {}
            try:
                for n in os.listdir(self.data_dir):
                    low = n.lower()
                    for race in RACES:
                        suffix = '.%s.bin' % race
                        if low.endswith(suffix):
                            bases.setdefault(n[:-len(suffix)], set()).add(race)
            except OSError as e:
                messagebox.showerror(APP_TITLE, 'Cannot read %s:\n%s' % (self.data_dir, e), parent=self)
            self.bases = bases

        def fill_list(self):
            flt = self.filter_var.get().strip().lower()
            self.list.delete(*self.list.get_children(''))
            names = self.names
            rows = []
            for base in self.bases:
                info = names.bin_info(base)
                if not info and names.loaded:
                    info = '(no monster loads this file)'
                rows.append((base, info, False))
            if self.show_missing.get():
                for base, ms in names.by_bin.items():
                    if base not in self.bases and any(m['treasure'] for m in ms):
                        rows.append((base, names.bin_info(base), True))

            def key(r):
                b = r[0]
                if b.startswith('Class') and b[5:].isdigit():
                    return (0, int(b[5:]), b)
                return (1, 0, b.lower())

            for base, info, missing in sorted(rows, key=key):
                hay = (base + ' ' + info + ' ' + ' '.join(m['e'] + ' ' + m['n'] for m in names.by_bin.get(base, []))).lower()
                if flt and flt not in hay:
                    continue
                tags = ['missing'] if missing else []
                if self.base == base and any(v.dirty for v in self.views.values()):
                    tags.append('dirty')
                self.list.insert('', 'end', iid=base, text=base + ('  (no .bin)' if missing else ''), values=(info,), tags=tags)
            if self.base and self.list.exists(self.base):
                self.list.selection_set(self.base)
                self.list.see(self.base)

        def on_pick_base(self, event=None):
            sel = self.list.selection()
            if not sel or sel[0] == self.base:
                return
            if not self.confirm_discard():
                if self.base and self.list.exists(self.base):
                    self.list.selection_set(self.base)
                return
            self.load_base(sel[0])

        def load_base(self, base):
            self.base = base
            for race, v in self.views.items():
                v.load(os.path.join(self.data_dir, '%s.%s.bin' % (base, race)))
            self.fill_monsters(base)
            self.update_title()
            self.status('%s: %s' % (base, self.names.bin_info(base) or 'no monster in names.json uses this file'))

        def fill_monsters(self, base):
            """Every MonsterInfo row that loads <base>.*.bin, regular monsters first, then chiefs and variants."""
            m = self.monsters
            m.delete(*m.get_children(''))
            ms = self.names.by_bin.get(base, [])
            label = m.master
            if not self.names.loaded:
                label.config(text='Monsters that load this file (names.json not loaded)')
                return
            if not ms:
                label.config(text='Monsters that load this file: none in MonsterInfo (the server never reads it)')
                return
            regular = sum(1 for x in ms if not x['chief'] and x['regen'])
            label.config(text='Monsters that load this file: %d (%d regular, %d chiefs/variants)' % (len(ms), regular, len(ms) - regular))

            def key(x):
                return (1 if x['chief'] else 0, 0 if x['regen'] else 1, x['lvl'], x['t'])

            for x in sorted(ms, key=key):
                flags = []
                if x['chief']:
                    flags.append('chief (re-rolls up to %d more items)' % self.names.chief_bonus)
                if not x['regen']:
                    flags.append('no NormalRegen (spawned by events/scripts)')
                if not x['treasure']:
                    flags.append('HasTreasure 0: does not load it')
                tags = ['chief'] if x['chief'] else []
                if not x['treasure']:
                    tags = ['off']
                m.insert('', 'end', values=(x['t'], x['e'], x['n'], x['lvl'] or '', ', '.join(flags)), tags=tags)

        def choose_data_dir(self):
            if not self.confirm_discard():
                return
            d = filedialog.askdirectory(title='Pick the gameserver data folder', initialdir=self.data_dir or None, parent=self)
            if d:
                self.base = None
                self.set_data_dir(os.path.normpath(d))

        def open_file(self):
            if not self.confirm_discard():
                return
            f = filedialog.askopenfilename(title='Open a .bin file', filetypes=[('Treasure bin', '*.bin'), ('All', '*')],
                                           initialdir=self.data_dir or None, parent=self)
            if f:
                self.open_path(f)

        def open_path(self, f):
            f = os.path.normpath(f)
            d, n = os.path.split(f)
            base = n
            for race in RACES:
                suffix = '.%s.bin' % race
                if n.lower().endswith(suffix):
                    base = n[:-len(suffix)]
                    break
            if d != self.data_dir:
                self.base = None
                self.set_data_dir(d)
            self.load_base(base)
            self.fill_list()

        def new_base(self):
            if not self.data_dir:
                messagebox.showinfo(APP_TITLE, 'Open a data folder first.', parent=self)
                return
            if not self.confirm_discard():
                return
            base = simpledialog.askstring(APP_TITLE, 'Base name (the EName without spaces and [..], e.g. QuestKid, or ClassN):', parent=self)
            if not base:
                return
            base = base.strip()
            self.bases.setdefault(base, set())
            self.load_base(base)
            for v in self.views.values():
                if not v.treasures:
                    v.treasures = [Treasure()]
                    v.refresh()
            self.fill_list()
            self.update_title()

        # ---- save / dirty -----------------------------------------------------------------------
        def any_dirty(self):
            return any(v.dirty for v in self.views.values())

        def update_title(self):
            dirty = self.any_dirty()
            self.title('%s%s - %s' % (self.base + (' *' if dirty else '') + ' - ' if self.base else '', self.data_dir, APP_TITLE))
            if self.base and self.list.exists(self.base):
                self.list.item(self.base, tags=['dirty'] if dirty else [])

        def save(self):
            if not self.base:
                return
            saved = []
            for race, v in self.views.items():
                if v.dirty:
                    if v.save():
                        saved.append(race)
            self.bases.setdefault(self.base, set()).update(saved)
            self.update_title()
            self.fill_list()
            if saved:
                self.status('saved %s for %s. Restart the gameserver to load it.' % (', '.join(saved), self.base))
            else:
                self.status('nothing to save')

        def revert(self):
            for v in self.views.values():
                v.revert()
            self.update_title()

        def confirm_discard(self):
            if not self.any_dirty():
                return True
            r = messagebox.askyesnocancel(APP_TITLE, 'Save the changes to %s first?' % self.base, parent=self)
            if r is None:
                return False
            if r:
                self.save()
            return True

        def on_close(self):
            if not self.confirm_discard():
                return
            self.settings['geometry'] = self.geometry()
            save_settings(self.settings)
            self.destroy()

        # ---- edit menu ---------------------------------------------------------------------------
        def copy_race(self):
            src = self.current_view()
            if not src.path:
                return
            for v in self.views.values():
                if v is not src:
                    v.snapshot()
                    v.treasures = copy.deepcopy(src.treasures)
                    v.refresh()
            self.update_title()
            self.status('copied %s loot to the other races (not saved yet)' % src.race)

        def copy_race_to(self):
            src = self.current_view()
            others = [r for r in RACES if r != src.race]
            r = simpledialog.askstring(APP_TITLE, 'Copy %s loot to which race? (%s)' % (src.race, ' or '.join(others)), parent=self)
            if not r or r.strip().lower() not in others:
                return
            v = self.views[r.strip().lower()]
            v.snapshot()
            v.treasures = copy.deepcopy(src.treasures)
            v.refresh()
            self.update_title()

        def export_json(self):
            v = self.current_view()
            f = filedialog.asksaveasfilename(title='Export JSON', defaultextension='.json',
                                             initialfile='%s.%s.json' % (self.base, v.race), parent=self)
            if f:
                with open(f, 'w', encoding='utf-8') as fh:
                    json.dump(to_jsonable(v.treasures), fh, indent=1)
                self.status('exported %s' % f)

        def import_json(self):
            v = self.current_view()
            f = filedialog.askopenfilename(title='Import JSON', filetypes=[('JSON', '*.json')], parent=self)
            if not f:
                return
            try:
                with open(f, encoding='utf-8') as fh:
                    treasures = from_jsonable(json.load(fh))
            except (ValueError, KeyError, BinFormatError) as e:
                messagebox.showerror(APP_TITLE, 'Cannot import: %s' % e, parent=self)
                return
            v.snapshot()
            v.treasures = treasures
            v.refresh()
            self.update_title()

        def copy_text(self):
            v = self.current_view()
            self.clipboard_clear()
            self.clipboard_append(dump_text(v.treasures, self.names, v.path or ''))
            self.status('copied to the clipboard')

        # ---- tools menu -------------------------------------------------------------------------
        def check_all(self):
            if not self.data_dir:
                return
            lines = []
            for f in collect_bins([self.data_dir]):
                try:
                    treasures = read_bin(f)
                except (BinFormatError, OSError) as e:
                    lines.append('%s: ERROR %s' % (os.path.basename(f), e))
                    continue
                info = analyse(treasures, self.names)
                base = os.path.basename(f).split('.')[0]
                if self.names.loaded and base not in self.names.by_bin:
                    lines.append('%s: no monster in names.json loads this file' % os.path.basename(f))
                for path, i in sorted(info.items()):
                    for w in i['warn']:
                        lines.append('%s  entry %s: %s' % (os.path.basename(f), '/'.join(str(x + 1) for x in path), w))
            self.show_text('Check: %d line(s)' % len(lines), '\n'.join(lines) or 'No problems found.')

        def set_probe(self):
            v = simpledialog.askinteger(APP_TITLE, 'ITEM_PROBE_RATIO (AttrInfo 3) in percent, used only for the displayed chances:',
                                        initialvalue=self.names.probe_percent, minvalue=0, maxvalue=100000, parent=self)
            if v is not None:
                self.names.probe_percent = v
                self.settings['probe_percent'] = v
                for view in self.views.values():
                    view.refresh()

        def reload_names(self):
            self.names = Names(self.settings.get('names'))
            if self.settings.get('probe_percent'):
                self.names.probe_percent = int(self.settings['probe_percent'])
            for v in self.views.values():
                v.refresh()
                v.load_editor()
            self.fill_list()
            if self.base:
                self.fill_monsters(self.base)
            self.status('names: %s' % (self.names.path or 'names.json not found (numbers only)'))

        def pick_names(self):
            f = filedialog.askopenfilename(title='names.json', filetypes=[('JSON', '*.json')], parent=self)
            if f:
                self.settings['names'] = f
                self.reload_names()

        def show_text(self, title, text):
            w = tk.Toplevel(self)
            w.title(title)
            w.geometry('900x600')
            t = tk.Text(w, wrap='none', font=('Consolas', 10))
            ys = ttk.Scrollbar(w, orient='vertical', command=t.yview)
            xs = ttk.Scrollbar(w, orient='horizontal', command=t.xview)
            t.configure(yscrollcommand=ys.set, xscrollcommand=xs.set)
            t.insert('1.0', text)
            t.config(state='disabled')
            ys.pack(side='right', fill='y')
            xs.pack(side='bottom', fill='x')
            t.pack(fill='both', expand=True)

    app = App()
    if start:
        if os.path.isdir(start):
            app.set_data_dir(os.path.normpath(start))
        elif os.path.isfile(start):
            app.open_path(start)
    if os.environ.get('BIN_EDITOR_SELFTEST'):
        # exercise the widgets once without showing a window, then quit (used by build.bat)
        app.update()
        if app.bases:
            app.load_base(sorted(app.bases)[0])
            v = app.views['slayer']
            if v.treasures:
                v.select_path((0,))
                v.add_class()
                v.add_type()
                v.do_undo()
                v.do_undo()
                assert not v.dirty, 'undo left the file dirty'
        app.update()
        app.destroy()
        return
    app.mainloop()


def attach_console():
    """A windowed exe has no stdout; when run from a console with CLI arguments, borrow that console."""
    if os.name != 'nt' or not getattr(sys, 'frozen', False):
        return
    try:
        import ctypes
        if ctypes.windll.kernel32.AttachConsole(-1):
            sys.stdout = open('CONOUT$', 'w', encoding='utf-8', errors='replace', buffering=1)
            sys.stderr = sys.stdout
    except Exception:
        pass


def main():
    args = sys.argv[1:]
    if args and args[0] in ('dump', 'check', 'export', 'import', 'roundtrip', '-h', '--help', 'help'):
        attach_console()
        buffered = None
        if sys.stdout is None:
            # windowed exe started without a console (double-clicked .bat, Start-Process): keep the output
            # and hand it over as a file at the end
            import io
            buffered = sys.stdout = sys.stderr = io.StringIO()
        try:
            sys.stdout.reconfigure(errors='replace')
        except Exception:
            pass
        try:
            rc = cli(args)
        except BrokenPipeError:
            return 0
        except (BinFormatError, OSError, ValueError) as e:
            sys.stdout.write('error: %s\n' % e)
            rc = 1
        if buffered is not None and buffered.getvalue():
            out = os.path.join(os.getcwd(), 'bin_editor.output.txt')
            with open(out, 'w', encoding='utf-8') as f:
                f.write(buffered.getvalue())
            try:
                import ctypes
                ctypes.windll.user32.MessageBoxW(None, 'No console to print to. The output is in:\n%s' % out, APP_TITLE, 0x40)
            except Exception:
                pass
        return rc
    run_gui(args[0] if args else None)
    return 0


if __name__ == '__main__':
    sys.exit(main())
