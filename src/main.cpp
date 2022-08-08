#include <iostream>
#include <memory>

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QString>
#include <QStringList>

#include "database.h"
#include "main_window.h"
#include "utils.h"

int main(int argc, char* argv[]) {

  QApplication app(argc, argv);
  QApplication::setApplicationName("labelbuddy");
  QApplication::setApplicationVersion(labelbuddy::getVersion());

  QCommandLineParser parser;
  labelbuddy::prepareParser(parser);
  parser.process(app);
  const QStringList args = parser.positionalArguments();
  const QStringList labelsFiles = parser.values("import-labels");
  const QStringList docsFiles = parser.values("import-docs");
  const QString exportLabelsFile = parser.value("export-labels");
  const QString exportDocsFile = parser.value("export-docs");
  QString dbPath = (args.length() == 0) ? QString() : args[0];

  if (labelsFiles.length() || docsFiles.length() ||
      (exportLabelsFile != QString()) || (exportDocsFile != QString()) ||
      parser.isSet("vacuum")) {
    if (dbPath == QString()) {
      std::cerr << "Specify database path explicitly to import / export "
                << "labels and documents or vacuum db" << std::endl;
      return 1;
    }
    return labelbuddy::batchImportExport(
        dbPath, labelsFiles, docsFiles, exportLabelsFile, exportDocsFile,
        parser.isSet("labelled-only"), !parser.isSet("no-text"),
        !parser.isSet("no-annotations"), parser.isSet("vacuum"));
  }

  std::unique_ptr<labelbuddy::LabelBuddy> labelBuddy(
      new labelbuddy::LabelBuddy(nullptr, dbPath, parser.isSet("demo")));
  app.setWindowIcon(QIcon(":/data/icons/LB.png"));
  labelBuddy->show();
  return app.exec();
}
