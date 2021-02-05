#include "no_deselect_all_view.h"

namespace labelbuddy {

QItemSelectionModel::SelectionFlags
NoDeselectAllView::selectionCommand(const QModelIndex& index,
                                    const QEvent* event) const {
  auto selected = selectedIndexes();
  if ((selected.length() == 1) && (selected[0] == index)) {
    return QItemSelectionModel::NoUpdate;
  }
  return QListView::selectionCommand(index, event);
}
} // namespace labelbuddy
