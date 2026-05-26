// SPDX-License-Identifier: MIT
#pragma once

// ============================================================================
// Snapdragon 8 Gen 2 (SM8550) / Adreno 740 micro-architecture constants.
//
// This header collects MIDR values, cache geometry, and micro-architectural
// notes for the Qualcomm SM8550 SoC used in devices such as the OnePlus 12R
// (codename "aston").  It is consumed by HostFeatures detection and the JIT
// backend to enable targeted optimisations.
//
// CPU cluster topology
// --------------------
//   Core         Count  MIDR impl  MIDR part  Clock    ARMv8 revision
//   Cortex-X3    1      0x41       0xd4e      3.2 GHz  ARMv9.0-A
//   Cortex-A715  2      0x41       0xd4d      2.8 GHz  ARMv9.0-A
//   Cortex-A710  2      0x41       0xd47      2.8 GHz  ARMv9.0-A
//   Cortex-A510  3      0x41       0xd46      2.0 GHz  ARMv9.0-A
//
// All cores are ARM-implemented (implementer = 0x41 = 'A').
// Qualcomm does not use its own Kryo micro-architecture on this SoC.
//
// ISA extensions available on SM8550
// ------------------------------------
//  Feature      Cores         Notes
//  FEAT_DotProd X3,A715,A710,A510  SDOT/UDOT  (8-bit dot product → 32-bit)
//  FEAT_I8MM    X3,A715,A710   SMMLA/UMMLA/USMMLA (8-bit matrix multiply)
//  FEAT_BF16    X3,A715        BFDOT/BFMMLA/BFMLALB/BFMLALT
//  FEAT_SVE2    all            128-bit vector length (no 256-bit SVE on SM8550)
//  FEAT_MOPS    X3,A715,A710   SETP/SETM/SETE, CPYP/CPYM/CPYE
//  FEAT_RPRES   X3,A715        Reciprocal Precision (1/x, 1/sqrt(x) higher accuracy)
//  FEAT_AFP     all            Alternate Float Policy (FPCR.AH etc.)
//  FEAT_CSSC    X3             Common Short Sequence Compression (ABS, SMAX, UMIN …)
//  FEAT_WFxt    X3,A715        WFET/WFIT timer-driven wait
//  FEAT_RPRFM   X3             Range Prefetch Memory hint
//  FEAT_LRCPC   all            LDAPR family
//  FEAT_LRCPC2  all (errata)   LDAPUR executes with full acquire ordering — disabled
//  FEAT_LSE     all            Atomic operations (LDADD, STADD, CAS …)
//  FEAT_CRC32   all            CRC32/CRC32C hardware assist
//  FEAT_AES     all            AES/PMULL encryption extensions
//  FEAT_SHA2    all            SHA-256 hardware
//  FEAT_SHA3    X3,A715        SHA-512 / SHA-3 hardware
//
// Cache hierarchy
// ----------------
//  Level  Type          Size     Line size  Shared
//  L1D    per-core      64 KB    64 bytes   No
//  L1I    per-core      64 KB    64 bytes   No
//  L2     per-cluster   512 KB   64 bytes   Within cluster
//  L3     system (SLC)  8 MB     64 bytes   CPU ∪ Adreno 740
//
//  DC ZVA block size = 64 bytes  (matches cache line; SupportsCLZERO = true).
//
// Adreno 740 GPU / SLC coherency
// --------------------------------
// The Adreno 740 is connected to the CPU via Qualcomm's System Cache
// Interconnect (SCI), which makes the 8 MB SLC a hardware-coherent last-level
// cache shared between all CPU clusters and the GPU.
//
// Consequences for FEX:
//  1. CPU stores that reach the SLC are immediately visible to the GPU.
//     The synchronisation boundary is at Inner Shareable level, so:
//       DSB ISH   — sufficient for CPU→GPU sync in cached access mode.
//       DSB OSH   — not required (the SLC sits inside the IS domain on SM8550).
//  2. Prefetching to ARM L3 (PRFM PLDL3KEEP) brings data into the SLC, making
//     it warm for the GPU without an extra cache-flush step.
//  3. x86 PREFETCHNTA maps to PLDL1STRM by default; on SM8550 we remap it to
//     PLDL3KEEP so that NTA-hinted data lands in the SLC (GPU-visible) rather
//     than being marked for early L1/L2 eviction.
//
// Cortex-X3 micro-architecture notes
// -------------------------------------
//  Decode width  : 8 instructions/cycle
//  Issue width   : 10 μops/cycle
//  ROB size      : ~288 entries (est.)
//  Branch pred   : TAGE + indirect BTB
//  Loop alignment: 64-byte boundaries preferred for decode-fetch efficiency.
//                  The frontend fetches a full cache line per cycle, so a loop
//                  header crossing a 64-byte boundary incurs an extra fetch.
//  ASIMD units   : 2 × 128-bit FMLA/FADD/FMUL
//                  2 × 128-bit UDOT/SDOT/UMMLA/SMMLA  (I8MM path)
//                  2 × 128-bit ADDP/ADDV/UMULL/SMULL
//  Load/Store    : 3 loads + 2 stores per cycle
//
// Cortex-A715 micro-architecture notes
// ---------------------------------------
//  Decode width  : 6 instructions/cycle
//  ASIMD units   : 2 × 128-bit (shared with integer)
//  I8MM          : yes (SMMLA/UMMLA throughput: 1 per 2 cycles)
//  BF16          : yes
//  SVE2 VL       : 128-bit
//
// Cortex-A710 micro-architecture notes
// ---------------------------------------
//  Decode width  : 6 instructions/cycle
//  I8MM          : yes
//  BF16          : no (BF16 is optional on A710; not present on SM8550 A710)
//  SVE2 VL       : 128-bit
//
// Cortex-A510 micro-architecture notes
// ---------------------------------------
//  Decode width  : 4 instructions/cycle (dual-issue, 2 wide × 2 clusters)
//  I8MM          : yes (but slower than X3/A715)
//  BF16          : no
//  SVE2 VL       : 128-bit
//  Note: A510 runs as a merged pair internally; two logical cores share
//        one L1 cache and ASIMD unit.
// ============================================================================

