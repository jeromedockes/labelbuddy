#ifndef LABELBUDDY_MAIN_WINDOW_H
#define LABELBUDDY_MAIN_WINDOW_H

#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QString>
#include <QTabWidget>

#include "annotator.h"
#include "database.h"
#include "dataset_menu.h"
#include "doc_list_model.h"
#include "import_export_menu.h"
#include "label_list_model.h"

/// \file
/// Main window, containing the menu bar and Annotate, Dataset, I/E tabs.

namespace labelbuddy {

/// Main window, containing the menu bar and Annotate, Dataset, I/E tabs.
class LabelBuddy : public QMainWindow {

  Q_OBJECT

public:
  LabelBuddy(QWidget* parent = nullptr,
             const QString& database_path = QString(),
             bool start_from_temp_db = false);
  void closeEvent(QCloseEvent* event);

  /// `false` if database could not be opened when object was constructed.

  /// There must always be a database opened.
  ///
  /// When constructing the object, if the specified database could not be
  /// opened, or if the database was not specified and the default ones could
  /// not be opened, the children widgets are not created and `is_valid` is set
  /// to false.
  /// In this case the application is closed.
  ///
  /// During application execution, if the user tries to open a database that
  /// cannot be opened (eg not a sqlite3 file), it is not opened and the current
  /// database remains, and `is_valid` remains `true`.
  bool is_valid() const;

signals:

  /// User selected a different database through "New", "Open" or "Demo"
  void database_changed(const QString& new_database_name);
  void about_to_close();

public slots:

  /// focus the Annotate tab
  void go_to_annotations();

private slots:

  /// Open a file dialog to select and open a database

  /// We don't use a native dialog to allow selecting an existing or a new file.
  void open_database();

  /// Open the temporary demo database
  void open_temp_database();

  /// show the "about" message box
  void show_about_info();

  /// Open documentation in a web browser.

  /// If it is found in /user/share/doc or in the executable's parent directory
  /// the local file is opened, otherwise the online documentation
  void open_documentation_url();

  void update_status_bar();
  void update_current_doc_info();

private:
  DocListModel* doc_model;
  LabelListModel* label_model;
  AnnotationsModel* annotations_model;
  QTabWidget* notebook;
  Annotator* annotator;
  DatasetMenu* dataset_menu;
  ImportExportMenu* import_export_menu;
  DatabaseCatalog database_catalog;
  QLabel* db_name_label;
  QLabel* db_summary_label;
  QLabel* current_doc_info_label;

  void store_notebook_page();
  void add_connections();
  void add_menubar();
  void set_geometry();

  void warn_failed_to_open_db(const QString& database_path);
  bool valid_state{true};
};
} // namespace labelbuddy
#endif
