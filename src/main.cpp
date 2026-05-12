#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

#include "DatabaseManager.h"
#include "MainWindow.h"
#include "USDAImporter.h"

namespace {
int buildFoodDatabase(const QStringList &arguments)
{
    QTextStream out(stdout);
    QTextStream err(stderr);

    const int commandIndex = arguments.indexOf("--build-food-db");
    const QString sourceDirectory = arguments.value(commandIndex + 1, QDir::currentPath() + "/data/usda");
    const QString outputDatabase = arguments.value(commandIndex + 2, QDir::currentPath() + "/data/nutrient_tracker.sqlite");

    out << "Building food database from: " << sourceDirectory << Qt::endl;
    out << "Output database: " << outputDatabase << Qt::endl;

    const QFileInfo outputInfo(outputDatabase);
    QDir outputDirectory(outputInfo.absolutePath());
    if (!outputDirectory.exists() && !outputDirectory.mkpath(".")) {
        err << "Food database build failed: could not create output directory." << Qt::endl;
        return 1;
    }

    const QStringList outputFiles = {
        outputInfo.absoluteFilePath(),
        outputInfo.absoluteFilePath() + "-wal",
        outputInfo.absoluteFilePath() + "-shm"
    };
    for (const QString &path : outputFiles) {
        if (QFile::exists(path) && !QFile::remove(path)) {
            err << "Food database build failed: could not replace " << path << Qt::endl;
            return 1;
        }
    }

    DatabaseManager databaseManager(outputDatabase);
    USDAImporter importer(databaseManager);

    QString errorMessage;
    if (!importer.importCsvDirectory(sourceDirectory, &errorMessage)) {
        err << "Food database build failed: " << errorMessage << Qt::endl;
        return 1;
    }

    out << "Food database build complete." << Qt::endl;
    return 0;
}
}

int main(int argc, char *argv[])
{
    QStringList rawArguments;
    for (int index = 0; index < argc; ++index) {
        rawArguments.append(QString::fromLocal8Bit(argv[index]));
    }

    if (rawArguments.contains("--build-food-db")) {
        QCoreApplication app(argc, argv);
        return buildFoodDatabase(QCoreApplication::arguments());
    }

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
