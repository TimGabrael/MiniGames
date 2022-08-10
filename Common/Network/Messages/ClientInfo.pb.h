// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: ClientInfo.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_ClientInfo_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_ClientInfo_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021001 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_ClientInfo_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_ClientInfo_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_ClientInfo_2eproto;
namespace Base {
class ClientInfo;
struct ClientInfoDefaultTypeInternal;
extern ClientInfoDefaultTypeInternal _ClientInfo_default_instance_;
}  // namespace Base
PROTOBUF_NAMESPACE_OPEN
template<> ::Base::ClientInfo* Arena::CreateMaybeMessage<::Base::ClientInfo>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace Base {

// ===================================================================

class ClientInfo final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:Base.ClientInfo) */ {
 public:
  inline ClientInfo() : ClientInfo(nullptr) {}
  ~ClientInfo() override;
  explicit PROTOBUF_CONSTEXPR ClientInfo(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  ClientInfo(const ClientInfo& from);
  ClientInfo(ClientInfo&& from) noexcept
    : ClientInfo() {
    *this = ::std::move(from);
  }

  inline ClientInfo& operator=(const ClientInfo& from) {
    CopyFrom(from);
    return *this;
  }
  inline ClientInfo& operator=(ClientInfo&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const ClientInfo& default_instance() {
    return *internal_default_instance();
  }
  static inline const ClientInfo* internal_default_instance() {
    return reinterpret_cast<const ClientInfo*>(
               &_ClientInfo_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(ClientInfo& a, ClientInfo& b) {
    a.Swap(&b);
  }
  inline void Swap(ClientInfo* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(ClientInfo* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  ClientInfo* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<ClientInfo>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const ClientInfo& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const ClientInfo& from) {
    ClientInfo::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(ClientInfo* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "Base.ClientInfo";
  }
  protected:
  explicit ClientInfo(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kNameFieldNumber = 1,
    kListenGroupFieldNumber = 2,
    kIdFieldNumber = 3,
  };
  // string name = 1;
  void clear_name();
  const std::string& name() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_name(ArgT0&& arg0, ArgT... args);
  std::string* mutable_name();
  PROTOBUF_NODISCARD std::string* release_name();
  void set_allocated_name(std::string* name);
  private:
  const std::string& _internal_name() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_name(const std::string& value);
  std::string* _internal_mutable_name();
  public:

  // uint32 listenGroup = 2;
  void clear_listengroup();
  uint32_t listengroup() const;
  void set_listengroup(uint32_t value);
  private:
  uint32_t _internal_listengroup() const;
  void _internal_set_listengroup(uint32_t value);
  public:

  // uint32 id = 3;
  void clear_id();
  uint32_t id() const;
  void set_id(uint32_t value);
  private:
  uint32_t _internal_id() const;
  void _internal_set_id(uint32_t value);
  public:

  // @@protoc_insertion_point(class_scope:Base.ClientInfo)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr name_;
    uint32_t listengroup_;
    uint32_t id_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_ClientInfo_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// ClientInfo

// string name = 1;
inline void ClientInfo::clear_name() {
  _impl_.name_.ClearToEmpty();
}
inline const std::string& ClientInfo::name() const {
  // @@protoc_insertion_point(field_get:Base.ClientInfo.name)
  return _internal_name();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void ClientInfo::set_name(ArgT0&& arg0, ArgT... args) {
 
 _impl_.name_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:Base.ClientInfo.name)
}
inline std::string* ClientInfo::mutable_name() {
  std::string* _s = _internal_mutable_name();
  // @@protoc_insertion_point(field_mutable:Base.ClientInfo.name)
  return _s;
}
inline const std::string& ClientInfo::_internal_name() const {
  return _impl_.name_.Get();
}
inline void ClientInfo::_internal_set_name(const std::string& value) {
  
  _impl_.name_.Set(value, GetArenaForAllocation());
}
inline std::string* ClientInfo::_internal_mutable_name() {
  
  return _impl_.name_.Mutable(GetArenaForAllocation());
}
inline std::string* ClientInfo::release_name() {
  // @@protoc_insertion_point(field_release:Base.ClientInfo.name)
  return _impl_.name_.Release();
}
inline void ClientInfo::set_allocated_name(std::string* name) {
  if (name != nullptr) {
    
  } else {
    
  }
  _impl_.name_.SetAllocated(name, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.name_.IsDefault()) {
    _impl_.name_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:Base.ClientInfo.name)
}

// uint32 listenGroup = 2;
inline void ClientInfo::clear_listengroup() {
  _impl_.listengroup_ = 0u;
}
inline uint32_t ClientInfo::_internal_listengroup() const {
  return _impl_.listengroup_;
}
inline uint32_t ClientInfo::listengroup() const {
  // @@protoc_insertion_point(field_get:Base.ClientInfo.listenGroup)
  return _internal_listengroup();
}
inline void ClientInfo::_internal_set_listengroup(uint32_t value) {
  
  _impl_.listengroup_ = value;
}
inline void ClientInfo::set_listengroup(uint32_t value) {
  _internal_set_listengroup(value);
  // @@protoc_insertion_point(field_set:Base.ClientInfo.listenGroup)
}

// uint32 id = 3;
inline void ClientInfo::clear_id() {
  _impl_.id_ = 0u;
}
inline uint32_t ClientInfo::_internal_id() const {
  return _impl_.id_;
}
inline uint32_t ClientInfo::id() const {
  // @@protoc_insertion_point(field_get:Base.ClientInfo.id)
  return _internal_id();
}
inline void ClientInfo::_internal_set_id(uint32_t value) {
  
  _impl_.id_ = value;
}
inline void ClientInfo::set_id(uint32_t value) {
  _internal_set_id(value);
  // @@protoc_insertion_point(field_set:Base.ClientInfo.id)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace Base

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_ClientInfo_2eproto
