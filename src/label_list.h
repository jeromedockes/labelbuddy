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
  LabelDelegate(bool withDragHandles = false, QObject* parent = nullptr);
  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const override;

private:
  // dimensions of drag handles as multiples of line width, eg margin is 2 line
  // widths
  static constexpr int handleInnerWidthFactor_ = 12;
  static constexpr int handleMarginFactor_ = 2;
  static constexpr int handleOuterWidthFactor_ =
      handleInnerWidthFactor_ + 2 * handleMarginFactor_;

  bool withDragHandles_ = false;
  int lineWidth_ = 1;
  int margin_ = 2;

  int radioButtonWidth() const;
  int handleWidth() const;
  QStyleOptionViewItem textStyleOption(const QStyleOptionViewItem& option,
                                       QRect rect) const;
  void paintRadioButton(QPainter* painter, const QStyleOptionViewItem& option,
                        const QColor& labelColor) const;
  void paintDragHandle(QPainter* painter, const QStyleOptionViewItem& option,
                       const QColor& labelColor) const;
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
  void setModel(LabelListModel* newModel);

  /// set the label list view, needed to know which label is selected
  void setView(QListView* newView);

private:
  LabelListModel* model_ = nullptr;
  QListView* view_ = nullptr;
};

/// Buttons above the label list in the Dataset tab
class LabelListButtons : public QFrame {
  Q_OBJECT

public:
  LabelListButtons(QWidget* parent = nullptr);
  void setModel(LabelListModel* newModel);
  void setView(QListView* newView);

signals:

  void selectAll();
  void deleteSelectedRows();
  void setLabelColor();
  void setLabelShortcut(const QString& newShortcut);
  void addLabel(const QString& name);
  void renameLabel(const QString& name);

public slots:

  /// Update the button states

  /// \param nSelected number of items selected in the label list
  /// \param total total number of labels in the database
  /// \param firstSelected first selected label in the list, used to set the
  /// current shortcut in the shortcut edit box.
  void updateButtonStates(int nSelected, int total,
                          const QModelIndex& firstSelected);

protected:
  bool eventFilter(QObject* object, QEvent* event) override;

private:
  QPushButton* selectAllButton_;
  QPushButton* deleteButton_;
  QLineEdit* addLabelEdit_;
  QPushButton* setColorButton_;
  QLineEdit* shortcutEdit_;
  QLabel* shortcutTitle_;
  ShortcutValidator shortcutValidator_;
  QLineEdit* renameEdit_;
  QLabel* renameTitle_;
  QListView* labelListView_ = nullptr;

private slots:
  void addLabelEditPressed();
  void onRenameEditFinished();
};

/// List of labels shown in the Dataset and Annotate tabs
class LabelList : public QFrame {
  Q_OBJECT

public:
  LabelList(QWidget* parent = nullptr);
  void setModel(LabelListModel*);

private slots:

  /// Asks for confirmation and tells the model to delete selected labels
  void deleteSelectedRows();
  void updateButtonStates();

  /// Opens a color dialog and asks the model to set the new color for the label

  /// color is validated by the model before update
  void setLabelColor();

  /// When shortcut is changed in the line edit, ask the model to update it

  /// shortcut is validated by the model before update
  void setLabelShortcut(const QString& newShortcut);

  /// Tell model to rename the label when changed in line edit.

  /// Validation is done by the model & database
  void renameLabel(const QString& newName);

  void addLabel(const QString& name);

private:
  LabelListButtons* buttonsFrame_;
  QListView* labelsView_;
  LabelListModel* model_ = nullptr;
  std::unique_ptr<LabelDelegate> labelDelegate_ = nullptr;
};

} // namespace labelbuddy

#endif
