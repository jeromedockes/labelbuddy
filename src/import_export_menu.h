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
  ImportExportMenu(DatabaseCatalog* catalog, QWidget* parent = nullptr);

public slots:
  /// Update db path and user name when current database changes
  void update_database_info();

signals:

  void documents_added();
  void labels_added();

private slots:
  void import_documents();
  void import_labels();
  void export_annotations();
  void export_labels();

private:
  enum class DirRole {
    import_docs,
    import_labels,
    export_annotations,
    export_labels
  };
  QString default_user_name();

  /// Find a directory from which to start a filedialog

  /// Depends on the kind of file to be opened and what is stored in the
  /// QSettings
  QString suggest_dir(DirRole role) const;

  /// Remember directory from which a file was selected
  void store_parent_dir(const QString& file_path, DirRole role);

  DatabaseCatalog* database_catalog;
  QLineEdit* annotator_name_edit;
  QCheckBox* labelled_only_checkbox;
  QCheckBox* include_docs_checkbox;
  QLabel* db_path_line;
};
} // namespace labelbuddy

#endif
