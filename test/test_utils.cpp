#include "test_utils.h"
#include "utils.h"

namespace labelbuddy {

void TestUtils::test_get_doc_url() {
  auto url = get_doc_url();
  QVERIFY(url.fileName().endsWith("documentation.html"));
}
} // namespace labelbuddy
