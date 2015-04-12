#include <QCoreApplication>
#include <QCommandLineParser>
#include <QStringList>
#include <QFile>
#include <QXmlStreamReader>
#include <QTextStream>
#include <QList>
#include <QMap>
#include <QSet>
#include <QPoint>
#include <QRect>
#include "common/vector.h"

typedef Vector<int> Point;

struct Block
{
    QString type;
    Point p;
    int rotation;
};

enum CommandType
{
    PLACE,
    LOAD
};

struct Command
{
    CommandType type;
    Block block;
    int amount;
};

struct TurtleState
{
    Point p;
    QMap<QString, int> blockTypeIndex;
};

Point originPoint;
Point originDirection;
QList<Block> blocks;
QList<Block> chests;
QList<QString> gCode;

bool
readNumber(int &v, const QString &name, QXmlStreamAttributes &attributes)
{
    QString s = attributes.value(name).toString();
    if (s.isEmpty()) return false;

    bool ok;
    v = s.toInt(&ok);

    return ok;
}

bool
readPoint(Point &p, QXmlStreamAttributes &attributes)
{
    if (!readNumber(p.x, "x", attributes)) return false;
    if (!readNumber(p.y, "y", attributes)) return false;
    if (!readNumber(p.z, "z", attributes)) return false;
    return true;
}

