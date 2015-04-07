#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QXmlStreamWriter>
#include <QMap>
#include <QPointF>
#include <QSizeF>
#include <QSet>
#include "common/netlist.h"
#include "common/cellvariantfile.h"
#include "placementjob.h"
#include "qp.h"
#include "legalization.h"

bool
fixNetlist(Netlist * netlist, PlacementJob * job)
{
    QList<QString> items = netlist->allPads();
    if (items.isEmpty())
    {
        qWarning("Error: no pads found");
        return false;
    }

    foreach (const QString &item, items)
    {
        QString padName = netlist->externalName(item);
        if (padName.isEmpty())
        {
            qWarning("can't get pad name for %s", qPrintable(item));
            return false;
        }

        PlacementJob::PadInfo info;
        if (!job->padByName(info, padName))
        {
            qWarning("can't find pad information for %s", qPrintable(padName));
            return false;
        }

        netlist->setPosition(item, info.x, info.y, info.z);
    }

    netlist->deleteItem("VCC");
    netlist->deleteItem("GND");

    return true;
}

void
applyPlacement(Netlist * netlist, NetPlacement &p)
{
    foreach (const QString &gateName, p.keys())
    {
        GatePlacement gp = p[gateName];
        netlist->setPosition(gateName, gp.x, gp.y, gp.z); // Cast to int
    }
}

bool
optimizeVariants(Netlist * netlist, CellVariantFile * variants)
{
    // Just take first cell variant for each gate

    QList<QString> items = netlist->allItems();
    foreach (const QString &itemName, items)
    {
        QString cellType = netlist->cellType(itemName);
        if (cellType.isEmpty())
        {
            qWarning("Can't get cell type for item %s", qPrintable(itemName));
            return false;
        }

        CellVariantList varList = variants->variantList(cellType);
        if (!varList.isValid())
        {
            qWarning("Can't get variant list for cell %s", qPrintable(cellType));
            return false;
        }

        CellVariant variant = varList.allVariants().first();
        netlist->setVariant(itemName, variant.name());
    }

    return true;
}

bool
writeItems(QXmlStreamWriter &stream, const Netlist *netlist, CellVariantFile *variantFile)
{
    QMap<QString, Vector<int> > positions;
    QMap<QString, QString> variantNames;

    foreach (const QString &name, netlist->allItems())
    {
        Vector<int> p;
        if (!netlist->position(name, p.x, p.y, p.z))
        {
            qWarning("Can't get position for gate %s", qPrintable(name));
            return false;
        }
        positions[name] = p;

        QString v = netlist->variant(name);
        if (v.isEmpty())
        {
            qWarning("Can't get variant for gate %s", qPrintable(name));
            return false;
        }
        variantNames[name] = v;
    }

    QList<CellVariant::Block> allBlocks;

    stream.writeStartElement("ports");

    foreach (const QString &name, positions.keys())
    {
        Vector<int> p = positions[name];

        QString variantName = variantNames[name];

        QString cellType = netlist->cellType(name);
        if (cellType.isEmpty())
        {
            qWarning("Can't get cell type for gate %s", qPrintable(name));
            return false;
        }

        QList<QString> ports = netlist->itemPorts(name);

        CellVariantList lst = variantFile->variantList(cellType);
        CellVariant v = lst.variant(variantName);

        foreach (const QString &portName, ports)
        {
            Vector<int> p2;
            if (!v.portPosition(p2, portName))
            {
                qWarning("Can't get port position for port %s (gate %s)", qPrintable(portName), qPrintable(name));
            }

            QString portInstanceName = name + "$" + portName;

            stream.writeStartElement("port");
            stream.writeAttribute("name", portInstanceName);
            stream.writeAttribute("x", QString::number(p.x + p2.x));
            stream.writeAttribute("y", QString::number(p.y + p2.y));
            stream.writeAttribute("z", QString::number(p.z + p2.z));
            stream.writeEndElement();
        }

        QList<CellVariant::Block> gateBlocks = v.allBlocks();
        for (int i = 0; i < gateBlocks.size(); i++)
        {
            CellVariant::Block &block = gateBlocks[i];
            block.position.x += p.x;
            block.position.y += p.y;
            block.position.z += p.z;
        }
        allBlocks.append(gateBlocks);
    }

    stream.writeEndElement();

    stream.writeStartElement("blocks");
    foreach (const CellVariant::Block &block, allBlocks)
    {
        stream.writeStartElement("block");
        stream.writeAttribute("type", block.type);
        stream.writeAttribute("rotation", QString::number(block.rotation));
        stream.writeAttribute("x", QString::number(block.position.x));
        stream.writeAttribute("y", QString::number(block.position.y));
        stream.writeAttribute("z", QString::number(block.position.z));
        stream.writeEndElement();
    }
    stream.writeEndElement();

    return true;
}

