#include "qp.h"
#include <QString>
#include <QMap>
#include <QMultiMap>
#include <QSet>
#include <QPointF>
#include <QSizeF>
#include "coomatrix.h"
#include "common/netlist.h"
#include "placementjob.h"

typedef struct {
    int n, nGates, nPads;
    QMap<int, QString> indexToName;
    QMap<QString, int> nameToIndex;
    QSet<int> gateIndexes;
    QSet<int> padIndexes;

    QMap<int, QPointF> coords; // index->point
    QMultiMap<int, int> nets; // net->index

    // Geometrical parameters
    QPointF topLeft;
    QPointF bottomRight;
    QSizeF size;
} Job;

bool
readJob(Job &job, const Netlist *netlist, const PlacementJob *placementJob)
{
    QList<QString> gates = netlist->allGates();
    QList<QString> pads = netlist->allPads();

    job.n = 0;
    job.nGates = gates.size();
    job.nPads = pads.size();

    for (int i = 0; i < job.nGates; i++)
    {
        QString name = gates[i];
        int index = job.n++;

        job.indexToName[index] = name;
        job.nameToIndex[name] = index;
        job.gateIndexes.insert(index);
    }

    for (int i = 0; i < job.nPads; i++)
    {
        QString name = pads[i];
        int index = job.n++;

        job.indexToName[index] = name;
        job.nameToIndex[name] = index;
        job.padIndexes.insert(index);

        int x, y, z;
        if (!netlist->position(name, x, y, z))
        {
            qWarning("Can't get pad coordinates for %s", qPrintable(name));
            return false;
        }
        job.coords[index] = QPointF(x, y);
    }

    QList<QString> nets = netlist->allNets();
    for (int i = 0; i < nets.size(); i++)
    {
        QString netName = nets[i];

        QList<QString> items = netlist->itemsByNet(netName);
        foreach (const QString &item, items)
        {
            int itemIndex = job.nameToIndex.value(item, -1);
            if (itemIndex == -1)
            {
                qWarning("Can't get index for item %s", qPrintable(item));
                return false;
            }
            job.nets.insertMulti(i, itemIndex);
        }
    }

    Vector<int> min = placementJob->minCoordinates();
    Vector<int> max = placementJob->maxCoordinates();

    job.topLeft = QPointF(min.x, min.y);
    job.bottomRight = QPointF(max.x, max.y);
    job.size = QSizeF(job.bottomRight.x() - job.topLeft.x(), job.bottomRight.y() - job.topLeft.y());

    return true;
}

void
savePlacement(NetPlacement &placement, const Job &job)
{
    foreach (int index, job.gateIndexes)
    {
        QString name = job.indexToName[index];
        QPointF p = job.coords[index];

        GatePlacement gp(p.x(), p.y(), 0);
        placement[name] = gp;
    }
}

void solveQP(Job &job)
{
    // Build equivalent graph
    QMap<QPair<int, int>, double> edgeWeights;
    foreach (int net, job.nets.keys().toSet())
    {
        QList<int> points = job.nets.values(net);
        if (points.size() <= 1) continue;
        if (points.size() == 2)
        {
            int p1 = points.first();
            int p2 = points.last();
            edgeWeights[qMakePair(p1, p2)] += 1.0;
            edgeWeights[qMakePair(p2, p1)] += 1.0;
        }
        else
        {
            double k = 1.0 / (points.size() - 1);
            for (int i = 0; i < points.size(); i++)
            {
                for (int j = 0; j < points.size(); j++)
                {
                    if (i != j)
                    {
                        edgeWeights[qMakePair(points[i], points[j])] += k;
                    }
                }
            }
        }
    }

    // Build Connectivity matrix
    CooMatrix C(job.nGates);

    QList<QPair<int, int> > edges = edgeWeights.keys();
    for (int i = 0; i < edges.size(); i++)
    {
        QPair<int, int> edge = edges[i];
        if (job.gateIndexes.contains(edge.first) && job.gateIndexes.contains(edge.second))
        {
            C.add(edge.first, edge.second, edgeWeights[edge]);
        }
    }

    // Build matrix A
    CooMatrix A(job.nGates);

    QMap<int, double> sums;
    for (int i = 0; i < C.nnz(); i++)
    {
        int x, y;
        double v;

        v = C.getElement(i, x, y);
        A.add(x, y, -v);

        sums[x] += v;
    }
    for (int i = 0; i < edges.size(); i++)
    {
        QPair<int, int> edge = edges[i];
        if (job.padIndexes.contains(edge.first) && job.gateIndexes.contains(edge.second))
        {
            sums[edge.second] += edgeWeights[edge];
        }
    }
    for (int i = 0; i < job.nGates; i++)
    {
        A.add(i, i, sums[i]);
    }

    // Build bx and by
    Array bx(job.nGates);
    Array by(job.nGates);

    for (int i = 0; i < edges.size(); i++)
    {
        QPair<int, int> edge = edges[i];
        if (job.padIndexes.contains(edge.first) && job.gateIndexes.contains(edge.second))
        {
            bx[edge.second] += edgeWeights[edge] * (job.coords[edge.first].x() - job.topLeft.x());
            by[edge.second] += edgeWeights[edge] * (job.coords[edge.first].y() - job.topLeft.y());
        }
    }

    // Solve
    Array x = A.solve(bx);
    Array y = A.solve(by);
    for (int i = 0; i < job.nGates; i++)
    {
        QPointF p(x[i] + job.topLeft.x(), y[i] + job.topLeft.y());

        job.coords[i] = p;
    }
}

