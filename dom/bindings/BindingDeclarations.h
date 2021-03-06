/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * A header for declaring various things that binding implementation headers
 * might need.  The idea is to make binding implementation headers safe to
 * include anywhere without running into include hell like we do with
 * BindingUtils.h
 */
#ifndef mozilla_dom_BindingDeclarations_h__
#define mozilla_dom_BindingDeclarations_h__

#include "nsStringGlue.h"
#include "jsapi.h"
#include "mozilla/Util.h"
#include "nsCOMPtr.h"
#include "nsDOMString.h"
#include "nsStringBuffer.h"

namespace mozilla {
namespace dom {

struct MainThreadDictionaryBase
{
protected:
  JSContext* ParseJSON(const nsAString& aJSON,
                       mozilla::Maybe<JSAutoRequest>& aAr,
                       mozilla::Maybe<JSAutoCompartment>& aAc,
                       JS::Value& aVal);
};

struct EnumEntry {
  const char* value;
  size_t length;
};

class NS_STACK_CLASS GlobalObject
{
public:
  GlobalObject(JSContext* aCx, JSObject* aObject);

  nsISupports* Get() const
  {
    return mGlobalObject;
  }

  bool Failed() const
  {
    return !Get();
  }

private:
  js::RootedObject mGlobalJSObject;
  nsISupports* mGlobalObject;
  nsCOMPtr<nsISupports> mGlobalObjectRef;
};

class NS_STACK_CLASS WorkerGlobalObject
{
public:
  WorkerGlobalObject(JSContext* aCx, JSObject* aObject);

  JSObject* Get() const
  {
    return mGlobalJSObject;
  }
  // The context that this returns is not guaranteed to be in the compartment of
  // the object returned from Get(), in fact it's generally in the caller's
  // compartment.
  JSContext* GetContext() const
  {
    return mCx;
  }

  bool Failed() const
  {
    return !Get();
  }

private:
  js::RootedObject mGlobalJSObject;
  JSContext* mCx;
};

/**
 * A class for representing string return values.  This can be either passed to
 * callees that have an nsString or nsAString out param or passed to a callee
 * that actually knows about this class and can work with it.  Such a callee may
 * call SetStringBuffer on this object, but only if it plans to keep holding a
 * strong ref to the stringbuffer!
 *
 * The proper way to store a value in this class is to either to do nothing
 * (which leaves this as an empty string), to call SetStringBuffer with a
 * non-null stringbuffer, to call SetNull(), or to call AsAString() and set the
 * value in the resulting nsString.  These options are mutually exclusive!
 * Don't do more than one of them.
 *
 * The proper way to extract a value is to check IsNull().  If not null, then
 * check HasStringBuffer().  If that's true, check for a zero length, and if the
 * length is nonzero call StringBuffer().  If the length is zero this is the
 * empty string.  If HasStringBuffer() returns false, call AsAString() and get
 * the value from that.
 */
class NS_STACK_CLASS DOMString {
public:
  DOMString()
    : mStringBuffer(nullptr)
    , mLength(0)
    , mIsNull(false)
  {}
  ~DOMString()
  {
    MOZ_ASSERT(mString.empty() || !mStringBuffer,
               "Shouldn't have both present!");
  }

  operator nsString&()
  {
    return AsAString();
  }

  nsString& AsAString()
  {
    MOZ_ASSERT(!mStringBuffer, "We already have a stringbuffer?");
    MOZ_ASSERT(!mIsNull, "We're already set as null");
    if (mString.empty()) {
      mString.construct();
    }
    return mString.ref();
  }

  bool HasStringBuffer() const
  {
    MOZ_ASSERT(mString.empty() || !mStringBuffer,
               "Shouldn't have both present!");
    MOZ_ASSERT(!mIsNull, "Caller should have checked IsNull() first");
    return mString.empty();
  }

  // Get the stringbuffer.  This can only be called if HasStringBuffer()
  // returned true and StringBufferLength() is nonzero.  If that's true, it will
  // never return null.
  nsStringBuffer* StringBuffer() const
  {
    MOZ_ASSERT(!mIsNull, "Caller should have checked IsNull() first");
    MOZ_ASSERT(HasStringBuffer(),
               "Don't ask for the stringbuffer if we don't have it");
    MOZ_ASSERT(StringBufferLength() != 0, "Why are you asking for this?");
    MOZ_ASSERT(mStringBuffer,
               "If our length is nonzero, we better have a stringbuffer.");
    return mStringBuffer;
  }

  // Get the length of the stringbuffer.  Can only be called if
  // HasStringBuffer().
  uint32_t StringBufferLength() const
  {
    MOZ_ASSERT(HasStringBuffer(), "Don't call this if there is no stringbuffer");
    return mLength;
  }

  void SetStringBuffer(nsStringBuffer* aStringBuffer, uint32_t aLength)
  {
    MOZ_ASSERT(mString.empty(), "We already have a string?");
    MOZ_ASSERT(!mIsNull, "We're already set as null");
    MOZ_ASSERT(!mStringBuffer, "Setting stringbuffer twice?");
    MOZ_ASSERT(aStringBuffer, "Why are we getting null?");
    mStringBuffer = aStringBuffer;
    mLength = aLength;
  }

  void SetNull()
  {
    MOZ_ASSERT(!mStringBuffer, "Should have no stringbuffer if null");
    MOZ_ASSERT(mString.empty(), "Should have no string if null");
    mIsNull = true;
  }

  bool IsNull() const
  {
    MOZ_ASSERT(!mStringBuffer || mString.empty(),
               "How could we have a stringbuffer and a nonempty string?");
    return mIsNull || (!mString.empty() && mString.ref().IsVoid());
  }

  void ToString(nsAString& aString)
  {
    if (IsNull()) {
      SetDOMStringToNull(aString);
    } else if (HasStringBuffer()) {
      if (StringBufferLength() == 0) {
        aString.Truncate();
      } else {
        StringBuffer()->ToString(StringBufferLength(), aString);
      }
    } else {
      aString = AsAString();
    }
  }

private:
  // We need to be able to act like a string as needed
  Maybe<nsString> mString;

  // For callees that know we exist, we can be a stringbuffer/length/null-flag
  // triple.
  nsStringBuffer* mStringBuffer;
  uint32_t mLength;
  bool mIsNull;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_BindingDeclarations_h__
