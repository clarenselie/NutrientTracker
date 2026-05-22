#pragma once

#include "CoreComponent.h"
#include "DomainTypes.h"

class UnitConverter : public CoreComponent
{
public:
    QString componentName() const override;
    double toGrams(const ParsedFoodItem &item, const FoodRecord &food) const;
};
