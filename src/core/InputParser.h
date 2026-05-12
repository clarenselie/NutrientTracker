#pragma once

#include <QVector>

#include "DomainTypes.h"

class InputParser
{
public:
    QVector<ParsedFoodItem> parse(const QString &input) const;

private:
    QString normalizeUnit(const QString &unit) const;
};

