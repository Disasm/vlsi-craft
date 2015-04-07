#include "legalization.h"
#include "common/netlist.h"
#include "placementjob.h"
#include <math.h>

bool
legalize(NetPlacement &p, const Netlist *netlist, const PlacementJob *placementJob)
{
    Vector<int> min = placementJob->minCoordinates();
    Vector<int> max = placementJob->maxCoordinates();

    qDebug("BoundingBox: [%d;%d] [%d;%d]", min.x, min.y, max.x, max.y);

    int xSize = max.x - min.x + 1;
    int ySize = max.y - min.y + 1;

    int xBlocks = xSize / 3;
    int yBlocks = ySize / 3;

    qDebug("Blocks: %dx%d", xBlocks, yBlocks);

    if ((xBlocks * yBlocks) < p.size())
    {
        qDebug("No enough place for legalization");
        return false;
    }

    QSet<QString> unplacedGates = p.keys().toSet();
    for (int xi = 0; xi < xBlocks; xi++)
    {
        for (int yi = 0; yi < yBlocks; yi++)
        {
            int xc = min.x + xi * 3 + 1;
            int yc = min.y + yi * 3 + 1;

            float minDistance = xSize + ySize;
            QString minGate;

            foreach (const QString &gateName, unplacedGates)
            {
                GatePlacement gp = p[gateName];

                float dx = xc - gp.x;
                float dy = yc - gp.y;
                float d = sqrtf(dx*dx + dy*dy);
                if (d < minDistance)
                {
                    minDistance = d;
                    minGate = gateName;
                }
            }

            Q_ASSERT(!minGate.isEmpty());

            GatePlacement &gp = p[minGate];
            gp.x = xc - 1;
            gp.y = yc - 1;
            gp.z = min.z;
            unplacedGates.remove(minGate);

            if (unplacedGates.isEmpty()) break;
        }
        if (unplacedGates.isEmpty()) break;
    }

    if (!unplacedGates.isEmpty())
    {
        qDebug("Can't legalize");
        return false;
    }

    return true;
}
