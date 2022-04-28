#include <cassert>

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
#include <QSqlDatabase>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <qobject.h>

#include "annotations_model.h"
#include "database.h"
#include "dataset_menu.h"
#include "import_export_menu.h"
#include "utils.h"

#include "main_window.h"

namespace labelbuddy {

void LabelBuddy::warn_failed_to_open_db(const QString& database_path) {
  assert(!QSqlDatabase::contains(database_path));
  QString db_msg(
      database_path != QString() ? QString(":\n%0").arg(database_path) : "");
  QMessageBox::critical(this, "labelbuddy",
                        QString("Could not open database%0").arg(db_msg),
                        QMessageBox::Ok);
}

LabelBuddy::LabelBuddy(QWidget* parent, const QString& database_path,
                       bool start_from_temp_db)
    : QMainWindow(parent) {

  notebook_owner_.reset(new QTabWidget());
  notebook_ = notebook_owner_.get();
  annotator_ = new Annotator();
  notebook_->addTab(annotator_, "&Annotate");
  dataset_menu_ = new DatasetMenu();
  notebook_->addTab(dataset_menu_, "&Dataset");
  import_export_menu_ = new ImportExportMenu(&database_catalog_);
  notebook_->addTab(import_export_menu_, "&Import / Export");

  doc_model_ = new DocListModel(this);
  doc_model_->set_database(database_catalog_.get_current_database());
  label_model_ = new LabelListModel(this);
  label_model_->set_database(database_catalog_.get_current_database());
  annotations_model_ = new AnnotationsModel(this);
  annotations_model_->set_database(database_catalog_.get_current_database());
  dataset_menu_->set_doc_list_model(doc_model_);
  dataset_menu_->set_label_list_model(label_model_);
  annotator_->set_annotations_model(annotations_model_);
  annotator_->set_label_list_model(label_model_);

  add_status_bar();
  add_menubar();
  set_geometry();
  init_annotator_settings();
  add_connections();
  add_welcome_label();

  bool opened{};
  if (start_from_temp_db) {
    database_catalog_.open_temp_database();
    opened = true;
  } else {
    opened = database_catalog_.open_database(database_path);
  }
  if (opened) {
    emit database_changed(database_catalog_.get_current_database());
    display_notebook();
  } else if (database_path != QString()) {
    need_warn_failed_to_open_init_db_ = true;
    init_db_path_ = database_path;
  }
  update_window_title();
  check_tab_focus();
}

void LabelBuddy::add_connections() {

  QObject::connect(dataset_menu_, &DatasetMenu::visit_doc_requested,
                   annotations_model_, &AnnotationsModel::visit_doc);
  QObject::connect(dataset_menu_, &DatasetMenu::visit_doc_requested, this,
                   &LabelBuddy::go_to_annotations);

  QObject::connect(doc_model_, &DocListModel::docs_deleted, annotations_model_,
                   &AnnotationsModel::check_current_doc);
  QObject::connect(label_model_, &LabelListModel::labels_changed, annotator_,
                   &Annotator::update_annotations);
  QObject::connect(label_model_, &LabelListModel::labels_changed, annotator_,
                   &Annotator::update_nav_buttons);
  QObject::connect(label_model_, &LabelListModel::labels_order_changed,
                   annotator_, &Annotator::update_annotations);
  QObject::connect(doc_model_, &DocListModel::docs_deleted, annotator_,
                   &Annotator::update_nav_buttons);
  QObject::connect(import_export_menu_, &ImportExportMenu::documents_added,
                   annotator_, &Annotator::update_nav_buttons);

  QObject::connect(annotations_model_,
                   &AnnotationsModel::document_status_changed, doc_model_,
                   &DocListModel::document_status_changed);
  QObject::connect(annotations_model_, &AnnotationsModel::document_gained_label,
                   doc_model_, &DocListModel::document_gained_label);
  QObject::connect(annotations_model_, &AnnotationsModel::document_lost_label,
                   doc_model_, &DocListModel::document_lost_label);

  QObject::connect(import_export_menu_, &ImportExportMenu::documents_added,
                   doc_model_, &DocListModel::refresh_current_query);
  QObject::connect(import_export_menu_, &ImportExportMenu::labels_added,
                   doc_model_, &DocListModel::labels_changed);
  QObject::connect(import_export_menu_, &ImportExportMenu::documents_added,
                   doc_model_, &DocListModel::labels_changed);
  QObject::connect(import_export_menu_, &ImportExportMenu::documents_added,
                   annotations_model_, &AnnotationsModel::check_current_doc);
  QObject::connect(import_export_menu_, &ImportExportMenu::documents_added,
                   annotator_, &Annotator::update_annotations);

  QObject::connect(import_export_menu_, &ImportExportMenu::labels_added,
                   label_model_, &LabelListModel::refresh_current_query);
  QObject::connect(import_export_menu_, &ImportExportMenu::labels_added,
                   annotator_, &Annotator::update_annotations);

  QObject::connect(this, &LabelBuddy::database_changed, annotator_,
                   &Annotator::reset_skip_updating_nav_buttons);
  QObject::connect(this, &LabelBuddy::database_changed, doc_model_,
                   &DocListModel::set_database);
  QObject::connect(this, &LabelBuddy::database_changed, label_model_,
                   &LabelListModel::set_database);
  QObject::connect(this, &LabelBuddy::database_changed, annotations_model_,
                   &AnnotationsModel::set_database);
  QObject::connect(this, &LabelBuddy::database_changed, import_export_menu_,
                   &ImportExportMenu::update_database_info);

  QObject::connect(this, &LabelBuddy::about_to_close, dataset_menu_,
                   &DatasetMenu::store_state, Qt::DirectConnection);
  QObject::connect(this, &LabelBuddy::about_to_close, annotator_,
                   &Annotator::store_state, Qt::DirectConnection);

  QObject::connect(this, &LabelBuddy::database_changed, this,
                   &LabelBuddy::update_status_bar);
  QObject::connect(this, &LabelBuddy::database_changed, this,
                   &LabelBuddy::update_window_title);
  QObject::connect(doc_model_, &DocListModel::docs_deleted, this,
                   &LabelBuddy::update_status_bar);
  QObject::connect(label_model_, &LabelListModel::labels_deleted, this,
                   &LabelBuddy::update_status_bar);
  QObject::connect(import_export_menu_, &ImportExportMenu::documents_added,
                   this, &LabelBuddy::update_status_bar);
  QObject::connect(annotations_model_,
                   &AnnotationsModel::document_status_changed, this,
                   &LabelBuddy::update_status_bar);
  QObject::connect(notebook_, &QTabWidget::currentChanged, this,
                   &LabelBuddy::update_n_selected_docs);
  QObject::connect(dataset_menu_, &DatasetMenu::n_selected_docs_changed, this,
                   &LabelBuddy::set_n_selected_docs);
  QObject::connect(notebook_, &QTabWidget::currentChanged, this,
                   &LabelBuddy::update_current_doc_info);
  QObject::connect(annotator_, &Annotator::current_status_display_changed, this,
                   &LabelBuddy::update_current_doc_info);

  QObject::connect(notebook_, &QTabWidget::currentChanged, this,
                   &LabelBuddy::check_tab_focus);
}

void LabelBuddy::add_status_bar() {
  statusBar()->hide();
  statusBar()->setSizeGripEnabled(false);
  statusBar()->setStyleSheet(
      "QStatusBar::item {border: none; border-right: 1px solid black;} "
      "QStatusBar QLabel {margin-left: 1px; margin-right: 1px}");
  status_db_name_ = new QLabel();
  statusBar()->addWidget(status_db_name_);
  status_db_summary_ = new QLabel();
  statusBar()->addWidget(status_db_summary_);
  status_n_selected_docs_ = new QLabel();
  statusBar()->addWidget(status_n_selected_docs_);
  status_current_annotation_label_ = new QLabel();
  statusBar()->addWidget(status_current_annotation_label_);
  status_current_annotation_info_ = new QLabel();
  statusBar()->addWidget(status_current_annotation_info_);
  status_current_doc_info_ = new QLabel();
  statusBar()->addWidget(status_current_doc_info_);

  update_status_bar();
}

void LabelBuddy::go_to_annotations() { notebook_->setCurrentIndex(0); }

void LabelBuddy::check_tab_focus() {
  if (notebook_->currentIndex() == 0) {
    annotator_->setFocus();
  }
}

void LabelBuddy::add_menubar() {
  auto file_menu = menuBar()->addMenu("File");
  auto open_db_action = new QAction("Open database...", this);
  file_menu->addAction(open_db_action);
  open_db_action->setShortcut(QKeySequence::Open);
  opened_databases_submenu_ = new QMenu("Opened during this session", this);
  file_menu->addMenu(opened_databases_submenu_);
  QObject::connect(opened_databases_submenu_, &QMenu::triggered, this,
                   &LabelBuddy::open_database_from_action);
  QObject::connect(&database_catalog_, &DatabaseCatalog::new_database_opened,
                   this, &LabelBuddy::add_connection_to_menu);
  // we connect to new_database_opened after catalog construction so the temp db
  // only shows up in the if we explicitly ask to open it and fill it with
  // example docs, which emits `temporary_database_filled`.
  QObject::connect(&database_catalog_,
                   &DatabaseCatalog::temporary_database_filled, this,
                   &LabelBuddy::add_connection_to_menu);
  auto temp_db_action = new QAction("Demo", this);
  file_menu->addAction(temp_db_action);
  auto quit_action = new QAction("Quit", this);
  file_menu->addAction(quit_action);
  quit_action->setShortcut(QKeySequence::Quit);
  QObject::connect(open_db_action, &QAction::triggered, this,
                   &LabelBuddy::choose_and_open_database);
  QObject::connect(temp_db_action, &QAction::triggered, this,
                   &LabelBuddy::open_temp_database);
  QObject::connect(quit_action, &QAction::triggered, this, &LabelBuddy::close);

  auto preferences_menu = menuBar()->addMenu("Preferences");
  QSettings settings("labelbuddy", "labelbuddy");
  auto set_use_bold_action =
      new QAction("Show selected annotation in bold font", this);
  set_use_bold_action->setCheckable(true);
  set_use_bold_action->setChecked(
      settings.value(bf_setting_key_, bf_default_).toBool());
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
  auto open_keybindings_doc_action = new QAction("Keyboard shortcuts", this);
  help_menu->addAction(open_keybindings_doc_action);
  QObject::connect(show_about_action, &QAction::triggered, this,
                   &LabelBuddy::show_about_info);
  QObject::connect(open_doc_action, &QAction::triggered, this,
                   &LabelBuddy::open_documentation_url);
  QObject::connect(open_keybindings_doc_action, &QAction::triggered, this,
                   &LabelBuddy::open_documentation_url_at_keybindings_section);
}

void LabelBuddy::add_connection_to_menu(const QString& db_name) {
  assert(QSqlDatabase::contains(db_name));
  auto action = opened_databases_submenu_->addAction(
      database_name_display(db_name, true, false));
  action->setData(db_name);
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
    auto db_name = database_name_display(
        database_catalog_.get_current_database(), false, false);
    setWindowTitle(QString("labelbuddy — %0").arg(db_name));
  }
}

