/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

namespace {

constexpr uint8_t kZCashSigHashAll = 0x01;

}  // namespace

ZCashTransaction::ZCashTransaction() = default;
ZCashTransaction::~ZCashTransaction() = default;
ZCashTransaction& ZCashTransaction::operator=(const ZCashTransaction& other) =
    default;
ZCashTransaction::ZCashTransaction(const ZCashTransaction& other) = default;
ZCashTransaction::ZCashTransaction(ZCashTransaction&& other) = default;
ZCashTransaction& ZCashTransaction::operator=(ZCashTransaction&& other) =
    default;
bool ZCashTransaction::operator==(const ZCashTransaction& other) const {
  return std::tie(this->transparent_part_, this->orchard_part_, this->locktime_,
                  this->to_, this->amount_, this->fee_, this->memo()) ==
         std::tie(other.transparent_part_, this->orchard_part_, other.locktime_,
                  other.to_, other.amount_, other.fee_, other.memo_);
}
bool ZCashTransaction::operator!=(const ZCashTransaction& other) const {
  return !(*this == other);
}

ZCashTransaction::Outpoint::Outpoint() = default;
ZCashTransaction::Outpoint::~Outpoint() = default;
ZCashTransaction::Outpoint::Outpoint(const Outpoint& other) = default;
ZCashTransaction::Outpoint& ZCashTransaction::Outpoint::operator=(
    const Outpoint& other) = default;
ZCashTransaction::Outpoint::Outpoint(Outpoint&& other) = default;
ZCashTransaction::Outpoint& ZCashTransaction::Outpoint::operator=(
    Outpoint&& other) = default;
bool ZCashTransaction::Outpoint::operator==(
    const ZCashTransaction::Outpoint& other) const {
  return std::tie(this->txid, this->index) == std::tie(other.txid, other.index);
}
bool ZCashTransaction::Outpoint::operator!=(
    const ZCashTransaction::Outpoint& other) const {
  return !(*this == other);
}

ZCashTransaction::OrchardPart::OrchardPart() = default;
ZCashTransaction::OrchardPart::~OrchardPart() = default;
ZCashTransaction::OrchardPart::OrchardPart(OrchardPart&& other) = default;
ZCashTransaction::OrchardPart::OrchardPart(const OrchardPart& other) = default;
ZCashTransaction::OrchardPart& ZCashTransaction::OrchardPart::operator=(
    OrchardPart&& other) = default;
ZCashTransaction::OrchardPart& ZCashTransaction::OrchardPart::operator=(
    const OrchardPart& other) = default;
bool ZCashTransaction::OrchardPart::operator==(const OrchardPart& other) const {
  return std::tie(this->digest, this->outputs, this->raw_tx) ==
         std::tie(other.digest, other.outputs, other.raw_tx);
}
bool ZCashTransaction::OrchardPart::operator!=(const OrchardPart& other) const {
  return !(*this == other);
}

ZCashTransaction::TransparentPart::TransparentPart() = default;
ZCashTransaction::TransparentPart::~TransparentPart() = default;
ZCashTransaction::TransparentPart::TransparentPart(
    const TransparentPart& other) = default;
ZCashTransaction::TransparentPart::TransparentPart(TransparentPart&& other) =
    default;
ZCashTransaction::TransparentPart& ZCashTransaction::TransparentPart::operator=(
    TransparentPart&& other) = default;
ZCashTransaction::TransparentPart& ZCashTransaction::TransparentPart::operator=(
    const TransparentPart& other) = default;
bool ZCashTransaction::TransparentPart::operator==(
    const TransparentPart& other) const {
  return std::tie(this->inputs, this->outputs) ==
         std::tie(other.inputs, other.outputs);
}
bool ZCashTransaction::TransparentPart::operator!=(
    const TransparentPart& other) const {
  return !(*this == other);
}
bool ZCashTransaction::TransparentPart::IsEmpty() const {
  return inputs.empty() && outputs.empty();
}

base::Value::Dict ZCashTransaction::Outpoint::ToValue() const {
  base::Value::Dict dict;

  dict.Set("txid", base::HexEncode(txid));
  dict.Set("index", static_cast<int>(index));

  return dict;
}

// static
std::optional<ZCashTransaction::Outpoint> ZCashTransaction::Outpoint::FromValue(
    const base::Value::Dict& value) {
  Outpoint result;

  auto* txid_hex = value.FindString("txid");
  if (!txid_hex) {
    return std::nullopt;
  }
  if (!base::HexStringToSpan(*txid_hex, result.txid)) {
    return std::nullopt;
  }

  auto index_value = value.FindInt("index");
  if (!index_value) {
    return std::nullopt;
  }
  result.index = *index_value;

  return result;
}

ZCashTransaction::TxInput::TxInput() = default;
ZCashTransaction::TxInput::~TxInput() = default;
ZCashTransaction::TxInput::TxInput(const ZCashTransaction::TxInput& other) =
    default;
ZCashTransaction::TxInput::TxInput(ZCashTransaction::TxInput&& other) = default;
ZCashTransaction::TxInput& ZCashTransaction::TxInput::operator=(
    const ZCashTransaction::TxInput& other) = default;
