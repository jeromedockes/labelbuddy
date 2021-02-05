#include <QAbstractItemView>
#include <QApplication>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QSize>
#include <QStyle>

#include "label_list.h"
#include "label_list_model.h"

namespace labelbuddy {

void LabelDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const {
  QStyleOptionViewItem new_option(option);
  auto label_color = index.data(Qt::BackgroundRole).value<QColor>();
  painter->fillRect(option.rect, label_color);
  new_option.showDecorationSelected = false;
  auto button_size = option.rect.height();
  auto text_palette = option.palette;
  text_palette.setColor(QPalette::Text, QColor("black"));
  text_palette.setColor(QPalette::Background, QColor(0, 0, 0, 0));
  text_palette.setColor(QPalette::Highlight, QColor(0, 0, 0, 0));
  text_palette.setColor(QPalette::HighlightedText, QColor("black"));
  text_palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#444444"));
  new_option.palette = text_palette;
  new_option.rect = QRect(
      QPoint(option.rect.left() + int(button_size * .8), option.rect.top()),
      option.rect.bottomRight());
  QStyledItemDelegate::paint(painter, new_option, index);
  QStyleOptionButton checkbox_opt;
  checkbox_opt.rect = QRect(QPoint(option.rect.left() + 2, option.rect.top()),
                            option.rect.bottomRight());
  if (option.state & QStyle::State_Selected) {
    checkbox_opt.state |= QStyle::State_On;
  } else {
    checkbox_opt.state |= QStyle::State_Off;
  }
  if (option.state & QStyle::State_Enabled) {
    checkbox_opt.state |= QStyle::State_Enabled;
  } else {
    checkbox_opt.state = QStyle::State_NoChange;
    auto palette = checkbox_opt.palette;
    palette.setColor(QPalette::Base, label_color.lighter(100));
    checkbox_opt.palette = palette;
  }
  QApplication::style()->drawControl(QStyle::CE_RadioButton, &checkbox_opt,
                                     painter);
}

QSize LabelDelegate::sizeHint(const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
  auto size = QStyledItemDelegate::sizeHint(option, index);
  return QSize(size.width() + size.height(), size.height() + 6);
}

LabelListButtons::LabelListButtons(QWidget* parent) : QFrame(parent) {
  QHBoxLayout* layout = new QHBoxLayout();
  setLayout(layout);
  select_all_button = new QPushButton("Select all");
  layout->addWidget(select_all_button);
  delete_button = new QPushButton("Delete");
  layout->addWidget(delete_button);
  set_color_button = new QPushButton("Set color");
  layout->addWidget(set_color_button);
  QObject::connect(select_all_button, &QPushButton::clicked, this,
                   &LabelListButtons::select_all);

  QObject::connect(set_color_button, &QPushButton::clicked, this,
                   &LabelListButtons::set_label_color);

  QObject::connect(delete_button, SIGNAL(clicked()), this,
                   SIGNAL(delete_selected_rows()));
}

void LabelListButtons::update_button_states(int n_selected, int total) {
  select_all_button->setEnabled(total > 0);
  delete_button->setEnabled(n_selected > 0);
  set_color_button->setEnabled(n_selected == 1);
}

LabelList::LabelList(QWidget* parent) : QFrame(parent) {
  QVBoxLayout* layout = new QVBoxLayout();
  setLayout(layout);

  buttons_frame = new LabelListButtons();
  layout->addWidget(buttons_frame);

  this->setStyleSheet("QListView::item {background: transparent;}");
  labels_view = new QListView();
  layout->addWidget(labels_view);
  labels_view->setSpacing(3);
  labels_view->setItemDelegate(new LabelDelegate);
  labels_view->setFocusPolicy(Qt::NoFocus);

  labels_view->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QObject::connect(buttons_frame, &LabelListButtons::select_all, labels_view,
                   &QListView::selectAll);

  QObject::connect(buttons_frame, &LabelListButtons::delete_selected_rows, this,
                   &LabelList::delete_selected_rows);

  QObject::connect(buttons_frame, &LabelListButtons::set_label_color, this,
                   &LabelList::set_label_color);
}

void LabelList::setModel(LabelListModel* new_model) {
  labels_view->setModel(new_model);
  model = new_model;
  QObject::connect(model, &LabelListModel::modelReset, this,
                   &LabelList::update_button_states);
  QObject::connect(labels_view->selectionModel(),
                   &QItemSelectionModel::selectionChanged, this,
                   &LabelList::update_button_states);
  update_button_states();
}

void LabelList::delete_selected_rows() {
  if (model == nullptr) {
    return;
  }
  QModelIndexList selected = labels_view->selectionModel()->selectedIndexes();
  if (selected.length() == 0) {
    return;
  }
  int resp =
      QMessageBox::question(this, "labelbuddy",
                            QString("Really delete selected labels?\n"
                                    "All associated annotations will be lost"),
                            QMessageBox::Ok | QMessageBox::Cancel);
  if (resp != QMessageBox::Ok) {
    return;
  }
  int n_before = model->total_n_labels();
  model->delete_labels(selected);
  int n_after = model->total_n_labels();
  int n_deleted = n_before - n_after;
  labels_view->reset();
  QMessageBox::information(this, "labelbuddy",
                           QString("Deleted %0 label%1")
                               .arg(n_deleted)
                               .arg(n_deleted > 1 ? "s" : ""),
                           QMessageBox::Ok);
}

void LabelList::update_button_states() {
  if (model == nullptr) {
    return;
  }
  auto selected = labels_view->selectionModel()->selectedIndexes();
  buttons_frame->update_button_states(selected.length(),
                                      model->total_n_labels());
}

void LabelList::set_label_color() {
  auto selected = labels_view->selectionModel()->selectedIndexes();
  if (selected.length() != 1) {
    return;
  }
  auto label_name = model->data(selected[0], Qt::DisplayRole).toString();
  auto current_color =
      model->data(selected[0], Qt::BackgroundRole).value<QColor>();
  auto color = QColorDialog::getColor(
      current_color, this, QString("Set color for '%0'").arg(label_name));
  if (!color.isValid()) {
    return;
  }
  model->set_label_color(selected[0], color.name());
}
} // namespace labelbuddy
