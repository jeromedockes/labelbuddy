#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QString>
#include <QStringList>

#include "main_window.h"
#include "utils.h"

int main(int argc, char* argv[]) {

  QApplication app(argc, argv);
  QApplication::setApplicationName("labelbuddy");
  QApplication::setApplicationVersion(labelbuddy::get_version());

  QCommandLineParser parser;
  parser.setApplicationDescription("Annotate documents");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument("database", "Database to open");
  parser.process(app);
  const QStringList args = parser.positionalArguments();

  QString db_path = (args.length() == 0) ? QString() : args[0];
  labelbuddy::LabelBuddy* label_buddy =
      new labelbuddy::LabelBuddy(nullptr, db_path);
  app.setWindowIcon(QIcon(":/data/LB.png"));
  label_buddy->show();
  return app.exec();
}
