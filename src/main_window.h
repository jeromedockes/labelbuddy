#ifndef LABELBUDDY_MAIN_WINDOW_H
#define LABELBUDDY_MAIN_WINDOW_H

#include <memory>

#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
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

  /// Unless `startFromTempDb` is true, tries to open `databasePath` if it
  /// is not empty, otherwise the last opened database. If that fails, the tab
  /// widget is not shown; the welcome banner is shown instead. However the tab
  /// widget is still created (and managed by a `uniquePtr`) and the models are
  /// positioned on a valid database (the empty temp database, see
  /// `DatabaseCatalog`).
  ///
  /// If `startFromTempDb` is true, opens the temp database and inserts the
  /// example docs.
  ///
  /// The tab widget is set as the central widget the first time a database is
  /// successfully opened (either a SQLite file or asking explicitly for the
  /// temp database).
  LabelBuddy(QWidget* parent = nullptr, const QString& databasePath = QString(),
             bool startFromTempDb = false);
signals:

  /// User selected a different database through "Open" or "Demo"
  void databaseChanged(const QString& newDatabaseName);
  void aboutToClose();

public slots:

  /// focus the Annotate tab
  void goToAnnotations();

protected:
  void closeEvent(QCloseEvent* event) override;
  void showEvent(QShowEvent* event) override;

private slots:

  /// Open a file dialog to select and open a database

  /// We don't use a native dialog to allow selecting an existing or a new file.
  void chooseAndOpenDatabase();

  /// Previously opened database selecte from the menu bar
  // ("from_action" because signals and slots should not be overloaded)
  void openDatabaseFromAction(QAction* action);

  /// Open the temporary demo database

  /// emits `databaseChanged`
  void openTempDatabase();

  /// show the "about" message box
  void showAboutInfo();

  /// Open documentation in a web browser.

  /// If it is found in /usr/share/doc/labelbuddy or in the executable's parent
  /// directory the local file is opened, otherwise the online documentation
  void openDocumentationUrl();

  /// If it is found in /usr/share/doc/labelbuddy or in the executable's parent
  /// directory the local file is opened, otherwise the online documentation
  /// opens it at the "#keybindings-summary" anchor
  void openDocumentationUrlAtKeybindingsSection();

  void updateStatusBar();
  void updateWindowTitle();
  void setNSelectedDocs(int nDocs);
  void updateNSelectedDocs();
  void updateCurrentDocInfo();

  /// message box when the db given in command line failed to open

  /// displayed after the window is shown
  void warnFailedToOpenInitDb();

  /// set focus on text when opening the Annotate tab
  void checkTabFocus();
  void chooseFont();
  void resetFont();
  void setUseBoldFont(bool useBold);

private:
  DocListModel* docModel_;
  LabelListModel* labelModel_;
  AnnotationsModel* annotationsModel_;
  QTabWidget* notebook_;
  Annotator* annotator_;
  DatasetMenu* datasetMenu_;
  ImportExportMenu* importExportMenu_;
  DatabaseCatalog databaseCatalog_;

  // QLabels in the status bar
  QLabel* statusDbName_ = nullptr;
  QLabel* statusDbSummary_ = nullptr;
  QLabel* statusNSelectedDocs_ = nullptr;
  QLabel* statusCurrentDocInfo_ = nullptr;
  QLabel* statusCurrentAnnotationInfo_ = nullptr;
  // having the label in its own widget means it doesn't affect the rest if it
  // is written from right to left (eg in Arabic), has rich text, several lines,
  // etc.
  QLabel* statusCurrentAnnotationLabel_ = nullptr;

  QMenu* openedDatabasesSubmenu_ = nullptr;

  /// Open the given database and warn user if it failed

  /// emits `databaseChanged`
  void openDatabase(const QString& dbName);

  // store info about the db passed to constructor if it failed to open. Used to
  // display an error message after the main window is shown.
  // set to false once the error message has been shown.
  bool needWarnFailedToOpenInitDb_{};
  QString initDbPath_{};

  /// own the notebook when it is not set as central widget
  std::unique_ptr<QTabWidget> notebookOwner_ = nullptr;

  /// Sets the tab widget as central widget if not already the case.
  void displayNotebook();
  void storeNotebookPage();
  void addConnections();
  void addStatusBar();
  void addMenubar();
  void addConnectionToMenu(const QString& dbName);
  void setGeometry();
  /// Set stored fond and bold selected annotation option
  void initAnnotatorSettings();
  void addWelcomeLabel();

  /// Display a message box saying database could not be opened
  void warnFailedToOpenDb(const QString& databasePath);

  const QString bfSettingKey_{"LabelBuddy/selected_annotation_bold"};
  const bool bfDefault_{true};
  const QString fontSettingKey_{"LabelBuddy/annotator_font"};
  static constexpr int defaultWindowWidth_ = 600;
  static constexpr int defaultWindowHeight_ = 600;
};
} // namespace labelbuddy
#endif
