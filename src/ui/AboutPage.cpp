#include "AboutPage.h"

#include <QLabel>
#include <QVBoxLayout>

AboutPage::AboutPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(14);

    auto *titleLabel = new QLabel("Nutrient Tracker", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: 700;");

    auto *descriptionLabel = new QLabel(
        "Track nutrients from your food intake using a simple database-driven system.",
        this);
    descriptionLabel->setWordWrap(true);

    auto *builtWithLabel = new QLabel("Built using C++ and Qt", this);
    builtWithLabel->setStyleSheet("color: #555;");

    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    layout->addWidget(builtWithLabel);
    layout->addStretch();
}

