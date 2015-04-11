#include "compactgrid.h"

CompactGrid::CompactGrid(int xSize, int ySize, int zSize, int xOffset, int yOffset, int zOffset)
{
    m_xSize = xSize;
    m_ySize = ySize;
    m_zSize = zSize;
    m_xOffset = xOffset;
    m_yOffset = yOffset;
    m_zOffset = zOffset;

    m_data.resize(xSize * ySize * zSize);
}

int
CompactGrid::get(Point p) const
{
    int xIndex = p.x - m_xOffset;
    int yIndex = p.y - m_yOffset;
    int zIndex = p.z - m_zOffset;

    if ((xIndex < 0) || (xIndex >= m_xSize)) return 0;
    if ((yIndex < 0) || (yIndex >= m_ySize)) return 0;
    if ((zIndex < 0) || (zIndex >= m_zSize)) return 0;

    int index = getIndex(xIndex, yIndex, zIndex);
    return m_data[index];
}

void
CompactGrid::set(int x, int y, int z, int weight)
{
    Q_ASSERT(weight >= SHRT_MIN);
    Q_ASSERT(weight <= SHRT_MAX);

    int xIndex = x - m_xOffset;
    int yIndex = y - m_yOffset;
    int zIndex = z - m_zOffset;

    Q_ASSERT((xIndex >= 0) && (xIndex < m_xSize));
    Q_ASSERT((yIndex >= 0) && (yIndex < m_ySize));
    Q_ASSERT((zIndex >= 0) && (zIndex < m_zSize));

    int index = getIndex(xIndex, yIndex, zIndex);
    m_data[index] = weight;
}

int
CompactGrid::getIndex(int x, int y, int z) const
{
    return z * (m_xSize * m_ySize) + y * m_xSize + x;
}