#include <cstdint>

namespace FEX::SnapdragonGen2 {

// ---------------------------------------------------------------------------
// MIDR (Main ID Register) constants for SM8550 cores.
// MIDR layout: [31:24] Implementer | [23:20] Variant | [19:16] Architecture
//              [15:4]  PartNum     | [3:0]   Revision
// ---------------------------------------------------------------------------

constexpr uint32_t MIDR_IMPLEMENTER_ARM  = 0x41u; ///< ARM Ltd.

/// Cortex-X3  — 1× prime core @ 3.2 GHz.  Supports I8MM, BF16, RPRES, RPRFM, CSSC, WFxt.
constexpr uint32_t MIDR_PARTNUM_X3       = 0xd4eu;

/// Cortex-A715 — 2× performance-big @ 2.8 GHz.  Supports I8MM, BF16, RPRES, WFxt.
constexpr uint32_t MIDR_PARTNUM_A715     = 0xd4du;

/// Cortex-A710 — 2× performance-mid @ 2.8 GHz.  Supports I8MM; no BF16, RPRES.
constexpr uint32_t MIDR_PARTNUM_A710     = 0xd47u;

/// Cortex-A510 — 3× efficiency @ 2.0 GHz.  Supports I8MM; no BF16, RPRES.
constexpr uint32_t MIDR_PARTNUM_A510     = 0xd46u;

/// Expected core count for SM8550: X3(1) + A715(2) + A710(2) + A510(3) = 8.
constexpr uint32_t EXPECTED_CORE_COUNT   = 8u;

// ---------------------------------------------------------------------------
// Cache geometry.
// ---------------------------------------------------------------------------

/// D-cache and I-cache line size on all SM8550 cores (bytes).
constexpr uint32_t CACHELINE_BYTES       = 64u;

/// DC ZVA block size (bytes).  Matches CACHELINE_BYTES, so a single DC ZVA
/// zeroes exactly one cache line.
constexpr uint32_t DCZVA_BLOCK_BYTES     = 64u;

/// L3 / System Level Cache (SLC) size (bytes), shared CPU + Adreno 740.
constexpr uint32_t SLC_SIZE_BYTES        = 8u * 1024u * 1024u; // 8 MB

// ---------------------------------------------------------------------------
// Helper: extract MIDR fields.
// ---------------------------------------------------------------------------

inline constexpr uint32_t MIDRImplementer(uint32_t midr) noexcept {
  return (midr >> 24u) & 0xFFu;
}

inline constexpr uint32_t MIDRPartNum(uint32_t midr) noexcept {
  return (midr >> 4u) & 0xFFFu;
}

// ---------------------------------------------------------------------------
// Helper: classify a single MIDR value as one of the SM8550 core types.
// Returns true if the MIDR matches ANY SM8550 core.
// ---------------------------------------------------------------------------

inline constexpr bool IsSM8550Core(uint32_t midr) noexcept {
  if (MIDRImplementer(midr) != MIDR_IMPLEMENTER_ARM) {
    return false;
  }
  const uint32_t part = MIDRPartNum(midr);
  return part == MIDR_PARTNUM_X3  || part == MIDR_PARTNUM_A715 ||
         part == MIDR_PARTNUM_A710 || part == MIDR_PARTNUM_A510;
}

// ---------------------------------------------------------------------------
// Adreno 740 SLC coherency note (compile-time documentation only).
//
// On SM8550, the following barrier choices apply when synchronising data
// between the CPU JIT and any Vulkan/OpenGL ES compute shader on the GPU:
//
//   CPU writes → GPU visible:
//     All stores to cacheable memory that have retired from the CPU store
//     buffer are GPU-visible once they reach the SLC.  A DSB ISH ensures
//     the CPU store buffer is drained before the GPU command buffer begins.
//
//   GPU writes → CPU visible:
//     The Adreno driver issues a GPU cache-flush + memory barrier before
//     signalling the CPU fence.  The CPU only needs a DMB ISH (or acquire
//     load on the fence variable) to observe GPU writes.
//
// This is an implementation detail of the Qualcomm SCI fabric and is not
// architecturally guaranteed for other SoCs, so these optimisations are
// gated behind HostSupportsSnapdragonGen2.
// ---------------------------------------------------------------------------

} // namespace FEX::SnapdragonGen2
