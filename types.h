#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <QDateTime>
#include <QDataStream>

struct HistoryEntry {
    QString url;
    QString title;
    QDateTime timestamp;
};

struct Bookmark {
    QString url;
    QString title;
};

// Операторы для сериализации
QDataStream &operator<<(QDataStream &out, const HistoryEntry &entry)
{
    out << entry.url << entry.title << entry.timestamp;
    return out;
}

QDataStream &operator>>(QDataStream &in, HistoryEntry &entry)
{
    in >> entry.url >> entry.title >> entry.timestamp;
    return in;
}

QDataStream &operator<<(QDataStream &out, const Bookmark &bookmark)
{
    out << bookmark.url << bookmark.title;
    return out;
}

QDataStream &operator>>(QDataStream &in, Bookmark &bookmark)
{
    in >> bookmark.url >> bookmark.title;
    return in;
}

// Регистрация типов для QVariant
Q_DECLARE_METATYPE(HistoryEntry)
Q_DECLARE_METATYPE(QVector<HistoryEntry>)
Q_DECLARE_METATYPE(Bookmark)
Q_DECLARE_METATYPE(QVector<Bookmark>)

#endif // TYPES_H 