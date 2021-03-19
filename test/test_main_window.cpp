#include <QTextEdit>

#include "main_window.h"
#include "test_main_window.h"

namespace labelbuddy {
void TestLabelBuddy::test_label_buddy() {
  LabelBuddy buddy(nullptr, "", true);
  auto te = buddy.findChild<QTextEdit*>();
  QVERIFY(te->toPlainText().startsWith("THIS IS A TEMPORARY DATABASE"));
}
} // namespace labelbuddy
