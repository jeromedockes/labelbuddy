#include <cassert>

#include <qmap.h>

#include "char_indices.h"

namespace labelbuddy {

CharIndices::CharIndices(const QString& text) { setText(text); }

int CharIndices::unicodeLength() const { return unicodeLength_; }

int CharIndices::qStringLength() const { return qStringLength_; }

int CharIndices::utf8Length() const { return text_.toUtf8().size(); }

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
  assert(isValidUnicodeIndex(unicodeIndex));

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
  assert(isValidQStringIndex(qStringIndex));

  int unicodeIndex{qStringIndex};
  for (auto surrogatesIt = surrogateIndicesInQString_.cbegin();
       surrogatesIt != surrogateIndicesInQString_.cend() &&
       *surrogatesIt < qStringIndex;
       ++surrogatesIt) {
    --unicodeIndex;
  }
  return unicodeIndex;
}

bool CharIndices::isValidUnicodeIndex(int index) const {
  return 0 <= index && index <= unicodeLength_;
}

bool CharIndices::isValidQStringIndex(int index) const {
  return 0 <= index && index <= qStringLength_;
  // here we could also check that it does not point to the second qchar of a
  // surrogate pair; however ordering depends on the host and as qstring
  // positions do not come from user data it is probably not necessary.
}

bool CharIndices::isValidUtf8Index(int index, const QByteArray& text) {
  if (index < 0) {
    return false;
  }
  if (text.size() < index) {
    return false;
  }
  if (index == text.size()) {
    return true;
  }
  auto utf8Byte = static_cast<unsigned char>(text.at(index));
  if (0x80 <= utf8Byte && utf8Byte < 0xc0) {
    // the byte at the index is a UTF-8 trailing byte (10xx_xxxx), ie the
    // index does not point to the leading byte of a unicode char's
    // representation but in the middle.
    return false;
  }
  return true;
}

bool CharIndices::isValidUtf8Index(int index) const {
  return isValidUtf8Index(index, text_.toUtf8());
}

} // namespace labelbuddy
