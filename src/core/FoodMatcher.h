#pragma once

#include <optional>

#include "CoreComponent.h"
#include "DatabaseManager.h"
#include "DomainTypes.h"

class FoodMatcher : public CoreComponent
{
public:
    explicit FoodMatcher(DatabaseManager &databaseManager);

    QString componentName() const override;
    std::optional<FoodRecord> match(const ParsedFoodItem &item) const;

private:
    QString normalize(const QString &text) const;
    int scoreCandidate(const ParsedFoodItem &item, const FoodRecord &candidate) const;

    DatabaseManager &m_databaseManager;
};
