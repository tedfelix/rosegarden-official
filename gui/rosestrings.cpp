
#include "rosestrings.h"
#include "rosestrings.h"

QString strtoqstr(const std::string &str)
{
    return QString::fromUtf8(str.c_str());
}

QString strtoqstr(const Rosegarden::PropertyName &p)
{
    return QString::fromUtf8(p.c_str());
}

std::string qstrtostr(const QString &qstr)
{
    return std::string(qstr.utf8().data());
}

