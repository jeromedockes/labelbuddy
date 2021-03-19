#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFontDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSize>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>

#include "annotations_model.h"
#include "database.h"
#include "dataset_menu.h"
#include "import_export_menu.h"
#include "utils.h"

#include "main_window.h"

namespace labelbuddy {

const QString LabelBuddy::bf_setting_key_{
    "LabelBuddy/selected_annotation_bold"};
const QString LabelBuddy::font_setting_key_{"LabelBuddy/annotator_font"};

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

  notebook_owner_.reset(new QTabWidget());
  notebook = notebook_owner_.get();
  annotator = new Annotator();
  notebook->addTab(annotator, "&Annotate");
  dataset_menu = new DatasetMenu();
  notebook->addTab(dataset_menu, "&Dataset");
  import_export_menu = new ImportExportMenu(&database_catalog);
  notebook->addTab(import_export_menu, "&Import / Export");

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

  statusBar()->hide();
  statusBar()->setSizeGripEnabled(false);
  statusBar()->setStyleSheet(
      "QStatusBar::item {border: none; border-right: 1px solid black;} "
      "QStatusBar QLabel {margin-left: 1px; margin-right: 1px}");
  status_db_name_ = new QLabel();
  statusBar()->addWidget(status_db_name_);
  status_db_summary_ = new QLabel();
  statusBar()->addWidget(status_db_summary_);
  status_current_annotation_label_ = new QLabel();
  statusBar()->addWidget(status_current_annotation_label_);
  status_current_annotation_info_ = new QLabel();
  statusBar()->addWidget(status_current_annotation_info_);
  status_current_doc_info_ = new QLabel();
  statusBar()->addWidget(status_current_doc_info_);

  update_status_bar();

  add_menubar();
  set_geometry();
  init_annotator_settings();
  add_connections();
  add_welcome_label();

  bool opened{};
  if (start_from_temp_db) {
    database_catalog.open_temp_database();
    opened = true;
  } else {
    opened = database_catalog.open_database(database_path);
  }
  if (opened) {
    emit database_changed(database_catalog.get_current_database());
    display_notebook();
  } else if (database_path != QString()) {
    need_warn_failed_to_open_init_db_ = true;
    init_db_path_ = database_path;
  }
  update_window_title();
  check_tab_focus();
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
  QObject::connect(this, &LabelBuddy::database_changed, this,
                   &LabelBuddy::update_window_title);
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

  QObject::connect(notebook, &QTabWidget::currentChanged, this,
                   &LabelBuddy::check_tab_focus);
}

void LabelBuddy::go_to_annotations() { notebook->setCurrentIndex(0); }

void LabelBuddy::check_tab_focus() {
  if (notebook->currentIndex() == 0) {
    annotator->setFocus();
  }
}

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
  QSettings settings("labelbuddy", "labelbuddy");
  auto set_use_bold_action = new QAction("Bold selected annotation", this);
  set_use_bold_action->setCheckable(true);
  set_use_bold_action->setChecked(
      settings.value(bf_setting_key_, false).toBool());
  preferences_menu->addAction(set_use_bold_action);
  QObject::connect(set_use_bold_action, &QAction::triggered, this,
                   &LabelBuddy::set_use_bold_font);

  auto choose_font_action = new QAction("Choose font", this);
  preferences_menu->addAction(choose_font_action);
  QObject::connect(choose_font_action, &QAction::triggered, this,
                   &LabelBuddy::choose_font);
  auto reset_font_action = new QAction("Reset font to default", this);
  preferences_menu->addAction(reset_font_action);
  QObject::connect(reset_font_action, &QAction::triggered, this,
                   &LabelBuddy::reset_font);

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

void LabelBuddy::add_welcome_label() {
  auto welcome_label = new QLabel();
  setCentralWidget(welcome_label);
  welcome_label->setTextFormat(Qt::RichText);
  welcome_label->setText(get_welcome_message());
  welcome_label->setWordWrap(true);
  welcome_label->setIndent(welcome_label->fontMetrics().averageCharWidth());
  welcome_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  welcome_label->setOpenExternalLinks(true);
}

void LabelBuddy::update_window_title() {
  if (notebook_owner_ != nullptr) {
    setWindowTitle("labelbuddy");
  } else {
    auto db_name =
        database_name_display(database_catalog.get_current_database(), true);
    setWindowTitle(QString("labelbuddy — %0").arg(db_name));
  }
}

