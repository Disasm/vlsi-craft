#include "parser.h"
#include <QFile>
#include <QVector>

struct ParserState
{
    const char* ptr;
    const char* end;
};

typedef QString ParserError;

void
skipSpaces(ParserState &st)
{
    while (st.ptr < st.end)
    {
        char c = *st.ptr;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            st.ptr++;
        }
        else
        {
            break;
        }
    }
}

void
checkEndOfFile(ParserState &st)
{
    if (st.ptr >= st.end)
    {
        throw ParserError("unexpected end of file");
    }
}

void
parseSpace(ParserState &st)
{
    checkEndOfFile(st);

    if (!isspace(*st.ptr))
    {
        throw ParserError("space expected");
    }

    while (st.ptr < st.end)
    {
        if (isspace(*st.ptr))
        {
            st.ptr++;
        }
        else break;
    }
}

QString
parseIdentifier(ParserState &st)
{
    checkEndOfFile(st);

    QString s;
    if (!isalpha(*st.ptr))
    {
        throw ParserError("identifier must begin with alpha character");
    }
    s += *st.ptr++;
    while (st.ptr < st.end)
    {
        char c = *st.ptr;
        if (isalnum(c) || (c == '_'))
        {
            s += c;
            st.ptr++;
        }
        else break;
    }
    return s;
}

QString
parseString(ParserState &st)
{
    QString s;

    checkEndOfFile(st);

    if (*st.ptr != '"')
    {
        throw ParserError("string must begin with quote");
    }

    s += *st.ptr++;

    while (st.ptr < st.end)
    {
        char c = *st.ptr++;
        s += c;
        if (c == '"') return s;
    }

    throw ParserError("unexpected end of file");
}

qint64
parseNumber(ParserState &st)
{
    checkEndOfFile(st);

    qint64 v = 0;
    if (!isdigit(*st.ptr))
    {
        throw ParserError("number must begin with digit");
    }
    while (st.ptr < st.end)
    {
        char c = *st.ptr;
        if (isdigit(c))
        {
            v *= 10;
            v += c - '0';
            st.ptr++;
        }
        else break;
    }
    return v;
}

QVariant
parseAtom(ParserState &st)
{
    checkEndOfFile(st);

    if (isalpha(*st.ptr))
    {
        return parseIdentifier(st);
    }
    else if (isdigit(*st.ptr))
    {
        return parseNumber(st);
    }
    else if (*st.ptr == '-')
    {
        st.ptr++;
        return -parseNumber(st);
    }
    else if (*st.ptr == '"')
    {
        return parseString(st);
    }
    else
    {
        throw ParserError(QString("unexpected character '%1'").arg(*st.ptr));
    }
}

QVariantList
parseList(ParserState &st)
{
    skipSpaces(st);
    checkEndOfFile(st);
    if (*st.ptr != '(')
    {
        throw ParserError("list must be started with '('");
    }
    st.ptr++;

    QVariantList list;
    QString i = parseIdentifier(st);
    list.append(i);

    while (st.ptr < st.end)
    {
        if (*st.ptr == ')')
        {
            st.ptr++;
            return list;
        }

        parseSpace(st);

        if (*st.ptr == ')')
        {
            st.ptr++;
            return list;
        }

        if (*st.ptr == '(')
        {
            QVariant v = parseList(st);
            list.append(v);
        }
        else
        {
            list.append(parseAtom(st));
        }
    }

    throw ParserError("unexpected end of file");
}

QVariantList
parseLispFile(QString filePath)
{
    QFile f(filePath);
    if (!f.open(QFile::ReadOnly))
    {
        qCritical("Can't open file %s", qPrintable(filePath));
        return QVariantList();
    }

    QByteArray data = f.readAll();
    f.close();

    ParserState st;
    st.ptr = data.data();
    st.end = st.ptr + data.size();

    try
    {
        return parseList(st);
    }
    catch (ParserError &e)
    {
        qCritical("Parser: %s", qPrintable(e));
        return QVariantList();
    }
}
