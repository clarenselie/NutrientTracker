#pragma once

#include <QString>

class CoreComponent
{
public:
    virtual ~CoreComponent() = default;

    virtual QString componentName() const = 0;
};