void LabelBuddy::set_use_bold_font(bool use_bold) {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue(bf_setting_key_, use_bold);
  annotator_->set_use_bold_font(use_bold);
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
  annotator_->set_font(new_font);
}

void LabelBuddy::reset_font() {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.remove(font_setting_key_);
  annotator_->set_font(QFont());
}

void LabelBuddy::update_status_bar() {
  auto db_name = database_name_display(database_catalog_.get_current_database(),
                                       false, false);
  status_db_name_->setText(db_name);
  auto n_docs = doc_model_->total_n_docs();
  auto n_labelled = doc_model_->total_n_docs(DocListModel::DocFilter::labelled);
  assert(0 <= n_labelled && n_labelled <= n_docs);
  status_db_summary_->setText(QString("%0 document%1 (%2 labelled)")
                                  .arg(n_docs)
                                  .arg(n_docs != 1 ? "s" : "")
                                  .arg(n_labelled));
  update_n_selected_docs();
  update_current_doc_info();
}

void LabelBuddy::update_n_selected_docs() {
  if (notebook_->currentIndex() != 1) {
    status_n_selected_docs_->hide();
    return;
  }
  set_n_selected_docs(dataset_menu_->n_selected_docs());
  status_n_selected_docs_->show();
}

void LabelBuddy::set_n_selected_docs(int n_docs) {
  status_n_selected_docs_->setText(
      QString("%0 doc%1 selected").arg(n_docs).arg(n_docs != 1 ? "s" : ""));
}

