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

void ImportExportMenu::storeParentDir(const QString& filePath, DirRole role) {
  QString name;
  switch (role) {
  case DirRole::importDocuments:
    name = "import_documents";
    break;
  case DirRole::importLabels:
    name = "import_labels";
    break;
  case DirRole::exportDocuments:
    name = "export_documents";
    break;
  case DirRole::exportLabels:
    name = "export_labels";
    break;
  }
  auto settingName = QString("ImportExportMenu/directory_%0").arg(name);
  databaseCatalog_->setAppStateExtra(settingName, parentDirectory(filePath));
}

QString ImportExportMenu::suggestDir(ImportExportMenu::DirRole role) const {

  QList<QString> options;
  switch (role) {
  case DirRole::importDocuments:
    options = QList<QString>{"import_documents", "import_labels",
                             "export_documents", "export_labels"};
    break;
  case DirRole::importLabels:
    options = QList<QString>{"import_labels", "import_documents",
                             "export_labels", "export_documents"};
    break;
  case DirRole::exportDocuments:
    options = QList<QString>{"export_documents", "export_labels",
                             "import_documents", "import_labels"};
    break;
  case DirRole::exportLabels:
    options = QList<QString>{"export_labels", "export_documents",
                             "import_labels", "import_documents"};
    break;
  }
  for (const auto& opt : options) {
    auto settingName = QString("ImportExportMenu/directory_%0").arg(opt);
    auto settingValue =
        databaseCatalog_->getAppStateExtra(settingName, QString()).toString();
    if (settingValue != QString()) {
      return settingValue;
    }
  }
  return DatabaseCatalog::getLastOpenedDirectory();
}

void ImportExportMenu::updateDatabaseInfo() {
  dbPathLine_->setText(
      databaseNameDisplay(databaseCatalog_->getCurrentDatabase()));
  initCheckboxStates();
}

ImportExportMenu::ImportExportMenu(DatabaseCatalog* catalog, QWidget* parent)
    : QWidget(parent), databaseCatalog_{catalog} {
  auto layout = new QVBoxLayout();
  setLayout(layout);
  auto importFrame = new QGroupBox("Importing");
  layout->addWidget(importFrame);
  auto exportFrame = new QGroupBox("Exporting");
  layout->addWidget(exportFrame);
  auto importLayout = new QHBoxLayout();
  importFrame->setLayout(importLayout);
  auto exportLayout = new QGridLayout();
  exportFrame->setLayout(exportLayout);

  importDocsButton_ = new QPushButton("Import docs && annotations");
  importLayout->addWidget(importDocsButton_, Qt::AlignLeft);
  importDocsButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  importLabelsButton_ = new QPushButton("Import labels");
  importLayout->addWidget(importLabelsButton_, Qt::AlignLeft);
  importLabelsButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  importLayout->addStretch(1);

  labelledOnlyCheckbox_ = new QCheckBox("Export only annotated documents");
  exportLayout->addWidget(labelledOnlyCheckbox_, 0, 0, 1, 2);
  includeTextCheckbox_ = new QCheckBox("Include document text");
  exportLayout->addWidget(includeTextCheckbox_, 1, 0, 1, 2);
  includeAnnotationsCheckbox_ = new QCheckBox("Include annotations");
  exportLayout->addWidget(includeAnnotationsCheckbox_, 2, 0, 1, 2);

  auto exportButtonsFrame = new QFrame();
  exportLayout->addWidget(exportButtonsFrame, 3, 0, 1, 2);
  auto exportButtonsLayout = new QHBoxLayout();
  exportButtonsFrame->setLayout(exportButtonsLayout);
  exportButtonsLayout->setContentsMargins(0, 0, 0, 0);
  exportDocsButton_ = new QPushButton("Export docs && annotations");
  exportButtonsLayout->addWidget(exportDocsButton_);
  exportDocsButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  exportLabelsButton_ = new QPushButton("Export labels");
  exportButtonsLayout->addWidget(exportLabelsButton_);
  exportLabelsButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  exportButtonsLayout->addStretch(1);
  exportLayout->setColumnStretch(2, 1);

  auto dbPathFrame = new QFrame();
  layout->addWidget(dbPathFrame);
  auto dbPathLayout = new QVBoxLayout();
  dbPathFrame->setLayout(dbPathLayout);
  auto dbPathLabel = new QLabel("Current database path:");
  dbPathLayout->addWidget(dbPathLabel);
  dbPathLine_ = new QLabel();
  dbPathLayout->addWidget(dbPathLine_);
  dbPathLine_->setWordWrap(true);
  dbPathLine_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  dbPathLayout->addStretch(1);

  layout->addStretch(1);

  updateDatabaseInfo();

  QObject::connect(importDocsButton_, &QPushButton::clicked, this,
                   &ImportExportMenu::importDocuments);
  QObject::connect(importLabelsButton_, &QPushButton::clicked, this,
                   &ImportExportMenu::importLabels);
  QObject::connect(exportDocsButton_, &QPushButton::clicked, this,
                   &ImportExportMenu::exportDocuments);
  QObject::connect(exportLabelsButton_, &QPushButton::clicked, this,
                   &ImportExportMenu::exportLabels);
}

