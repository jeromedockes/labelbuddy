#ifndef LABELBUDDY_NO_DESELECT_ALL_VIEW_H
#define LABELBUDDY_NO_DESELECT_ALL_VIEW_H

#include <QEvent>
#include <QItemSelectionModel>
#include <QListView>

namespace labelbuddy {

class NoDeselectAllView : public QListView {
  QItemSelectionModel::SelectionFlags
  selectionCommand(const QModelIndex& index,
                   const QEvent* event = nullptr) const override;
};
} // namespace labelbuddy
#endif
