#ifndef LABELBUDDY_DOC_LIST_MODEL_H
#define LABELBUDDY_DOC_LIST_MODEL_H

#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QWidget>
#include <QProgressDialog>

/// \file
/// Interface providing information about documents in the database.

namespace labelbuddy {

/// Interface providing information about documents in the database.
class DocListModel : public QSqlQueryModel {

  Q_OBJECT

public:
  DocListModel(QObject* parent = nullptr);

  enum DocFilter { all, labelled, unlabelled };

  /// Number of (any, labelled or unlabelled) documents in the database
  int total_n_docs(DocFilter doc_filter = DocFilter::all);

  /// data for a document. `Roles::RowIdRole` can be used to get the doc's `id`
  QVariant data(const QModelIndex& index, int role) const override;

  /// Items in the second (hidden) column that contains id cannot be selected.
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  /// Delete specified docs, reset query and emit `docs_deleted`
  int delete_docs(const QModelIndexList& indices);

  /// Delete all docs, reset query and emit `docs_deleted`
  int delete_all_docs(QProgressDialog* progress=nullptr);

public slots:

  /// Change database
  void set_database(const QString& new_database_name);

  /// Set current query and reset model.
  void adjust_query(DocFilter doc_filter = DocFilter::all, int limit = 100,
                    int offset = 0);

  /// reset model
  void refresh_current_query();

signals:

  void docs_deleted();

private:
  QSqlQuery get_query() const;

  DocFilter doc_filter = DocFilter::all;
  int offset = 0;
  int limit = 100;
  QString database_name;
};
} // namespace labelbuddy
#endif // LABELBUDDY_DOC_LIST_MODEL_H
