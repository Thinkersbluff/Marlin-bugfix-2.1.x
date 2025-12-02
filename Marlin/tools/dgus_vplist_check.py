#!/usr/bin/env python3
"""
DGUS VPList Checker

Parses the `creality_touch/DGUSDisplayDef.cpp` and `creality_touch/DGUSDisplayDef.h` files
and validates that every named VP in each VPList has a send handler defined in
`ListOfVP[]`.

Output:
 - For each screen: name and numeric ID
 - The list of VPs (symbol names) that will be auto-updated on that screen
 - For each VP: OK (has handler) | SKIPPED (send_to_display_handler == nullptr) | MISSING (no DGUS_VP entry)

Usage:
  From repository root run:

  python tools/dgus_vplist_check.py

It prints a human-readable report to stdout and exits with code 0. No build required.
"""

import re
import os
import sys

ROOT = os.path.join(os.path.dirname(__file__), '..')
CREALITY_DIR = os.path.join(ROOT, 'src', 'lcd', 'extui', 'cr6_community_ui', 'creality_touch')
DGUS_CPP = os.path.join(CREALITY_DIR, 'DGUSDisplayDef.cpp')
DGUS_H = os.path.join(CREALITY_DIR, 'DGUSDisplayDef.h')
COMMON_H = os.path.join(os.path.dirname(DGUS_H), '..', 'DGUSDisplayDef.h')

if not os.path.exists(DGUS_CPP):
    print("ERROR: cannot find", DGUS_CPP)
    sys.exit(2)

with open(DGUS_CPP, 'r', encoding='utf-8') as f:
    cpp = f.read()
with open(DGUS_H, 'r', encoding='utf-8') as f:
    creality_h = f.read()
with open(COMMON_H, 'r', encoding='utf-8') as f:
    common_h = f.read()

# 1) parse VPList macro definitions (e.g. '#define VPList_Common VP_BACK_BUTTON_STATE')
macro_re = re.compile(r'^\s*#define\s+(VPList_[A-Za-z0-9_]+)\s+(.*)$', re.MULTILINE)
macros = {m.group(1): m.group(2).strip() for m in macro_re.finditer(cpp)}

# 2) parse VPList array definitions
vplist_re = re.compile(r'const\s+uint16_t\s+(VPList_[A-Za-z0-9_]+)\s*\[\]\s*PROGMEM\s*=\s*\{(.*?)\};', re.S)
vplists = {}
for m in vplist_re.finditer(cpp):
    name = m.group(1)
    body = m.group(2)
    # remove comments and newlines
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
    body = re.sub(r'//.*?\n', '\n', body)
    tokens = [t.strip() for t in body.replace('\n', ',').split(',') if t.strip()]
    # drop trailing 0x0000 terminator tokens in the raw tokens list
    vplists[name] = tokens

# 3) expand macros inside vplist tokens (recursive)
def expand_token(token, depth=0):
    token = token.strip()
    if not token:
        return []
    if token.endswith(','):
        token = token[:-1].strip()
    # ignore preprocessor directives or stray tokens
    if token.startswith('#'):
        return []
    # macro name
    if token in macros:
        expansion = macros[token]
        # split expansion into tokens by whitespace and commas, keep 'VP_' tokens and nested macros
        parts = [p.strip().strip(',') for p in re.split(r'[\s,]+', expansion) if p.strip()]
        result = []
        for p in parts:
            result += expand_token(p, depth+1)
        return result
    # if token looks like '0x0000' treat as terminator
    if re.match(r'0x0+|0', token):
        return []
    # normal VP symbol or other token
    return [token]

expanded_vplists = {}
for name, tokens in vplists.items():
    expanded = []
    for t in tokens:
        # skip preprocessor directives embedded in lists
        tt = t.strip()
        if tt.startswith('#'):
            continue
        expanded += expand_token(t)
    # remove duplicates while preserving order
    seen = set()
    uniq = []
    for x in expanded:
        if x not in seen:
            seen.add(x)
            uniq.append(x)
    expanded_vplists[name] = uniq

# 4) parse ListOfVP[] entries to determine which VPs have send handlers
listofvp_re = re.compile(r'const\s+struct\s+DGUS_VP_Variable\s+ListOfVP\[\]\s*PROGMEM\s*=\s*\{(.*)\};', re.S)
m = listofvp_re.search(cpp)
if not m:
    print('ERROR: ListOfVP[] not found')
    sys.exit(2)
list_body = m.group(1)

