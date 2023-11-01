#ifndef LABELBUDDY_LABEL_LIST_MODEL_H
#define LABELBUDDY_LABEL_LIST_MODEL_H

#include <memory>

#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QAbstractItemModel>

#include "utils.h"

/// \file
/// Interface to labels in the database.

namespace labelbuddy {

/// Interface to labels in the database.
class LabelListModel : public QSqlQueryModel {
  Q_OBJECT

public:
  LabelListModel(QObject* parent = nullptr);

  /// data for a label.

  /// besides Qt roles `Roles::RowIdRole` can be used to get the doc's `id`
  /// and Roles::ShortcutKeyRole to get the `shortcutKey`.
  QVariant data(const QModelIndex& index, int role) const override;

  /// Items in the second (hidden) column that contains id cannot be selected.
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  Qt::DropActions supportedDropActions() const override;

  QMimeData* mimeData(const QModelIndexList& indexes) const override;

  bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row,
                       int column, const QModelIndex& parent) const override;

  bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row,
                    int column, const QModelIndex& parent) override;

  QStringList mimeTypes() const override;

  /// Get the QModelIndex given the `id` in the database
  QModelIndex labelIdToModelIndex(int labelId) const;

  int totalNLabels() const;

  /// Delete labels, reset query, emit `labelsDeleted` and `labelsChanged`
  int deleteLabels(const QModelIndexList& indices);

  /// Check that `shortcut` is a valid `shortcutKey` for the label at `index`

  /// Valid if it matches the allowed pattern and is not used by another label.
  bool isValidShortcut(const QString& shortcut, const QModelIndex& index) const;

  /// name is not empty and not already used by another label.
  bool isValidRename(const QString& newName, const QModelIndex& index) const;

  int addLabel(const QString& name);

public slots:

  /// Set the current database
  void setDatabase(const QString& newDatabaseName);

  /// Reset model
  void refreshCurrentQuery();

  /// Set `color` for a label

  /// Nothing is done if `! color.isValid()`
  /// Otherwise insert the color's html name ie #xxxxxx
  /// Emits `dataChanged` and `labelsChanged` otherwise
  void setLabelColor(const QModelIndex& index, const QColor& color);

  /// Set `shortcutKey` for the label at index

  /// Nothing is done if not a a valid shortcut or empty string.
  /// `sortcutKey` set to NULL if `shortcut` is the empty string. If shortcut
  /// used by another label db constraint will prevent inserting it but signals
  /// will still be emited.
  /// Emits `dataChanged` and `labelsChanged`
  void setLabelShortcut(const QModelIndex& index, const QString& shortcut);

  /// Set a label's name, keep all annotations

  /// Has no effect if new name is already taken or is the empty string.
  /// Emits dataChanged, labelsChanged, labelRenamed.
  void renameLabel(const QModelIndex& index, const QString& newName);

signals:

  void labelsChanged();
  void labelsDeleted();
  void labelsAdded();
  void labelsOrderChanged();
  void labelRenamed(int labelId, QString newName);

private:
  QSqlQuery getQuery() const;

  bool isValidShortcut(const QString& shortcut, int labelId) const;

  /// `allLabels` is an empty list to be filled with label ids in the new order
  void getReorderedLabelsInto(const QList<int>& movedLabels, int row,
                              std::list<int>& allLabels) const;

  void updateLabelsOrder(const std::list<int>& labels);

  QString databaseName_;
  const QString selectQueryText_ = ("select name, id from sorted_label;");
  QRegularExpression re_ = shortcutKeyPattern(true);
};

QList<int> getLabelIds(const LabelListModel& model);
} // namespace labelbuddy
#endif // LABELBUDDY_DOC_LIST_MODEL_H
