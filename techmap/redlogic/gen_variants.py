#!/usr/bin/env python2

import sys

"""
          [ 0; 1]
    [-1; 0]  X  [ 1; 0]
          [ 0;-1]
"""
def get_rotations():
    r = []
    r.append(( 0,  1))
    r.append(( 1,  0))
    r.append(( 0, -1))
    r.append((-1,  0))
    return r

def gen_rotations(base):
    rs = get_rotations()
    rs += rs

    d = {}
    for p in base:
        d[p[0]] = p[1:]

    variants = []
    for i in range(4):
        cell = []
        for p in d.keys():
            c = d[p]
            index = rs.index(c)
            c = rs[index + i]
            cell.append((p,) + c)
        cell.sort()
        variants.append(cell)

    return variants

def gen_variants_3inp():
    rs = get_rotations()[1:]

    variants = []
    for i1 in range(3):
        for i2 in range(3):
            if i1 == i2: continue

            i3 = 3 - i1 - i2
            r1 = rs[i1]
            r2 = rs[i2]
            r3 = rs[i3]

            cell = []
            cell.append(('A',) + r1)
            cell.append(('B',) + r2)
            cell.append(('C',) + r3)
            cell.append(('Y',  0,  1))
            variants += gen_rotations(cell)

    return variants

def gen_variants_xor():
    variants = []

    cell = []
    cell.append(('A', -1,  0))
    cell.append(('B',  1,  0))
    cell.append(('Y',  0,  1))
    variants += gen_rotations(cell)

    cell = []
    cell.append(('A',  1,  0))
    cell.append(('B', -1,  0))
    cell.append(('Y',  0,  1))
    variants += gen_rotations(cell)

    return variants

def gen_variants_not():
    rs = get_rotations()[1:]

    variants = []
    for r in rs:
        cell = []
        cell.append(('A',) + r)
        cell.append(('Y',  0,  1))
        variants += gen_rotations(cell)

    return variants

def gen_variants_pad():
    cell = []
    cell.append(('X', 0, 0))

    return [cell]

def gen_all():
    cells = {}

    cells['PAD'] = gen_variants_pad()

    cells['NOT'] = gen_variants_not()

    cells['XOR2'] = gen_variants_xor()
    cells['XNOR2'] = gen_variants_xor()

    cells['AND3'] = gen_variants_3inp()
    cells['NAND3'] = gen_variants_3inp()
    cells['OR3'] = gen_variants_3inp()
    cells['NOR3'] = gen_variants_3inp()

    return cells

def save_cell_blocks_xml(f, tabs, cellName, rotation, ports):
    tabsStr = "    " * tabs
    tabsStr2 = "    " * (tabs + 1)

    xSize = zSize = 3
    dx = dz = 1
    if cellName == "PAD":
        xSize = zSize = 1
        dx = dz = 0

    f.write("%s<blocks>\n" % tabsStr)
    for x in range(xSize):
        for z in range(zSize):
            f.write("%s<block type=\"$stone\" rotation=\"0\" x=\"%d\" y=\"%d\" z=\"%d\"/>\n" % (tabsStr2, x, 0, z))

    for p in ports:
        f.write("%s<block type=\"$blockage\" rotation=\"0\" x=\"%d\" y=\"%d\" z=\"%d\"/>\n" % (tabsStr2, p[1] + dx, 1, p[2] + dz))

    if cellName != "PAD":
        name = "$" + cellName.lower()
        f.write("%s<block type=\"%s\" rotation=\"%s\" x=\"%d\" y=\"%d\" z=\"%d\"/>\n" % (tabsStr2, name, rotation, dx, 1, dz))
    f.write("%s</blocks>\n" % tabsStr)

def save_cell_xml(f, tabs, cellName, variants):
    tabsStr = "    " * tabs
    tabsStr2 = "    " * (tabs + 1)
    tabsStr3 = "    " * (tabs + 2)
    tabsStr4 = "    " * (tabs + 3)
    rs = get_rotations()

    xSize = zSize = 3
    ySize = 2
    dx = dy = dz = 1
    if cellName == "PAD":
        xSize = zSize = 1
        dx = dy = 0

    f.write("%s<cell name=\"%s\" xSize=\"%d\" ySize=\"%d\" zSize=\"%d\">\n" % (tabsStr, cellName, xSize, ySize, zSize))
    index = 1
    for v in variants:
        varName = "%s$%03d" % (cellName, index)
        index += 1

        # deduce rotation
        rotation = 0
        for p in v:
            if p[0] == 'Y':
                rotation = rs.index(p[1:])
                break

        f.write("%s<variant name=\"%s\">\n" % (tabsStr2, varName))

        f.write("%s<ports>\n" % tabsStr3)
        for p in v:
            f.write("%s<port name=\"%s\" x=\"%d\" y=\"%d\" z=\"%d\"/>\n" % (tabsStr4, p[0], p[1] + dx, dy, p[2] + dz))
        f.write("%s</ports>\n" % tabsStr3)

        save_cell_blocks_xml(f, tabs + 2, cellName, rotation, v)

        f.write("%s</variant>\n" % tabsStr2)
    f.write("%s</cell>\n" % tabsStr)

def save_xml(fileName, cells):
    f = open(fileName, 'w')
    f.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
    f.write("<cells>\n")
    for cellName in cells.keys():
        save_cell_xml(f, 1, cellName, cells[cellName])
    f.write("</cells>\n")

def main():
    if len(sys.argv) < 2:
        print "Usage: %s <output.xml>" % sys.argv[0]
        return

    cells = gen_all()
    save_xml(sys.argv[1], cells)

if __name__=="__main__":
    main()
