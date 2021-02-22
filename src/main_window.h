#ifndef LABELBUDDY_MAIN_WINDOW_H
#define LABELBUDDY_MAIN_WINDOW_H

#include <QCloseEvent>
#include <QMainWindow>
#include <QString>
#include <QTabWidget>

#include "annotator.h"
#include "database.h"
#include "dataset_menu.h"
#include "doc_list_model.h"
#include "import_export_menu.h"
#include "label_list_model.h"

namespace labelbuddy {

class LabelBuddy : public QMainWindow {

  Q_OBJECT

public:
  LabelBuddy(QWidget* parent = nullptr,
             const QString& database_path = QString());
  void closeEvent(QCloseEvent* event);

signals:

  void database_changed(const QString& new_database_name);
  void about_to_close();

public slots:

  void go_to_annotations();

private slots:

  void open_database();
  void open_new_database();
  void open_temp_database();
  void show_about_info();
  void open_documentation_url();

private:
  QTabWidget* notebook;
  Annotator* annotator;
  DatabaseCatalog database_catalog;

  void store_notebook_page();
  void add_menubar();
  void set_geometry();
};
} // namespace labelbuddy
#endif
