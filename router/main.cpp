#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QXmlStreamReader>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QSet>
#include "point.h"
#include "direction.h"
#include "gridstack.h"
#include "bordergrid.h"
#include "customgrid.h"
#include "compactgrid.h"
#include "uniformgrid.h"
#include "routerrules.h"
#include "common/pqueue.h"

typedef QList<Point> Net;
typedef QMap<QString, Net> Netlist;
typedef QSet<Point> Route;
typedef QMap<QString, Route> Routes;

struct Block
{
    QString type;
    Point p;
    int rotation;
};

struct CellState
{
    Direction direction;
    int pathCost;
};

QMap<QString, Point> ports;
QList<Block> blocks;

CompactGrid* costGrid;
GridStack gridStack;
RouterRules rules;
Netlist netlist;
Routes routes;
QMap<QString, int> netColors;

bool
parsePorts(QXmlStreamReader &xml)
{
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "ports"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "port")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                QString name = attributes.value("name").toString();
                QString sx = attributes.value("x").toString();
                QString sy = attributes.value("y").toString();
                QString sz = attributes.value("z").toString();

                if (name.isEmpty())
                {
                    qWarning("port name is invalid");
                    return false;
                }
                if (sx.isEmpty() || sy.isEmpty() || sz.isEmpty())
                {
                    qWarning("port coordinates are invalid");
                    return false;
                }

                bool okX, okY, okZ;
                Point p;

                p.x = sx.toInt(&okX);
                p.y = sy.toInt(&okY);
                p.z = sz.toInt(&okZ);

                if (!okX || !okY || !okZ)
                {
                    qWarning("can't convert port coordinates into numbers");
                    return false;
                }
                ports[name] = p;
            }
        }
    }
    return true;
}

bool
parseBlocks(QXmlStreamReader &xml)
{
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "blocks"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "block")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                QString type = attributes.value("type").toString();
                QString sx = attributes.value("x").toString();
                QString sy = attributes.value("y").toString();
                QString sz = attributes.value("z").toString();
                QString sr = attributes.value("rotation").toString();

                if (type.isEmpty())
                {
                    qWarning("block type is invalid");
                    return false;
                }
                if (sx.isEmpty() || sy.isEmpty() || sz.isEmpty() || sr.isEmpty())
                {
                    qWarning("block coordinates are invalid");
                    return false;
                }

                bool okX, okY, okZ, okR;
                Block b;

                b.type = type;
                b.p.x = sx.toInt(&okX);
                b.p.y = sy.toInt(&okY);
                b.p.z = sz.toInt(&okZ);
                b.rotation = sr.toInt(&okR);

                if (!okX || !okY || !okZ || !okR)
                {
                    qWarning("can't convert block coordinates into numbers");
                    return false;
                }
                blocks.append(b);
            }
        }
    }
    return true;
}

bool
parseNet(QXmlStreamReader &xml, const QString &netName)
{
    Net net;
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "net"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "port")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                QString portName = attributes.value("name").toString();

                if (portName.isEmpty())
                {
                    qWarning("port name is invalid");
                    return false;
                }
                if (!ports.contains(portName))
                {
                    qWarning("can't find port %s", qPrintable(portName));
                    return false;
                }

                Point p = ports[portName];
                net.append(p);
            }
        }
    }
    netlist[netName] = net;
    return true;
}

bool
parseNets(QXmlStreamReader &xml)
{
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "nets"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "net")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                QString netName = attributes.value("name").toString();

                if (netName.isEmpty())
                {
                    qWarning("net name is invalid");
                    return false;
                }

                if (!parseNet(xml, netName)) return false;
            }
        }
    }
    return true;
}

bool
readPlacement(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QFile::ReadOnly))
    {
        qWarning("Can't open input file");
        return false;
    }

    QXmlStreamReader xml(&f);

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartDocument) continue;

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "ports")
            {
                if (!parsePorts(xml)) return false;
            }
            if (xml.name() == "blocks")
            {
                if (!parseBlocks(xml)) return false;
            }
            if (xml.name() == "nets")
            {
                if (!parseNets(xml)) return false;
            }
        }
    }

    if (xml.hasError())
    {
        qWarning("XML error");
        return false;
    }
    return true;
}