void LabelBuddy::update_current_doc_info() {
  if ((notebook_->currentIndex() != 0) ||
      (!annotations_model_->is_positioned_on_valid_doc())) {
    status_current_doc_info_->hide();
    status_current_annotation_info_->hide();
    status_current_annotation_label_->hide();
    return;
  }
  auto status = annotator_->current_status_info();
  status_current_doc_info_->setText(status.doc_info);
  status_current_doc_info_->show();
  if (status.annotation_label == "") {
    status_current_annotation_info_->hide();
    status_current_annotation_label_->hide();
    return;
  }
  status_current_annotation_info_->setText(status.annotation_info);
  status_current_annotation_label_->setText(
      QString("<b>%0</b>").arg(status.annotation_label.toHtmlEscaped()));
  status_current_annotation_info_->show();
  status_current_annotation_label_->show();
}

void LabelBuddy::open_database(const QString& db_name) {
  store_notebook_page();
  bool opened = database_catalog_.open_database(db_name);
  if (opened) {
    display_notebook();
    emit database_changed(db_name);
  } else {
    warn_failed_to_open_db(db_name);
  }
}

void LabelBuddy::choose_and_open_database() {
  QFileDialog dialog(this, "Open database");
  dialog.setOption(QFileDialog::DontUseNativeDialog);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setNameFilter("labelbuddy databases (*.labelbuddy *.lb *.sqlite3 "
                       "*.sqlite *.db);; All files (*)");
  dialog.setViewMode(QFileDialog::Detail);
  dialog.setOption(QFileDialog::HideNameFilterDetails, false);
  dialog.setOption(QFileDialog::DontUseCustomDirectoryIcons, true);
  dialog.setOption(QFileDialog::DontConfirmOverwrite, true);
  dialog.setDirectory(DatabaseCatalog::get_last_opened_directory());
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  if (!dialog.exec() || dialog.selectedFiles().isEmpty()) {
    return;
  }
  assert(dialog.selectedFiles().size() == 1);
  auto file_path = dialog.selectedFiles()[0];
  if (file_path != QString()) {
    open_database(file_path);
  }
}

