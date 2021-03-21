#ifndef LABELBUDDY_MAIN_WINDOW_H
#define LABELBUDDY_MAIN_WINDOW_H

#include <memory>

#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QShowEvent>
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
  /// Create the main window and children widgets.

  /// Unless `start_from_temp_db` is true, tries to open `database_path` if it
  /// is not empty, otherwise the last opened database. If that fails, the tab
  /// widget is not shown; the welcome banner is shown instead. However the tab
  /// widget is still created (and managed by a `unique_ptr`) and the models are
  /// positioned on a valid database (the empty temp database, see
  /// `DatabaseCatalog`).
  ///
  /// If `start_from_temp_db` is true, opens the temp database and inserts the
  /// example docs.
  ///
  /// The tab widget is set as the central widget the first time a database is
  /// successfully opened (either a SQLite file or asking explicitly for the
  /// temp database).
  LabelBuddy(QWidget* parent = nullptr,
             const QString& database_path = QString(),
             bool start_from_temp_db = false);
  void closeEvent(QCloseEvent* event) override;
  void showEvent(QShowEvent* event) override;

signals:

  /// User selected a different database through "Open" or "Demo"
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
  void update_window_title();
  void update_current_doc_info();

private slots:
  void warn_failed_to_open_init_db();
  void check_tab_focus();
  void choose_font();
  void reset_font();
  void set_use_bold_font(bool use_bold);

private:
  DocListModel* doc_model;
  LabelListModel* label_model;
  AnnotationsModel* annotations_model;
  QTabWidget* notebook;
  Annotator* annotator;
  DatasetMenu* dataset_menu;
  ImportExportMenu* import_export_menu;
  DatabaseCatalog database_catalog;

  // QLabels in the status bar
  QLabel* status_db_name_;
  QLabel* status_db_summary_;
  QLabel* status_current_doc_info_;
  QLabel* status_current_annotation_info_;
  // having the label in its own widget means it doesn't affect the rest if it
  // is written from right to left (eg in Arabic), has rich text, several lines,
  // etc.
  QLabel* status_current_annotation_label_;

  // store info about the db passed to constructor if it failed to open. Used to
  // display an error message after the main window is shown.
  // set to false once the error message has been shown.
  bool need_warn_failed_to_open_init_db_{};
  QString init_db_path_{};

  /// own the notebook when it is not set as central widget
  std::unique_ptr<QTabWidget> notebook_owner_ = nullptr;

  /// Sets the tab widget as central widget if not already the case.
  void display_notebook();
  void store_notebook_page();
  void add_connections();
  void add_menubar();
  void set_geometry();
  void init_annotator_settings();
  void add_welcome_label();

  void warn_failed_to_open_db(const QString& database_path);

  static const QString bf_setting_key_;
  static const QString font_setting_key_;
};
} // namespace labelbuddy
#endif
