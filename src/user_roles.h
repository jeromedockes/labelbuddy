#ifndef LABELBUDDY_USER_ROLES_H
#define LABELBUDDY_USER_ROLES_H

#include <Qt>

/// \file
/// additional roles for the models' `data` member

namespace labelbuddy {

enum Roles {
  RowIdRole = Qt::UserRole,
  ShortcutKeyRole,
  LabelNameRole,
  AnnotationIdRole,
  AnnotationPrefixRole,
  SelectedTextRole,
  AnnotationSuffixRole,
  AnnotationStartCharRole,
  AnnotationExtraDataRole
};

enum class DocumentStatus { Unlabelled, Labelled };

} // namespace labelbuddy

#endif
