#include <cassert>

#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QtGlobal>

#include "database.h"
#include "import_export_menu.h"
#include "utils.h"

namespace labelbuddy {

QString ImportExportMenu::default_user_name() {
  auto name = QString::fromUtf8(qgetenv("USER"));
  if (name == QString()) {
    name = QString::fromUtf8(qgetenv("USERNAME"));
  }
  return database_catalog_->get_app_state_extra("approver_name", name)
      .toString();
}

void ImportExportMenu::store_parent_dir(const QString& file_path,
                                        DirRole role) {
  QString name;
  switch (role) {
  case DirRole::import_documents:
    name = "import_documents";
    break;
  case DirRole::import_labels:
    name = "import_labels";
    break;
  case DirRole::export_documents:
    name = "export_documents";
    break;
  case DirRole::export_labels:
    name = "export_labels";
    break;
  }
  auto setting_name = QString("ImportExportMenu/directory_%0").arg(name);
  database_catalog_->set_app_state_extra(
      setting_name, database_catalog_->parent_directory(file_path));
}

QString ImportExportMenu::suggest_dir(ImportExportMenu::DirRole role) const {

  QList<QString> options;
  switch (role) {
  case DirRole::import_documents:
    options = QList<QString>{"import_documents", "import_labels",
                             "export_documents", "export_labels"};
    break;
  case DirRole::import_labels:
    options = QList<QString>{"import_labels", "import_documents",
                             "export_labels", "export_documents"};
    break;
  case DirRole::export_documents:
    options = QList<QString>{"export_documents", "export_labels",
                             "import_documents", "import_labels"};
    break;
  case DirRole::export_labels:
    options = QList<QString>{"export_labels", "export_documents",
                             "import_labels", "import_documents"};
    break;
  }
  for (const auto& opt : options) {
    auto setting_name = QString("ImportExportMenu/directory_%0").arg(opt);
    auto setting_value =
        database_catalog_->get_app_state_extra(setting_name, QString())
            .toString();
    if (setting_value != QString()) {
      return setting_value;
    }
  }
  return database_catalog_->get_last_opened_directory();
}

void ImportExportMenu::update_database_info() {
  annotator_name_edit_->setText(default_user_name());
  db_path_line_->setText(
      database_name_display(database_catalog_->get_current_database()));
  init_checkbox_states();
}

ImportExportMenu::ImportExportMenu(DatabaseCatalog* catalog, QWidget* parent)
    : QWidget(parent), database_catalog_{catalog} {
  auto layout = new QVBoxLayout();
  setLayout(layout);
  auto import_frame = new QGroupBox("Importing");
  layout->addWidget(import_frame);
  auto export_frame = new QGroupBox("Exporting");
  layout->addWidget(export_frame);
  auto import_layout = new QHBoxLayout();
  import_frame->setLayout(import_layout);
  auto export_layout = new QGridLayout();
  export_frame->setLayout(export_layout);

  import_docs_button_ = new QPushButton("Import docs && annotations");
  import_layout->addWidget(import_docs_button_, Qt::AlignLeft);
  import_docs_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  import_labels_button_ = new QPushButton("Import labels");
  import_layout->addWidget(import_labels_button_, Qt::AlignLeft);
  import_labels_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  import_layout->addStretch(1);

  labelled_only_checkbox_ = new QCheckBox("Export only annotated documents");
  export_layout->addWidget(labelled_only_checkbox_, 0, 0, 1, 2);
  include_text_checkbox_ = new QCheckBox("Include document text");
  export_layout->addWidget(include_text_checkbox_, 1, 0, 1, 2);
  include_annotations_checkbox_ = new QCheckBox("Include annotations");
  export_layout->addWidget(include_annotations_checkbox_, 2, 0, 1, 2);

  auto annotator_name_label = new QLabel("A&nnotation approver (optional): ");
  export_layout->addWidget(annotator_name_label, 3, 0, 1, 1);
  annotator_name_edit_ = new QLineEdit();
  export_layout->addWidget(annotator_name_edit_, 3, 1, 1, 1);
  annotator_name_label->setBuddy(annotator_name_edit_);
  annotator_name_edit_->setFixedWidth(annotator_name_label->sizeHint().width());
  auto export_buttons_frame = new QFrame();
  export_layout->addWidget(export_buttons_frame, 4, 0, 1, 2);
  auto export_buttons_layout = new QHBoxLayout();
  export_buttons_frame->setLayout(export_buttons_layout);
  export_buttons_layout->setContentsMargins(0, 0, 0, 0);
  export_docs_button_ = new QPushButton("Export docs && annotations");
  export_buttons_layout->addWidget(export_docs_button_);
  export_docs_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  export_labels_button_ = new QPushButton("Export labels");
  export_buttons_layout->addWidget(export_labels_button_);
  export_labels_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  export_buttons_layout->addStretch(1);
  export_layout->setColumnStretch(2, 1);

  auto db_path_frame = new QFrame();
  layout->addWidget(db_path_frame);
  auto db_path_layout = new QVBoxLayout();
  db_path_frame->setLayout(db_path_layout);
  auto db_path_label = new QLabel("Current database path:");
  db_path_layout->addWidget(db_path_label);
  db_path_line_ = new QLabel();
  db_path_layout->addWidget(db_path_line_);
  db_path_line_->setWordWrap(true);
  db_path_line_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  db_path_layout->addStretch(1);

  layout->addStretch(1);

  update_database_info();

  QObject::connect(import_docs_button_, &QPushButton::clicked, this,
                   &ImportExportMenu::import_documents);
  QObject::connect(import_labels_button_, &QPushButton::clicked, this,
                   &ImportExportMenu::import_labels);
  QObject::connect(export_docs_button_, &QPushButton::clicked, this,
                   &ImportExportMenu::export_documents);
  QObject::connect(export_labels_button_, &QPushButton::clicked, this,
                   &ImportExportMenu::export_labels);
}

void ImportExportMenu::init_checkbox_states() {
  labelled_only_checkbox_->setChecked(
      database_catalog_->get_app_state_extra("export_labelled_only", 1).toInt());

  include_text_checkbox_->setChecked(
      database_catalog_->get_app_state_extra("export_include_doc_text", 1)
          .toInt());

  include_annotations_checkbox_->setChecked(
      database_catalog_->get_app_state_extra("export_include_annotations", 1)
          .toInt());
}

bool ImportExportMenu::ask_confirm_unknown_extension(
    const QString& file_path, DatabaseCatalog::Action action,
    DatabaseCatalog::ItemKind kind) {
  bool accept_default{action == DatabaseCatalog::Action::Export};
  auto msg = database_catalog_->file_extension_error_message(
      file_path, action, kind, accept_default);
  if (msg == QString()) {
    return true;
  }
  if (!accept_default) {
    QMessageBox::critical(this, "labelbuddy", msg, QMessageBox::Ok);
    return false;
  }
  auto answer = QMessageBox::warning(this, "labelbuddy", msg,
                                     QMessageBox::Ok | QMessageBox::Cancel);
  if (answer == QMessageBox::Ok) {
    return true;
  }
  return false;
}

template <typename T>
void ImportExportMenu::report_result(const T& result,
                                     const QString& file_path) {
  if (result.error_code != ErrorCode::NoError) {
    assert(result.error_message != "");
    QMessageBox::critical(
        this, "labelbuddy",
        QString("Error: file %0\n%1").arg(file_path).arg(result.error_message),
        QMessageBox::Ok);
  } else {
    QMessageBox::information(this, "labelbuddy", get_report_msg(result),
                             QMessageBox::Ok);
  }
}

QString ImportExportMenu::get_report_msg(const ImportDocsResult& result) const {
  return QString("Added %0 new document%1 and %2 new annotation%3")
      .arg(result.n_docs)
      .arg(result.n_docs != 1 ? "s" : "")
      .arg(result.n_annotations)
      .arg(result.n_annotations != 1 ? "s" : "");
}

QString
ImportExportMenu::get_report_msg(const ImportLabelsResult& result) const {
  return QString("Added %0 new label%1")
      .arg(result.n_labels)
      .arg(result.n_labels != 1 ? "s" : "");
}

QString ImportExportMenu::get_report_msg(const ExportDocsResult& result) const {

  return QString("Exported %0 annotation%1 for %2 document%3")
      .arg(result.n_annotations)
      .arg(result.n_annotations == 1 ? "" : "s")
      .arg(result.n_docs)
      .arg(result.n_docs == 1 ? "" : "s");
}

QString
ImportExportMenu::get_report_msg(const ExportLabelsResult& result) const {
  return QString("Exported %0 label%1")
      .arg(result.n_labels)
      .arg(result.n_labels == 1 ? "" : "s");
}

void ImportExportMenu::import_documents() {
  auto start_dir = suggest_dir(DirRole::import_documents);
  auto file_path = QFileDialog::getOpenFileName(
      this, "Docs & annotations file", start_dir,
      "labelbuddy documents (*.txt *.json *.jsonl);; Text files "
      "(*.txt);; JSON files (*.json);; JSONLines files (*.jsonl);; "
      "All files (*)");
  if (file_path == QString()) {
    return;
  }
  if (!ask_confirm_unknown_extension(file_path, DatabaseCatalog::Action::Import,
                                     DatabaseCatalog::ItemKind::Document)) {
    return;
  }
  store_parent_dir(file_path, DirRole::import_documents);
  ImportDocsResult result;
  {
    QProgressDialog progress("Importing documents...", "Stop", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(2000);
    result = database_catalog_->import_documents(file_path, &progress);
  }
  emit documents_added();
  emit labels_added();
  report_result(result, file_path);
}

void ImportExportMenu::import_labels() {
  auto start_dir = suggest_dir(DirRole::import_labels);
  auto file_path = QFileDialog::getOpenFileName(
      this, "Labels file", start_dir,
      "labelbuddy labels (*.txt *.json *.jsonl);; "
      "JSON files (*.json);; JSONLines files (*.jsonl);; "
      "Text files (*.txt);; All files (*)");
  if (file_path == QString()) {
    return;
  }
  if (!ask_confirm_unknown_extension(file_path, DatabaseCatalog::Action::Import,
                                     DatabaseCatalog::ItemKind::Label)) {
    return;
  }
  store_parent_dir(file_path, DirRole::import_labels);
  auto result = database_catalog_->import_labels(file_path);
  emit labels_added();
  report_result(result, file_path);
}

void ImportExportMenu::export_documents() {
  auto start_dir = suggest_dir(DirRole::export_documents);
  auto file_path = QFileDialog::getSaveFileName(
      this, "Docs & annotations file", start_dir,
      "labelbuddy documents (*.json *.jsonl);;"
      " JSON files (*.json);; JSONLines files (*.jsonl);; "
      "All files (*)");
  if (file_path == QString()) {
    return;
  }
  if (!ask_confirm_unknown_extension(file_path, DatabaseCatalog::Action::Export,
                                     DatabaseCatalog::ItemKind::Document)) {
    return;
  }
  store_parent_dir(file_path, DirRole::export_documents);
  database_catalog_->set_app_state_extra("approver_name",
                                        annotator_name_edit_->text());
  ExportDocsResult result;
  {
    QProgressDialog progress("Exporting documents...", "Stop", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(2000);

    result = database_catalog_->export_documents(
        file_path, labelled_only_checkbox_->isChecked(),
        include_text_checkbox_->isChecked(),
        include_annotations_checkbox_->isChecked(), annotator_name_edit_->text(),
        &progress);
  }
  database_catalog_->set_app_state_extra("export_labelled_only",
                                        labelled_only_checkbox_->isChecked());
  database_catalog_->set_app_state_extra("export_include_doc_text",
                                        include_text_checkbox_->isChecked());
  database_catalog_->set_app_state_extra(
      "export_include_annotations", include_annotations_checkbox_->isChecked());
  report_result(result, file_path);
}

void ImportExportMenu::export_labels() {
  auto start_dir = suggest_dir(DirRole::export_labels);
  auto file_path = QFileDialog::getSaveFileName(
      this, "Labels file", start_dir,
      "labelbuddy labels (*.json *.jsonl);; JSON files (*.json);; "
      "JSONLines files (*.jsonl);; All files (*)");
  if (file_path == QString()) {
    return;
  }
  if (!ask_confirm_unknown_extension(file_path, DatabaseCatalog::Action::Export,
                                     DatabaseCatalog::ItemKind::Label)) {
    return;
  }
  store_parent_dir(file_path, DirRole::export_labels);
  auto result = database_catalog_->export_labels(file_path);
  report_result(result, file_path);
}

} // namespace labelbuddy