struct QPairFirstComparer
{
    template<typename T1, typename T2>
    bool operator()(const QPair<T1,T2> & a, const QPair<T1,T2> & b) const
    {
        return a.first < b.first;
    }
};

void
split(Job &childJob1, Job &childJob2, const Job &parentJob)
{
    bool splitByX = !(parentJob.size.height() > 1.5 * parentJob.size.width());

    QList<QPair<double, int> > coords;
    for (int i = 0; i < parentJob.nGates; i++)
    {
        QPointF p = parentJob.coords.value(i);
        coords.append(qMakePair(splitByX?p.x():p.y(), i));
    }
    qSort(coords.begin(), coords.end(), QPairFirstComparer());

    int half = coords.size() / 2;
    QList<int> ind1, ind2;
    for (int i = 0; i < coords.size(); i++)
    {
        QPair<double, int> p = coords[i];
        if (i < half)
        {
            ind1.append(p.second);
        }
        else
        {
            ind2.append(p.second);
        }
    }

    childJob1.nGates = ind1.size();
    childJob2.nGates = ind2.size();

    // Create pads for all other gates
    childJob1.nPads = parentJob.nPads + childJob2.nGates;
    childJob2.nPads = parentJob.nPads + childJob1.nGates;

    // Update gate names and indexes
    for (int i = 0; i < ind1.size(); i++)
    {
        int oldIndex = ind1[i];
        QString name = parentJob.indexToName[oldIndex];
        childJob1.indexToName[i] = name;
        childJob1.nameToIndex[name] = i;
        childJob1.gateIndexes.insert(i);
    }
    for (int i = 0; i < ind2.size(); i++)
    {
        int oldIndex = ind2[i];
        QString name = parentJob.indexToName[oldIndex];
        childJob2.indexToName[i] = name;
        childJob2.nameToIndex[name] = i;
        childJob2.gateIndexes.insert(i);
    }
    childJob1.n = ind1.size();
    childJob2.n = ind2.size();
    for (int i = 0; i < parentJob.nPads; i++)
    {
        int oldIndex = parentJob.nGates + i;
        QString name = parentJob.indexToName[oldIndex];

        int i1 = childJob1.n + i;
        int i2 = childJob1.n + i;
        childJob1.indexToName[i1] = name;
        childJob2.indexToName[i2] = name;
        childJob1.nameToIndex[name] = i1;
        childJob2.nameToIndex[name] = i2;
        childJob1.padIndexes.insert(i1);
        childJob2.padIndexes.insert(i2);
    }
    childJob1.n += parentJob.nPads;
    childJob2.n += parentJob.nPads;
    for (int i = 0; i < ind1.size(); i++)
    {
        int oldIndex = ind1[i];
        QString name = parentJob.indexToName[oldIndex];
        int newIndex = childJob2.n + i;
        childJob2.indexToName[newIndex] = name;
        childJob2.nameToIndex[name] = newIndex;
        childJob2.padIndexes.insert(newIndex);
    }
    for (int i = 0; i < ind2.size(); i++)
    {
        int oldIndex = ind2[i];
        QString name = parentJob.indexToName[oldIndex];
        int newIndex = childJob1.n + i;
        childJob1.indexToName[newIndex] = name;
        childJob1.nameToIndex[name] = newIndex;
        childJob1.padIndexes.insert(newIndex);
    }
    childJob1.n += ind2.size();
    childJob2.n += ind1.size();

    // Update geometrical parameters
    childJob1.topLeft = parentJob.topLeft;
    childJob2.bottomRight = parentJob.bottomRight;
    if (splitByX)
    {
        float x = (parentJob.topLeft.x() + parentJob.bottomRight.x()) / 2;
        childJob1.bottomRight = QPointF(x, parentJob.bottomRight.y());
        childJob2.topLeft = QPointF(x, parentJob.topLeft.y());
    }
    else
    {
        float y = (parentJob.topLeft.y() + parentJob.bottomRight.y()) / 2;
        childJob1.bottomRight = QPointF(parentJob.bottomRight.x(), y);
        childJob2.topLeft = QPointF(parentJob.topLeft.x(), y);
    }
    childJob1.size = QSizeF(childJob1.bottomRight.x() - childJob1.topLeft.x(), childJob1.bottomRight.y() - childJob1.topLeft.y());

    // Recalculate coordinates
    for (int i = 0; i < childJob1.nPads; i++)
    {
        int newPadIndex = childJob1.nGates + i;
        QString name = childJob1.indexToName[newPadIndex];
        int oldIndex = parentJob.nameToIndex[name];
        QPointF p = parentJob.coords.value(oldIndex);

        p.setX(qMin(p.x(), childJob1.bottomRight.x()));
        p.setX(qMax(p.x(), childJob1.topLeft.x()));
        p.setY(qMin(p.y(), childJob1.bottomRight.y()));
        p.setY(qMax(p.y(), childJob1.topLeft.y()));

        childJob1.coords[newPadIndex] = p;
    }
    for (int i = 0; i < childJob2.nPads; i++)
    {
        int newPadIndex = childJob2.nGates + i;
        QString name = childJob2.indexToName[newPadIndex];
        int oldIndex = parentJob.nameToIndex[name];
        QPointF p = parentJob.coords.value(oldIndex);

        p.setX(qMin(p.x(), childJob2.bottomRight.x()));
        p.setX(qMax(p.x(), childJob2.topLeft.x()));
        p.setY(qMin(p.y(), childJob2.bottomRight.y()));
        p.setY(qMax(p.y(), childJob2.topLeft.y()));

        childJob2.coords[newPadIndex] = p;
    }

    // Recalculate nets
    QSet<int> nets = parentJob.nets.keys().toSet();
    foreach (int net, nets)
    {
        QList<int> oldIndexes = parentJob.nets.values(net);
        bool hasGates1 = false;
        bool hasGates2 = false;
        QList<QString> names;
        foreach (int oldIndex, oldIndexes)
        {
            QString name = parentJob.indexToName[oldIndex];
            names.append(name);

            if (parentJob.gateIndexes.contains(oldIndex))
            {
                int i1 = childJob1.nameToIndex.value(name, -1);
                if (childJob1.gateIndexes.contains(i1))
                {
                    hasGates1 = true;
                }

                int i2 = childJob2.nameToIndex.value(name, -1);
                if (childJob2.gateIndexes.contains(i2))
                {
                    hasGates2 = true;
                }
            }
        }
        if (hasGates1)
        {
            foreach (const QString &name, names)
            {
                int i = childJob1.nameToIndex.value(name, -1);
                if (i != -1) childJob1.nets.insertMulti(net, i);
            }
        }
        if (hasGates2)
        {
            foreach (const QString &name, names)
            {
                int i = childJob2.nameToIndex.value(name, -1);
                if (i != -1) childJob2.nets.insertMulti(net, i);
            }
        }
    }
}

void
backAnnotate(Job &parentJob, const Job &childJob)
{
    for (int i = 0; i < childJob.nGates; i++)
    {
        QString name = childJob.indexToName[i];
        QPointF c = childJob.coords[i];

        int parentIndex = parentJob.nameToIndex[name];
        parentJob.coords[parentIndex] = c;
    }
}

void
solveRecursively(Job &job)
{
    solveQP(job);

    if (job.nGates >= 4)
    {
        Job childJob1;
        Job childJob2;

        split(childJob1, childJob2, job);

        solveRecursively(childJob1);
        solveRecursively(childJob2);

        backAnnotate(job, childJob1);
        backAnnotate(job, childJob2);
    }
}

NetPlacement
placeQuad(const Netlist *netlist, const PlacementJob *placementJob)
{
    NetPlacement placement;

    Job job;
    if (!readJob(job, netlist, placementJob))
    {
        qWarning("Can't initialize QP job");
        return placement;
    }

    solveRecursively(job);

    savePlacement(placement, job);

    return placement;
}