void ImportExportMenu::initCheckboxStates() {
  labelledOnlyCheckbox_->setChecked(
      databaseCatalog_->getAppStateExtra("export_labelled_only", 1).toInt());

  includeTextCheckbox_->setChecked(
      databaseCatalog_->getAppStateExtra("export_include_doc_text", 1).toInt());

  includeAnnotationsCheckbox_->setChecked(
      databaseCatalog_->getAppStateExtra("export_include_annotations", 1)
          .toInt());
}

bool ImportExportMenu::askConfirmUnknownExtension(
    const QString& filePath, DatabaseCatalog::Action action,
    DatabaseCatalog::ItemKind kind) {
  bool acceptDefault{action == DatabaseCatalog::Action::Export};
  auto msg = DatabaseCatalog::fileExtensionErrorMessage(filePath, action, kind,
                                                        acceptDefault);
  if (msg == QString()) {
    return true;
  }
  if (!acceptDefault) {
    QMessageBox::critical(this, "labelbuddy", msg, QMessageBox::Ok);
    return false;
  }
  auto answer = QMessageBox::warning(this, "labelbuddy", msg,
                                     QMessageBox::Ok | QMessageBox::Cancel);
  return (answer == QMessageBox::Ok);
}

template <typename T>
void ImportExportMenu::reportResult(const T& result, const QString& filePath) {
  if (result.errorCode != ErrorCode::NoError) {
    assert(result.errorMessage != "");
    QMessageBox::critical(
        this, "labelbuddy",
        QString("Error: file %0\n%1").arg(filePath).arg(result.errorMessage),
        QMessageBox::Ok);
  } else {
    QMessageBox::information(this, "labelbuddy", getReportMsg(result),
                             QMessageBox::Ok);
  }
}

QString ImportExportMenu::getReportMsg(const ImportDocsResult& result) {
  return QString("Added %0 new document%1 and %2 new annotation%3")
      .arg(result.nDocs)
      .arg(result.nDocs != 1 ? "s" : "")
      .arg(result.nAnnotations)
      .arg(result.nAnnotations != 1 ? "s" : "");
}

QString ImportExportMenu::getReportMsg(const ImportLabelsResult& result) {
  return QString("Added %0 new label%1")
      .arg(result.nLabels)
      .arg(result.nLabels != 1 ? "s" : "");
}

QString ImportExportMenu::getReportMsg(const ExportDocsResult& result) {

  return QString("Exported %0 annotation%1 for %2 document%3")
      .arg(result.nAnnotations)
      .arg(result.nAnnotations == 1 ? "" : "s")
      .arg(result.nDocs)
      .arg(result.nDocs == 1 ? "" : "s");
}

QString ImportExportMenu::getReportMsg(const ExportLabelsResult& result) {
  return QString("Exported %0 label%1")
      .arg(result.nLabels)
      .arg(result.nLabels == 1 ? "" : "s");
}

