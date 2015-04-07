#ifndef PLACEMENTJOB_H
#define PLACEMENTJOB_H

#include <QString>
#include <QMap>
#include <QXmlStreamReader>
#include "common/vector.h"

class PlacementJob
{
public:
    struct PadInfo
    {
        QString name;
        int x, y, z;
    };

public:
    PlacementJob();

    void
    addPad(const PadInfo &padInfo);

    bool
    padByName(PadInfo &padInfo, const QString &name) const;

public:
    QString
    padCellType() const;

    QString
    padCellNameProperty() const;

public:
    Vector<int>
    minCoordinates() const;

    Vector<int>
    maxCoordinates() const;

public:
    static PlacementJob *
    readFromFile(const QString &filePath);

private:
    bool
    parseArea(QXmlStreamReader &xml);

    bool
    parsePads(QXmlStreamReader &xml);

    static void
    parserError(QXmlStreamReader &xml, const char* msg);

private:
    int m_minX, m_minY, m_minZ;
    int m_maxX, m_maxY, m_maxZ;

    QString m_padCellLibrary;
    QString m_padCellType;
    QString m_padCellNameProperty;

    QMap<QString, PadInfo> m_pads;
};

#endif // PLACEMENTJOB_H