bool
parseArea(QXmlStreamReader &xml)
{
    rules.maxX = rules.maxY = rules.maxZ = 0;
    rules.minX = rules.minY = rules.minZ = 0;

    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "area"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if ((xml.name() == "max") || (xml.name() == "min"))
            {
                QXmlStreamAttributes attributes = xml.attributes();
                QString sx = attributes.value("x").toString();
                QString sy = attributes.value("y").toString();
                QString sz = attributes.value("z").toString();

                if (sx.isEmpty() || sy.isEmpty() || sz.isEmpty())
                {
                    qWarning("area coordinates are invalid");
                    return false;
                }

                bool okX, okY, okZ;
                int x, y, z;

                x = sx.toInt(&okX);
                y = sy.toInt(&okY);
                z = sz.toInt(&okZ);

                if (!okX || !okY || !okZ)
                {
                    qWarning("can't convert area coordinates into numbers");
                    return false;
                }

                if (xml.name() == "max")
                {
                    rules.maxX = x;
                    rules.maxY = y;
                    rules.maxZ = z;
                }
                else
                {
                    rules.minX = x;
                    rules.minY = y;
                    rules.minZ = z;
                }
            }
        }
    }
    return true;
}

bool
readJob(const QString &filePath)
{
    rules.useAstarApproximation = false;
    rules.aStarMultiplier = 1;
    rules.sortByHPWL = false;

    QFile f(filePath);
    if (!f.open(QFile::ReadOnly))
    {
        qWarning("Can't open input file");
        return false;
    }

    QXmlStreamReader xml(&f);

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartDocument) continue;

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "area")
            {
                if (!parseArea(xml)) return false;
            }
            if (xml.name() == "useAstar")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                QString multiplierStr = attributes.value("multiplier").toString();
                if (multiplierStr.isEmpty())
                {
                    qWarning("can't get A* multiplier value");
                    return false;
                }

                bool ok;
                rules.aStarMultiplier = multiplierStr.toFloat(&ok);
                if (!ok)
                {
                    qWarning("can't parse A* multiplier value");
                    return false;
                }
            }
            if (xml.name() == "sortByHPWL")
            {
                rules.sortByHPWL = true;
            }
        }
    }

    if (xml.hasError())
    {
        qWarning("XML error");
        return false;
    }
    return true;
}

void
initializeGrid()
{
    /* Construct main grid.  */
    int xSize = rules.maxX - rules.minX + 1;
    int ySize = rules.maxY - rules.minY + 1;
    int zSize = rules.maxZ - rules.minZ + 1;
    costGrid = new CompactGrid(xSize, ySize, zSize, rules.minX, rules.minY, rules.minZ);
    for (int x = rules.minX; x <= rules.maxX; x++)
    for (int y = rules.minY; y <= rules.maxY; y++)
    for (int z = rules.minZ; z <= rules.maxZ; z++)
    {
        costGrid->set(x, y, z, 1);
    }
    gridStack.push(costGrid);

    /* Construct blockage grid.  */
    CustomGrid* blockGrid = new CustomGrid();
    foreach (const Block &block, blocks)
    {
        blockGrid->add(block.p, -1);
    }
    gridStack.push(blockGrid);

    /* Construct border grid.  */
    BorderGrid* borderGrid = new BorderGrid(rules.minX, rules.minY, rules.minZ,
                                            rules.maxX, rules.maxY, rules.maxZ);
    gridStack.push(borderGrid);
}

Direction
getDirectionByIndex(int index)
{
    char di[3] = {0, 0, 0};
    di[index / 2] = (index%2)?1:-1;
    return Direction(di[0], di[1], di[2]);
}

