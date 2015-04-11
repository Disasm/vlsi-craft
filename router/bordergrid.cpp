#include "bordergrid.h"

BorderGrid::BorderGrid(int x1, int y1, int z1, int x2, int y2, int z2)
{
    m_x1 = x1;
    m_x2 = x2;
    m_y1 = y1;
    m_y2 = y2;
    m_z1 = z1;
    m_z2 = z2;
}

int BorderGrid::get(Point p) const
{
    if ((p.x < m_x1) || (m_x2 < p.x)) return -1;
    if ((p.y < m_y1) || (m_y2 < p.y)) return -1;
    if ((p.z < m_z1) || (m_z2 < p.z)) return -1;
    return 0;
}
