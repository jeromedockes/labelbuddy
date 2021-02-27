#ifndef LABELBUDDY_LABEL_LIST_H
#define LABELBUDDY_LABEL_LIST_H

#include <QFrame>
#include <QLineEdit>
#include <QListView>
#include <QModelIndex>
#include <QPainter>
#include <QPushButton>
#include <QSize>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QValidator>
#include <QLabel>

#include "label_list_model.h"

namespace labelbuddy {

class LabelDelegate : public QStyledItemDelegate {

public:
  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const override;
};

class ShortcutValidator : public QValidator {
public:
  State validate(QString& input, int& pos) const override;
  void setModel(LabelListModel* new_model);
  void setView(QListView* new_view);

private:
  LabelListModel* model = nullptr;
  QListView* view = nullptr;
};

class LabelListButtons : public QFrame {
  Q_OBJECT

public:
  LabelListButtons(QWidget* parent = nullptr);
  void set_model(LabelListModel* new_model);
  void set_view(QListView* new_view);

signals:

  void select_all();
  void delete_selected_rows();
  void set_label_color();
  void set_label_shortcut(const QString& new_shortcut);

public slots:

  void update_button_states(int n_selected, int total,
                            const QModelIndex& first_selected);

private:
  QPushButton* select_all_button;
  QPushButton* delete_button;
  QPushButton* set_color_button;
  QLineEdit* shortcut_edit;
  QLabel* shortcut_label;
  ShortcutValidator validator;

private slots:
  void shortcut_edit_pressed();
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
  void set_label_shortcut(const QString& new_shortcut);

private:
  LabelListButtons* buttons_frame;
  QListView* labels_view;
  LabelListModel* model = nullptr;
};

} // namespace labelbuddy

#endif
