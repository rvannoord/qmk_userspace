# Three 8x8 status-bar glyphs. Drawing area cols 0-6, rows 0-6:
# col 7 is the inter-glyph gutter (pitch 9 -> 2px clear), row 7 is reserved
# for the SYS underline. Baseline row 6, shared with the badge letters.

CAPSLOCK = [
    "..###...",
    "..#.#...",
    "..#.#...",
    "#######.",
    "#######.",
    "#######.",
    ".#####..",
    "........",
]
CAPSWORD = [
    "...#....",
    "..###...",
    "#######.",
    "...#....",
    "...#....",
    "........",
    "#####...",
    "........",
]
SCROLLLOCK = [
    ".#####..",
    "#.....#.",
    "#.....#.",
    "#######.",
    "#.....#.",
    "#.....#.",
    ".#####..",
    "........",
]
# badge letters, for the side-by-side confusability check (dungeon-mode r4c9/r4c13/r4c14 etc.)
LETTERS = {
 'W': ["........",".#....#.","##...##.","##.#.##.","#######.","###.###.","##..##..","........"],
 'I': ["........",".######.","...##...","...##...","...##...","...##...",".######.","........"],
 'N': ["........","###..###",".###.##.",".######.",".##.###.",".##..##.","###..###","........"],
 'M': ["........","###...##",".###.###",".#######",".##.#.##",".##...##","###...##","........"],
 'A': ["........","..######",".#.#.##.","..##.##.","..#####.",".##..##.","###..###","........"],
 'C': ["........","..#####.",".##..##.",".##..#..",".##.....",".##..##.","..####..","........"],
}

SET = [("Caps Lock  (padlock)", CAPSLOCK),
       ("Caps Word  (latched shift, rule stops short)", CAPSWORD),
       ("Scroll Lock(struck ring)", SCROLLLOCK)]

inv = lambda g: [''.join('#' if c=='.' else '.' for c in r) for r in g]

def check(g):
    errs=[]
    if len(g)!=8: errs.append('height')
    for i,r in enumerate(g):
        if len(r)!=8: errs.append(f'row {i} width {len(r)}')
    if '#' in g[7]: errs.append('row 7 (reserved for SYS underline) has ink')
    for r in g:
        if r[7]=='#': errs.append('col 7 (gutter) has ink')
    return sum(r.count('#') for r in g), errs

for name,g in SET:
    ink,errs = check(g)
    print(f'=== {name}  ink={ink}/64 ({100*ink//64}%)' + (f'  !! {errs}' if errs else '') + ' ===')
    for a,b in zip(g,inv(g)): print(a+'   '+b)
    print()

# the row as it actually appears: badge at x=0, glyphs at x=100/109/118
def row(badge, flags):
    band=[['.']*128 for _ in range(8)]
    for i,ch in enumerate(badge):
        for r in range(8):
            for c in range(8):
                if LETTERS[ch][r][c]=='#': band[r][i*8+c]='#'
    for x,g in zip((100,109,118), flags):
        if g is None: continue
        for r in range(8):
            for c in range(8):
                if g[r][c]=='#': band[r][x+c]='#'
    return [''.join(r) for r in band]

for title,b,f in [("WIN, all three latched",'WIN',(CAPSLOCK,CAPSWORD,SCROLLLOCK)),
                  ("MAC, Caps Word only",'MAC',(None,CAPSWORD,None)),
                  ("MAC, Scroll Lock only (the accident)",'MAC',(None,None,SCROLLLOCK))]:
    print(title)
    print('+'+'-'*128+'+')
    for r in row(b,f): print('|'+r+'|')
    print('+'+'-'*128+'+')

print('\nSILHOUETTE CHECK  glyphs vs every badge letter, 2px apart:')
order=[('CL',CAPSLOCK),('CW',CAPSWORD),('SL',SCROLLLOCK)]+[(k,v) for k,v in LETTERS.items()]
print('  '.join(f'{n:<8}' for n,_ in order))
for r in range(8):
    print('  '.join(g[r] for _,g in order))
