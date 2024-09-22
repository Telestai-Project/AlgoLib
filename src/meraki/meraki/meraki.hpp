// meraki: C/C++ implementation of Meraki, the Telestai Proof of Work algorithm.
// Copyright 2018-2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

/// @file
///
/// API design decisions:
///
/// 1. Signed integer type is used whenever the size of the type is not
///    restricted by the Meraki specification.
///    See http://www.aristeia.com/Papers/C++ReportColumns/sep95.pdf.
///    See https://stackoverflow.com/questions/10168079/why-is-size-t-unsigned/.
///    See https://github.com/Microsoft/GSL/issues/171.

#pragma once

#include "meraki/meraki/meraki.h"
#include "meraki/support/hash_types.hpp"

#include <cstdint>
#include <cstring>
#include <cstddef>
#include <memory>

namespace meraki
{
static const int epoch_length = MERAKI_EPOCH_LENGTH;
static const int light_cache_item_size = MERAKI_LIGHT_CACHE_ITEM_SIZE;
static const int full_dataset_item_size = MERAKI_FULL_DATASET_ITEM_SIZE;
static const int num_dataset_accesses = MERAKI_NUM_DATASET_ACCESSES;

using epoch_context = meraki_epoch_context;
using epoch_context_full = meraki_epoch_context_full;

using result = meraki_result;

/// Constructs a 256-bit hash from an array of bytes.
///
/// @param bytes  A pointer to array of at least 32 bytes.
/// @return       The constructed hash.
inline meraki_hash256 hash256_from_bytes(const uint8_t bytes[32]) NOEXCEPT
{
    meraki_hash256 h;
    std::memcpy(&h, bytes, sizeof(h));
    return h;
}

struct search_result
{
    bool solution_found = false;
    uint64_t nonce = 0;
    meraki_hash256 final_hash = {};
    meraki_hash256 mix_hash = {};

    search_result() NOEXCEPT = default;

    search_result(result res, uint64_t n) NOEXCEPT
      : solution_found(true), nonce(n), final_hash(res.final_hash), mix_hash(res.mix_hash)
    {}
};

/// Calculates the epoch number out of the block number.
inline constexpr int get_epoch_number(int block_number) NOEXCEPT
{
    return block_number / epoch_length;
}

/**
 * Coverts the number of items of a light cache to size in bytes.
 *
 * @param num_items  The number of items in the light cache.
 * @return           The size of the light cache in bytes.
 */
inline constexpr size_t get_light_cache_size(int num_items) NOEXCEPT
{
    return static_cast<size_t>(num_items) * light_cache_item_size;
}

/**
 * Coverts the number of items of a full dataset to size in bytes.
 *
 * @param num_items  The number of items in the full dataset.
 * @return           The size of the full dataset in bytes.
 */
inline constexpr uint64_t get_full_dataset_size(int num_items) NOEXCEPT
{
    return static_cast<uint64_t>(num_items) * full_dataset_item_size;
}

/// Owned unique pointer to an epoch context.
using epoch_context_ptr = std::unique_ptr<epoch_context, decltype(&meraki_destroy_epoch_context)>;

using epoch_context_full_ptr =
    std::unique_ptr<epoch_context_full, decltype(&meraki_destroy_epoch_context_full)>;

/// Creates Meraki epoch context.
///
/// This is a wrapper for meraki_create_epoch_number C function that returns
/// the context as a smart pointer which handles the destruction of the context.
inline epoch_context_ptr create_epoch_context(int epoch_number) NOEXCEPT
{
    return {meraki_create_epoch_context(epoch_number), meraki_destroy_epoch_context};
}

inline epoch_context_full_ptr create_epoch_context_full(int epoch_number) NOEXCEPT
{
    return {meraki_create_epoch_context_full(epoch_number), meraki_destroy_epoch_context_full};
}


inline result hash(
    const epoch_context& context, const meraki_hash256& header_hash, uint64_t nonce) NOEXCEPT
{
    return meraki_hash(&context, &header_hash, nonce);
}

result hash(const epoch_context_full& context, const meraki_hash256& header_hash, uint64_t nonce) NOEXCEPT;

inline bool verify_final_hash(const meraki_hash256& header_hash, const meraki_hash256& mix_hash, uint64_t nonce,
    const meraki_hash256& boundary) NOEXCEPT
{
    return meraki_verify_final_hash(&header_hash, &mix_hash, nonce, &boundary);
}

inline bool verify(const epoch_context& context, const meraki_hash256& header_hash, const meraki_hash256& mix_hash,
    uint64_t nonce, const meraki_hash256& boundary) NOEXCEPT
{
    return meraki_verify(&context, &header_hash, &mix_hash, nonce, &boundary);
}

search_result search_light(const epoch_context& context, const meraki_hash256& header_hash,
    const meraki_hash256& boundary, uint64_t start_nonce, size_t iterations) NOEXCEPT;

search_result search(const epoch_context_full& context, const meraki_hash256& header_hash,
    const meraki_hash256& boundary, uint64_t start_nonce, size_t iterations) NOEXCEPT;


/// Tries to find the epoch number matching the given seed hash.
///
/// Mining pool protocols (many variants of stratum and "getwork") send out
/// seed hash instead of epoch number to workers. This function tries to recover
/// the epoch number from this seed hash.
///
/// @param seed  Meraki seed hash.
/// @return      The epoch number or -1 if not found.
int find_epoch_number(const meraki_hash256& seed) NOEXCEPT;


/// Get global shared epoch context.
inline const epoch_context& get_global_epoch_context(int epoch_number) NOEXCEPT
{
    return *meraki_get_global_epoch_context(epoch_number);
}

/// Get global shared epoch context with full dataset initialized.
inline const epoch_context_full& get_global_epoch_context_full(int epoch_number) NOEXCEPT
{
    return *meraki_get_global_epoch_context_full(epoch_number);
}
}  // namespace meraki
