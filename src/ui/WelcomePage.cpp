#include "WelcomePage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("welcomePage");

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(48, 42, 48, 42);

    outerLayout->addStretch();

    auto *contentLayout = new QVBoxLayout();
    contentLayout->setSpacing(18);

    auto *titleLabel = new QLabel("Nutrient Tracker", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setObjectName("welcomeTitle");

    auto *projectLabel = new QLabel("Honors Project", this);
    projectLabel->setAlignment(Qt::AlignCenter);
    projectLabel->setObjectName("projectLabel");

    auto *topicLabel = new QLabel(
        "Topic: Accessible Health, Nutrition, and Daily Wellness Tracking",
        this);
    topicLabel->setAlignment(Qt::AlignCenter);
    topicLabel->setWordWrap(true);
    topicLabel->setObjectName("topicLabel");

    auto *descriptionLabel = new QLabel(
        "This app helps users log daily foods, estimate calories and nutrients, "
        "review nutrition totals, and support healthier everyday wellness habits.",
        this);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setAlignment(Qt::AlignCenter);
    descriptionLabel->setObjectName("descriptionLabel");

    auto *detailsLabel = new QLabel(
        "Student: Claren's Guibson Elie\nProfessor: Dr. Azhar\nSemester: Spring 26",
        this);
    detailsLabel->setAlignment(Qt::AlignCenter);
    detailsLabel->setObjectName("detailsLabel");

    auto *startButton = new QPushButton("Start", this);
    startButton->setObjectName("startButton");
    startButton->setMinimumWidth(160);
    startButton->setMinimumHeight(42);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    buttonRow->addWidget(startButton);
    buttonRow->addStretch();

    contentLayout->addWidget(titleLabel);
    contentLayout->addWidget(projectLabel);
    contentLayout->addWidget(topicLabel);
    contentLayout->addWidget(descriptionLabel);
    contentLayout->addSpacing(4);
    contentLayout->addWidget(detailsLabel);
    contentLayout->addSpacing(10);
    contentLayout->addLayout(buttonRow);

    outerLayout->addLayout(contentLayout);
    outerLayout->addStretch();

    setStyleSheet(
        "QWidget#welcomePage { background: #111827; color: #F9FAFB; "
        "font-family: 'Inter', 'Segoe UI', 'Helvetica Neue', sans-serif; }"
        "QLabel#welcomeTitle { font-size: 34px; font-weight: 800; color: #F9FAFB; }"
        "QLabel#projectLabel { font-size: 18px; font-weight: 750; color: #E5E7EB; }"
        "QLabel#topicLabel { font-size: 17px; font-weight: 700; color: #D1D5DB; }"
        "QLabel#descriptionLabel { font-size: 15px; color: #CBD5E1; line-height: 130%; }"
        "QLabel#detailsLabel { font-size: 15px; font-weight: 650; color: #E5E7EB; }"
        "QPushButton#startButton { background: #3CB371; color: #F9FAFB; border: none; "
        "border-radius: 8px; padding: 10px 22px; font-size: 15px; font-weight: 750; }"
        "QPushButton#startButton:hover { background: #49C27F; }");

    connect(startButton, &QPushButton::clicked, this, &WelcomePage::startRequested);
}
