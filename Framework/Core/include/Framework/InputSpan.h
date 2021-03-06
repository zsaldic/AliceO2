// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#ifndef FRAMEWORK_INPUTSPAN_H
#define FRAMEWORK_INPUTSPAN_H

namespace o2
{
namespace framework
{

/// Mapping helper between the store of all inputs being processed and the
/// actual inputs to be processed in a given go.
/// In general this will use an helper which returns
/// `FairMQMessages->GetData()` from the Message cache, but in principle
/// the mechanism should be flexible enough to allow all kind of input stores.
class InputSpan
{
 public:
  InputSpan() = delete;
  InputSpan(InputSpan const&) = delete;
  InputSpan(InputSpan&&) = default;

  /// @a getter is the mapping between an element of the span referred by
  /// index and the buffer associated.
  /// @a size is the number of elements in the span.
  InputSpan(std::function<DataRef(size_t)> getter, size_t size)
    : mGetter{}, mNofPartsGetter{}, mSize{size}
  {
    mGetter = [getter](size_t index, size_t) -> DataRef {
      return getter(index);
    };
  }

  /// @a getter is the mapping between an element of the span referred by
  /// index and the buffer associated.
  /// @a size is the number of elements in the span.
  InputSpan(std::function<DataRef(size_t, size_t)> getter, size_t size)
    : mGetter{getter}, mNofPartsGetter{}, mSize{size}
  {
  }

  /// @a getter is the mapping between an element of the span referred by
  /// index and the buffer associated.
  /// @nofPartsGetter is the getter for the number of parts associated with an index
  /// @a size is the number of elements in the span.
  InputSpan(std::function<DataRef(size_t, size_t)> getter, std::function<size_t(size_t)> nofPartsGetter, size_t size)
    : mGetter{getter}, mNofPartsGetter{nofPartsGetter}, mSize{size}
  {
  }

  /// @a i-th element of the InputSpan
  DataRef get(size_t i, size_t partidx = 0) const
  {
    return mGetter(i, partidx);
  }

  /// @a number of parts in the i-th element of the InputSpan
  size_t getNofParts(size_t i) const
  {
    if (i >= mSize) {
      return 0;
    }
    if (!mNofPartsGetter) {
      return 1;
    }
    return mNofPartsGetter(i);
  }

  /// Number of elements in the InputSpan
  size_t size() const
  {
    return mSize;
  }

  const char* header(size_t i) const
  {
    return get(i).header;
  }

  const char* payload(size_t i) const
  {
    return get(i).payload;
  }

 private:
  std::function<DataRef(size_t, size_t)> mGetter;
  std::function<size_t(size_t)> mNofPartsGetter;
  size_t mSize;
};

} // namespace framework
} // namespace o2

#endif // FRAMEWORK_INPUTSSPAN_H
