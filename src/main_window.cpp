#include <QAction>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSize>
#include <QTabWidget>
#include <QUrl>

#include "annotations_model.h"
#include "database.h"
#include "dataset_menu.h"
#include "import_export_menu.h"
#include "utils.h"

#include "main_window.h"

namespace labelbuddy {

LabelBuddy::LabelBuddy(QWidget* parent, const QString& database_path)
    : QMainWindow(parent), database_catalog{} {

  database_catalog.open_database(database_path);

  notebook = new QTabWidget();
  setCentralWidget(notebook);

  Annotator* annotator = new Annotator();
  notebook->addTab(annotator, "Annotate");

  DatasetMenu* dataset_menu = new DatasetMenu();
  notebook->addTab(dataset_menu, "Dataset");

  ImportExportMenu* import_export_menu =
      new ImportExportMenu(&database_catalog);
  notebook->addTab(import_export_menu, "Import / Export");

  notebook->setCurrentIndex(
      database_catalog.get_app_state_extra("notebook_page", 2).toInt());

  DocListModel* doc_model = new DocListModel(this);
  doc_model->set_database(database_catalog.get_current_database());
  LabelListModel* label_model = new LabelListModel(this);
  label_model->set_database(database_catalog.get_current_database());
  AnnotationsModel* annotations_model = new AnnotationsModel(this);
  annotations_model->set_database(database_catalog.get_current_database());
  dataset_menu->set_doc_list_model(doc_model);
  dataset_menu->set_label_list_model(label_model);
  annotator->set_annotations_model(annotations_model);
  annotator->set_label_list_model(label_model);

  add_menubar();
  set_geometry();

  QObject::connect(notebook, &QTabWidget::currentChanged, this,
                   &LabelBuddy::store_notebook_page);
  QObject::connect(dataset_menu, &DatasetMenu::visit_doc_requested,
                   annotations_model, &AnnotationsModel::visit_doc);
  QObject::connect(dataset_menu, &DatasetMenu::visit_doc_requested, this,
                   &LabelBuddy::go_to_annotations);

  QObject::connect(doc_model, &DocListModel::docs_deleted, annotations_model,
                   &AnnotationsModel::check_current_doc);
  QObject::connect(label_model, &LabelListModel::labels_changed, annotator,
                   &Annotator::update_annotations);
  QObject::connect(label_model, &LabelListModel::labels_changed, annotator,
                   &Annotator::update_nav_buttons);
  QObject::connect(doc_model, &DocListModel::docs_deleted, annotator,
                   &Annotator::update_nav_buttons);
  QObject::connect(import_export_menu, &ImportExportMenu::documents_added,
                   annotator, &Annotator::update_nav_buttons);

  QObject::connect(annotations_model,
                   &AnnotationsModel::document_status_changed, doc_model,
                   &DocListModel::refresh_current_query);

  QObject::connect(import_export_menu, &ImportExportMenu::documents_added,
                   doc_model, &DocListModel::refresh_current_query);
  QObject::connect(import_export_menu, &ImportExportMenu::documents_added,
                   annotations_model, &AnnotationsModel::check_current_doc);

  QObject::connect(import_export_menu, &ImportExportMenu::labels_added,
                   label_model, &LabelListModel::refresh_current_query);
  QObject::connect(import_export_menu, &ImportExportMenu::labels_added,
                   annotator, &Annotator::update_annotations);

  QObject::connect(this, &LabelBuddy::database_changed, doc_model,
                   &DocListModel::set_database);
  QObject::connect(this, &LabelBuddy::database_changed, label_model,
                   &LabelListModel::set_database);
  QObject::connect(this, &LabelBuddy::database_changed, annotations_model,
                   &AnnotationsModel::set_database);
  QObject::connect(this, &LabelBuddy::database_changed, import_export_menu,
                   &ImportExportMenu::update_database_info);

  QObject::connect(this, &LabelBuddy::about_to_close, dataset_menu,
                   &DatasetMenu::store_state, Qt::DirectConnection);
  QObject::connect(this, &LabelBuddy::about_to_close, annotator,
                   &Annotator::store_state, Qt::DirectConnection);
}

void LabelBuddy::go_to_annotations() { notebook->setCurrentIndex(0); }

void LabelBuddy::add_menubar() {
  auto file_menu = menuBar()->addMenu("File");
  auto open_db_action = new QAction("Open", this);
  file_menu->addAction(open_db_action);
  auto open_new_db_action = new QAction("New", this);
  file_menu->addAction(open_new_db_action);
  auto quit_action = new QAction("Quit", this);
  file_menu->addAction(quit_action);
  QObject::connect(open_db_action, &QAction::triggered, this,
                   &LabelBuddy::open_database);
  QObject::connect(open_new_db_action, &QAction::triggered, this,
                   &LabelBuddy::open_new_database);
  QObject::connect(quit_action, &QAction::triggered, this, &LabelBuddy::close);

  auto help_menu = menuBar()->addMenu("Help");
  auto show_about_action = new QAction("About", this);
  help_menu->addAction(show_about_action);
  auto open_doc_action = new QAction("Documentation", this);
  help_menu->addAction(open_doc_action);
  QObject::connect(show_about_action, &QAction::triggered, this,
                   &LabelBuddy::show_about_info);
  QObject::connect(open_doc_action, &QAction::triggered, this,
                   &LabelBuddy::open_documentation_url);
}

void LabelBuddy::open_database() {
  auto file_path = QFileDialog::getOpenFileName(
      this, "Documents file", database_catalog.get_current_directory(),
      "SQLite databses (*.sql *.sqlite3);; All files (*)");
  if (file_path == QString()) {
    return;
  }
  database_catalog.set_app_state_extra("notebook_page",
                                       notebook->currentIndex());
  database_catalog.open_database(file_path);
  notebook->setCurrentIndex(
      database_catalog.get_app_state_extra("notebook_page", 2).toInt());
  emit database_changed(file_path);
}

void LabelBuddy::open_new_database() {
  auto file_path = QFileDialog::getSaveFileName(
      this, "Documents file", database_catalog.get_current_directory(),
      "SQLite databses (*.sql *.sqlite *.sqlite3);; All files (*)", nullptr,
      QFileDialog::DontConfirmOverwrite);
  if (file_path == QString()) {
    return;
  }
  database_catalog.open_database(file_path);
  emit database_changed(file_path);
}

void LabelBuddy::store_notebook_page(int new_index) {
  database_catalog.set_app_state_extra("notebook_page", new_index);
}

void LabelBuddy::closeEvent(QCloseEvent* event) {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("LabelBuddy/geometry", saveGeometry());
  settings.setValue("last_opened_database",
                    database_catalog.get_current_database());
  emit about_to_close();
  QMainWindow::closeEvent(event);
}

void LabelBuddy::set_geometry() {
  QSettings settings("labelbuddy", "labelbuddy");
  if (settings.contains("LabelBuddy/geometry")) {
    restoreGeometry(settings.value("LabelBuddy/geometry").toByteArray());
  } else {
    resize(QSize(600, 600));
  }
}

void LabelBuddy::show_about_info() {
  auto message =
      QString(
          "labelbuddy version %0\nhttps://github.com/jeromedockes/labelbuddy")
          .arg(get_version());
  QMessageBox::information(this, "labelbuddy", message, QMessageBox::Ok);
}

void LabelBuddy::open_documentation_url() {
  QDesktopServices::openUrl(QUrl("https://github.com/jeromedockes/labelbuddy"));
}
} // namespace labelbuddy