void LabelBuddy::set_use_bold_font(bool use_bold) {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue(bf_setting_key_, use_bold);
  annotator->set_use_bold_font(use_bold);
}

void LabelBuddy::choose_font() {
  QSettings settings("labelbuddy", "labelbuddy");
  bool ok{};
  QFont new_font;
  if (settings.contains(font_setting_key_)) {
    new_font = QFontDialog::getFont(
        &ok, settings.value(font_setting_key_).value<QFont>(), this);
  } else {
    new_font = QFontDialog::getFont(&ok, this);
  }
  if (!ok) {
    return;
  }
  settings.setValue(font_setting_key_, new_font);
  annotator->set_font(new_font);
}

void LabelBuddy::reset_font() {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.remove(font_setting_key_);
  annotator->set_font(QFont());
}

void LabelBuddy::update_status_bar() {
  auto db_name =
      database_name_display(database_catalog.get_current_database(), true);
  status_db_name_->setText(db_name);
  auto n_docs = doc_model->total_n_docs();
  auto n_labelled = doc_model->total_n_docs(DocListModel::DocFilter::labelled);
  status_db_summary_->setText(QString("%0 document%1 (%2 labelled)")
                                  .arg(n_docs)
                                  .arg(n_docs != 1 ? "s" : "")
                                  .arg(n_labelled));
  update_current_doc_info();
}

void LabelBuddy::update_current_doc_info() {
  if ((notebook->currentIndex() != 0) ||
      (!annotations_model->is_positioned_on_valid_doc())) {
    status_current_doc_info_->hide();
    status_current_annotation_info_->hide();
    status_current_annotation_label_->hide();
    return;
  }
  auto status = annotator->current_status_info();
  status_current_doc_info_->setText(status.doc_info);
  status_current_doc_info_->show();
  if (status.annotation_label == "") {
    status_current_annotation_info_->hide();
    status_current_annotation_label_->hide();
    return;
  }
  status_current_annotation_info_->setText(status.annotation_info);
  status_current_annotation_label_->setText(status.annotation_label);
  status_current_annotation_info_->show();
  status_current_annotation_label_->show();
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
    display_notebook();
    emit database_changed(file_path);
  } else {
    warn_failed_to_open_db(file_path);
  }
}

void LabelBuddy::display_notebook() {
  if (notebook_owner_ == nullptr) {
    return;
  }
  notebook->setCurrentIndex(
      database_catalog.get_app_state_extra("notebook_page", 2).toInt());
  setCentralWidget(notebook_owner_.release());
  statusBar()->show();
  check_tab_focus();
}

void LabelBuddy::open_temp_database() {
  store_notebook_page();
  auto db_path = database_catalog.open_temp_database();
  emit database_changed(db_path);
  display_notebook();
}

void LabelBuddy::store_notebook_page() {
  database_catalog.set_app_state_extra("notebook_page",
                                       notebook->currentIndex());
}

void LabelBuddy::showEvent(QShowEvent* event) {
  QMainWindow::showEvent(event);
  if (need_warn_failed_to_open_init_db_) {
    // use a timer otherwise the main window is only drawn after the warning is
    // closed
    QTimer::singleShot(0, this, &LabelBuddy::warn_failed_to_open_init_db);
  }
}

void LabelBuddy::warn_failed_to_open_init_db() {
  warn_failed_to_open_db(init_db_path_);
  need_warn_failed_to_open_init_db_ = false;
  init_db_path_ = "";
}

void LabelBuddy::closeEvent(QCloseEvent* event) {
  store_notebook_page();
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("LabelBuddy/geometry", saveGeometry());
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

void LabelBuddy::init_annotator_settings() {
  QSettings settings("labelbuddy", "labelbuddy");
  annotator->set_use_bold_font(settings.value(bf_setting_key_, false).toBool());
  if (settings.contains(font_setting_key_)) {
    annotator->set_font(settings.value(font_setting_key_).value<QFont>());
  }
}

void LabelBuddy::show_about_info() {
  auto message = QString("<p>labelbuddy version %0<br/>"
                         "<a href='https://jeromedockes.github.io/labelbuddy/'>"
                         "jeromedockes.github.io/labelbuddy/</a></p>")
                     .arg(get_version());
  QMessageBox::information(this, "labelbuddy", message, QMessageBox::Ok);
}

void LabelBuddy::open_documentation_url() {
  QDesktopServices::openUrl(get_doc_url());
}
} // namespace labelbuddy
