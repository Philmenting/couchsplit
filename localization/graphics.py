# SPDX-License-Identifier: GPL-3.0-or-later
"""Import the original German touch-button texture, without bundled assets.

The German edition stores it at a different NitroFS path. Palette, all eight
animation maps and the decompressed graphics size are identical to US/PM.
"""
from .formats import Narc

TOUCH_BUTTONS = 'graphic/touch_subwindow.narc'
GERMAN_TOUCH_BUTTONS = 'resource/ger/touch_subwindow/touch_subwindow.narc'


def localize_buttons(us, german, pm):
    original = us.file(TOUCH_BUTTONS)
    current = pm.file(TOUCH_BUTTONS)
    translated = german.file(GERMAN_TOUCH_BUTTONS)
    if current != original:
        raise ValueError('PM hat die Touch-Grafiken geändert; kein blindes Überschreiben.')
    before, after = Narc(current), Narc(translated)
    if len(before.files) != 10 or len(after.files) != 10:
        raise ValueError('Unerwartete Anzahl von Touch-Grafikressourcen.')
    for i, (a, b) in enumerate(zip(before.files, after.files)):
        if i != 1 and a != b:
            raise ValueError('Palette oder Touch-Animationskarten unterscheiden sich.')
    # Member 1 is an LZ10-compressed NCGR texture. Equal decoded byte counts
    # preserve its VRAM allocation; no new tiles or executable changes required.
    a, b = before.files[1], after.files[1]
    if len(a) < 4 or len(b) < 4 or a[0] != 0x10 or b[0] != 0x10 or a[1:4] != b[1:4]:
        raise ValueError('Touch-Grafikformat oder VRAM-Größe unterscheiden sich.')
    return {TOUCH_BUTTONS: translated}
