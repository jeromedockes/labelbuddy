#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSize>
#include <QStatusBar>
#include <QTabWidget>

#include "annotations_model.h"
#include "database.h"
#include "dataset_menu.h"
#include "import_export_menu.h"
#include "utils.h"

#include "main_window.h"

namespace labelbuddy {

void LabelBuddy::warn_failed_to_open_db(const QString& database_path) {
  QString db_msg(
      database_path != QString() ? QString(":\n%0").arg(database_path) : "");
  QMessageBox::critical(this, "labelbuddy",
                        QString("Could not open database%0").arg(db_msg),
                        QMessageBox::Ok);
}

LabelBuddy::LabelBuddy(QWidget* parent, const QString& database_path,
                       bool start_from_temp_db)
    : QMainWindow(parent), database_catalog{} {

  if (start_from_temp_db) {
    database_catalog.open_temp_database();
  } else {
    bool opened = database_catalog.open_database(database_path);
    if (!opened) {
      warn_failed_to_open_db(database_path);
      valid_state = false;
      return;
    }
  }

  notebook = new QTabWidget();
  setCentralWidget(notebook);

  annotator = new Annotator();
  notebook->addTab(annotator, "Annotate");

  dataset_menu = new DatasetMenu();
  notebook->addTab(dataset_menu, "Dataset");

  import_export_menu = new ImportExportMenu(&database_catalog);
  notebook->addTab(import_export_menu, "Import / Export");

  notebook->setCurrentIndex(
      start_from_temp_db
          ? 0
          : database_catalog.get_app_state_extra("notebook_page", 2).toInt());

  doc_model = new DocListModel(this);
  doc_model->set_database(database_catalog.get_current_database());
  label_model = new LabelListModel(this);
  label_model->set_database(database_catalog.get_current_database());
  annotations_model = new AnnotationsModel(this);
  annotations_model->set_database(database_catalog.get_current_database());
  dataset_menu->set_doc_list_model(doc_model);
  dataset_menu->set_label_list_model(label_model);
  annotator->set_annotations_model(annotations_model);
  annotator->set_label_list_model(label_model);

  statusBar()->setSizeGripEnabled(false);
  statusBar()->setStyleSheet("QStatusBar::item {border: None;}");
  db_name_label = new QLabel();
  statusBar()->addWidget(db_name_label);
  db_summary_label = new QLabel();
  statusBar()->addWidget(db_summary_label);
  current_doc_info_label = new QLabel();
  statusBar()->addWidget(current_doc_info_label);
  update_status_bar();

  add_menubar();
  set_geometry();

  add_connections();
}

void LabelBuddy::add_connections() {

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
  QObject::connect(import_export_menu, &ImportExportMenu::documents_added,
                   annotator, &Annotator::update_annotations);

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

  QObject::connect(this, &LabelBuddy::database_changed, this,
                   &LabelBuddy::update_status_bar);
  QObject::connect(doc_model, &DocListModel::docs_deleted, this,
                   &LabelBuddy::update_status_bar);
  QObject::connect(label_model, &LabelListModel::labels_deleted, this,
                   &LabelBuddy::update_status_bar);
  QObject::connect(import_export_menu, &ImportExportMenu::documents_added, this,
                   &LabelBuddy::update_status_bar);
  QObject::connect(annotations_model,
                   &AnnotationsModel::document_status_changed, this,
                   &LabelBuddy::update_status_bar);
  QObject::connect(notebook, &QTabWidget::currentChanged, this,
                   &LabelBuddy::update_current_doc_info);
  QObject::connect(annotator, &Annotator::current_status_display_changed, this,
                   &LabelBuddy::update_current_doc_info);
}

bool LabelBuddy::is_valid() const { return valid_state; }

void LabelBuddy::go_to_annotations() { notebook->setCurrentIndex(0); }

void LabelBuddy::add_menubar() {
  auto file_menu = menuBar()->addMenu("File");
  auto open_db_action = new QAction("Open...", this);
  file_menu->addAction(open_db_action);
  open_db_action->setShortcut(QKeySequence::Open);
  auto temp_db_action = new QAction("Demo", this);
  file_menu->addAction(temp_db_action);
  auto quit_action = new QAction("Quit", this);
  file_menu->addAction(quit_action);
  quit_action->setShortcut(QKeySequence::Quit);
  QObject::connect(open_db_action, &QAction::triggered, this,
                   &LabelBuddy::open_database);
  QObject::connect(temp_db_action, &QAction::triggered, this,
                   &LabelBuddy::open_temp_database);
  QObject::connect(quit_action, &QAction::triggered, this, &LabelBuddy::close);

  auto preferences_menu = menuBar()->addMenu("Preferences");
  auto set_monospace_action = new QAction("Monospace font", this);
  set_monospace_action->setCheckable(true);
  QSettings settings("labelbuddy", "labelbuddy");
  set_monospace_action->setChecked(
      settings.value("Labelbuddy/monospace_font").toInt());
  preferences_menu->addAction(set_monospace_action);
  annotator->set_monospace_font(set_monospace_action->isChecked());
  QObject::connect(set_monospace_action, &QAction::triggered, annotator,
                   &Annotator::set_monospace_font);

  auto set_use_bold_action = new QAction("Bold selected region", this);
  set_use_bold_action->setCheckable(true);
  set_use_bold_action->setChecked(
      settings.value("Labelbuddy/selected_annotation_bold", 0).toInt());
  preferences_menu->addAction(set_use_bold_action);
  annotator->set_use_bold_font(set_use_bold_action->isChecked());
  QObject::connect(set_use_bold_action, &QAction::triggered, annotator,
                   &Annotator::set_use_bold_font);

  auto help_menu = menuBar()->addMenu("Help");
  auto show_about_action = new QAction("About", this);
  help_menu->addAction(show_about_action);
  auto open_doc_action = new QAction("Documentation", this);
  help_menu->addAction(open_doc_action);
  open_doc_action->setShortcut(QKeySequence::HelpContents);
  QObject::connect(show_about_action, &QAction::triggered, this,
                   &LabelBuddy::show_about_info);
  QObject::connect(open_doc_action, &QAction::triggered, this,
                   &LabelBuddy::open_documentation_url);
}

void LabelBuddy::update_status_bar() {
  auto db_name =
      database_name_display(database_catalog.get_current_database(), true);
  db_name_label->setText(QString(" %0").arg(db_name));
  auto n_docs = doc_model->total_n_docs();
  auto n_labelled = doc_model->total_n_docs(DocListModel::DocFilter::labelled);
  db_summary_label->setText(QString(" |  %0 document%1 (%2 labelled)")
                                .arg(n_docs)
                                .arg(n_docs != 1 ? "s" : "")
                                .arg(n_labelled));
  update_current_doc_info();
}

void LabelBuddy::update_current_doc_info() {
  if (notebook->currentIndex() != 0) {
    current_doc_info_label->setText("");
  } else {
    current_doc_info_label->setText(
        QString(" |  %0").arg(annotator->current_status_display()));
  }
}

void LabelBuddy::open_database() {
  QFileDialog dialog(this, "Open database");
  dialog.setOption(QFileDialog::DontUseNativeDialog);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setNameFilter("labelbuddy databases (*.labelbuddy *.sqlite3 *.sqlite "
                       "*.db);; All files (*)");
  dialog.setViewMode(QFileDialog::Detail);
  dialog.setOption(QFileDialog::HideNameFilterDetails, false);
  dialog.setOption(QFileDialog::DontUseCustomDirectoryIcons, true);
  dialog.setOption(QFileDialog::DontConfirmOverwrite, true);
  dialog.setDirectory(database_catalog.get_last_opened_directory());
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  if (!dialog.exec() || dialog.selectedFiles().isEmpty()) {
    return;
  }
  auto file_path = dialog.selectedFiles()[0];
  if (file_path == QString()) {
    return;
  }
  store_notebook_page();
  bool opened = database_catalog.open_database(file_path);
  if (opened) {
    emit database_changed(file_path);
  } else {
    warn_failed_to_open_db(file_path);
  }
}

void LabelBuddy::open_temp_database() {
  store_notebook_page();
  auto db_path = database_catalog.open_temp_database();
  emit database_changed(db_path);
}

void LabelBuddy::store_notebook_page() {
  database_catalog.set_app_state_extra("notebook_page",
                                       notebook->currentIndex());
}

void LabelBuddy::closeEvent(QCloseEvent* event) {
  if (!valid_state) {
    QMainWindow::closeEvent(event);
    return;
  }
  store_notebook_page();
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("LabelBuddy/geometry", saveGeometry());
  settings.setValue("Labelbuddy/monospace_font",
                    int(annotator->is_monospace()));
  settings.setValue("Labelbuddy/selected_annotation_bold",
                    int(annotator->is_using_bold()));

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
          "labelbuddy version %0\nhttps://jeromedockes.github.io/labelbuddy/")
          .arg(get_version());
  QMessageBox::information(this, "labelbuddy", message, QMessageBox::Ok);
}

void LabelBuddy::open_documentation_url() {
  QDesktopServices::openUrl(get_doc_url());
}
} // namespace labelbuddy
