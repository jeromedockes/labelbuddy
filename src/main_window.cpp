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
#include <QObject>

#include "annotations_model.h"
#include "database.h"
#include "dataset_menu.h"
#include "import_export_menu.h"
#include "utils.h"

#include "main_window.h"

namespace labelbuddy {

void LabelBuddy::warnFailedToOpenDb(const QString& databasePath) {
  assert(!QSqlDatabase::contains(databasePath));
  QString dbMsg(databasePath != QString() ? QString(":\n%0").arg(databasePath)
                                          : "");
  QMessageBox::critical(this, "labelbuddy",
                        QString("Could not open database%0").arg(dbMsg),
                        QMessageBox::Ok);
}

LabelBuddy::LabelBuddy(QWidget* parent, const QString& databasePath,
                       bool startFromTempDb)
    : QMainWindow(parent) {

  notebookOwner_.reset(new QTabWidget());
  notebook_ = notebookOwner_.get();
  annotator_ = new Annotator();
  notebook_->addTab(annotator_, "&Annotate");
  datasetMenu_ = new DatasetMenu();
  notebook_->addTab(datasetMenu_, "&Labels && Documents");
  importExportMenu_ = new ImportExportMenu(&databaseCatalog_);
  notebook_->addTab(importExportMenu_, "&Import && Export");

  docModel_ = new DocListModel(this);
  docModel_->setDatabase(databaseCatalog_.getCurrentDatabase());
  labelModel_ = new LabelListModel(this);
  labelModel_->setDatabase(databaseCatalog_.getCurrentDatabase());
  annotationsModel_ = new AnnotationsModel(this);
  annotationsModel_->setDatabase(databaseCatalog_.getCurrentDatabase());
  datasetMenu_->setDocListModel(docModel_);
  datasetMenu_->setLabelListModel(labelModel_);
  annotator_->setAnnotationsModel(annotationsModel_);
  annotator_->setLabelListModel(labelModel_);

  addStatusBar();
  addMenubar();
  setGeometry();
  initAnnotatorSettings();
  addConnections();
  addWelcomeLabel();

  bool opened{};
  if (startFromTempDb) {
    databaseCatalog_.openTempDatabase();
    opened = true;
  } else {
    opened = databaseCatalog_.openDatabase(databasePath);
  }
  if (opened) {
    emit databaseChanged(databaseCatalog_.getCurrentDatabase());
    displayNotebook();
  } else if (databasePath != QString()) {
    needWarnFailedToOpenInitDb_ = true;
    initDbPath_ = databasePath;
  }
  updateWindowTitle();
  checkTabFocus();
}

void LabelBuddy::addConnections() {

  QObject::connect(datasetMenu_, &DatasetMenu::visitDocRequested,
                   annotationsModel_, &AnnotationsModel::visitDoc);
  QObject::connect(datasetMenu_, &DatasetMenu::visitDocRequested, this,
                   &LabelBuddy::goToAnnotations);

  QObject::connect(docModel_, &DocListModel::docsDeleted, annotationsModel_,
                   &AnnotationsModel::checkCurrentDoc);
  QObject::connect(labelModel_, &LabelListModel::labelsChanged, annotator_,
                   &Annotator::updateAnnotations);
  QObject::connect(labelModel_, &LabelListModel::labelsChanged, annotator_,
                   &Annotator::updateNavButtons);
  QObject::connect(labelModel_, &LabelListModel::labelsOrderChanged, annotator_,
                   &Annotator::updateAnnotations);
  QObject::connect(docModel_, &DocListModel::docsDeleted, annotator_,
                   &Annotator::updateNavButtons);
  QObject::connect(importExportMenu_, &ImportExportMenu::documentsAdded,
                   annotator_, &Annotator::updateNavButtons);

  QObject::connect(annotationsModel_, &AnnotationsModel::documentStatusChanged,
                   docModel_, &DocListModel::documentStatusChanged);
  QObject::connect(annotationsModel_, &AnnotationsModel::documentGainedLabel,
                   docModel_, &DocListModel::documentGainedLabel);
  QObject::connect(annotationsModel_, &AnnotationsModel::documentLostLabel,
                   docModel_, &DocListModel::documentLostLabel);

  QObject::connect(importExportMenu_, &ImportExportMenu::documentsAdded,
                   docModel_, &DocListModel::refreshCurrentQuery);
  QObject::connect(importExportMenu_, &ImportExportMenu::labelsAdded, docModel_,
                   &DocListModel::labelsChanged);
  QObject::connect(importExportMenu_, &ImportExportMenu::documentsAdded,
                   docModel_, &DocListModel::labelsChanged);
  QObject::connect(importExportMenu_, &ImportExportMenu::documentsAdded,
                   annotationsModel_, &AnnotationsModel::checkCurrentDoc);
  QObject::connect(importExportMenu_, &ImportExportMenu::documentsAdded,
                   annotator_, &Annotator::updateAnnotations);

  QObject::connect(importExportMenu_, &ImportExportMenu::labelsAdded,
                   labelModel_, &LabelListModel::refreshCurrentQuery);
  QObject::connect(importExportMenu_, &ImportExportMenu::labelsAdded,
                   annotator_, &Annotator::updateAnnotations);

  QObject::connect(this, &LabelBuddy::databaseChanged, annotator_,
                   &Annotator::resetSkipUpdatingNavButtons);
  QObject::connect(this, &LabelBuddy::databaseChanged, docModel_,
                   &DocListModel::setDatabase);
  QObject::connect(this, &LabelBuddy::databaseChanged, labelModel_,
                   &LabelListModel::setDatabase);
  QObject::connect(this, &LabelBuddy::databaseChanged, annotationsModel_,
                   &AnnotationsModel::setDatabase);
  QObject::connect(this, &LabelBuddy::databaseChanged, importExportMenu_,
                   &ImportExportMenu::updateDatabaseInfo);

  QObject::connect(this, &LabelBuddy::aboutToClose, datasetMenu_,
                   &DatasetMenu::storeState, Qt::DirectConnection);
  QObject::connect(this, &LabelBuddy::aboutToClose, annotator_,
                   &Annotator::storeState, Qt::DirectConnection);

  QObject::connect(this, &LabelBuddy::databaseChanged, this,
                   &LabelBuddy::updateStatusBar);
  QObject::connect(this, &LabelBuddy::databaseChanged, this,
                   &LabelBuddy::updateWindowTitle);
  QObject::connect(docModel_, &DocListModel::docsDeleted, this,
                   &LabelBuddy::updateStatusBar);
  QObject::connect(labelModel_, &LabelListModel::labelsDeleted, this,
                   &LabelBuddy::updateStatusBar);
  QObject::connect(importExportMenu_, &ImportExportMenu::documentsAdded, this,
                   &LabelBuddy::updateStatusBar);
  QObject::connect(annotationsModel_, &AnnotationsModel::documentStatusChanged,
                   this, &LabelBuddy::updateStatusBar);
  QObject::connect(notebook_, &QTabWidget::currentChanged, this,
                   &LabelBuddy::updateNSelectedDocs);
  QObject::connect(datasetMenu_, &DatasetMenu::nSelectedDocsChanged, this,
                   &LabelBuddy::setNSelectedDocs);
  QObject::connect(notebook_, &QTabWidget::currentChanged, this,
                   &LabelBuddy::updateCurrentDocInfo);
  QObject::connect(annotator_, &Annotator::currentStatusDisplayChanged, this,
                   &LabelBuddy::updateCurrentDocInfo);

  QObject::connect(notebook_, &QTabWidget::currentChanged, this,
                   &LabelBuddy::checkTabFocus);
}

void LabelBuddy::addStatusBar() {
  statusBar()->hide();
  statusBar()->setSizeGripEnabled(false);
  statusBar()->setStyleSheet(
      "QStatusBar::item {border: none; border-right: 1px solid black;} "
      "QStatusBar QLabel {margin-left: 1px; margin-right: 1px}");
  statusDbName_ = new QLabel();
  statusBar()->addWidget(statusDbName_);
  statusDbSummary_ = new QLabel();
  statusBar()->addWidget(statusDbSummary_);
  statusNSelectedDocs_ = new QLabel();
  statusBar()->addWidget(statusNSelectedDocs_);
  statusCurrentAnnotationLabel_ = new QLabel();
  statusBar()->addWidget(statusCurrentAnnotationLabel_);
  statusCurrentAnnotationInfo_ = new QLabel();
  statusBar()->addWidget(statusCurrentAnnotationInfo_);
  statusCurrentDocInfo_ = new QLabel();
  statusBar()->addWidget(statusCurrentDocInfo_);

  updateStatusBar();
}

void LabelBuddy::goToAnnotations() { notebook_->setCurrentIndex(0); }

void LabelBuddy::checkTabFocus() {
  if (notebook_->currentIndex() == 0) {
    annotator_->setFocus();
  }
}

void LabelBuddy::addMenubar() {
  auto fileMenu = menuBar()->addMenu("File");
  auto openDbAction = new QAction("Open database...", this);
  fileMenu->addAction(openDbAction);
  openDbAction->setShortcut(QKeySequence::Open);
  openedDatabasesSubmenu_ = new QMenu("Opened during this session", this);
  fileMenu->addMenu(openedDatabasesSubmenu_);
  QObject::connect(openedDatabasesSubmenu_, &QMenu::triggered, this,
                   &LabelBuddy::openDatabaseFromAction);
  QObject::connect(&databaseCatalog_, &DatabaseCatalog::newDatabaseOpened, this,
                   &LabelBuddy::addConnectionToMenu);
  // we connect to newDatabaseOpened after catalog construction so the temp db
  // only shows up in the if we explicitly ask to open it and fill it with
  // example docs, which emits `temporaryDatabaseFilled`.
  QObject::connect(&databaseCatalog_, &DatabaseCatalog::temporaryDatabaseFilled,
                   this, &LabelBuddy::addConnectionToMenu);
  auto tempDbAction = new QAction("Demo", this);
  fileMenu->addAction(tempDbAction);
  auto quitAction = new QAction("Quit", this);
  fileMenu->addAction(quitAction);
  quitAction->setShortcut(QKeySequence::Quit);
  QObject::connect(openDbAction, &QAction::triggered, this,
                   &LabelBuddy::chooseAndOpenDatabase);
  QObject::connect(tempDbAction, &QAction::triggered, this,
                   &LabelBuddy::openTempDatabase);
  QObject::connect(quitAction, &QAction::triggered, this, &LabelBuddy::close);

  auto preferencesMenu = menuBar()->addMenu("Preferences");
  QSettings settings("labelbuddy", "labelbuddy");
  auto setUseBoldAction =
      new QAction("Show selected annotation in bold font", this);
  setUseBoldAction->setCheckable(true);
  setUseBoldAction->setChecked(
      settings.value(bfSettingKey_, bfDefault_).toBool());
  preferencesMenu->addAction(setUseBoldAction);
  QObject::connect(setUseBoldAction, &QAction::triggered, this,
                   &LabelBuddy::setUseBoldFont);

  auto chooseFontAction = new QAction("Choose font", this);
  preferencesMenu->addAction(chooseFontAction);
  QObject::connect(chooseFontAction, &QAction::triggered, this,
                   &LabelBuddy::chooseFont);
  auto resetFontAction = new QAction("Reset font to default", this);
  preferencesMenu->addAction(resetFontAction);
  QObject::connect(resetFontAction, &QAction::triggered, this,
                   &LabelBuddy::resetFont);

  auto helpMenu = menuBar()->addMenu("Help");
  auto showAboutAction = new QAction("About", this);
  helpMenu->addAction(showAboutAction);
  auto openDocAction = new QAction("Documentation", this);
  helpMenu->addAction(openDocAction);
  openDocAction->setShortcut(QKeySequence::HelpContents);
  auto openKeybindingsDocAction = new QAction("Keyboard shortcuts", this);
  helpMenu->addAction(openKeybindingsDocAction);
  QObject::connect(showAboutAction, &QAction::triggered, this,
                   &LabelBuddy::showAboutInfo);
  QObject::connect(openDocAction, &QAction::triggered, this,
                   &LabelBuddy::openDocumentationUrl);
  QObject::connect(openKeybindingsDocAction, &QAction::triggered, this,
                   &LabelBuddy::openDocumentationUrlAtKeybindingsSection);
}

void LabelBuddy::addConnectionToMenu(const QString& dbName) {
  assert(QSqlDatabase::contains(dbName));
  auto action = openedDatabasesSubmenu_->addAction(
      databaseNameDisplay(dbName, true, false));
  action->setData(dbName);
}

void LabelBuddy::addWelcomeLabel() {
  auto welcomeLabel = new QLabel();
  setCentralWidget(welcomeLabel);
  welcomeLabel->setTextFormat(Qt::RichText);
  welcomeLabel->setText(getWelcomeMessage());
  welcomeLabel->setWordWrap(true);
  welcomeLabel->setIndent(welcomeLabel->fontMetrics().averageCharWidth());
  welcomeLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  welcomeLabel->setOpenExternalLinks(true);
}

void LabelBuddy::updateWindowTitle() {
  if (notebookOwner_ != nullptr) {
    setWindowTitle("labelbuddy");
  } else {
    auto dbName = databaseNameDisplay(databaseCatalog_.getCurrentDatabase(),
                                      false, false);
    setWindowTitle(QString("labelbuddy — %0").arg(dbName));
  }
}

void LabelBuddy::setUseBoldFont(bool useBold) {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue(bfSettingKey_, useBold);
  annotator_->setUseBoldFont(useBold);
}

void LabelBuddy::chooseFont() {
  QSettings settings("labelbuddy", "labelbuddy");
  bool ok{};
  QFont newFont;
  if (settings.contains(fontSettingKey_)) {
    newFont = QFontDialog::getFont(
        &ok, settings.value(fontSettingKey_).value<QFont>(), this);
  } else {
    newFont = QFontDialog::getFont(&ok, this);
  }
  if (!ok) {
    return;
  }
  settings.setValue(fontSettingKey_, newFont);
  annotator_->setFont(newFont);
}

void LabelBuddy::resetFont() {
  QSettings settings("labelbuddy", "labelbuddy");
  settings.remove(fontSettingKey_);
  annotator_->setFont(QFont());
}

void LabelBuddy::updateStatusBar() {
  auto dbName =
      databaseNameDisplay(databaseCatalog_.getCurrentDatabase(), false, false);
  statusDbName_->setText(dbName);
  auto nDocs = docModel_->totalNDocs();
  auto nLabelled = docModel_->totalNDocs(DocListModel::DocFilter::labelled);
  assert(0 <= nLabelled && nLabelled <= nDocs);
  statusDbSummary_->setText(QString("%0 document%1 (%2 labelled)")
                                .arg(nDocs)
                                .arg(nDocs != 1 ? "s" : "")
                                .arg(nLabelled));
  updateNSelectedDocs();
  updateCurrentDocInfo();
}

void LabelBuddy::updateNSelectedDocs() {
  if (notebook_->currentIndex() != 1) {
    statusNSelectedDocs_->hide();
    return;
  }
  setNSelectedDocs(datasetMenu_->nSelectedDocs());
  statusNSelectedDocs_->show();
}

void LabelBuddy::setNSelectedDocs(int nDocs) {
  statusNSelectedDocs_->setText(
      QString("%0 doc%1 selected").arg(nDocs).arg(nDocs != 1 ? "s" : ""));
}

void LabelBuddy::updateCurrentDocInfo() {
  if ((notebook_->currentIndex() != 0) ||
      (!annotationsModel_->isPositionedOnValidDoc())) {
    statusCurrentDocInfo_->hide();
    statusCurrentAnnotationInfo_->hide();
    statusCurrentAnnotationLabel_->hide();
    return;
  }
  auto status = annotator_->currentStatusInfo();
  statusCurrentDocInfo_->setText(status.docInfo);
  statusCurrentDocInfo_->show();
  if (status.annotationLabel == "") {
    statusCurrentAnnotationInfo_->hide();
    statusCurrentAnnotationLabel_->hide();
    return;
  }
  statusCurrentAnnotationInfo_->setText(status.annotationInfo);
  statusCurrentAnnotationLabel_->setText(
      QString("<b>%0</b>").arg(status.annotationLabel.toHtmlEscaped()));
  statusCurrentAnnotationInfo_->show();
  statusCurrentAnnotationLabel_->show();
}

void LabelBuddy::openDatabase(const QString& dbName) {
  storeNotebookPage();
  bool opened = databaseCatalog_.openDatabase(dbName);
  if (opened) {
    displayNotebook();
    emit databaseChanged(dbName);
  } else {
    warnFailedToOpenDb(dbName);
  }
}

void LabelBuddy::chooseAndOpenDatabase() {
  QFileDialog dialog(this, "Open database");
  dialog.setOption(QFileDialog::DontUseNativeDialog);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setNameFilter("labelbuddy databases (*.labelbuddy *.lb *.sqlite3 "
                       "*.sqlite *.db);; All files (*)");
  dialog.setViewMode(QFileDialog::Detail);
  dialog.setOption(QFileDialog::HideNameFilterDetails, false);
  dialog.setOption(QFileDialog::DontUseCustomDirectoryIcons, true);
  dialog.setOption(QFileDialog::DontConfirmOverwrite, true);
  dialog.setDirectory(DatabaseCatalog::getLastOpenedDirectory());
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  if (!dialog.exec() || dialog.selectedFiles().isEmpty()) {
    return;
  }
  assert(dialog.selectedFiles().size() == 1);
  auto filePath = dialog.selectedFiles()[0];
  if (filePath != QString()) {
    openDatabase(filePath);
  }
}

void LabelBuddy::openDatabaseFromAction(QAction* action) {
  auto dbName = action->data().toString();
  if (dbName != QString()) {
    openDatabase(dbName);
  }
}

void LabelBuddy::displayNotebook() {
  if (notebookOwner_ == nullptr) {
    return;
  }
  notebook_->setCurrentIndex(
      databaseCatalog_.getAppStateExtra("notebook_page", 2).toInt());
  setCentralWidget(notebookOwner_.release());
  statusBar()->show();
  checkTabFocus();
}

void LabelBuddy::openTempDatabase() {
  storeNotebookPage();
  auto dbPath = databaseCatalog_.openTempDatabase();
  emit databaseChanged(dbPath);
  displayNotebook();
}

void LabelBuddy::storeNotebookPage() {
  databaseCatalog_.setAppStateExtra("notebook_page", notebook_->currentIndex());
}

void LabelBuddy::showEvent(QShowEvent* event) {
  QMainWindow::showEvent(event);
  if (needWarnFailedToOpenInitDb_) {
    // use a timer otherwise the main window is only drawn after the warning is
    // closed
    QTimer::singleShot(0, this, &LabelBuddy::warnFailedToOpenInitDb);
  }
}

void LabelBuddy::warnFailedToOpenInitDb() {
  warnFailedToOpenDb(initDbPath_);
  needWarnFailedToOpenInitDb_ = false;
  initDbPath_ = "";
}

void LabelBuddy::closeEvent(QCloseEvent* event) {
  storeNotebookPage();
  QSettings settings("labelbuddy", "labelbuddy");
  settings.setValue("LabelBuddy/geometry", saveGeometry());
  emit aboutToClose();
  QMainWindow::closeEvent(event);
}

void LabelBuddy::setGeometry() {
  QSettings settings("labelbuddy", "labelbuddy");
  if (settings.contains("LabelBuddy/geometry")) {
    restoreGeometry(settings.value("LabelBuddy/geometry").toByteArray());
  } else {
    resize(QSize(defaultWindowWidth_, defaultWindowHeight_));
  }
}

void LabelBuddy::initAnnotatorSettings() {
  QSettings settings("labelbuddy", "labelbuddy");
  annotator_->setUseBoldFont(
      settings.value(bfSettingKey_, bfDefault_).toBool());
  if (settings.contains(fontSettingKey_)) {
    annotator_->setFont(settings.value(fontSettingKey_).value<QFont>());
  }
}

void LabelBuddy::showAboutInfo() {
  auto message = QString("<p>labelbuddy version %0<br/>"
                         "<a href='https://jeromedockes.github.io/labelbuddy/'>"
                         "jeromedockes.github.io/labelbuddy/</a></p>")
                     .arg(getVersion());
  QMessageBox::information(this, "labelbuddy", message, QMessageBox::Ok);
}

void LabelBuddy::openDocumentationUrl() {
  QDesktopServices::openUrl(getDocUrl());
}

void LabelBuddy::openDocumentationUrlAtKeybindingsSection() {
  QDesktopServices::openUrl(getDocUrl("keybindings"));
}
} // namespace labelbuddy
