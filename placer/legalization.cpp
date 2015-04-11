#include "legalization.h"
#include "common/netlist.h"
#include "placementjob.h"
#include <math.h>

bool
legalize(NetPlacement &p, const Netlist *netlist, const PlacementJob *placementJob)
{
    Vector<int> min = placementJob->minCoordinates();
    Vector<int> max = placementJob->maxCoordinates();

    qDebug("BoundingBox: [%d;%d] [%d;%d]", min.x, min.z, max.x, max.z);

    int xSize = max.x - min.x + 1;
    int zSize = max.z - min.z + 1;

    int blockSize = 3;

    int xBlocks = xSize / blockSize;
    int zBlocks = zSize / blockSize;

    qDebug("Blocks: %dx%d", xBlocks, zBlocks);

    if ((xBlocks * zBlocks) < p.size())
    {
        qDebug("No enough place for legalization");
        return false;
    }

    QSet<QString> unplacedGates = p.keys().toSet();
    for (int xi = 0; xi < xBlocks; xi++)
    {
        for (int zi = 0; zi < zBlocks; zi++)
        {
            int xc = min.x + xi * blockSize + (blockSize / 2);
            int zc = min.z + zi * blockSize + (blockSize / 2);

            float minDistance = xSize + zSize;
            QString minGate;

            foreach (const QString &gateName, unplacedGates)
            {
                GatePlacement gp = p[gateName];

                float dx = xc - gp.x;
                float dz = zc - gp.z;
                float d = sqrtf(dx*dx + dz*dz);
                if (d < minDistance)
                {
                    minDistance = d;
                    minGate = gateName;
                }
            }

            Q_ASSERT(!minGate.isEmpty());

            GatePlacement &gp = p[minGate];
            gp.x = xc - 1;
            gp.y = min.y;
            gp.z = zc - 1;
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