bool
route(const QString &netName, bool allowSharing)
{
    QList<Point> net = netlist[netName];

    QSet<Point> sources; sources.insert(net.first());
    QSet<Point> targets = net.mid(1).toSet();
    QSet<Point> routePoints;
    bool error = false;

    /* Unblock current net.  */
    CustomGrid* grid = new CustomGrid();
    foreach (const Point &p, net)
    {
        grid->add(p, 1);
    }
    gridStack.push(grid);

    while (!targets.isEmpty() && !error)
    {
        QMap<Point, CellState> reached;
        PQueue<int, Point> wavefront;

        /* Initialize.  */
        foreach (const Point &source, sources)
        {
            int cost = gridStack.get(source);
            wavefront.push(cost, source);

            CellState st;
            st.pathCost = cost;
            st.direction = Direction();
            reached[source] = st;
        }

        /* Run.  */
        forever
        {
            if (wavefront.size() == 0)
            {
                qWarning("network %s routing failed", qPrintable(netName));
                error = true;
                break;
            }

            /* Get lowest cost cell.  */
            Point p = wavefront.pop();

            if (targets.contains(p))
            {
                targets.remove(p);

                /* Traceback.  */
                while (!sources.contains(p))
                {
                    routePoints.insert(p);

                    CellState st = reached[p];

                    p = p - st.direction;
                }

                routePoints.insert(p);
                sources = routePoints;

                break;
            }

            /* Expand.  */
            CellState st = reached[p];
            for (int directionIndex = 0; directionIndex < 6; directionIndex++)
            {
                Direction nextDirection = getDirectionByIndex(directionIndex);

                Point next = p + nextDirection;
                if (reached.contains(next)) continue;

                int weight = gridStack.get(next);
                if (weight == -1) continue;

                int nextPathCost = st.pathCost + weight;

                /* Calculate cost approximation.  */
                int costApproximation = nextPathCost;
                if (rules.useAstarApproximation)
                {
                    /* Calculate nearest target.  */
                    Point target = targets.toList().first();
                    foreach (const Point &t, targets)
                    {
                        if ((t - next).length() < (target - next).length())
                        {
                            target = t;
                        }
                    }

                    Point delta = next - target;
                    int distanceApproximation = delta.length();

                    costApproximation += distanceApproximation * rules.aStarMultiplier;
                }

                CellState st2;
                st2.direction = nextDirection;
                st2.pathCost = nextPathCost;
                reached[next] = st2;

                wavefront.push(costApproximation, next);

                //qDebug("[%d;%d]@%d[%d;%d]->[%d;%d]@%d d[%d;%d]", p.x, p.y, st.pathCost, st.direction.x, st.direction.y,
                //                       next.x, next.y, nextPathCost, nextDirection.x, nextDirection.y);
            }
        }
    }

    /* Remove unblocking grid.  */
    delete gridStack.pop();

    if (!error)
    {
        if (!allowSharing)
        {
            /* Mark all route points as blockages.  */
            CustomGrid* grid = new CustomGrid();
            foreach (const Point &p, routePoints)
            {
                grid->add(p, -1);
            }
            gridStack.push(grid);
        }

        /* Register route.  */
        routes[netName] = routePoints;

        /*QList<Point> ps = routePoints.toList();
        qSort(ps);
        qDebug("\nNet %s", qPrintable(netName));
        foreach (const Point &p, net)
        {
            qDebug("[%3d;%3d;%3d]", p.x, p.y, p.z);
        }
        qDebug("Route:");
        foreach (const Point &p, ps)
        {
            qDebug("[%3d;%3d;%3d]", p.x, p.y, p.z);
        }*/
    }
    /*else
    {
        qDebug("\nNet %s", qPrintable(netName));
        foreach (const Point &p, net)
        {
            qDebug("[%3d;%3d;%3d]", p.x, p.y, p.z);
            foreach (const QString &netName, routes.keys())
            {
                Route route = routes[netName];
                foreach (const Point &p2, route)
                {
                    if ((p - p2).length() == 1)
                    {
                        qDebug("  [%3d;%3d;%3d] %s", p2.x, p2.y, p2.z, qPrintable(netName));
                    }
                }
            }
            foreach (const Block &b, blocks)
            {
                if ((p - b.p).length() == 1)
                {
                    qDebug("  [%3d;%3d;%3d] %s", b.p.x, b.p.y, b.p.z, qPrintable(b.type));
                }
            }
        }
    }*/
    return !error;
}

bool
routeAllNets(bool oneShot)
{
    QList<QString> nets = netlist.keys();

    if (rules.sortByHPWL)
    {
        QMultiMap<int, QString> netMap; // HPWL->netName
        foreach (const QString &netName, nets)
        {
            QList<Point> net = netlist[netName];

            qint16 minX = rules.maxX, maxX = rules.minX;
            qint16 minY = rules.maxY, maxY = rules.minY;
            qint16 minZ = rules.maxZ, maxZ = rules.minZ;
            foreach (const Point &p, net)
            {
                minX = qMin(minX, p.x);
                maxX = qMax(maxX, p.x);
                minY = qMin(minX, p.y);
                maxY = qMax(maxX, p.y);
                minZ = qMin(minX, p.z);
                maxZ = qMax(maxX, p.z);
            }

            int hpwl = qAbs(maxX - minX) + qAbs(maxY - minY) + qAbs(maxZ - minZ);
            netMap.insertMulti(hpwl, netName);

            nets = netMap.values();
        }
    }

    if (oneShot)
    {
        foreach (const QString &netName, nets)
        {
            qDebug("Routing net %s...", qPrintable(netName));
            route(netName, false);
        }
    }
    else
    {
        QMap<Point, int> conflictCounters;
        bool hasConflicts = false;
        for (int iteration = 0; iteration < 10000; iteration++)
        {
            qDebug("Iteration %d...", iteration);

            foreach (const QString &netName, nets)
            {
                if (!routes.contains(netName))
                {
                    if (!route(netName, true))
                    {
                        qWarning("Can't route net %s", qPrintable(netName));
                        return false;
                    }
                }
            }

            /* Check for conflicts.  */
            hasConflicts = false;
            QMap<Point, QList<QString> > conflictingNets;
            foreach (const QString &netName, nets)
            {
                Route route = routes[netName];
                foreach (const Point &p, route)
                {
                    conflictingNets[p].append(netName);
                }
            }
            foreach (const Point &p, conflictingNets.keys())
            {
                if (conflictingNets[p].size() > 1)
                {
                    hasConflicts = true;

                    conflictCounters[p] += 1;
                    int newWeight = (1 + conflictCounters[p]) * (conflictingNets[p].size() - 1);
                    costGrid->set(p.x, p.y, p.z, newWeight);

                    foreach (const QString &netName, conflictingNets[p])
                    {
                        routes.remove(netName);
                    }
                }
            }
            if (!hasConflicts) break;

            qDebug("%d/%d nets unrouted", nets.size() - routes.size(), nets.size());
        }
        if (hasConflicts)
        {
            qWarning("We still have conflicts!");
        }
    }
    return routes.size() == nets.size();
}

