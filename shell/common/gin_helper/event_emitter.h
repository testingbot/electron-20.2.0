// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_H_
#define SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_H_

#include <utility>
#include <vector>

#include "base/optional.h"
#include "electron/shell/common/api/api.mojom.h"
#include "shell/common/gin_helper/event_emitter_caller.h"

namespace gin_helper {

namespace internal {

v8::Local<v8::Object> CreateEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> sender = v8::Local<v8::Object>(),
    v8::Local<v8::Object> custom_event = v8::Local<v8::Object>());
v8::Local<v8::Object> CreateEventFromFlags(v8::Isolate* isolate, int flags);

}  // namespace internal

// Provide helperers to emit event in JavaScript.
//
// TODO(zcbenz): Inherit from Wrappable directly after removing native_mate.
template <typename Base>
class EventEmitter : public Base {
 public:
  typedef std::vector<v8::Local<v8::Value>> ValueArray;

  // Make the convinient methods visible:
  // https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members
  v8::Isolate* isolate() const { return Base::isolate(); }
  v8::Local<v8::Object> GetWrapper() const { return Base::GetWrapper(); }
  v8::MaybeLocal<v8::Object> GetWrapper(v8::Isolate* isolate) const {
    return Base::GetWrapper(isolate);
  }

  // this.emit(name, event, args...);
  template <typename... Args>
  bool EmitCustomEvent(base::StringPiece name,
                       v8::Local<v8::Object> event,
                       Args&&... args) {
    return EmitWithEvent(name,
                         internal::CreateEvent(isolate(), GetWrapper(), event),
                         std::forward<Args>(args)...);
  }

  // this.emit(name, new Event(flags), args...);
  template <typename... Args>
  bool EmitWithFlags(base::StringPiece name, int flags, Args&&... args) {
    return EmitCustomEvent(name,
                           internal::CreateEventFromFlags(isolate(), flags),
                           std::forward<Args>(args)...);
  }

  // this.emit(name, new Event(), args...);
  template <typename... Args>
  bool Emit(base::StringPiece name, Args&&... args) {
    v8::Locker locker(isolate());
    v8::HandleScope handle_scope(isolate());
    v8::Local<v8::Object> wrapper = GetWrapper();
    if (wrapper.IsEmpty()) {
      return false;
    }
    v8::Local<v8::Object> event = internal::CreateEvent(isolate(), wrapper);
    return EmitWithEvent(name, event, std::forward<Args>(args)...);
  }

 protected:
  EventEmitter() {}

 private:
  // this.emit(name, event, args...);
  template <typename... Args>
  bool EmitWithEvent(base::StringPiece name,
                     v8::Local<v8::Object> event,
                     Args&&... args) {
    // It's possible that |this| will be deleted by EmitEvent, so save anything
    // we need from |this| before calling EmitEvent.
    auto* isolate = this->isolate();
    v8::Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    auto context = isolate->GetCurrentContext();
    gin_helper::EmitEvent(isolate, GetWrapper(), name, event,
                          std::forward<Args>(args)...);
    v8::Local<v8::Value> defaultPrevented;
    if (event->Get(context, gin::StringToV8(isolate, "defaultPrevented"))
            .ToLocal(&defaultPrevented)) {
      return defaultPrevented->BooleanValue(isolate);
    }
    return false;
  }

  DISALLOW_COPY_AND_ASSIGN(EventEmitter);
};

}  // namespace gin_helper

#endif  // SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_H_