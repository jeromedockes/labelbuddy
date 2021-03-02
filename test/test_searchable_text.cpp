#include "testing_utils.h"

#include "searchable_text.h"
#include "test_searchable_text.h"

namespace labelbuddy {

void TestSearchableText::test_search() {
  SearchableText text{};
  text.show();
  auto content = example_doc();
  text.fill(content);
  QCOMPARE(text.get_text_edit()->toPlainText(), example_doc());
  QVERIFY(text.get_text_edit()->isReadOnly());
  auto search_box = text.findChild<QLineEdit*>();
  auto te = text.findChild<QTextEdit*>();
  search_box->setText(u8"maçã");

  text.search_forward();
  auto selected =
      content.mid(text.current_selection()[0],
                  text.current_selection()[1] - text.current_selection()[0]);
  QCOMPARE(selected, QString(u8"maçã"));
  auto cursor = te->textCursor();
  QCOMPARE(cursor.selectedText(), QString(u8"maçã"));
  cursor.clearSelection();
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QCOMPARE(cursor.selectedText(), QString("1"));

  text.search();
  cursor = te->textCursor();
  QCOMPARE(cursor.selectedText(), QString(u8"maçã"));
  cursor.clearSelection();
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QCOMPARE(cursor.selectedText(), QString("2"));

  text.search_backward();
  cursor = te->textCursor();
  QCOMPARE(cursor.selectedText(), QString(u8"maçã"));
  cursor.clearSelection();
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QCOMPARE(cursor.selectedText(), QString("1"));

  text.search_backward();
  cursor = te->textCursor();
  QCOMPARE(cursor.selectedText(), QString(u8"maçã"));
  cursor.clearSelection();
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QCOMPARE(cursor.selectedText(), QString("3"));
}
} // namespace labelbuddy
