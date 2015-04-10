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
#include <QStringList>

typedef struct {
    //int n, nGates, nPads;
    //QMap<int, QString> indexToName;
    //QMap<QString, int> nameToIndex;
    QSet<QString> gates;
    QSet<QString> pads;

    QMap<QString, QPointF> coords; // index->point
    QMultiMap<QString, QString> nets; // net->index

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

    job.gates = gates.toSet();
    job.pads = pads.toSet();

    foreach (const QString &padName, pads)
    {
        int x, y, z;
        if (!netlist->position(padName, x, y, z))
        {
            qWarning("Can't get pad coordinates for %s", qPrintable(padName));
            return false;
        }
        job.coords[padName] = QPointF(x, y);
    }

    QList<QString> nets = netlist->allNets();
    for (int i = 0; i < nets.size(); i++)
    {
        QString netName = nets[i];

        QList<QString> items = netlist->itemsByNet(netName);
        foreach (const QString &item, items)
        {
            if (!job.gates.contains(item) && !job.pads.contains(item))
            {
                qWarning("Can't find item %s", qPrintable(item));
                return false;
            }
            job.nets.insertMulti(netName, item);
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
    foreach (const QString &gateName, job.gates)
    {
        QPointF p = job.coords[gateName];

        GatePlacement gp(p.x(), p.y(), 0);
        placement[gateName] = gp;
    }
}

void solveQP(Job &job)
{
    // Build equivalent graph
    QMap<QPair<QString, QString>, double> edgeWeights;
    foreach (const QString &net, job.nets.keys().toSet())
    {
        QList<QString> points = job.nets.values(net);
        if (points.size() <= 1) continue;
        if (points.size() == 2)
        {
            QString p1 = points.first();
            QString p2 = points.last();
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
    CooMatrix C(job.gates.size());

    QMap<QString, int> gateIndexes;
    QMap<int, QString> gateIndexesRev;
    foreach (const QString &gateName, job.gates)
    {
        int index = gateIndexes.size();
        gateIndexes[gateName] = index;
        gateIndexesRev[index] = gateName;
    }

    QList<QPair<QString, QString> > edges = edgeWeights.keys();
    for (int i = 0; i < edges.size(); i++)
    {
        QPair<QString, QString> edge = edges[i];
        if (job.gates.contains(edge.first) && job.gates.contains(edge.second))
        {
            int i1 = gateIndexes[edge.first];
            int i2 = gateIndexes[edge.second];
            C.add(i1, i2, edgeWeights[edge]);
        }
    }

    // Build matrix A
    CooMatrix A(job.gates.size());

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
        QPair<QString, QString> edge = edges[i];
        if (job.pads.contains(edge.first) && job.gates.contains(edge.second))
        {
            int i2 = gateIndexes[edge.second];
            sums[i2] += edgeWeights[edge];
        }
    }
    for (int i = 0; i < job.gates.size(); i++)
    {
        A.add(i, i, sums[i]);
    }

    // Build bx and by
    Array bx(job.gates.size());
    Array by(job.gates.size());

    for (int i = 0; i < edges.size(); i++)
    {
        QPair<QString, QString> edge = edges[i];
        if (job.pads.contains(edge.first) && job.gates.contains(edge.second))
        {
            int i2 = gateIndexes[edge.second];
            bx[i2] += edgeWeights[edge] * (job.coords[edge.first].x() - job.topLeft.x());
            by[i2] += edgeWeights[edge] * (job.coords[edge.first].y() - job.topLeft.y());
        }
    }

    // Solve
    Array x = A.solve(bx);
    Array y = A.solve(by);
    for (int i = 0; i < job.gates.size(); i++)
    {
        QPointF p(x[i] + job.topLeft.x(), y[i] + job.topLeft.y());

        QString gateName = gateIndexesRev[i];
        job.coords[gateName] = p;
    }
}

bool operator <(const QPointF &a, const QPointF &b)
{
    if (a.x() < b.x()) return true;
    if (a.x() > b.x()) return false;
    return a.y() < b.y();
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

    QList<QPair<QPointF, QString> > coords;
    foreach (const QString &gateName, parentJob.gates)
    {
        QPointF p = parentJob.coords.value(gateName);
        QPointF p2 = splitByX?p:QPointF(p.y(), p.x());
        coords.append(qMakePair(p2, gateName));
    }
    qSort(coords.begin(), coords.end(), QPairFirstComparer());

    int half = coords.size() / 2;
    QList<QString> gates1, gates2;
    for (int i = 0; i < coords.size(); i++)
    {
        QPair<QPointF, QString> p = coords[i];
        if (i < half)
        {
            gates1.append(p.second);
        }
        else
        {
            gates2.append(p.second);
        }
    }

    // Create gates
    childJob1.gates = gates1.toSet();
    childJob2.gates = gates2.toSet();

    // Create pads
    childJob1.pads = parentJob.pads + childJob2.gates;
    childJob2.pads = parentJob.pads + childJob1.gates;

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
    childJob2.size = QSizeF(childJob2.bottomRight.x() - childJob2.topLeft.x(), childJob2.bottomRight.y() - childJob2.topLeft.y());

    // Recalculate coordinates
    foreach (const QString &padName, childJob1.pads)
    {
        QPointF p = parentJob.coords.value(padName);

        p.setX(qMin(p.x(), childJob1.bottomRight.x()));
        p.setX(qMax(p.x(), childJob1.topLeft.x()));
        p.setY(qMin(p.y(), childJob1.bottomRight.y()));
        p.setY(qMax(p.y(), childJob1.topLeft.y()));

        childJob1.coords[padName] = p;
    }
    foreach (const QString &padName, childJob2.pads)
    {
        QPointF p = parentJob.coords.value(padName);

        p.setX(qMin(p.x(), childJob2.bottomRight.x()));
        p.setX(qMax(p.x(), childJob2.topLeft.x()));
        p.setY(qMin(p.y(), childJob2.bottomRight.y()));
        p.setY(qMax(p.y(), childJob2.topLeft.y()));

        childJob2.coords[padName] = p;
    }

    // Recalculate nets
    QSet<QString> nets = parentJob.nets.keys().toSet();
    foreach (const QString &net, nets)
    {
        QList<QString> itemNames = parentJob.nets.values(net);
        bool hasGates1 = false;
        bool hasGates2 = false;
        QList<QString> names;
        foreach (const QString &itemName, itemNames)
        {
            names.append(itemName);

            if (parentJob.gates.contains(itemName))
            {
                if (childJob1.gates.contains(itemName))
                {
                    hasGates1 = true;
                }

                if (childJob2.gates.contains(itemName))
                {
                    hasGates2 = true;
                }
            }
        }
        if (hasGates1)
        {
            foreach (const QString &name, names)
            {
                childJob1.nets.insertMulti(net, name);
            }
        }
        if (hasGates2)
        {
            foreach (const QString &name, names)
            {
                childJob2.nets.insertMulti(net, name);
            }
        }
    }
}

void
backAnnotate(Job &parentJob, const Job &childJob)
{
    foreach (const QString &gateName, childJob.gates)
    {
        parentJob.coords[gateName] = childJob.coords[gateName];
    }
}

float
calculateHPWL(Job &job)
{
    float hpwl = 0;

    QSet<QString> nets = job.nets.keys().toSet();
    foreach (const QString &netName, nets)
    {
        QList<QString> items = job.nets.values(netName);
        QList<QPointF> points;
        foreach (const QString &itemName, items)
        {
            if (!job.coords.contains(itemName))
            {
                return -1;
            }
            QPointF p = job.coords.value(itemName);
            points.append(p);
        }

        QPointF min = points.first();
        QPointF max = min;
        foreach (const QPointF &p, points)
        {
            min.setX(qMin(min.x(), p.x()));
            min.setY(qMin(min.y(), p.y()));

            max.setX(qMax(max.x(), p.x()));
            max.setY(qMax(max.y(), p.y()));
        }

        hpwl += qAbs(max.x() - min.x()) + qAbs(max.y() - min.y());
    }
    return hpwl;
}

void
printSolution(Job &job)
{
    QList<QString> gates = job.gates.toList();
    qSort(gates);

    foreach (const QString &gateName, gates)
    {
        QPointF p = job.coords.value(gateName);
        qDebug("%s [%5.2f;%5.2f]", qPrintable(gateName), p.x(), p.y());
    }
}

void
solveRecursively(Job &job)
{
    //qDebug("QP[0]: %d gates, HPWL %0.2f", job.gates.size(), calculateHPWL(job));
    solveQP(job);
    //qDebug("QP[1]: %d gates, HPWL %0.2f", job.gates.size(), calculateHPWL(job));
    //printSolution(job);

    if (job.gates.size() >= 6)
    {
        Job childJob1;
        Job childJob2;

        split(childJob1, childJob2, job);

        solveRecursively(childJob1);
        solveRecursively(childJob2);

        backAnnotate(job, childJob1);
        backAnnotate(job, childJob2);
    }

    //qDebug("QP[2]: %d gates, HPWL %0.2f", job.gates.size(), calculateHPWL(job));
    //printSolution(job);
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
