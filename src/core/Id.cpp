#include "core/Id.h"

#include <atomic>

namespace Konata {
namespace Core {

namespace {
std::atomic<quint64> g_counter{0};
}

QString IdGenerator::create(const QString &prefix)
{
    const quint64 next = ++g_counter;
    const QString number = QString::number(next).rightJustified(6, QChar::fromLatin1('0'));
    if (prefix.isEmpty())
        return number;
    return prefix + QChar::fromLatin1('_') + number;
}

void IdGenerator::resetForTests()
{
    g_counter.store(0);
}

void IdGenerator::noteLoadedId(const QString &id)
{
    const int sep = id.lastIndexOf(QChar::fromLatin1('_'));
    if (sep < 0)
        return;
    bool ok = false;
    const quint64 value = id.mid(sep + 1).toULongLong(&ok);
    if (!ok)
        return;
    quint64 current = g_counter.load();
    while (value > current && !g_counter.compare_exchange_weak(current, value)) {
    }
}

} // namespace Core
} // namespace Konata
