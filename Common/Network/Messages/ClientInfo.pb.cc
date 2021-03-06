// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: ClientInfo.proto

#include "ClientInfo.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace Base {
PROTOBUF_CONSTEXPR ClientInfo::ClientInfo(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.name_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_.listengroup_)*/0u
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct ClientInfoDefaultTypeInternal {
  PROTOBUF_CONSTEXPR ClientInfoDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~ClientInfoDefaultTypeInternal() {}
  union {
    ClientInfo _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 ClientInfoDefaultTypeInternal _ClientInfo_default_instance_;
}  // namespace Base
static ::_pb::Metadata file_level_metadata_ClientInfo_2eproto[1];
static constexpr ::_pb::EnumDescriptor const** file_level_enum_descriptors_ClientInfo_2eproto = nullptr;
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_ClientInfo_2eproto = nullptr;

const uint32_t TableStruct_ClientInfo_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::Base::ClientInfo, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::Base::ClientInfo, _impl_.name_),
  PROTOBUF_FIELD_OFFSET(::Base::ClientInfo, _impl_.listengroup_),
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::Base::ClientInfo)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::Base::_ClientInfo_default_instance_._instance,
};

const char descriptor_table_protodef_ClientInfo_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\020ClientInfo.proto\022\004Base\"/\n\nClientInfo\022\014"
  "\n\004name\030\001 \001(\t\022\023\n\013listenGroup\030\002 \001(\rb\006proto"
  "3"
  ;
static ::_pbi::once_flag descriptor_table_ClientInfo_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_ClientInfo_2eproto = {
    false, false, 81, descriptor_table_protodef_ClientInfo_2eproto,
    "ClientInfo.proto",
    &descriptor_table_ClientInfo_2eproto_once, nullptr, 0, 1,
    schemas, file_default_instances, TableStruct_ClientInfo_2eproto::offsets,
    file_level_metadata_ClientInfo_2eproto, file_level_enum_descriptors_ClientInfo_2eproto,
    file_level_service_descriptors_ClientInfo_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_ClientInfo_2eproto_getter() {
  return &descriptor_table_ClientInfo_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_ClientInfo_2eproto(&descriptor_table_ClientInfo_2eproto);
namespace Base {

// ===================================================================

class ClientInfo::_Internal {
 public:
};

ClientInfo::ClientInfo(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:Base.ClientInfo)
}
ClientInfo::ClientInfo(const ClientInfo& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  ClientInfo* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.name_){}
    , decltype(_impl_.listengroup_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  _impl_.name_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.name_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_name().empty()) {
    _this->_impl_.name_.Set(from._internal_name(), 
      _this->GetArenaForAllocation());
  }
  _this->_impl_.listengroup_ = from._impl_.listengroup_;
  // @@protoc_insertion_point(copy_constructor:Base.ClientInfo)
}

inline void ClientInfo::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.name_){}
    , decltype(_impl_.listengroup_){0u}
    , /*decltype(_impl_._cached_size_)*/{}
  };
  _impl_.name_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.name_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
}

ClientInfo::~ClientInfo() {
  // @@protoc_insertion_point(destructor:Base.ClientInfo)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void ClientInfo::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  _impl_.name_.Destroy();
}

void ClientInfo::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void ClientInfo::Clear() {
// @@protoc_insertion_point(message_clear_start:Base.ClientInfo)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.name_.ClearToEmpty();
  _impl_.listengroup_ = 0u;
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* ClientInfo::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // string name = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          auto str = _internal_mutable_name();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
          CHK_(::_pbi::VerifyUTF8(str, "Base.ClientInfo.name"));
        } else
          goto handle_unusual;
        continue;
      // uint32 listenGroup = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 16)) {
          _impl_.listengroup_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* ClientInfo::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:Base.ClientInfo)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // string name = 1;
  if (!this->_internal_name().empty()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_name().data(), static_cast<int>(this->_internal_name().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "Base.ClientInfo.name");
    target = stream->WriteStringMaybeAliased(
        1, this->_internal_name(), target);
  }

  // uint32 listenGroup = 2;
  if (this->_internal_listengroup() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteUInt32ToArray(2, this->_internal_listengroup(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:Base.ClientInfo)
  return target;
}

size_t ClientInfo::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:Base.ClientInfo)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string name = 1;
  if (!this->_internal_name().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_name());
  }

  // uint32 listenGroup = 2;
  if (this->_internal_listengroup() != 0) {
    total_size += ::_pbi::WireFormatLite::UInt32SizePlusOne(this->_internal_listengroup());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData ClientInfo::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    ClientInfo::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*ClientInfo::GetClassData() const { return &_class_data_; }


void ClientInfo::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<ClientInfo*>(&to_msg);
  auto& from = static_cast<const ClientInfo&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:Base.ClientInfo)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_name().empty()) {
    _this->_internal_set_name(from._internal_name());
  }
  if (from._internal_listengroup() != 0) {
    _this->_internal_set_listengroup(from._internal_listengroup());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void ClientInfo::CopyFrom(const ClientInfo& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:Base.ClientInfo)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool ClientInfo::IsInitialized() const {
  return true;
}

void ClientInfo::InternalSwap(ClientInfo* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.name_, lhs_arena,
      &other->_impl_.name_, rhs_arena
  );
  swap(_impl_.listengroup_, other->_impl_.listengroup_);
}

::PROTOBUF_NAMESPACE_ID::Metadata ClientInfo::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_ClientInfo_2eproto_getter, &descriptor_table_ClientInfo_2eproto_once,
      file_level_metadata_ClientInfo_2eproto[0]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace Base
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::Base::ClientInfo*
Arena::CreateMaybeMessage< ::Base::ClientInfo >(Arena* arena) {
  return Arena::CreateMessageInternal< ::Base::ClientInfo >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
