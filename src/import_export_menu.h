#ifndef LABELBUDDY_IMPORT_EXPORT_MENU_H
#define LABELBUDDY_IMPORT_EXPORT_MENU_H

#include <QCheckBox>
#include <QLineEdit>
#include <QWidget>

#include "database.h"

namespace labelbuddy {

class ImportExportMenu : public QWidget {

  Q_OBJECT

public:
  ImportExportMenu(DatabaseCatalog* catalog, QWidget* parent = nullptr);

public slots:

  void import_documents();
  void import_labels();
  void export_annotations();
  void update_database_info();

signals:

  void documents_added();
  void labels_added();

private:
  enum class DirRole { import_docs, import_labels, export_annotations };
  QString default_user_name();
  QString suggest_dir(DirRole role) const;
  void store_parent_dir(const QString& file_path, DirRole role);

  DatabaseCatalog* database_catalog;
  QLineEdit* annotator_name_edit;
  QCheckBox* labelled_only_checkbox;
  QCheckBox* include_docs_checkbox;
  QLineEdit* db_path_line;
};
} // namespace labelbuddy

#endif