ZCashTransaction::TxInput& ZCashTransaction::TxInput::operator=(
    ZCashTransaction::TxInput&& other) = default;
bool ZCashTransaction::TxInput::operator==(
    const ZCashTransaction::TxInput& other) const {
  return std::tie(this->utxo_address, this->utxo_outpoint, this->utxo_value,
                  this->script_sig, this->script_pub_key) ==
         std::tie(other.utxo_address, other.utxo_outpoint, other.utxo_value,
                  other.script_sig, other.script_pub_key);
}
bool ZCashTransaction::TxInput::operator!=(
    const ZCashTransaction::TxInput& other) const {
  return !(*this == other);
}

base::Value::Dict ZCashTransaction::TxInput::ToValue() const {
  base::Value::Dict dict;

  dict.Set("utxo_address", utxo_address);
  dict.Set("utxo_outpoint", utxo_outpoint.ToValue());
  dict.Set("utxo_value", base::NumberToString(utxo_value));
  dict.Set("script_pub_key", base::HexEncode(script_pub_key));
  dict.Set("script_sig", base::HexEncode(script_sig));

  return dict;
}

// static
std::optional<ZCashTransaction::TxInput> ZCashTransaction::TxInput::FromValue(
    const base::Value::Dict& value) {
  ZCashTransaction::TxInput result;

  if (!ReadStringTo(value, "utxo_address", result.utxo_address)) {
    return std::nullopt;
  }

  if (!ReadDictTo(value, "utxo_outpoint", result.utxo_outpoint)) {
    return std::nullopt;
  }

  if (!ReadUint64StringTo(value, "utxo_value", result.utxo_value)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo(value, "script_pub_key", result.script_pub_key)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo(value, "script_sig", result.script_sig)) {
    return std::nullopt;
  }

  return result;
}

// static
std::optional<ZCashTransaction::TxInput> ZCashTransaction::TxInput::FromRpcUtxo(
    const std::string& address,
    const zcash::mojom::ZCashUtxo& utxo) {
  if (address != utxo.address) {
    return std::nullopt;
  }
  ZCashTransaction::TxInput result;
  result.utxo_address = utxo.address;
  result.script_pub_key.insert(result.script_pub_key.begin(),
                               utxo.script.begin(), utxo.script.end());
  if (utxo.tx_id.size() != 32) {
    return std::nullopt;
  }
  std::copy_n(utxo.tx_id.begin(), 32, result.utxo_outpoint.txid.begin());
  result.utxo_outpoint.index = utxo.index;
  result.utxo_value = utxo.value_zat;
  return result;
}

bool ZCashTransaction::TxInput::IsSigned() const {
  return !script_sig.empty();
}

ZCashTransaction::TxOutput::TxOutput() = default;
ZCashTransaction::TxOutput::~TxOutput() = default;
ZCashTransaction::TxOutput::TxOutput(ZCashTransaction::TxOutput&& other) =
    default;
ZCashTransaction::TxOutput::TxOutput(const ZCashTransaction::TxOutput& other) =
    default;
ZCashTransaction::TxOutput& ZCashTransaction::TxOutput::operator=(
    ZCashTransaction::TxOutput&& other) = default;
ZCashTransaction::TxOutput& ZCashTransaction::TxOutput::operator=(
    const ZCashTransaction::TxOutput& other) = default;
bool ZCashTransaction::TxOutput::operator==(
    const ZCashTransaction::TxOutput& other) const {
  return std::tie(this->address, this->amount, this->script_pubkey) ==
         std::tie(other.address, other.amount, other.script_pubkey);
}
bool ZCashTransaction::TxOutput::operator!=(
    const ZCashTransaction::TxOutput& other) const {
  return !(*this == other);
}

base::Value::Dict ZCashTransaction::TxOutput::ToValue() const {
  base::Value::Dict dict;

  dict.Set("address", address);
  dict.Set("amount", base::NumberToString(amount));
  dict.Set("script_pub_key", base::HexEncode(script_pubkey));

  return dict;
}

// static
std::optional<ZCashTransaction::TxOutput> ZCashTransaction::TxOutput::FromValue(
    const base::Value::Dict& value) {
  ZCashTransaction::TxOutput result;
  if (!ReadStringTo(value, "address", result.address)) {
    return std::nullopt;
  }

  if (!ReadUint64StringTo(value, "amount", result.amount)) {
    return std::nullopt;
  }

  if (!ReadHexByteArrayTo(value, "script_pub_key", result.script_pubkey)) {
    return std::nullopt;
  }

  return result;
}

