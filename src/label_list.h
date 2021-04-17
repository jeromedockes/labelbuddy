#ifndef LABELBUDDY_LABEL_LIST_H
#define LABELBUDDY_LABEL_LIST_H

#include <memory>

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QModelIndex>
#include <QPainter>
#include <QPushButton>
#include <QSize>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QValidator>

#include "label_list_model.h"

/// \file
/// List of labels in the Dataset and Annotate tabs

namespace labelbuddy {

/// Delegate to paint items of the labels list

/// - Selection is indicated with a radio button not by changing color
/// - Background color is the label's `color` in the database
class LabelDelegate : public QStyledItemDelegate {

public:
  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const override;
};

/// Validator for shortcut entered in the shortcut QLineEdit

/// shortcut is valid if it is a single lowercase letter and not used by a
/// different label. This class needs references to the list view to know which
/// label is currently selected and to the model to know which shortcuts are
/// currently used. Before setting the model and view always returns invalid.
class ShortcutValidator : public QValidator {
public:
  /// validate a shortcut. `pos` is ignored
  State validate(QString& input, int& pos) const override;

  /// set the model, needed to validate shortcuts
  void setModel(LabelListModel* new_model);

  /// set the label list view, needed to know which label is selected
  void setView(QListView* new_view);

private:
  LabelListModel* model = nullptr;
  QListView* view = nullptr;
};

/// Buttons above the label list in the Dataset tab
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
  void add_label(const QString& name);

public slots:

  /// Update the button states

  /// \param n_selected number of items selected in the label list
  /// \param total total number of labels in the database
  /// \param first_selected first selected label in the list, used to set the
  /// current shortcut in the shortcut edit box.
  void update_button_states(int n_selected, int total,
                            const QModelIndex& first_selected);

protected:
  bool eventFilter(QObject* object, QEvent* event) override;

private:
  QPushButton* select_all_button;
  QPushButton* delete_button;
  QLineEdit* add_label_edit;
  QPushButton* set_color_button;
  QLineEdit* shortcut_edit;
  QLabel* shortcut_label;
  ShortcutValidator validator;
  QListView* label_list_view_ = nullptr;

private slots:
  void add_label_edit_pressed();
};

/// List of labels shown in the Dataset and Annotate tabs
class LabelList : public QFrame {
  Q_OBJECT

public:
  LabelList(QWidget* parent = nullptr);
  void setModel(LabelListModel*);

private slots:

  /// Asks for confirmation and tells the model to delete selected labels
  void delete_selected_rows();
  void update_button_states();

  /// Opens a color dialog and asks the model to set the new color for the label

  /// color is validated by the model before update
  void set_label_color();

  /// When shortcut is changed in the line edit, ask the model to update it

  /// shortcut is validated by the model before update
  void set_label_shortcut(const QString& new_shortcut);

  void add_label(const QString& name);

private:
  LabelListButtons* buttons_frame;
  QListView* labels_view;
  LabelListModel* model = nullptr;
  std::unique_ptr<LabelDelegate> label_delegate_ = nullptr;
};

} // namespace labelbuddy

#endif
