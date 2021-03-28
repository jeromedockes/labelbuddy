#include <QAbstractItemView>
#include <QApplication>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QSize>
#include <QStyle>
#include <QVBoxLayout>

#include "label_list.h"
#include "label_list_model.h"
#include "user_roles.h"
#include "utils.h"

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
  auto outer_layout = new QVBoxLayout();
  setLayout(outer_layout);
  auto top_layout = new QHBoxLayout();
  outer_layout->addLayout(top_layout);
  select_all_button = new QPushButton("Select all");
  top_layout->addWidget(select_all_button);
  delete_button = new QPushButton("Delete");
  top_layout->addWidget(delete_button);

  auto add_label_layout = new QHBoxLayout();
  outer_layout->addLayout(add_label_layout);
  auto add_label_instruction = new QLabel("&New label:");
  add_label_layout->addWidget(add_label_instruction);
  add_label_edit = new QLineEdit();
  add_label_layout->addWidget(add_label_edit);
  add_label_instruction->setBuddy(add_label_edit);
  add_label_edit->installEventFilter(this);

  auto bottom_layout = new QHBoxLayout();
  outer_layout->addLayout(bottom_layout);
  set_color_button = new QPushButton("Set color");
  bottom_layout->addWidget(set_color_button);
  shortcut_label = new QLabel("Shortcut key: ");
  bottom_layout->addWidget(shortcut_label);
  shortcut_edit = new QLineEdit();
  bottom_layout->addWidget(shortcut_edit);
  shortcut_edit->setMaxLength(1);
  shortcut_edit->setValidator(&validator);
  shortcut_edit->setFixedWidth(shortcut_edit->fontMetrics().maxWidth());
  bottom_layout->addStretch(1);

  QObject::connect(select_all_button, &QPushButton::clicked, this,
                   &LabelListButtons::select_all);
  QObject::connect(set_color_button, &QPushButton::clicked, this,
                   &LabelListButtons::set_label_color);
  QObject::connect(delete_button, &QPushButton::clicked, this,
                   &LabelListButtons::delete_selected_rows);
  QObject::connect(shortcut_edit, &QLineEdit::returnPressed, this,
                   &LabelListButtons::shortcut_edit_pressed);
  QObject::connect(add_label_edit, &QLineEdit::returnPressed, this,
                   &LabelListButtons::add_label_edit_pressed);
}

void LabelListButtons::update_button_states(int n_selected, int total,
                                            const QModelIndex& first_selected) {
  select_all_button->setEnabled(total > 0);
  delete_button->setEnabled(n_selected > 0);
  set_color_button->setEnabled(n_selected == 1);
  auto shortcut_edit_had_focus = shortcut_edit->hasFocus();
  shortcut_edit->setEnabled(n_selected == 1);
  shortcut_label->setEnabled(n_selected == 1);
  if (n_selected == 1 && first_selected.isValid()) {
    shortcut_edit->setText(first_selected.model()
                               ->data(first_selected, Roles::ShortcutKeyRole)
                               .toString());
    shortcut_edit->setFocus();
  } else {
    if (shortcut_edit_had_focus) {
      setFocus();
    }
    shortcut_edit->setText("");
  }
  if (n_selected != 0) {
    add_label_edit->setText("");
    if (add_label_edit->hasFocus()) {
      setFocus();
    }
  }
}

void LabelListButtons::set_model(LabelListModel* new_model) {
  validator.setModel(new_model);
}

void LabelListButtons::set_view(QListView* new_view) {
  label_list_view_ = new_view;
  validator.setView(new_view);
}

bool LabelListButtons::eventFilter(QObject* object, QEvent* event) {
  if (object == add_label_edit && event->type() == QEvent::FocusIn) {
    if (label_list_view_ != nullptr) {
      label_list_view_->clearSelection();
    }
  }
  return QWidget::eventFilter(object, event);
}

void LabelListButtons::shortcut_edit_pressed() {
  emit set_label_shortcut(shortcut_edit->text());
}

void LabelListButtons::add_label_edit_pressed() {
  auto label_name = add_label_edit->text().trimmed();
  if (label_name != "") {
    emit add_label(label_name);
  }
  add_label_edit->setText("");
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
  label_delegate_.reset(new LabelDelegate);
  labels_view->setItemDelegate(label_delegate_.get());
  labels_view->setFocusPolicy(Qt::NoFocus);
  buttons_frame->set_view(labels_view);

  labels_view->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QObject::connect(buttons_frame, &LabelListButtons::select_all, labels_view,
                   &QListView::selectAll);
  QObject::connect(buttons_frame, &LabelListButtons::delete_selected_rows, this,
                   &LabelList::delete_selected_rows);
  QObject::connect(buttons_frame, &LabelListButtons::set_label_color, this,
                   &LabelList::set_label_color);
  QObject::connect(buttons_frame, &LabelListButtons::set_label_shortcut, this,
                   &LabelList::set_label_shortcut);
  QObject::connect(buttons_frame, &LabelListButtons::add_label, this,
                   &LabelList::add_label);
}

void LabelList::setModel(LabelListModel* new_model) {
  labels_view->setModel(new_model);
  buttons_frame->set_model(new_model);
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
  setFocus();
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
  int n_rows{};
  for (const auto& sel : selected) {
    if (sel.column() == 0) {
      ++n_rows;
    }
  }
  auto first_selected = find_first_in_col_0(selected);
  QModelIndex selected_index{};
  if (first_selected != selected.constEnd()) {
    selected_index = *first_selected;
  }
  buttons_frame->update_button_states(n_rows, model->total_n_labels(),
                                      selected_index);
}

void LabelList::set_label_color() {
  auto all_selected = labels_view->selectionModel()->selectedIndexes();
  auto selected = find_first_in_col_0(all_selected);
  if (selected == all_selected.constEnd()) {
    return;
  }
  auto label_name = model->data(*selected, Qt::DisplayRole).toString();
  auto current_color =
      model->data(*selected, Qt::BackgroundRole).value<QColor>();
  auto color = QColorDialog::getColor(
      current_color, this, QString("Set color for '%0'").arg(label_name));
  model->set_label_color(*selected, color);
}

void LabelList::set_label_shortcut(const QString& new_shortcut) {
  auto all_selected = labels_view->selectionModel()->selectedIndexes();
  auto selected = find_first_in_col_0(all_selected);
  if (selected == all_selected.constEnd()) {
    return;
  }
  model->set_label_shortcut(*selected, new_shortcut);
}

void LabelList::add_label(const QString& name) {
  auto label_id = model->add_label(name);
  if (label_id == -1) {
    return;
  }
  labels_view->reset();
  auto model_index = model->label_id_to_model_index(label_id);
  labels_view->setCurrentIndex(model_index);
}

QValidator::State ShortcutValidator::validate(QString& input, int& pos) const {
  (void)pos;
  if (model == nullptr || view == nullptr) {
    return State::Invalid;
  }
  if (input == QString("")) {
    return State::Acceptable;
  }
  auto all_selected = view->selectionModel()->selectedIndexes();
  QModelIndex selected{};
  auto selected_it = find_first_in_col_0(all_selected);
  if (selected_it != all_selected.constEnd()) {
    selected = *selected_it;
  }
  if (!model->is_valid_shortcut(input, selected)) {
    return State::Invalid;
  }
  return State::Acceptable;
}
void ShortcutValidator::setModel(LabelListModel* new_model) {
  model = new_model;
}
void ShortcutValidator::setView(QListView* new_view) { view = new_view; }
} // namespace labelbuddy
