#ifndef LABELBUDDY_SEARCHABLE_TEXT_H
#define LABELBUDDY_SEARCHABLE_TEXT_H

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QTextEdit>
#include <QWidget>

#include "utils.h"

/// \file
/// read-only plain text display with a search bar and some custom key bindings.

namespace labelbuddy {

/// read-only plain text display with a search bar and some custom key bindings.
class SearchableText : public QWidget {

  Q_OBJECT

public:
  SearchableText(QWidget* parent = nullptr);

  void fill(const QString& content);
  void keyPressEvent(QKeyEvent*) override;

  /// start and end character positions for the currently selected text.

  /// returns `{start, end}`
  QList<int> current_selection() const;

  /// The QTextEdit's textCursor
  QTextCursor textCursor() const;

  /// Get a pointer to the child QTextEdit
  QTextEdit* get_text_edit();

  /// Get a pointer to the child search line
  QLineEdit* get_search_box();

public slots:

  void search(QTextDocument::FindFlags flags = QTextDocument::FindFlags());
  void search_forward();
  void search_backward();

protected:
  bool eventFilter(QObject* object, QEvent* event) override;

private slots:
  /// set position from which next search will start to the current cursor
  /// position
  void set_cursor_position();

  /// enable next/prev buttons iff search box is not empty
  void update_search_button_states();

private:
  QTextEdit* text_edit;
  QLineEdit* search_box;
  QPushButton* find_prev_button;
  QPushButton* find_next_button;

  QTextCursor last_match;
  QTextDocument::FindFlags current_search_flags;

  /// swap the cursor's position and anchor
  void swap_pos_anchor(QTextCursor& cursor) const;
  void handle_nav_event(QKeyEvent* event);

  /// cycle the cursor position between top, center, bottom by scrolling the
  /// text.
  void cycle_cursor_height();

  /// cycle the cursor position between top, center, bottom by scrolling the
  /// text.

  /// Called up to 3 times by `cycle_cursor_position` in case one of the
  /// positions results in no movement (when text cannot be scrolled in one or
  /// both directions).
  void cycle_cursor_height_once();

  /// Key events filtered (handled by this object and not the search bar) when
  /// Ctrl is pressed
  const QList<int> nav_keys{Qt::Key_K, Qt::Key_J, Qt::Key_N,
                            Qt::Key_P, Qt::Key_U, Qt::Key_D};

  /// Key events that are always filtered (handled by this object and not the
  /// search bar)
  const QList<int> nav_keys_nomodif{Qt::Key_BracketLeft, Qt::Key_BracketRight,
                                    Qt::Key_BraceLeft, Qt::Key_BraceRight};
  enum class CursorHeight { Center, Top, Bottom };
  CursorHeight last_cursor_height{};
  int last_cursor_pos{};

private slots:

  void extend_selection(QTextCursor::MoveOperation move_op, Side side);
};
} // namespace labelbuddy

#endif