base::Value::Dict ZCashTransaction::ToValue() const {
  base::Value::Dict dict;

  auto& inputs_value = dict.Set("inputs", base::Value::List())->GetList();
  for (auto& input : transparent_part_.inputs) {
    inputs_value.Append(input.ToValue());
  }

  auto& orchard_inputs_value =
      dict.Set("orchard_inputs", base::Value::List())->GetList();
  for (auto& input : orchard_part_.inputs) {
    orchard_inputs_value.Append(input.ToValue());
  }

  auto& outputs_value = dict.Set("outputs", base::Value::List())->GetList();
  for (auto& output : transparent_part_.outputs) {
    outputs_value.Append(output.ToValue());
  }
  // TODO(cypt4): Add orchard part serialization\deserialization
  auto& orchard_outputs_value =
      dict.Set("orchard_outputs", base::Value::List())->GetList();
  for (auto& orchard_output : orchard_part_.outputs) {
    orchard_outputs_value.Append(orchard_output.ToValue());
  }

  if (orchard_part_.anchor_block_height) {
    dict.Set("anchor_block_height",
             base::NumberToString(orchard_part_.anchor_block_height.value()));
  }

  dict.Set("locktime", base::NumberToString(locktime_));
  dict.Set("to", to_);
  dict.Set("amount", base::NumberToString(amount_));
  dict.Set("fee", base::NumberToString(fee_));
  dict.Set("expiry_height", base::NumberToString(expiry_height_));
  if (memo_) {
    dict.Set("memo", base::HexEncode(memo_.value()));
  }

  return dict;
}

// static
std::optional<ZCashTransaction> ZCashTransaction::FromValue(
    const base::Value::Dict& value) {
  ZCashTransaction result;

  auto* inputs_list = value.FindList("inputs");
  auto* orchard_inputs_list = value.FindList("orchard_inputs");
  if (!inputs_list && !orchard_inputs_list) {
    return std::nullopt;
  }
  if (inputs_list) {
    for (auto& item : *inputs_list) {
      if (!item.is_dict()) {
        return std::nullopt;
      }
      auto input_opt = ZCashTransaction::TxInput::FromValue(item.GetDict());
      if (!input_opt) {
        return std::nullopt;
      }
      result.transparent_part_.inputs.push_back(std::move(*input_opt));
    }
  }

  if (orchard_inputs_list) {
    for (auto& item : *orchard_inputs_list) {
      if (!item.is_dict()) {
        return std::nullopt;
      }
      auto input_opt =
          ZCashTransaction::OrchardInput::FromValue(item.GetDict());
      if (!input_opt) {
        return std::nullopt;
      }
      result.orchard_part().inputs.push_back(std::move(*input_opt));
    }
  }

  auto* outputs_list = value.FindList("outputs");
  auto* orchard_outputs_list = value.FindList("orchard_outputs");
  if (!outputs_list && !orchard_outputs_list) {
    return std::nullopt;
  }

  if (outputs_list) {
    for (auto& item : *outputs_list) {
      if (!item.is_dict()) {
        return std::nullopt;
      }
      auto output_opt = ZCashTransaction::TxOutput::FromValue(item.GetDict());
      if (!output_opt) {
        return std::nullopt;
      }
      result.transparent_part_.outputs.push_back(std::move(*output_opt));
    }
  }

  if (orchard_outputs_list) {
    for (auto& item : *orchard_outputs_list) {
      if (!item.is_dict()) {
        return std::nullopt;
      }
      auto output_opt =
          ZCashTransaction::OrchardOutput::FromValue(item.GetDict());
      if (!output_opt) {
        return std::nullopt;
      }
      result.orchard_part_.outputs.push_back(std::move(*output_opt));
    }
  }

  auto* anchor = value.Find("anchor_block_height");
  if (anchor) {
    uint32_t anchor_block_height = 0;
    if (ReadUint32StringTo(value, "anchor_block_height", anchor_block_height)) {
      result.orchard_part().anchor_block_height = anchor_block_height;
    } else {
      return std::nullopt;
    }
  }

  if (!ReadUint32StringTo(value, "locktime", result.locktime_)) {
    return std::nullopt;
  }

  if (!ReadStringTo(value, "to", result.to_)) {
    return std::nullopt;
  }

  if (!ReadUint64StringTo(value, "amount", result.amount_)) {
    return std::nullopt;
  }

  if (!ReadUint64StringTo(value, "fee", result.fee_)) {
    return std::nullopt;
  }

  if (value.Find("expiry_height")) {
    if (!ReadUint32StringTo(value, "expiry_height", result.expiry_height_)) {
      return std::nullopt;
    }
  }

  if (value.Find("memo")) {
    OrchardMemo memo;
    if (!ReadHexByteArrayTo<kOrchardMemoSize>(value, "memo", memo)) {
      return std::nullopt;
    }
    result.memo_ = memo;
  }

  return result;
}

bool ZCashTransaction::IsTransparentPartSigned() const {
  if (transparent_part_.inputs.empty()) {
    return false;
  }

  return std::ranges::all_of(transparent_part_.inputs,
                             [](auto& input) { return input.IsSigned(); });
}

uint64_t ZCashTransaction::TotalInputsAmount() const {
  uint64_t result = 0;
  for (auto& input : transparent_part_.inputs) {
    result += input.utxo_value;
  }
  for (auto& input : orchard_part_.inputs) {
    result += input.note.amount;
  }
  return result;
}

uint8_t ZCashTransaction::sighash_type() const {
  // We always sign all inputs.
  return kZCashSigHashAll;
}

}  // namespace brave_wallet
