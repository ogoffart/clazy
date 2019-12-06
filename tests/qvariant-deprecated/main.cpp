#include <QtCore/QVariant>

int foo(const QVariant &v) {
    switch (v.type()) {
        case QVariant::Map: return 0;
        case QVariant::List: return 1;
        case QVariant::Int: return 2;
        default: break;
    }

    if (v.type() == QVariant::Line) return 3;
    return 4;
}
