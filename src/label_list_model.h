#ifndef LABELBUDDY_LABEL_LIST_MODEL_H
#define LABELBUDDY_LABEL_LIST_MODEL_H

#include <memory>

#include <QSqlQuery>
#include <QSqlQueryModel>

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
  /// and Roles::ShortcutKeyRole to get the `shortcut_key`.
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
  QModelIndex label_id_to_model_index(int label_id) const;

  int total_n_labels() const;

  /// Delete labels, reset query, emit `labels_deleted` and `labels_changed`
  int delete_labels(const QModelIndexList& indices);

  /// Check that `shortcut` is a valid `shortcut_key` for the label at `index`

  /// Valid if it is a single lowercase letter not used by another label.
  bool is_valid_shortcut(const QString& shortcut,
                         const QModelIndex& index) const;

  int add_label(const QString& name);

public slots:

  /// Set the current database
  void set_database(const QString& new_database_name);

  /// Reset model
  void refresh_current_query();

  /// Set `color` for a label

  /// Nothing is done if `! color.isValid()`
  /// Otherwise insert the color's html name ie #xxxxxx
  /// Emits `dataChanged` and `labels_changed` otherwise
  void set_label_color(const QModelIndex& index, const QColor& color);

  /// Set `shortcut_key` for the label at index

  /// Nothing is done if not a single lowercase letter or empty string
  /// `sortcut_key` set to NULL if `shortcut` is the empty string If shortcut
  /// used by another label db constraint will prevent inserting it but signals
  /// will still be emited.
  /// Emits `dataChanged` and `labels_changed`
  void set_label_shortcut(const QModelIndex& index, const QString& shortcut);

signals:

  void labels_changed();
  void labels_deleted();
  void labels_added();
  void labels_order_changed();

private:
  QSqlQuery get_query() const;

  bool is_valid_shortcut(const QString& shortcut, int label_id) const;

  /// `all_labels` is an empty list to be filled with label ids in the new order
  void get_reordered_labels_into(const QList<int>& moved_labels, int row,
                                 std::list<int>& all_labels) const;

  void update_labels_order(const std::list<int>& labels);

  QString database_name;
  const QString select_query_text = ("select name, id from sorted_label;");
  QRegularExpression re = shortcut_key_pattern(true);
};

QList<int> get_label_ids(const LabelListModel& model);
} // namespace labelbuddy
#endif // LABELBUDDY_DOC_LIST_MODEL_H