bool
writeNets(QXmlStreamWriter &stream, const Netlist *netlist)
{
    stream.writeStartElement("nets");

    foreach (const QString &netName, netlist->allNets())
    {
        QList<QPair<QString, QString> > net = netlist->itemsAndPortsByNet(netName);

        stream.writeStartElement("net");
        stream.writeAttribute("name", netName);

        for (int i = 0; i < net.size(); i++)
        {
            QString gateName = net[i].first;
            QString portName = net[i].second;

            QString portInstanceName = gateName + "$" + portName;

            stream.writeStartElement("port");
            stream.writeAttribute("name", portInstanceName);
            stream.writeEndElement();
        }

        stream.writeEndElement();
    }

    stream.writeEndElement();

    return true;
}

bool
saveRountingTask(const QString &filePath, const Netlist *netlist, CellVariantFile *variants)
{
    QFile fo(filePath);
    if (!fo.open(QFile::WriteOnly | QFile::Truncate))
    {
        qCritical("Can't open XML file");
        return false;
    }

    bool err = false;

    QXmlStreamWriter stream(&fo);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeStartElement("placement");

    if (!writeItems(stream, netlist, variants)) err = true;
    if (!writeNets(stream, netlist)) err = true;

    stream.writeEndElement();
    stream.writeEndDocument();

    fo.close();

    return !err;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Placer");
    parser.addHelpOption();

    parser.addPositionalArgument("netlist", QCoreApplication::translate("main", "Netlist file (in XML format)"));
    parser.addPositionalArgument("job", QCoreApplication::translate("main", "Placement job file"));
    parser.addPositionalArgument("variants", QCoreApplication::translate("main", "Cell variants file"));
    parser.addPositionalArgument("result", QCoreApplication::translate("main", "Placement result"));

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.positionalArguments();

    Netlist * netlist = Netlist::readFromFile(args[0]);
    if (netlist == 0)
    {
        qWarning("Can't load netlist");
        return 1;
    }

    PlacementJob * job = PlacementJob::readFromFile(args[1]);
    if (job == 0)
    {
        qWarning("Can't load placement job");
        return 1;
    }

    CellVariantFile * variants = CellVariantFile::readFromFile(args[2]);
    if (variants == 0)
    {
        qWarning("Can't load variant list");
        return 1;
    }

    if (!fixNetlist(netlist, job))
    {
        return 1;
    }

    NetPlacement placement = placeQuad(netlist, job);

    if (!legalize(placement, netlist, job))
    {
        qWarning("Can't legalize placement");
        return 1;
    }

    applyPlacement(netlist, placement);

    if (!optimizeVariants(netlist, variants))
    {
        qWarning("Can't optimize variants");
        return 1;
    }

    foreach (GatePlacement p, placement)
    {
        qDebug("%0.3f;%0.3f", p.x, p.y);
    }

    if (!saveRountingTask(args[3], netlist, variants))
    {
        qWarning("Can't save routing task");
        return 1;
    }

    return 0;
}
