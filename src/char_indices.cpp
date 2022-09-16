#include "char_indices.h"
#include <qmap.h>

namespace labelbuddy {

CharIndices::CharIndices(const QString& text) { setText(text); }

int CharIndices::unicodeLength() const { return unicodeLength_; }

int CharIndices::qStringLength() const { return qStringLength_; }

void CharIndices::setText(const QString& newText) {
  text_ = newText;
  surrogateIndicesInQString_.clear();
  surrogateIndicesInUnicode_.clear();

  int qIdx{};
  int uIdx{};
  for (auto qchar : text_) {
    if (!qchar.isSurrogate()) {
      ++qIdx;
      ++uIdx;
    } else if (qchar.isHighSurrogate()) {
      surrogateIndicesInQString_ << qIdx;
      ++qIdx;
      surrogateIndicesInUnicode_ << uIdx;
      ++uIdx;
    } else {
      assert(qchar.isLowSurrogate());
      ++qIdx;
    }
  }
  unicodeLength_ = uIdx;
  qStringLength_ = qIdx;
}

int CharIndices::unicodeToQString(int unicodeIndex) const {
  assert(0 <= unicodeIndex && unicodeIndex <= unicodeLength_);

  int qStringIndex{unicodeIndex};
  for (auto surrogatesIt = surrogateIndicesInUnicode_.cbegin();
       surrogatesIt != surrogateIndicesInUnicode_.cend() &&
       *surrogatesIt < unicodeIndex;
       ++surrogatesIt) {
    ++qStringIndex;
  }
  return qStringIndex;
}

int CharIndices::qStringToUnicode(int qStringIndex) const {
  assert(0 <= qStringIndex && qStringIndex <= qStringLength_);

  int unicodeIndex{qStringIndex};
  for (auto surrogatesIt = surrogateIndicesInQString_.cbegin();
       surrogatesIt != surrogateIndicesInQString_.cend() &&
       *surrogatesIt < qStringIndex;
       ++surrogatesIt) {
    --unicodeIndex;
  }
  return unicodeIndex;
}

} // namespace labelbuddy
