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
  /// Update db path when current database changes
  void update_database_info();

signals:

  void documents_added();
  void labels_added();

private slots:
  void import_documents();
  void import_labels();
  void export_documents();
  void export_labels();

private:
  enum class DirRole {
    import_documents,
    import_labels,
    export_documents,
    export_labels
  };

  /// Find a directory from which to start a filedialog

  /// Depends on the kind of file to be opened and what is stored in the
  /// QSettings
  QString suggest_dir(DirRole role) const;

  /// Remember directory from which a file was selected
  void store_parent_dir(const QString& file_path, DirRole role);

  void init_checkbox_states();

  /// Warn user file extension unknown, ask if proceed with default format.

  /// Returns true if user asked to proceed. for Import operations this is not
  /// an option -- this function always returns false in that case.
  bool ask_confirm_unknown_extension(const QString& file_path,
                                     DatabaseCatalog::Action action,
                                     DatabaseCatalog::ItemKind kind);

  template <typename T>
  void report_result(const T& result, const QString& file_path);
  QString get_report_msg(const ImportDocsResult& result) const;
  QString get_report_msg(const ImportLabelsResult& result) const;
  QString get_report_msg(const ExportDocsResult& result) const;
  QString get_report_msg(const ExportLabelsResult& result) const;

  DatabaseCatalog* database_catalog_;
  QPushButton* import_docs_button_;
  QPushButton* import_labels_button_;
  QPushButton* export_docs_button_;
  QPushButton* export_labels_button_;
  QCheckBox* labelled_only_checkbox_;
  QCheckBox* include_text_checkbox_;
  QCheckBox* include_annotations_checkbox_;
  QLabel* db_path_line_;
};
} // namespace labelbuddy

#endif
