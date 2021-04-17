#ifndef LABELBUDDY_NO_DESELECT_ALL_VIEW_H
#define LABELBUDDY_NO_DESELECT_ALL_VIEW_H

#include <QEvent>
#include <QItemSelectionModel>
#include <QListView>

/// \file
/// A QListView that doesn't allow deselecting all items

namespace labelbuddy {

/// A QListView that doesn't allow deselecting all items

/// Used for the labels list in the Annotate tab as each annotation has exactly
/// one label.
class NoDeselectAllView : public QListView {
protected:
  QItemSelectionModel::SelectionFlags
  selectionCommand(const QModelIndex& index,
                   const QEvent* event = nullptr) const override;
};
} // namespace labelbuddy
#endif