void ImportExportMenu::importDocuments() {
  auto startDir = suggestDir(DirRole::importDocuments);
  auto filePath = QFileDialog::getOpenFileName(
      this, "Docs & annotations file", startDir,
      "labelbuddy documents (*.txt *.json *.jsonl);; Text files "
      "(*.txt);; JSON files (*.json);; JSONLines files (*.jsonl);; "
      "All files (*)");
  if (filePath == QString()) {
    return;
  }
  if (!askConfirmUnknownExtension(filePath, DatabaseCatalog::Action::Import,
                                  DatabaseCatalog::ItemKind::Document)) {
    return;
  }
  storeParentDir(filePath, DirRole::importDocuments);
  ImportDocsResult result;
  {
    QProgressDialog progress("Importing documents...", "Stop", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(progressDialogMinDurationMs_);
    result = databaseCatalog_->importDocuments(filePath, &progress);
  }
  emit documentsAdded();
  emit labelsAdded();
  reportResult(result, filePath);
}

void ImportExportMenu::importLabels() {
  auto startDir = suggestDir(DirRole::importLabels);
  auto filePath = QFileDialog::getOpenFileName(
      this, "Labels file", startDir,
      "labelbuddy labels (*.txt *.json *.jsonl);; "
      "JSON files (*.json);; JSONLines files (*.jsonl);; "
      "Text files (*.txt);; All files (*)");
  if (filePath == QString()) {
    return;
  }
  if (!askConfirmUnknownExtension(filePath, DatabaseCatalog::Action::Import,
                                  DatabaseCatalog::ItemKind::Label)) {
    return;
  }
  storeParentDir(filePath, DirRole::importLabels);
  auto result = databaseCatalog_->importLabels(filePath);
  emit labelsAdded();
  reportResult(result, filePath);
}

void ImportExportMenu::exportDocuments() {
  auto startDir = suggestDir(DirRole::exportDocuments);
  auto filePath = QFileDialog::getSaveFileName(
      this, "Docs & annotations file", startDir,
      "labelbuddy documents (*.json *.jsonl);;"
      " JSON files (*.json);; JSONLines files (*.jsonl);; "
      "All files (*)");
  if (filePath == QString()) {
    return;
  }
  if (!askConfirmUnknownExtension(filePath, DatabaseCatalog::Action::Export,
                                  DatabaseCatalog::ItemKind::Document)) {
    return;
  }
  storeParentDir(filePath, DirRole::exportDocuments);
  ExportDocsResult result;
  {
    QProgressDialog progress("Exporting documents...", "Stop", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(progressDialogMinDurationMs_);

    result = databaseCatalog_->exportDocuments(
        filePath, labelledOnlyCheckbox_->isChecked(),
        includeTextCheckbox_->isChecked(),
        includeAnnotationsCheckbox_->isChecked(), &progress);
  }
  databaseCatalog_->setAppStateExtra("export_labelled_only",
                                     labelledOnlyCheckbox_->isChecked());
  databaseCatalog_->setAppStateExtra("export_include_doc_text",
                                     includeTextCheckbox_->isChecked());
  databaseCatalog_->setAppStateExtra("export_include_annotations",
                                     includeAnnotationsCheckbox_->isChecked());
  reportResult(result, filePath);
}

void ImportExportMenu::exportLabels() {
  auto startDir = suggestDir(DirRole::exportLabels);
  auto filePath = QFileDialog::getSaveFileName(
      this, "Labels file", startDir,
      "labelbuddy labels (*.json *.jsonl);; JSON files (*.json);; "
      "JSONLines files (*.jsonl);; All files (*)");
  if (filePath == QString()) {
    return;
  }
  if (!askConfirmUnknownExtension(filePath, DatabaseCatalog::Action::Export,
                                  DatabaseCatalog::ItemKind::Label)) {
    return;
  }
  storeParentDir(filePath, DirRole::exportLabels);
  auto result = databaseCatalog_->exportLabels(filePath);
  reportResult(result, filePath);
}

} // namespace labelbuddy