void LabelBuddy::open_database_from_action(QAction* action) {
  auto db_name = action->data().toString();
  if (db_name != QString()) {
    open_database(db_name);
  }
}

void LabelBuddy::display_notebook() {
  if (notebook_owner_ == nullptr) {
    return;
  }
  notebook_->setCurrentIndex(
      database_catalog_.get_app_state_extra("notebook_page", 2).toInt());
  setCentralWidget(notebook_owner_.release());
  statusBar()->show();
  check_tab_focus();
}

void LabelBuddy::open_temp_database() {
  store_notebook_page();
  auto db_path = database_catalog_.open_temp_database();
  emit database_changed(db_path);
  display_notebook();
}

void LabelBuddy::store_notebook_page() {
  database_catalog_.set_app_state_extra("notebook_page",
                                        notebook_->currentIndex());
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
    resize(QSize(default_window_width_, default_window_height_));
  }
}

void LabelBuddy::init_annotator_settings() {
  QSettings settings("labelbuddy", "labelbuddy");
  annotator_->set_use_bold_font(
      settings.value(bf_setting_key_, bf_default_).toBool());
  if (settings.contains(font_setting_key_)) {
    annotator_->set_font(settings.value(font_setting_key_).value<QFont>());
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

void LabelBuddy::open_documentation_url_at_keybindings_section() {
  auto url = get_doc_url();
  url.setFragment("keybindings-summary");
  QDesktopServices::openUrl(url);
}
} // namespace labelbuddy
