#include "InputParser.h"

#include <QRegularExpression>
#include <QStringList>

QString InputParser::componentName() const
{
    return "Input Parser";
}

QVector<ParsedFoodItem> InputParser::parse(const QString &input) const
{
    QVector<ParsedFoodItem> items;
    QString normalizedInput = input.trimmed().toLower();

    if (normalizedInput.isEmpty()) {
        return items;
    }

    normalizedInput.replace(" and ", ",");
    normalizedInput.replace("+", ",");

    const QStringList segments = normalizedInput.split(",", Qt::SkipEmptyParts);
    const QRegularExpression expression(
        "^\\s*(?:(\\d+(?:\\.\\d+)?)\\s*)?(?:(cups?|cup|grams?|g|kg|oz|ounces?|lb|pounds?|servings?|pieces?|piece|units?)\\s+)?(.+?)\\s*$");

    for (const QString &segment : segments) {
        const QString trimmed = segment.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }

        ParsedFoodItem item;
        item.rawText = trimmed;
        item.quantity = 1.0;
        item.unit = "unit";

        const QRegularExpressionMatch match = expression.match(trimmed);
        if (match.hasMatch()) {
            if (!match.captured(1).isEmpty()) {
                item.quantity = match.captured(1).toDouble();
            }
            if (!match.captured(2).isEmpty()) {
                item.unit = normalizeUnit(match.captured(2));
            }
            item.foodName = match.captured(3).trimmed();
        } else {
            item.foodName = trimmed;
        }

        if (item.foodName.startsWith("brand ")) {
            item.foodName.remove(0, 6);
        }

        items.append(item);
    }

    return items;
}

QString InputParser::normalizeUnit(const QString &unit) const
{
    const QString value = unit.toLower();

    if (value == "g" || value == "gram" || value == "grams") {
        return "g";
    }
    if (value == "kg") {
        return "kg";
    }
    if (value == "cup" || value == "cups") {
        return "cup";
    }
    if (value == "oz" || value == "ounce" || value == "ounces") {
        return "oz";
    }
    if (value == "lb" || value == "pound" || value == "pounds") {
        return "lb";
    }
    if (value == "serving" || value == "servings") {
        return "serving";
    }
    if (value == "piece" || value == "pieces" || value == "unit" || value == "units") {
        return "unit";
    }

    return "unit";
}