bool
colorize()
{
    int colors = 16;

    QMap<Point, QString> pointMap;
    QMap<QString, int> colorMap;

    foreach (const QString &netName, routes.keys())
    {
        Route route = routes[netName];
        foreach (const Point &p, route)
        {
            pointMap[p] = netName;
        }
    }

    int maxColor = -1;
    foreach (const QString &netName, routes.keys())
    {
        Route route = routes[netName];
        QSet<int> adjColors;
        foreach (const Point &p, route)
        {
            for (int dirIndex = 0; dirIndex < 6; dirIndex++)
            {
                Point p2 = p + getDirectionByIndex(dirIndex);
                if (pointMap.contains(p2))
                {
                    QString netName2 = pointMap.value(p2);
                    if (netName2 != netName)
                    {
                        if (colorMap.contains(netName2))
                        {
                            adjColors.insert(colorMap.value(netName2));
                        }
                    }
                }
            }
        }

        int netColor = -1;
        for (int color = 0; color < colors; color++)
        {
            if (!adjColors.contains(color))
            {
                netColor = color;
                break;
            }
        }
        if (netColor == -1)
        {
            qWarning("Can't get color for net %s", qPrintable(netName));
            return false;
        }
        colorMap[netName] = netColor;
        maxColor = qMax(maxColor, netColor);
    }

    netColors = colorMap;
    qDebug("Colors used: %d", maxColor + 1);

    return true;
}

bool
saveResults(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
    {
        qCritical("Can't open XML file");
        return 1;
    }

    QXmlStreamWriter stream(&f);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("blocks");

    foreach (const Block &block, blocks)
    {
        if (block.type == "$blockage") continue;

        stream.writeStartElement("block");
        stream.writeAttribute("type", block.type);
        stream.writeAttribute("rotation", QString::number(block.rotation));
        stream.writeAttribute("x", QString::number(block.p.x));
        stream.writeAttribute("y", QString::number(block.p.y));
        stream.writeAttribute("z", QString::number(block.p.z));
        stream.writeEndElement();
    }
    int wirelength = 0;
    foreach (const QString &netName, routes.keys())
    {
        stream.writeComment(QString(" Net '%1' ").arg(netName));
        int color = netColors.value(netName, -1);

        /* Place wires on pads.  */
        foreach (const Point &p, netlist[netName])
        {
            stream.writeStartElement("block");
            stream.writeAttribute("type", QString("$wire%1").arg(color));
            stream.writeAttribute("rotation", "-1");
            stream.writeAttribute("x", QString::number(p.x));
            stream.writeAttribute("y", QString::number(p.y));
            stream.writeAttribute("z", QString::number(p.z));
            stream.writeEndElement();
        }

        QList<Point> route = routes[netName].toList();
        qSort(route);
        foreach (const Point &p, route)
        {
            stream.writeStartElement("block");
            stream.writeAttribute("type", QString("$fswire%1").arg(color));
            stream.writeAttribute("rotation", "-1");
            stream.writeAttribute("x", QString::number(p.x));
            stream.writeAttribute("y", QString::number(p.y));
            stream.writeAttribute("z", QString::number(p.z));
            stream.writeEndElement();
            wirelength++;
        }
    }

    stream.writeEndElement();

    stream.writeEndDocument();
    f.close();

    qDebug("Wirelength: %d", wirelength);

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Placer");
    parser.addHelpOption();

    parser.addPositionalArgument("placement", QCoreApplication::translate("main", "Placement file"));
    parser.addPositionalArgument("job", QCoreApplication::translate("main", "Routing job file"));
    parser.addPositionalArgument("result", QCoreApplication::translate("main", "Routing result"));

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (!readPlacement(args[0]))
    {
        qWarning("Can't parse placement file");
        return 1;
    }

    if (!readJob(args[1]))
    {
        qWarning("Can't parse job file");
        return 1;
    }

    initializeGrid();

    if (!routeAllNets(false))
    {
        qWarning("Can't route");
        return 1;
    }

    if (!colorize())
    {
        qWarning("Can't colorize");
        return 1;
    }

    saveResults(args[2]);

    return 0;
}
