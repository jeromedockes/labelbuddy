#ifndef LABELBUDDY_DOC_LIST_MODEL_H
#define LABELBUDDY_DOC_LIST_MODEL_H

#include <QPair>
#include <QProgressDialog>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QWidget>

#include "user_roles.h"

/// \file
/// Interface providing information about documents in the database.

namespace labelbuddy {

/// Interface providing information about documents in the database.
class DocListModel : public QSqlQueryModel {

  Q_OBJECT

public:
  DocListModel(QObject* parent = nullptr);

  enum class DocFilter {
    all,
    labelled,
    unlabelled,
    hasGivenLabel,
    notHasGivenLabel
  };

  /// Number of documents in the database matching the filter params
  int totalNDocs(DocFilter docFilter = DocFilter::all, int filterLabelId = -1);

  /// data for a document. `Roles::RowIdRole` can be used to get the doc's `id`
  QVariant data(const QModelIndex& index, int role) const override;

  /// Items in the second (hidden) column that contains id cannot be selected.
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  /// label names in the database sorted by id, used to filter docs
  QList<QPair<QString, int>> getLabelNames() const;

  /// Delete specified docs, reset query and emit `docsDeleted`
  int deleteDocs(const QModelIndexList& indices);

  /// Delete all docs, reset query and emit `docsDeleted`
  int deleteAllDocs(QProgressDialog* progress = nullptr);

public slots:

  /// Change database
  void setDatabase(const QString& newDatabaseName);

  /// Set current query and reset model.
  void adjustQuery(DocFilter docFilter = DocFilter::all, int filterLabelId = -1,
                   int limit = 100, int offset = 0);

  /// reset model
  void refreshCurrentQuery();

  void documentStatusChanged(DocumentStatus newStatus);
  void documentGainedLabel(int labelId, int docId);
  void documentLostLabel(int labelId, int docId);

  /// called before showing the view; filtered doc list updates are lazy
  void refreshCurrentQueryIfOutdated();

signals:

  void docsDeleted();
  void labelsChanged();
  void databaseChanged();

private:
  static constexpr int defaultNDocsLimit_{100};

  QSqlQuery getQuery() const;
  void refreshNLabelledDocs();
  int totalNDocsNoFilter();

  DocFilter docFilter_ = DocFilter::all;
  int filterLabelId_ = -1;
  int offset_ = 0;
  int limit_ = 100;
  QString databaseName_;
  bool resultSetOutdated_{};

  int nLabelledDocs_{};
};
} // namespace labelbuddy
#endif // LABELBUDDY_DOC_LIST_MODEL_H
