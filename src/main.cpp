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
  QApplication::setApplicationVersion(labelbuddy::get_version());

  QCommandLineParser parser;
  labelbuddy::prepare_parser(parser);
  parser.process(app);
  const QStringList args = parser.positionalArguments();
  const QStringList labels_files = parser.values("import-labels");
  const QStringList docs_files = parser.values("import-docs");
  const QString export_labels_file = parser.value("export-labels");
  const QString export_docs_file = parser.value("export-docs");
  QString db_path = (args.length() == 0) ? QString() : args[0];

  if (labels_files.length() || docs_files.length() ||
      (export_labels_file != QString()) || (export_docs_file != QString()) ||
      parser.isSet("vacuum")) {
    if (db_path == QString()) {
      std::cerr << "Specify database path explicitly to import / export "
                << "labels and documents or vacuum db" << std::endl;
      return 1;
    }
    return labelbuddy::batch_import_export(
        db_path, labels_files, docs_files, export_labels_file, export_docs_file,
        parser.isSet("labelled-only"), !parser.isSet("no-text"),
        !parser.isSet("no-annotations"), parser.value("approver"),
        parser.isSet("vacuum"));
  }

  std::unique_ptr<labelbuddy::LabelBuddy> label_buddy(
      new labelbuddy::LabelBuddy(nullptr, db_path, parser.isSet("demo")));
  app.setWindowIcon(QIcon(":/data/icons/LB.png"));
  label_buddy->show();
  return app.exec();
}
