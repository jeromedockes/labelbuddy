#ifndef LABELBUDDY_IMPORT_EXPORT_MENU_H
#define LABELBUDDY_IMPORT_EXPORT_MENU_H

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>

#include "database.h"

/// \file
/// Implementation of the Import / Export tab

namespace labelbuddy {

/// Implementation of the Import / Export tab
class ImportExportMenu : public QWidget {

  Q_OBJECT

public:
  explicit ImportExportMenu(DatabaseCatalog* catalog,
                            QWidget* parent = nullptr);

public slots:
  /// Update db path when current database changes
  void updateDatabaseInfo();

signals:

  void documentsAdded();
  void labelsAdded();

private slots:
  void importDocuments();
  void importLabels();
  void exportDocuments();
  void exportLabels();

private:
  enum class DirRole {
    importDocuments,
    importLabels,
    exportDocuments,
    exportLabels
  };

  static constexpr int progressDialogMinDurationMs_ = 2000;

  /// Find a directory from which to start a filedialog

  /// Depends on the kind of file to be opened and what is stored in the
  /// QSettings
  QString suggestDir(DirRole role) const;

  /// Remember directory from which a file was selected
  void storeParentDir(const QString& filePath, DirRole role);

  void initCheckboxStates();

  /// Warn user file extension unknown, ask if proceed with default format.

  /// Returns true if user asked to proceed. for Import operations this is not
  /// an option -- this function always returns false in that case.
  bool askConfirmUnknownExtension(const QString& filePath,
                                  DatabaseCatalog::Action action,
                                  DatabaseCatalog::ItemKind kind);

  template <typename T>
  void reportResult(const T& result, const QString& filePath);
  static QString getReportMsg(const ImportDocsResult& result);
  static QString getReportMsg(const ImportLabelsResult& result);
  static QString getReportMsg(const ExportDocsResult& result);
  static QString getReportMsg(const ExportLabelsResult& result);

  DatabaseCatalog* databaseCatalog_;
  QPushButton* importDocsButton_;
  QPushButton* importLabelsButton_;
  QPushButton* exportDocsButton_;
  QPushButton* exportLabelsButton_;
  QCheckBox* labelledOnlyCheckbox_;
  QCheckBox* includeTextCheckbox_;
  QCheckBox* includeAnnotationsCheckbox_;
  QLabel* dbPathLine_;
};
} // namespace labelbuddy

#endif