bool
readJob(const QString &filePath)
{
    originPoint = Point();
    originDirection = Point();

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
            if (xml.name() == "origin")
            {
                QXmlStreamAttributes attributes = xml.attributes();

                if (!readPoint(originPoint, attributes))
                {
                    qWarning("origin coordinates are invalid");
                    return false;
                }
                if (!readNumber(originDirection.x, "dx", attributes))
                {
                    qWarning("origin direction is invalid");
                    return false;
                }
                if (!readNumber(originDirection.z, "dz", attributes))
                {
                    qWarning("origin direction is invalid");
                    return false;
                }
            }
            if (xml.name() == "chest")
            {
                QXmlStreamAttributes attributes = xml.attributes();

                Block b;

                b.type = attributes.value("type").toString();
                if (b.type.isEmpty())
                {
                    qWarning("block type is invalid");
                    return false;
                }

                if (!readPoint(b.p, attributes))
                {
                    qWarning("block coordinates are invalid");
                    return false;
                }
                b.rotation = 0;

                chests.append(b);
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
readBlocks(const QString &filePath)
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
            if (xml.name() == "block")
            {
                QXmlStreamAttributes attributes = xml.attributes();

                Block b;

                b.type = attributes.value("type").toString();
                if (b.type.isEmpty())
                {
                    qWarning("block type is invalid");
                    return false;
                }

                if (!readPoint(b.p, attributes))
                {
                    qWarning("block coordinates are invalid");
                    return false;
                }
                if (!readNumber(b.rotation, "rotation", attributes))
                {
                    qWarning("block rotation is invalid");
                    return false;
                }

                blocks.append(b);
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
saveGCode(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
    {
        qWarning("Can't open output file");
        return false;
    }

    QTextStream stream(&f);

    foreach (const QString &line, gCode)
    {
        stream << line << endl;
    }

    return true;
}

bool
operator <(const Block &a, const Block &b)
{
    if (a.p.y < b.p.y) return true;
    if (a.p.y > b.p.y) return false;
    if (a.p.x < b.p.x) return true;
    if (a.p.x > b.p.x) return false;
    return a.p.z < b.p.z;
}

void
reorderWires(QList<Block> &blocks)
{
    if (blocks.size() == 1) return;
    if (blocks.size() != 2)
    {
        Point p = blocks.first().p;
        qWarning("Warning: more than 2 blocks in point [%d;%d;%d]", p.x, p.y, p.z);
    }

    for (int i = 1; i < blocks.size(); i++)
    {
        const Block &b = blocks[i];
        if (b.type.startsWith("$wire"))
        {
            blocks.swap(i, 0);
            break;
        }
    }
}

int
getRotationIndex(int dx, int dz)
{
    if ((dx ==  0) && (dz ==  1)) return 0;
    if ((dx ==  1) && (dz ==  0)) return 1;
    if ((dx ==  0) && (dz == -1)) return 2;
    if ((dx == -1) && (dz ==  0)) return 3;

    return -1;
}

bool
slice()
{
    if (blocks.isEmpty()) return false;

    /* Sort by Y.  */
    qSort(blocks);

    /* Get work zone dimensions.  */
    QMap<int, QList<Block> > layers;
    Point max = blocks.first().p;
    Point min = max;
    foreach (const Block &block, blocks)
    {
        const Point &p = block.p;

        layers[p.y].append(block);

        max.x = qMax(max.x, p.x);
        min.x = qMin(min.x, p.x);
        max.y = qMax(max.y, p.y);
        min.y = qMin(min.y, p.y);
        max.z = qMax(max.z, p.z);
        min.z = qMin(min.z, p.z);
    }
    qDebug("Work zone: [%d;%d;%d] - [%d;%d;%d]", min.x, min.y, min.z, max.x, max.y, max.z);

    /* Get chest zone dimensions.  */
    Point chestMax = originPoint;
    Point chestMin = chestMax;
    foreach (const Block &block, chests)
    {
        const Point &p = block.p;

        chestMax.x = qMax(chestMax.x, p.x);
        chestMin.x = qMin(chestMin.x, p.x);
        chestMax.y = qMax(chestMax.y, p.y);
        chestMin.y = qMin(chestMin.y, p.y);
        chestMax.z = qMax(chestMax.z, p.z);
        chestMin.z = qMin(chestMin.z, p.z);
    }
    qDebug("Chest zone: [%d;%d;%d] - [%d;%d;%d]", chestMin.x, chestMin.y, chestMin.z, chestMax.x, chestMax.y, chestMax.z);

    /* Check for zone intersection.  */
    QRect workZoneRect(QPoint(min.x, min.z), QPoint(max.x, max.z));
    QRect chestZoneRect(QPoint(chestMin.x, chestMin.z), QPoint(chestMax.x, chestMax.z));
    if (workZoneRect.intersects(chestZoneRect))
    {
        qWarning("Work and chest zones intersect!");
        return false;
    }

    /* Calculate safe flight height.  */
    int safeHeight = qMax(max.y, chestMax.y) + 1;

    /* Reorder blocks in zigzag order.  */
    QList<Block> blocksInOrder;
    foreach (int y, layers.keys())
    {
        const QList<Block> &layerBlocks = layers[y];
        QMultiMap<Point, Block> blockMap;
        foreach (const Block &block, layerBlocks)
        {
            blockMap.insertMulti(block.p, block);
        }

        int x = min.x;
        int z = min.z;
        int dx = 1;
        forever
        {
            Point p(x, y, z);
            if (blockMap.contains(p))
            {
                QList<Block> blocksInPoint = blockMap.values(p);
                reorderWires(blocksInPoint);
                foreach (const Block &block, blocksInPoint)
                {
                    blocksInOrder.append(block);
                }
            }

            x += dx;
            if (x > max.x)
            {
                x = max.x;
                dx = -1;
                z++;
            }
            else if (x < min.x)
            {
                x = min.x;
                dx = 1;
                z++;
            }
            if (z > max.z) break;
        }
    }

    /* Cache chests.  */
    QMap<QString, Block> chestByType;
    foreach (const Block &chest, chests)
    {
        chestByType[chest.type] = chest;
    }

    QList<Command> commands;
    QList<Command> placeCommands;

    QMap<QString, int> inventory;
    for (int i = 0; i < blocksInOrder.size(); i++)
    {
        Block &block = blocksInOrder[i];
        const QString &blockType = block.type;

        Command pc;
        pc.type = PLACE;
        pc.block = block;
        pc.amount = 1;

        if ((inventory.size() == 16) && !inventory.contains(blockType))
        {
            i--;
            goto flush;
        }
        if (inventory.value(blockType) == 16) // Workaround for ComputerCraft bug
        {
            i--;
            goto flush;
        }

        inventory[blockType] += 1;
        placeCommands.append(pc);

        if (i != (blocksInOrder.size() - 1)) continue; // Flush on last iteration

    flush:
        foreach (const QString &type, inventory.keys())
        {
            if (!chestByType.contains(type))
            {
                qWarning("Can't get chest for block type '%s'", qPrintable(type));
            }
            Command c;
            c.type = LOAD;
            c.block = chestByType[type];
            c.amount = inventory[type];
            commands.append(c);
        }
        commands += placeCommands;
        placeCommands.clear();
        inventory.clear();
    }

    TurtleState state;
    state.p = originPoint;

    gCode.append("G90"); // Set absolute positioning
    gCode.append(QString("G92 X%1 Y%2 Z%3 R%4")
                 .arg(originPoint.x).arg(originPoint.y).arg(originPoint.z)
                 .arg(getRotationIndex(originDirection.x, originDirection.z)));
    bool lastLoad = false;
    foreach (const Command &c, commands)
    {
        //qDebug("%s %s [%d;%d;%d]", (c.type==PLACE)?"PLACE":"LOAD ", qPrintable(c.block.type),
        //       c.block.p.x, c.block.p.y, c.block.p.z);

        if (c.type == LOAD)
        {
            if (!lastLoad)
            {
                state.blockTypeIndex.clear();
            }

            Point targetPoint = c.block.p;
            targetPoint.y++;

            /* Move to the chest.  */
            gCode.append(QString("G0 Y%1").arg(safeHeight));
            gCode.append(QString("G0 X%1 Z%2").arg(targetPoint.x).arg(targetPoint.z));
            gCode.append(QString("G0 Y%1").arg(targetPoint.y));

            int slotIndex = state.blockTypeIndex.size() + 1;
            state.blockTypeIndex[c.block.type] = slotIndex;

            /* Load from the chest.  */
            gCode.append(QString("M65 S%1").arg(slotIndex));
            gCode.append(QString("M57 C%1").arg(c.amount));

            state.p = targetPoint;
        }
        else
        {
            Point targetPoint = c.block.p;
            targetPoint.y++;

            if (lastLoad)
            {
                gCode.append(QString("G0 Y%1").arg(safeHeight));
                gCode.append(QString("G0 X%1 Z%2").arg(targetPoint.x).arg(targetPoint.z));
                gCode.append(QString("G0 Y%1").arg(targetPoint.y));
            }
            else
            {
                if (targetPoint.y != state.p.y)
                {
                    gCode.append(QString("G0 Y%1").arg(targetPoint.y));
                }
                gCode.append(QString("G0 X%1 Z%2").arg(targetPoint.x).arg(targetPoint.z));
            }

            if (c.block.rotation != -1)
            {
                gCode.append(QString("G0 R%1").arg(c.block.rotation));
            }

            /* Place.  */
            int index = state.blockTypeIndex[c.block.type];
            gCode.append(QString("M51 S%1").arg(index));

            state.p = targetPoint;
        }

        lastLoad = (c.type == LOAD);
    }

    gCode.append(QString("G0 Y%1").arg(safeHeight));
    gCode.append(QString("G0 X%1 Z%2 R%3")
                 .arg(originPoint.x).arg(originPoint.z)
                 .arg(getRotationIndex(originDirection.x, originDirection.z)));
    gCode.append(QString("G0 Y%1").arg(originPoint.y));

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Placer");
    parser.addHelpOption();

    parser.addPositionalArgument("block", QCoreApplication::translate("main", "Block list file"));
    parser.addPositionalArgument("job", QCoreApplication::translate("main", "Slicing job file"));
    parser.addPositionalArgument("result", QCoreApplication::translate("main", "Slicing result"));

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (!readBlocks(args[0]))
    {
        qWarning("Can't read block list");
        return 1;
    }

    if (!readJob(args[1]))
    {
        qWarning("Can't read job file");
        return 1;
    }

    if (!slice())
    {
        qWarning("Can't slice");
        return 1;
    }

    if (!saveGCode(args[2]))
    {
        qWarning("Can't save gCode");
        return 1;
    }

    return 0;
}
