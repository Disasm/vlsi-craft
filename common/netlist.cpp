#include "netlist.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QScopedPointer>

void
Netlist::addItem(const QString &itemName)
{
    m_itemNetMap[itemName].clear();
    m_cellTypes[itemName].clear();
    m_propertyMap[itemName].clear();
}

void
Netlist::addNet(const QString &netName)
{
    m_netMap[netName].clear();
}

void
Netlist::addItemToNet(const QString &netName, const QString &itemName, const QString &portName)
{
    m_netMap[netName].append(qMakePair(itemName, portName));
    m_itemNetMap[itemName][portName] = netName;
}

void
Netlist::setItemType(const QString &itemName, bool isPad)
{
    if (isPad)
    {
        m_padNames.insert(itemName);
    }
    else
    {
        m_padNames.remove(itemName);
    }
}

void
Netlist::deleteItem(const QString &itemName)
{
    m_padNames.remove(itemName);
    m_itemNetMap.remove(itemName);
    m_cellTypes.remove(itemName);
    m_propertyMap.remove(itemName);
}

QList<QString>
Netlist::allItems() const
{
    return m_itemNetMap.keys();
}

QList<QString>
Netlist::allGates() const
{
    QSet<QString> items = m_itemNetMap.keys().toSet();
    return (items - m_padNames).toList();
}

QList<QString>
Netlist::allPads() const
{
    return m_padNames.toList();
}

QList<QString>
Netlist::allNets() const
{
    return m_netMap.keys();
}

QList<QString>
Netlist::itemPorts(const QString &itemName) const
{
    return m_itemNetMap.value(itemName).keys();
}

QList<QString>
Netlist::itemsByNet(const QString &netName) const
{
    QSet<QString> items;
    QList<QPair<QString, QString> > net = m_netMap.value(netName);
    for (int i = 0; i < net.size(); i++)
    {
        items.insert(net[i].first);
    }
    return items.toList();
}

QList<QPair<QString, QString> >
Netlist::itemsAndPortsByNet(const QString &netName) const
{
    return m_netMap.value(netName);
}

QString
Netlist::cellType(const QString &itemName) const
{
    return m_cellTypes.value(itemName);
}

void
Netlist::setCellType(const QString &itemName, const QString &cellType)
{
    m_cellTypes[itemName] = cellType;
}

QString
Netlist::itemProperty(const QString &itemName, const QString &propertyName) const
{
    return m_propertyMap.value(itemName).value(propertyName);
}

void
Netlist::setItemProperty(const QString &itemName, const QString &propertyName, const QString &propertyValue)
{
    m_propertyMap[itemName][propertyName] = propertyValue;
}

QString
Netlist::externalName(const QString &itemName) const
{
    return itemProperty(itemName, "NAME");
}

bool
Netlist::position(const QString &itemName, int &x, int &y, int &z) const
{
    QString sx = itemProperty(itemName, "X");
    QString sy = itemProperty(itemName, "Y");
    QString sz = itemProperty(itemName, "Z");
    if (sx.isEmpty() || sy.isEmpty() || sz.isEmpty()) return false;

    bool okX, okY, okZ;
    x = sx.toInt(&okX);
    y = sy.toInt(&okY);
    z = sz.toInt(&okZ);

    return okX && okY && okZ;
}

void
Netlist::setPosition(const QString &itemName, int x, int y, int z)
{
    setItemProperty(itemName, "X", QString::number(x));
    setItemProperty(itemName, "Y", QString::number(y));
    setItemProperty(itemName, "Z", QString::number(z));
}

QString
Netlist::variant(const QString &itemName) const
{
    return itemProperty(itemName, "VARIANT");
}

void
Netlist::setVariant(const QString &itemName, const QString &variantName)
{
    setItemProperty(itemName, "VARIANT", variantName);
}

Netlist *
Netlist::readFromFile(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QFile::ReadOnly))
    {
        qWarning("Can't open input file");
        return 0;
    }

    QXmlStreamReader xml(&f);

    QScopedPointer<Netlist> netlist(new Netlist());

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartDocument) continue;

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "library")
            {
                if (!netlist->parseLibrary(xml)) return 0;
            }
        }
    }

    if (xml.hasError())
    {
        parserError(xml, "XML error");
        return 0;
    }

    return netlist.take();
}

bool
Netlist::parseLibrary(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attributes = xml.attributes();
    QString libraryName = attributes.value("name").toString();
    if (libraryName.isEmpty())
    {
        parserError(xml, "library name is empty");
        return false;
    }
    if (libraryName != "DESIGN")
    {
        // Skip
        while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "library")) xml.readNext();
        return true;
    }

    int cells = 0;
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "library"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "cell")
            {
                cells++;
                if (cells > 1)
                {
                    parserError(xml, "more than one cell in the design. Flatten it first.");
                    return false;
                }
                if (!parseCell(xml)) return false;
            }
        }
    }

    return true;
}

bool
Netlist::parseCell(QXmlStreamReader &xml)
{
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "cell"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "contents")
            {
                if (!parseContents(xml)) return false;
            }
        }
    }
    return true;
}

bool
Netlist::parseContents(QXmlStreamReader &xml)
{
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "contents"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "instance")
            {
                if (!parseInstance(xml)) return false;
            }
            else if (xml.name() == "net")
            {
                if (!parseNet(xml)) return false;
            }
        }
    }
    return true;
}

bool
Netlist::parseInstance(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attributes = xml.attributes();
    QString instanceName = attributes.value("name").toString();
    if (instanceName.isEmpty())
    {
        parserError(xml, "can't get instance name");
        return false;
    }

    addItem(instanceName);

    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "instance"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "cellRef")
            {
                attributes = xml.attributes();
                QString cellName = attributes.value("name").toString();
                QString library = attributes.value("library").toString();
                if (cellName.isEmpty())
                {
                    parserError(xml, "cell name is empty");
                    return false;
                }
                if (library != "LIB")
                {
                    parserError(xml, "cell is not from LIB");
                    return false;
                }

                setCellType(instanceName, cellName);
            }
            else if (xml.name() == "property")
            {
                attributes = xml.attributes();
                QString propName = attributes.value("name").toString();
                QString propValue = attributes.value("value").toString();
                if (propName.isEmpty())
                {
                    parserError(xml, "empty property name");
                    return false;
                }

                setItemProperty(instanceName, propName, propValue);

                if (propName == "NAME") setItemType(instanceName, true);
            }
        }
    }
    return true;
}

bool
Netlist::parseNet(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attributes = xml.attributes();
    QString netName = attributes.value("name").toString();
    if (netName.isEmpty())
    {
        parserError(xml, "can't get net name");
        return false;
    }

    addNet(netName);

    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "net"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "portRef")
            {
                attributes = xml.attributes();
                QString portName = attributes.value("name").toString();
                QString instanceName = attributes.value("instance").toString();
                if (portName.isEmpty())
                {
                    parserError(xml, "empty port name");
                    return false;
                }
                if (instanceName.isEmpty())
                {
                    parserError(xml, "empty instance name");
                    return false;
                }

                addItemToNet(netName, instanceName, portName);
            }
        }
    }
    return true;
}

void
Netlist::parserError(QXmlStreamReader &xml, const char *msg)
{
    qWarning("Error while parsing netlist file (line %d): %s", (int)xml.lineNumber(), msg);
}
