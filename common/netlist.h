#ifndef NETLIST_H
#define NETLIST_H

#include <QString>
#include <QList>
#include <QSet>
#include <QMap>
#include <QXmlStreamReader>

class Netlist
{
public:
    void
    addItem(const QString &itemName);

    void
    addNet(const QString &netName);

    void
    addItemToNet(const QString &netName, const QString &itemName, const QString &portName);

    void
    setItemType(const QString &itemName, bool isPad);

    /*
     * Warning: this function doesn't delete item from networks
     */
    void
    deleteItem(const QString &itemName);

public:
    QList<QString>
    allItems() const;

    QList<QString>
    allGates() const;

    QList<QString>
    allPads() const;

    QList<QString>
    allNets() const;

    bool
    isGate(const QString &itemName);

    bool
    isPad(const QString &itemName);

    QList<QString>
    itemPorts(const QString &itemName) const;

    QString
    netByItemPort(const QString &itemName, const QString &portName) const;

    QList<QString>
    itemsByNet(const QString &netName) const;

    QList<QPair<QString, QString> >
    itemsAndPortsByNet(const QString &netName) const;

public:
    QString
    cellType(const QString &itemName) const;

    void
    setCellType(const QString &itemName, const QString &cellType);

public:
    QString
    itemProperty(const QString &itemName, const QString &propertyName) const;

    void
    setItemProperty(const QString &itemName, const QString &propertyName, const QString &propertyValue);

    QString
    externalName(const QString &itemName) const;

    bool
    position(const QString &itemName, int &x, int &y, int &z) const;

    void
    setPosition(const QString &itemName, int x, int y, int z);

    QString
    variant(const QString &itemName) const;

    void
    setVariant(const QString &itemName, const QString &variantName);

public:
    static Netlist *
    readFromFile(const QString &filePath);

private:
    bool
    parseLibrary(QXmlStreamReader &xml);

    bool
    parseCell(QXmlStreamReader &xml);

    bool
    parseContents(QXmlStreamReader &xml);

    bool
    parseInstance(QXmlStreamReader &xml);

    bool
    parseNet(QXmlStreamReader &xml);

    static void
    parserError(QXmlStreamReader &xml, const char* msg);

private:
    QSet<QString>       m_padNames;
    QMap<QString, QList<QPair<QString, QString> > > m_netMap; // netName -> pair<itemName, portName>
    QMap<QString, QMap<QString, QString> > m_itemNetMap; // itemName -> portName -> netName
    QMap<QString, QString> m_cellTypes; // itemName -> cellType
    QMap<QString, QMap<QString, QString> > m_propertyMap; // itemName -> propertyName -> propertyValue
};

#endif // NETLIST_H
