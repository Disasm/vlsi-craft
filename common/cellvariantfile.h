#ifndef CELLVARIANTFILE_H
#define CELLVARIANTFILE_H

#include <QString>
#include <QMap>
#include <QXmlStreamReader>
#include "cellvariantlist.h"

class CellVariantFile
{
public:
    CellVariantList
    variantList(const QString &cellName) const;

public:
    static CellVariantFile *
    readFromFile(const QString &filePath);

private:
    bool
    parse(QXmlStreamReader &xml);

private:
    QMap<QString, CellVariantList> m_cells;
};

#endif // CELLVARIANTFILE_H
