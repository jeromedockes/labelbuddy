#ifndef LABELBUDDY_SEARCHABLE_TEXT_H
#define LABELBUDDY_SEARCHABLE_TEXT_H

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QTextEdit>
#include <QWidget>

namespace labelbuddy {

class SearchableText : public QWidget {

  Q_OBJECT

public:
  SearchableText(QWidget* parent = nullptr);

  void fill(const QString& content);
  void keyPressEvent(QKeyEvent*);
  QList<int> current_selection() const;
  QTextCursor textCursor() const;
  QTextEdit* get_text_edit();

public slots:

  void search(QTextDocument::FindFlags flags = QTextDocument::FindFlags());
  void search_forward();
  void search_backward();
  void continue_search();
  void set_cursor_position();
  void update_search_button_states();

protected:
  bool eventFilter(QObject* object, QEvent* event) override;

private:
  QTextEdit* text_edit;
  QLineEdit* search_box;
  QPushButton* find_prev_button;
  QPushButton* find_next_button;

  QTextCursor last_match;
  QTextDocument::FindFlags current_search_flags;

  void swap_pos_anchor(QTextCursor& cursor) const;
  void handle_nav_event(QKeyEvent* event);
  const QList<int> nav_keys{Qt::Key_K, Qt::Key_J, Qt::Key_N,
                            Qt::Key_P, Qt::Key_U, Qt::Key_D};
  const QList<int> nav_keys_nomodif{Qt::Key_BracketLeft, Qt::Key_BracketRight,
                                    Qt::Key_BraceLeft, Qt::Key_BraceRight};
  enum class Side { Right, Left };

private slots:

  void extend_selection(QTextCursor::MoveOperation move_op, Side side);
};
} // namespace labelbuddy

#endif
