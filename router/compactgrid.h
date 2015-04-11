#ifndef COMPACTGRID_H
#define COMPACTGRID_H

#include "grid.h"
#include <QVector>

class CompactGrid : public Grid
{
public:
    CompactGrid(int xSize, int ySize, int zSize, int xOffset, int yOffset, int zOffset);

    virtual int
    get(Point p) const;

    void
    set(int x, int y, int z, int weight);

private:
    int
    getIndex(int x, int y, int z) const;

private:
    int m_xSize;
    int m_ySize;
    int m_zSize;
    int m_xOffset;
    int m_yOffset;
    int m_zOffset;
    QVector<qint16> m_data;
};

#endif // COMPACTGRID_H
