"""Prepare the redistributable UI subset. Requires fonttools; no game compilation."""
import re
import sys
from pathlib import Path
from fontTools.ttLib import TTFont
from fontTools import subset
from fontTools.varLib.instancer import instantiateVariableFont

root = Path(__file__).resolve().parents[1]
font = TTFont(sys.argv[1])
text = ''.join(re.findall(r'u8"([^"\n]*)"', (root / 'src/ui.cpp').read_text(encoding='utf-8')))
codepoints = set(range(32, 127)) | {ord(c) for c in text}
options = subset.Options()
options.name_IDs = ['*']
options.name_legacy = True
subsetter = subset.Subsetter(options=options)
subsetter.populate(unicodes=codepoints)
subsetter.subset(font)
if 'fvar' in font:
    font = instantiateVariableFont(font, {'wght': 400}, inplace=True)
for record in font['name'].names:
    names = {1: 'Spark Grid UI', 2: 'Regular', 3: 'SparkGridUI-Regular',
             4: 'Spark Grid UI Regular', 6: 'SparkGridUI-Regular', 16: 'Spark Grid UI', 17: 'Regular'}
    if record.nameID in names:
        record.string = names[record.nameID].encode(record.getEncoding())
missing = codepoints - set(font.getBestCmap())
if missing:
    raise RuntimeError(f'Missing glyphs: {sorted(missing)}')
output = root / 'data/ui.ttf'
font.save(output)
print(f'UI subset: {len(codepoints)} characters, {output.stat().st_size} bytes')
