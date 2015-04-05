#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QVariant>
#include <QVariantList>
#include <QXmlStreamWriter>
#include "parser.h"

void lispParserInit(QIODevice* stream);
QVariant lispParse();

void writeCellRef(QXmlStreamWriter &stream, const QVariantList &v)
{
    stream.writeStartElement("cellRef");
    stream.writeAttribute("name", v[1].toString());

    if (v[2].type() == QVariant::List)
    {
        QVariantList l = v[2].toList();
        if (l[0].toString() == "libraryRef")
        {
            stream.writeAttribute("library", l[1].toString());
        }
        else
        {
            qWarning("Wrong cellRef description");
        }
    }
    else
    {
        qWarning("Wrong cellRef description");
    }

    // TODO

    stream.writeEndElement();
}

void writePort(QXmlStreamWriter &stream, const QVariantList &v)
{
    if (v.size() != 3)
    {
        qWarning("Wrong port description");
        return;
    }

    stream.writeStartElement("port");

    if (v[1].type() == QVariant::List)
    {
        QVariantList l = v[1].toList();
        if (l[0].toString() != "array")
        {
            qWarning("Wrong port name description");
        }
        else
        {
            stream.writeAttribute("name", l[1].toString());
            stream.writeAttribute("arraySize", QString::number(l[2].toInt()));
        }
    }
    else
    {
        stream.writeAttribute("name", v[1].toString());
    }

    stream.writeEndElement();
}

void writeNetlistInterface(QXmlStreamWriter &stream, const QVariantList &v)
{
    stream.writeStartElement("interface");

    for (int i = 1; i < v.size(); i++)
    {
        QVariant v2 = v[i];
        if (v2.type() == QVariant::List)
        {
            QVariantList l2 = v2.toList();
            if (l2.size() > 0)
            {
                QString tag = l2.first().toString();
                if (tag == "port")
                {
                    writePort(stream, l2);
                }
                else
                {
                    qWarning("Unexpected tag '%s' in 'interface' tag", qPrintable(tag));
                }
            }
        }
    }

    stream.writeEndElement();
}

// <name> or (rename <name> <altName>)
void writeName(QXmlStreamWriter &stream, const QVariant &v)
{
    if (v.type() == QVariant::List)
    {
        QVariantList l = v.toList();
        if (l[0].toString() != "rename")
        {
            qWarning("Wrong name description");
        }
        else
        {
            stream.writeAttribute("name", l[1].toString());
            //stream.writeAttribute("altName", l[2].toString()); // TODO: strip quotes
        }
    }
    else if (v.type() == QVariant::String)
    {
        stream.writeAttribute("name", v.toString());
    }
    else
    {
        qWarning("Wrong name description");
    }
}

void writeInstance(QXmlStreamWriter &stream, const QVariantList &v)
{
    stream.writeStartElement("instance");

    writeName(stream, v[1]);

    QVariant vViewRef = v[2];
    if (vViewRef.type() == QVariant::List)
    {
        QVariantList l = vViewRef.toList();
        if (l[0].toString() != "viewRef")
        {
            qWarning("Wrong instance cell description");
        }
        else
        {
            writeCellRef(stream, l[2].toList());
        }
    }
    else
    {
        qWarning("Wrong instance description");
    }

    stream.writeEndElement();
}

void writePortRef(QXmlStreamWriter &stream, const QVariantList &v)
{
    stream.writeStartElement("portRef");

    QVariant vName = v[1];
    if (vName.type() == QVariant::List)
    {
        QVariantList l = vName.toList();
        if (l[0].toString() == "member")
        {
            stream.writeAttribute("name", l[1].toString());
            stream.writeAttribute("arrayIndex", QString::number(l[2].toInt()));
        }
        else
        {
            qWarning("Wrong portRef name description");
        }
    }
    else if (vName.type() == QVariant::String)
    {
        stream.writeAttribute("name", vName.toString());
    }
    else
    {
        qWarning("Wrong portRef name description");
    }

    if (v.size() > 2)
    {
        QVariant vInstanceRef = v[2];
        if (vInstanceRef.type() == QVariant::List)
        {
            QVariantList l = vInstanceRef.toList();
            if (l[0].toString() == "instanceRef")
            {
                stream.writeAttribute("instance", l[1].toString());
            }
            else
            {
                qWarning("Wrong portRef instanceRef description");
            }
        }
        else
        {
            qWarning("Wrong portRef description");
        }
    }

    stream.writeEndElement();
}

void writeNet(QXmlStreamWriter &stream, const QVariantList &v)
{
    stream.writeStartElement("net");

    writeName(stream, v[1]);

    if (v[2].type() == QVariant::List)
    {
        QVariantList l = v[2].toList();
        if (l[0].toString() == "joined")
        {
            for (int i = 1; i < v.size(); i++)
            {
                QVariant v2 = l[i];
                if (v2.type() == QVariant::List)
                {
                    QVariantList l2 = v2.toList();
                    if (l2.size() > 0)
                    {
                        QString tag = l2.first().toString();
                        if (tag == "portRef")
                        {
                            writePortRef(stream, l2);
                        }
                        else
                        {
                            qWarning("Unexpected tag '%s' in 'joined' tag", qPrintable(tag));
                        }
                    }
                }
            }
        }
        else
        {
            qWarning("Wrong net connections description");
        }
    }
    else
    {
        qWarning("Wrong net description");
    }

    stream.writeEndElement();
}

