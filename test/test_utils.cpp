#include "test_utils.h"
#include "utils.h"

namespace labelbuddy {

void TestUtils::test_get_doc_url() {
  auto url = get_doc_url();
  QVERIFY(url.fileName().endsWith("documentation.html"));
}

void TestUtils::test_database_name_display() {
  QCOMPARE(database_name_display("/tmp/db1.labelbuddy"), "/tmp/db1.labelbuddy");
  QCOMPARE(database_name_display("/tmp/db1.labelbuddy", false),
           "db1.labelbuddy");

  QVERIFY(database_name_display(":LABELBUDDY_TEMPORARY_DATABASE:")
              .startsWith("Temporary"));
  QVERIFY(database_name_display(":memory:").startsWith("In-memory"));
}
} // namespace labelbuddy
