#include "cellvariantfile.h"
#include <QFile>

CellVariantList
CellVariantFile::variantList(const QString &cellName) const
{
    return m_cells.value(cellName);
}

CellVariantFile *
CellVariantFile::readFromFile(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QFile::ReadOnly))
    {
        qWarning("Can't open input file");
        return 0;
    }

    QXmlStreamReader xml(&f);

    QScopedPointer<CellVariantFile> file(new CellVariantFile());

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartDocument) continue;

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "cells")
            {
                if (!file->parse(xml)) return 0;
            }
        }
    }

    if (xml.hasError())
    {
        qWarning("XML error");
        return 0;
    }

    return file.take();
}

bool
CellVariantFile::parse(QXmlStreamReader &xml)
{
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "cells"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "cell")
            {
                CellVariantList list;
                if (!list.parse(xml)) return false;

                m_cells[list.name()] = list;
            }
        }
    }
    return true;
}
