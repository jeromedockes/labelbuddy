#include <QUrl>

#include "test_utils.h"
#include "utils.h"

namespace labelbuddy {

void TestUtils::testGetDocUrl() {
  auto url = getDocUrl();
  QVERIFY(url.fileName().endsWith("documentation.html") or
          url == QUrl{"https://jeromedockes.github.io/labelbuddy/labelbuddy/"
                      "current/documentation/"});
}

void TestUtils::testDatabaseNameDisplay() {
  QCOMPARE(databaseNameDisplay("/tmp/db1.labelbuddy"), "/tmp/db1.labelbuddy");
  QCOMPARE(databaseNameDisplay("/tmp/db1.labelbuddy", false), "db1.labelbuddy");

  QVERIFY(databaseNameDisplay(":LABELBUDDY_TEMPORARY_DATABASE:")
              .startsWith("Temporary"));
  QVERIFY(databaseNameDisplay(":memory:").startsWith("In-memory"));
}
} // namespace labelbuddy
