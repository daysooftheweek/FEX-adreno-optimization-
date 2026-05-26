// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/vector.h>
#include <cstdint>

namespace FEXCore {
struct HostFeatures {
  /**
   * @brief Backend features that change how codegen is generated from IR
   *
   * Specifically things that affect the IR->Codegen process
   * Not the x86->IR process
   */
  uint32_t DCacheLineSize {};
  uint32_t ICacheLineSize {};
  bool SupportsCacheMaintenanceOps {};
  bool SupportsAES {};
  bool SupportsCRC {};
  bool SupportsCLZERO {};
  bool SupportsAtomics {};
  bool SupportsRCPC {};
  bool SupportsTSOImm9 {};
  bool SupportsRAND {};
  bool SupportsAVX {};
  bool SupportsSVE128 {};
  bool SupportsSVE256 {};
  bool SupportsSHA {};
  bool SupportsPMULL_128Bit {};
  bool SupportsCSSC {};
  bool SupportsFCMA {};
  bool SupportsFlagM {};
  bool SupportsFlagM2 {};
  bool SupportsRPRES {};
  bool SupportsPreserveAllABI {};
  bool SupportsAES256 {};
  bool SupportsSVEBitPerm {};
  bool SupportsCPUIndexInTPIDRRO {};
  bool SupportsFRINTTS {};
  bool SupportsECV {};
  bool SupportsWFXT {};
  bool Supports3DNow {};
  bool SupportsSSE4a {};
  bool SupportsMOPS {};

  // Float exception behaviour
  bool SupportsAFP {};
  bool SupportsFloatExceptions {};

  // FEAT_DotProd: ASIMD SDOT/UDOT for 8-bit integer dot products into 32-bit accumulators.
  // Supported on Snapdragon 8 Gen 2 (SM8550) Cortex-X3, A715, A710, A510 and equivalent cores.
  bool SupportsDotProduct {};

  // FEAT_I8MM: 8-bit integer matrix multiply (SMMLA, UMMLA, USMMLA).
  // Provides significant throughput improvements for 8-bit inference and compute workloads.
  // Supported on Snapdragon 8 Gen 2 (SM8550) Cortex-X3, A715, A710.
  bool SupportsI8MM {};

  // FEAT_BF16: BFloat16 multiply-accumulate (BFDOT, BFMMLA, BFMLALB, BFMLALT).
  // Supported on Snapdragon 8 Gen 2 (SM8550) Cortex-X3, A715.
  bool SupportsBF16 {};

  // RPRFM: Range Prefetch Memory hint extension (FEAT_RPRFM, ISAR2[47:44]).
  // Allows prefetching an entire memory range with a single instruction.
  // Supported on Snapdragon 8 Gen 2 (SM8550) Cortex-X3 (prime core).
  // NOTE: This is a hint-only instruction; absence of hardware support silently falls
  // back to a hint-NOP, so it is safe to emit unconditionally on ARMv8.7+ targets.
  // However, only emit when verified present to avoid wasting instruction bandwidth.
  bool SupportsRPRFM {};

  // Snapdragon 8 Gen 2 (SM8550) platform detection flag.
  //
  // The SM8550 uses the following heterogeneous CPU cluster:
  //   - 1x Cortex-X3   (MIDR impl=0x41, part=0xd4e) - Prime core  @ 3.2 GHz
  //   - 2x Cortex-A715 (MIDR impl=0x41, part=0xd4d) - Perf-big    @ 2.8 GHz
  //   - 2x Cortex-A710 (MIDR impl=0x41, part=0xd47) - Perf-mid    @ 2.8 GHz
  //   - 3x Cortex-A510 (MIDR impl=0x41, part=0xd46) - Efficiency   @ 2.0 GHz
  //
  // The Adreno 740 GPU shares an 8 MB System Level Cache (SLC) with all CPU clusters
  // via a hardware-coherent interconnect.  CPU writes that reach the SLC are immediately
  // visible to the GPU without an explicit outer-shareable barrier.  This means that
  // PRFM instructions targeting L3 (SLC) can be used to warm data for GPU consumption,
  // and that DSB ISH is sufficient for CPU->GPU synchronisation in cached GPU access
  // mode.
  //
  // Additional micro-architectural notes:
  //   - The Cortex-X3 decode pipeline is 8-instructions wide; loop headers aligned to
  //     a 64-byte (cache-line) boundary minimise decode stalls.
  //   - D-cache and I-cache line size: 64 bytes on all cores.
  //   - DC ZVA block size: 64 bytes (matching the cacheline, so CLZero is a no-op
  //     equivalent to a single DC ZVA).
  //   - LDAPUR errata (Cortex-X3, A715, A710, A510): LDAPUR executes with full
  //     Load-Acquire ordering rather than the relaxed ordering defined in the
  //     architecture pseudocode; SupportsTSOImm9 is already forced false by the
  //     generic X3/A710 errata path in HandleErrata.
  bool SupportsSnapdragonGen2 {};

  // Flag if this is InstCountCI
  bool IsInstCountCI {};

  // MIDR information
  // Also used for determining number of CPU cores for CPUID
  fextl::vector<uint32_t> CPUMIDRs;
};
} // namespace FEXCore