void writeNetlistContents(QXmlStreamWriter &stream, const QVariantList &v)
{
    stream.writeStartElement("contents");

    for (int i = 1; i < v.size(); i++)
    {
        QVariant v2 = v[i];
        if (v2.type() == QVariant::List)
        {
            QVariantList l2 = v2.toList();
            if (l2.size() > 0)
            {
                QString tag = l2.first().toString();
                if (tag == "instance")
                {
                    writeInstance(stream, l2);
                }
                else if (tag == "net")
                {
                    writeNet(stream, l2);
                }
                else
                {
                    qWarning("Unexpected tag '%s' in 'contents' tag", qPrintable(tag));
                }
            }
        }
    }

    stream.writeEndElement();
}

void writeView(QXmlStreamWriter &stream, const QVariantList &v)
{
    QString name = v[1].toString();
    if (name != "VIEW_NETLIST")
    {
        qWarning("Unsupported view type: %s", qPrintable(name));
        return;
    }
    for (int i = 2; i < v.size(); i++)
    {
        QVariant v2 = v[i];
        if (v2.type() == QVariant::List)
        {
            QVariantList l2 = v2.toList();
            if (l2.size() > 0)
            {
                QString tag = l2.first().toString();
                if (tag == "interface")
                {
                    writeNetlistInterface(stream, l2);
                }
                else if (tag == "contents")
                {
                    writeNetlistContents(stream, l2);
                }
                else if (tag == "viewType")
                {
                    QString viewType = l2[1].toString();
                    if (viewType != "NETLIST")
                    {
                        qWarning("Unsupported view type: %s", qPrintable(viewType));
                    }
                }
                else
                {
                    qWarning("Unexpected tag '%s' in 'view' tag", qPrintable(tag));
                }
            }
        }
    }
}

void writeCell(QXmlStreamWriter &stream, const QVariantList &v)
{
    stream.writeStartElement("cell");
    stream.writeAttribute("name", v[1].toString());

    for (int i = 2; i < v.size(); i++)
    {
        QVariant v2 = v[i];
        if (v2.type() == QVariant::List)
        {
            QVariantList l2 = v2.toList();
            if (l2.size() > 0)
            {
                QString tag = l2.first().toString();
                if (tag == "cellType")
                {
                    //stream.writeAttribute("type", l2[1].toString());
                }
                else if (tag == "view")
                {
                    writeView(stream, l2);
                }
                else
                {
                    qWarning("Unexpected tag '%s' in 'cell' tag", qPrintable(tag));
                }
            }
        }
    }

    stream.writeEndElement();
}

void writeLibrary(QXmlStreamWriter &stream, const QVariantList &v)
{
    stream.writeStartElement("library");
    stream.writeAttribute("name", v[1].toString());

    for (int i = 2; i < v.size(); i++)
    {
        QVariant v2 = v[i];
        if (v2.type() == QVariant::List)
        {
            QVariantList l2 = v2.toList();
            if (l2.size() > 0)
            {
                if (l2.first() == "cell")
                {
                    writeCell(stream, l2);
                }
            }
        }
    }

    stream.writeEndElement();
}

void writeDesign(QXmlStreamWriter &stream, const QVariantList &v)
{
    stream.writeStartElement("design");
    stream.writeAttribute("name", v[1].toString());

    for (int i = 2; i < v.size(); i++)
    {
        QVariant v2 = v[i];
        if (v2.type() == QVariant::List)
        {
            QVariantList l2 = v2.toList();
            if (l2.size() > 0)
            {
                if (l2.first() == "cellRef")
                {
                    writeCellRef(stream, l2);
                }
                else
                {
                    qWarning("Unexpected tag '%s' in 'design' tag", qPrintable(l2.first().toString()));
                }
            }
        }
    }

    stream.writeEndElement();
}

void writeTop(QXmlStreamWriter &stream, const QVariantList &v)
{
    Q_ASSERT(v.size() > 2);

    stream.writeStartElement("netlist");
    stream.writeAttribute("name", v[1].toString());

    for (int i = 2; i < v.size(); i++)
    {
        QVariant v2 = v[i];
        if (v2.type() == QVariant::List)
        {
            QVariantList l2 = v2.toList();
            if (l2.size() > 0)
            {
                if ((l2.first() == "external") || (l2.first() == "library"))
                {
                    writeLibrary(stream, l2);
                }
                else if (l2.first() == "design")
                {
                    writeDesign(stream, l2);
                }
            }
        }
    }

    stream.writeEndElement();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QStringList args = a.arguments();
    if (args.size() < 3)
    {
        qCritical("Usage: %s <input.edif> <output.xml>", argv[0]);
        return 1;
    }

    /*QFile fi(args[1]);
    if (!fi.open(QFile::ReadOnly))
    {
        qCritical("Can't open EDIF file");
        return 1;
    }

    lispParserInit(&fi);
    QVariantList top = lispParse().toList();*/

    QVariantList top = parseLispFile(args[1]);

    //fi.close();

    QFile fo(args[2]);
    if (!fo.open(QFile::WriteOnly | QFile::Truncate))
    {
        qCritical("Can't open XML file");
        return 1;
    }

    QXmlStreamWriter stream(&fo);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    writeTop(stream, top);

    /*stream.writeStartElement("bookmark");
    stream.writeAttribute("href", "http://qt-project.org/");
    stream.writeTextElement("title", "Qt Project");
    stream.writeEndElement(); // bookmark*/

    stream.writeEndDocument();

    fo.close();

    return 0;
}