# parse entries using a few patterns
vp_entries = {}
# pattern: VPHELPER(arg1, arg2, arg3, arg4)
vphelper_re = re.compile(r'VPHELPER\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^\)]+)\)', re.S)
for vm in vphelper_re.finditer(list_body):
    vp = vm.group(1).strip()
    send = vm.group(4).strip()
    # normalize send: 'nullptr' or function pointer
    has_send = (send != 'nullptr' and send != 'NULL')
    vp_entries[vp] = {'has_send': has_send, 'raw_send': send}

# VPHELPER_STR pattern
vphelperstr_re = re.compile(r'VPHELPER_STR\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^\)]+)\)', re.S)
for vm in vphelperstr_re.finditer(list_body):
    vp = vm.group(1).strip()
    send = vm.group(5).strip()
    has_send = (send != 'nullptr' and send != 'NULL')
    vp_entries[vp] = {'has_send': has_send, 'raw_send': send}

# explicit initializer pattern: {.VP = VP_M117, ... .send_to_display_handler = &ScreenHandler.DGUSLCD_SendStringToDisplay }
explicit_re = re.compile(r'\{\s*[^\}]*?\.VP\s*=\s*([^,\s]+)\s*,(.*?)\.send_to_display_handler\s*=\s*([^,\}]+)', re.S)
for vm in explicit_re.finditer(list_body):
    vp = vm.group(1).strip()
    send = vm.group(3).strip()
    has_send = (send != 'nullptr' and send != 'NULL')
    vp_entries[vp] = {'has_send': has_send, 'raw_send': send}

# 5) parse VPMap[] table mapping screens to VPLists
vpmap_re = re.compile(r'const\s+struct\s+VPMapping\s+VPMap\[\]\s*PROGMEM\s*=\s*\{(.*?)\};', re.S)
vm = vpmap_re.search(cpp)
if not vm:
    print('ERROR: VPMap[] not found')
    sys.exit(2)
map_body = vm.group(1)
# entries like { DGUSLCD_SCREEN_BOOT, VPList_None },
map_entry_re = re.compile(r'\{\s*([^,\s]+)\s*,\s*([^\}\s]+)\s*\}', re.S)
vpmap = []
for m2 in map_entry_re.finditer(map_body):
    scr = m2.group(1).strip()
    vplist = m2.group(2).strip().rstrip(',')
    if vplist == 'nullptr':
        continue
    # skip final terminator { 0 , nullptr }
    if vplist == 'nullptr':
        continue
    vpmap.append((scr, vplist))

# 6) parse screen enum mapping to numbers from creality_h (or common_h)
# search both headers
enum_re = re.compile(r'enum\s+DGUSLCD_Screens\s*:\s*uint8_t\s*\{(.*?)\}', re.S)
num_map = {}
for text in (creality_h, common_h):
    m = enum_re.search(text)
    if not m:
        # try a simpler enum without ': uint8_t'
        enum_re2 = re.compile(r'enum\s+DGUSLCD_Screens\s*\{(.*?)\}', re.S)
        m = enum_re2.search(text)
    if not m:
        continue
    body = m.group(1)
    # parse lines like 'DGUSLCD_SCREEN_BOOT = 0,' or 'DGUSLCD_SCREEN_MAIN = 28,'
    line_re = re.compile(r'([A-Za-z0-9_]+)\s*(?:=\s*([^,\n]+))?\s*,')
    current = 0
    for ln in line_re.finditer(body):
        name = ln.group(1)
        val = ln.group(2)
        if val:
            try:
                v = int(val.strip())
            except:
                # maybe hex
                v = int(val.strip(), 0)
            current = v
        num_map[name] = current
        current += 1

# helper to convert screen symbol to string (use symbol if unknown)
def screen_name(sym):
    return sym

# 7) generate report

out = []
for scr_sym, vplist_sym in vpmap:
    scr_num = num_map.get(scr_sym, None)
    out.append(f"Screen: {scr_sym} ({scr_num if scr_num is not None else 'unknown'}) -> {vplist_sym}")
    vplist = expanded_vplists.get(vplist_sym, None)
    if vplist is None:
        out.append(f"  WARNING: VPList '{vplist_sym}' not found in file.")
        continue
    for vp in vplist:
        info = vp_entries.get(vp)
        if info is None:
            status = 'MISSING (no DGUS_VP entry)'
        else:
            if info['has_send']:
                status = 'OK (has send handler)'
            else:
                status = 'SKIPPED (send_to_display_handler == nullptr)'
        out.append(f"  - {vp}: {status}")
    out.append("")

print('\n'.join(out))

# exit success
sys.exit(0)
