#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QtGlobal>

#include "database.h"
#include "import_export_menu.h"

namespace labelbuddy {

QString ImportExportMenu::default_user_name() {
  auto name = QString::fromUtf8(qgetenv("USER"));
  if (name == QString()) {
    name = QString::fromUtf8(qgetenv("USERNAME"));
  }
  return database_catalog->get_app_state_extra("approver_name", name)
      .toString();
}

void ImportExportMenu::store_parent_dir(const QString& file_path,
                                        DirRole role) {
  QString name;
  switch (role) {
  case DirRole::import_docs:
    name = "import_docs";
    break;
  case DirRole::import_labels:
    name = "import_labels";
    break;
  case DirRole::export_annotations:
    name = "export_annotations";
    break;
  }
  auto setting_name = QString("ImportExportMenu/directory_%0").arg(name);
  database_catalog->set_app_state_extra(
      setting_name, database_catalog->parent_directory(file_path));
}

QString ImportExportMenu::suggest_dir(ImportExportMenu::DirRole role) const {

  QList<QString> options;
  switch (role) {
  case DirRole::import_docs:
    options =
        QList<QString>{"import_docs", "import_labels", "export_annotations"};
    break;
  case DirRole::import_labels:
    options =
        QList<QString>{"import_labels", "import_docs", "export_annotations"};
    break;
  case DirRole::export_annotations:
    options =
        QList<QString>{"export_annotations", "import_labels", "import_docs"};
    break;
  }
  for (const auto& opt : options) {
    auto setting_name = QString("ImportExportMenu/directory_%0").arg(opt);
    auto setting_value =
        database_catalog->get_app_state_extra(setting_name, QString())
            .toString();
    if (setting_value != QString()) {
      return setting_value;
    }
  }
  return database_catalog->get_last_opened_directory();
}

void ImportExportMenu::update_database_info() {
  annotator_name_edit->setText(default_user_name());
  db_path_line->setText(database_catalog->get_current_database());
}

ImportExportMenu::ImportExportMenu(DatabaseCatalog* catalog, QWidget* parent)
    : QWidget(parent), database_catalog{catalog} {
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

  auto import_docs_button = new QPushButton("Import docs && annotations");
  import_layout->addWidget(import_docs_button, Qt::AlignLeft);
  import_docs_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  auto import_labels_button = new QPushButton("Import labels");
  import_layout->addWidget(import_labels_button, Qt::AlignLeft);
  import_labels_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  import_layout->addStretch(1);

  labelled_only_checkbox = new QCheckBox("Export only annotated documents");
  labelled_only_checkbox->setChecked(
      database_catalog->get_app_state_extra("export_labelled_only", 1).toInt());
  export_layout->addWidget(labelled_only_checkbox, 0, 0, 1, 2);

  include_docs_checkbox =
      new QCheckBox("Include document text with exported annotations");
  include_docs_checkbox->setChecked(
      database_catalog->get_app_state_extra("export_include_doc_text", 1)
          .toInt());
  export_layout->addWidget(include_docs_checkbox, 1, 0, 1, 2);

  auto annotator_name_label = new QLabel("Annotation approver (optional): ");
  export_layout->addWidget(annotator_name_label, 2, 0, 1, 1);
  annotator_name_edit = new QLineEdit();
  export_layout->addWidget(annotator_name_edit, 2, 1, 1, 1);
  annotator_name_edit->setFixedWidth(annotator_name_label->sizeHint().width());
  auto export_button = new QPushButton("Export");
  export_layout->addWidget(export_button, 3, 0, 1, 1);
  export_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  export_layout->setColumnStretch(2, 1);

  auto db_path_frame = new QFrame();
  layout->addWidget(db_path_frame);
  auto db_path_layout = new QHBoxLayout();
  db_path_frame->setLayout(db_path_layout);
  auto db_path_label = new QLabel("Current database path:");
  db_path_layout->addWidget(db_path_label);
  db_path_line = new QLabel();
  db_path_layout->addWidget(db_path_line);
  db_path_line->setTextInteractionFlags(Qt::TextSelectableByMouse);
  db_path_layout->addStretch(1);

  layout->addStretch(1);

  update_database_info();

  QObject::connect(import_docs_button, &QPushButton::clicked, this,
                   &ImportExportMenu::import_documents);
  QObject::connect(import_labels_button, &QPushButton::clicked, this,
                   &ImportExportMenu::import_labels);
  QObject::connect(export_button, &QPushButton::clicked, this,
                   &ImportExportMenu::export_annotations);
}

void ImportExportMenu::import_documents() {
  auto start_dir = suggest_dir(DirRole::import_docs);
  auto file_path = QFileDialog::getOpenFileName(
      this, "Documents file", start_dir,
      "labelbuddy documents (*.txt *.json *.jsonl *.xml);; Text files "
      "(*.txt);; JSON files (*.json);; JSON Lines files (*.jsonl);; XML files "
      "(*.xml);; All files (*)");
  if (file_path == QString()) {
    return;
  }
  store_parent_dir(file_path, DirRole::import_docs);
  auto n_added = database_catalog->import_documents(file_path, this);
  emit documents_added();
  emit labels_added();
  QMessageBox::information(
      this, "labelbuddy",
      QString("Added %0 new document%1 and %2 new annotation%3")
          .arg(n_added[0])
          .arg(n_added[0] != 1 ? "s" : "")
          .arg(n_added[1])
          .arg(n_added[1] != 1 ? "s" : ""),
      QMessageBox::Ok);
}

void ImportExportMenu::import_labels() {
  auto start_dir = suggest_dir(DirRole::import_labels);
  auto file_path = QFileDialog::getOpenFileName(
      this, "Labels file", start_dir,
      "labelbuddy labels (*.txt *.json);; JSON files (*.json);; Text files "
      "(*.txt);; All files (*)");
  if (file_path == QString()) {
    return;
  }
  store_parent_dir(file_path, DirRole::import_labels);
  int n_added = database_catalog->import_labels(file_path);
  emit labels_added();
  QMessageBox::information(
      this, "labelbuddy",
      QString("Added %0 new label%1").arg(n_added).arg(n_added != 1 ? "s" : ""),
      QMessageBox::Ok);
}

void ImportExportMenu::export_annotations() {
  auto start_dir = suggest_dir(DirRole::export_annotations);
  auto file_path = QFileDialog::getSaveFileName(
      this, "Annotations file", start_dir,
      "JSON files (*.json);; JSON Lines files (*.jsonl);; XML files (*.xml);; "
      "All files (*)");
  if (file_path == QString()) {
    return;
  }
  store_parent_dir(file_path, DirRole::export_annotations);
  database_catalog->set_app_state_extra("approver_name",
                                        annotator_name_edit->text());
  auto n_exported = database_catalog->export_annotations(
      file_path, labelled_only_checkbox->isChecked(),
      include_docs_checkbox->isChecked(), annotator_name_edit->text(), this);
  database_catalog->set_app_state_extra("export_labelled_only",
                                        labelled_only_checkbox->isChecked());
  database_catalog->set_app_state_extra("export_include_doc_text",
                                        include_docs_checkbox->isChecked());
  QMessageBox::information(this, "labelbuddy",
                           QString("Exported %0 annotation%1 for %2 document%3")
                               .arg(n_exported[1])
                               .arg(n_exported[1] == 1 ? "" : "s")
                               .arg(n_exported[0])
                               .arg(n_exported[0] == 1 ? "" : "s"),
                           QMessageBox::Ok);
}

} // namespace labelbuddy
