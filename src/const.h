#ifndef CONST_H
#define CONST_H

#include <QColor>
#include <QHostInfo>
#include <QNetworkInterface>

namespace Const
{

const int TIME_LIMIT = 30;
const int SIZE = 14;

enum HostType
{
    Server,
    Client
};

enum Pattern
{
    UNANALYSED = 0,     // 未分析
    FIVE,           // 长连
    FOUR,           // 活四
    SLEEP_FOUR,     // 眠四
    THREE,          // 活三
    SLEEP_THREE,    // 眠三
    TWO,            // 活二
    SLEEP_TWO,      // 眠二
    DEAD_FOUR,      // 死四
    DEAD_THREE,     // 死三
    DEAD_TWO,       // 死二
    ANALYSED
};

namespace Score {
const int FIVE                    = 100000;
const int FOUR                    = 10000;
const int DOUBLE_SLEEP_FOUR       = 10000;
const int SLEEP_FOUR_AND_THREE    = 10000;
const int DOUBLE_THREE            = 5000;
const int SLEEP_THREE_AND_THREE   = 1000;
const int SLEEP_FOUR              = 500;
const int THREE                   = 200;
const int DOUBLE_TWO              = 100;
const int SLEEP_THREE             = 50;
const int SLEEP_TWO_AND_TWO       = 10;
const int TWO                     = 5;
const int SLEEP_TWO               = 3;
const int DEAD_FOUR               = -5;
const int DEAD_THREE              = -5;
const int DEAD_TWO                = -5;
}

const QColor PIECE_COLOR[3] =
{
    Qt::transparent,
    Qt::white,
    Qt::black
};

const int POINT[5][2] =
{
    {0,  0 },
    {4,  4 },
    {4,  -4},
    {-4, 4 },
    {-4, -4}
};

const unsigned char nodeValue[SIZE+1][SIZE+1] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0 },
    { 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0 },
    { 0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 3, 2, 1, 0 },
    { 0, 1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1, 0 },
    { 0, 1, 2, 3, 4, 5, 6, 6, 6, 5, 4, 3, 2, 1, 0 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1, 0 },
    { 0, 1, 2, 3, 4, 5, 6, 6, 6, 5, 4, 3, 2, 1, 0 },
    { 0, 1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1, 0 },
    { 0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 3, 2, 1, 0 },
    { 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0 },
    { 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

inline double Sqr(double x) { return x * x; }

inline QString getLocalIP()
{
#ifdef Q_OS_WIN
    auto list = QHostInfo::fromName(QHostInfo::localHostName()).addresses();
#else
    auto list = QNetworkInterface::allAddresses();
#endif
    for (auto i : list)
        if (i != QHostAddress::LocalHost && i.protocol() == QAbstractSocket::IPv4Protocol)
            return i.toString();
    return "";
}
}
#endif // CONST_H
