#pragma once

#include <QVector>

#include "CoreComponent.h"
#include "DomainTypes.h"

class InputParser : public CoreComponent
{
public:
    QString componentName() const override;
    QVector<ParsedFoodItem> parse(const QString &input) const;

private:
    QString normalizeUnit(const QString &unit) const;
};
