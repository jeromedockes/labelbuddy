#include <QPlainTextEdit>

#include "main_window.h"
#include "test_main_window.h"

namespace labelbuddy {
void TestLabelBuddy::testLabelBuddy() {
  LabelBuddy buddy(nullptr, "", true);
  auto te = buddy.findChild<QPlainTextEdit*>();
  QVERIFY(te->toPlainText().startsWith("THIS IS A TEMPORARY DATABASE"));
}
} // namespace labelbuddy
