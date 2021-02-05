#ifndef LABELBUDDY_LABEL_LIST_H
#define LABELBUDDY_LABEL_LIST_H

#include <QFrame>
#include <QListView>
#include <QModelIndex>
#include <QPainter>
#include <QPushButton>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QSize>

#include "label_list_model.h"

namespace labelbuddy {

class LabelDelegate : public QStyledItemDelegate {

public:
  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
  QSize sizeHint(const QStyleOptionViewItem& option,
                const QModelIndex& index) const override;
};

class LabelListButtons : public QFrame {
  Q_OBJECT

public:
  LabelListButtons(QWidget* parent = nullptr);

signals:

  void select_all();
  void delete_selected_rows();
  void set_label_color();

public slots:

  void update_button_states(int n_selected, int total);

private:
  QPushButton* select_all_button;
  QPushButton* delete_button;
  QPushButton* set_color_button;
};

class LabelList : public QFrame {
  Q_OBJECT

public:
  LabelList(QWidget* parent = nullptr);
  void setModel(LabelListModel*);

public slots:

  void delete_selected_rows();
  void update_button_states();
  void set_label_color();

private:
  LabelListButtons* buttons_frame;
  QListView* labels_view;
  LabelListModel* model = nullptr;
};
} // namespace labelbuddy

#endif
