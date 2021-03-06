// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HEAP_GC_TRACER_H_
#define V8_HEAP_GC_TRACER_H_

#include "src/base/compiler-specific.h"
#include "src/base/platform/platform.h"
#include "src/counters.h"
#include "src/globals.h"
#include "testing/gtest/include/gtest/gtest_prod.h"

namespace v8 {
namespace internal {

template <typename T>
class RingBuffer {
 public:
  RingBuffer() { Reset(); }
  static const int kSize = 10;
  void Push(const T& value) {
    if (count_ == kSize) {
      elements_[start_++] = value;
      if (start_ == kSize) start_ = 0;
    } else {
      DCHECK_EQ(start_, 0);
      elements_[count_++] = value;
    }
  }

  int Count() const { return count_; }

  template <typename Callback>
  T Sum(Callback callback, const T& initial) const {
    int j = start_ + count_ - 1;
    if (j >= kSize) j -= kSize;
    T result = initial;
    for (int i = 0; i < count_; i++) {
      result = callback(result, elements_[j]);
      if (--j == -1) j += kSize;
    }
    return result;
  }

  void Reset() { start_ = count_ = 0; }

 private:
  T elements_[kSize];
  int start_;
  int count_;
  DISALLOW_COPY_AND_ASSIGN(RingBuffer);
};

typedef std::pair<uint64_t, double> BytesAndDuration;

inline BytesAndDuration MakeBytesAndDuration(uint64_t bytes, double duration) {
  return std::make_pair(bytes, duration);
}

enum ScavengeSpeedMode { kForAllObjects, kForSurvivedObjects };

#define INCREMENTAL_SCOPES(F)                                      \
  /* MC_INCREMENTAL is the top-level incremental marking scope. */ \
  F(MC_INCREMENTAL)                                                \
  F(MC_INCREMENTAL_SWEEPING)                                       \
  F(MC_INCREMENTAL_WRAPPER_PROLOGUE)                               \
  F(MC_INCREMENTAL_WRAPPER_TRACING)                                \
  F(MC_INCREMENTAL_FINALIZE)                                       \
  F(MC_INCREMENTAL_FINALIZE_BODY)                                  \
  F(MC_INCREMENTAL_FINALIZE_OBJECT_GROUPING)                       \
  F(MC_INCREMENTAL_EXTERNAL_EPILOGUE)                              \
  F(MC_INCREMENTAL_EXTERNAL_PROLOGUE)

#define TRACER_SCOPES(F)                      \
  INCREMENTAL_SCOPES(F)                       \
  F(EXTERNAL_WEAK_GLOBAL_HANDLES)             \
  F(MC_CLEAR)                                 \
  F(MC_CLEAR_CODE_FLUSH)                      \
  F(MC_CLEAR_DEPENDENT_CODE)                  \
  F(MC_CLEAR_GLOBAL_HANDLES)                  \
  F(MC_CLEAR_MAPS)                            \
  F(MC_CLEAR_SLOTS_BUFFER)                    \
  F(MC_CLEAR_STORE_BUFFER)                    \
  F(MC_CLEAR_STRING_TABLE)                    \
  F(MC_CLEAR_WEAK_CELLS)                      \
  F(MC_CLEAR_WEAK_COLLECTIONS)                \
  F(MC_CLEAR_WEAK_LISTS)                      \
  F(MC_EVACUATE)                              \
  F(MC_EVACUATE_CANDIDATES)                   \
  F(MC_EVACUATE_CLEAN_UP)                     \
  F(MC_EVACUATE_COPY)                         \
  F(MC_EVACUATE_UPDATE_POINTERS)              \
  F(MC_EVACUATE_UPDATE_POINTERS_TO_EVACUATED) \
  F(MC_EVACUATE_UPDATE_POINTERS_TO_NEW)       \
  F(MC_EVACUATE_UPDATE_POINTERS_WEAK)         \
  F(MC_EXTERNAL_EPILOGUE)                     \
  F(MC_EXTERNAL_PROLOGUE)                     \
  F(MC_FINISH)                                \
  F(MC_MARK)                                  \
  F(MC_MARK_FINISH_INCREMENTAL)               \
  F(MC_MARK_PREPARE_CODE_FLUSH)               \
  F(MC_MARK_ROOTS)                            \
  F(MC_MARK_WEAK_CLOSURE)                     \
  F(MC_MARK_WEAK_CLOSURE_EPHEMERAL)           \
  F(MC_MARK_WEAK_CLOSURE_WEAK_HANDLES)        \
  F(MC_MARK_WEAK_CLOSURE_WEAK_ROOTS)          \
  F(MC_MARK_WEAK_CLOSURE_HARMONY)             \
  F(MC_MARK_WRAPPER_EPILOGUE)                 \
  F(MC_MARK_WRAPPER_PROLOGUE)                 \
  F(MC_MARK_WRAPPER_TRACING)                  \
  F(MC_MARK_OBJECT_GROUPING)                  \
  F(MC_SWEEP)                                 \
  F(MC_SWEEP_CODE)                            \
  F(MC_SWEEP_MAP)                             \
  F(MC_SWEEP_OLD)                             \
  F(SCAVENGER_CODE_FLUSH_CANDIDATES)          \
  F(SCAVENGER_EXTERNAL_EPILOGUE)              \
  F(SCAVENGER_EXTERNAL_PROLOGUE)              \
  F(SCAVENGER_OBJECT_GROUPS)                  \
  F(SCAVENGER_OLD_TO_NEW_POINTERS)            \
  F(SCAVENGER_ROOTS)                          \
  F(SCAVENGER_SCAVENGE)                       \
  F(SCAVENGER_SEMISPACE)                      \
  F(SCAVENGER_WEAK)

#define TRACE_GC(tracer, scope_id)                             \
  GCTracer::Scope::ScopeId gc_tracer_scope_id(scope_id);       \
  GCTracer::Scope gc_tracer_scope(tracer, gc_tracer_scope_id); \
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("v8.gc"),             \
               GCTracer::Scope::Name(gc_tracer_scope_id))

// GCTracer collects and prints ONE line after each garbage collector
// invocation IFF --trace_gc is used.
class GCTracer {
 public:
  struct IncrementalMarkingInfos {
    IncrementalMarkingInfos()
        : cumulative_duration(0), longest_step(0), steps(0) {}

    void Update(double duration) {
      steps++;
      cumulative_duration += duration;
      if (duration > longest_step) {
        longest_step = duration;
      }
    }

    void ResetCurrentCycle() {
      longest_step = 0;
      steps = 0;
    }

    double cumulative_duration;
    double longest_step;
    int steps;
  };

  class Scope {
   public:
    enum ScopeId {
#define DEFINE_SCOPE(scope) scope,
      TRACER_SCOPES(DEFINE_SCOPE)
#undef DEFINE_SCOPE
          NUMBER_OF_SCOPES,

      FIRST_INCREMENTAL_SCOPE = MC_INCREMENTAL,
      LAST_INCREMENTAL_SCOPE = MC_INCREMENTAL_EXTERNAL_PROLOGUE,
      NUMBER_OF_INCREMENTAL_SCOPES =
          LAST_INCREMENTAL_SCOPE - FIRST_INCREMENTAL_SCOPE + 1
    };

    Scope(GCTracer* tracer, ScopeId scope);
    ~Scope();
    static const char* Name(ScopeId id);

   private:
    GCTracer* tracer_;
    ScopeId scope_;
    double start_time_;
    RuntimeCallTimer timer_;

    DISALLOW_COPY_AND_ASSIGN(Scope);
  };


  class Event {
   public:
    enum Type {
      SCAVENGER = 0,
      MARK_COMPACTOR = 1,
      INCREMENTAL_MARK_COMPACTOR = 2,
      START = 3
    };

    Event(Type type, GarbageCollectionReason gc_reason,
          const char* collector_reason);

    // Returns a string describing the event type.
    const char* TypeName(bool short_name) const;

    // Type of event
    Type type;

    GarbageCollectionReason gc_reason;
    const char* collector_reason;

    // Timestamp set in the constructor.
    double start_time;

    // Timestamp set in the destructor.
    double end_time;

    // Memory reduction flag set.
    bool reduce_memory;

    // Size of objects in heap set in constructor.
    intptr_t start_object_size;

    // Size of objects in heap set in destructor.
    intptr_t end_object_size;

    // Size of memory allocated from OS set in constructor.
    intptr_t start_memory_size;

    // Size of memory allocated from OS set in destructor.
    intptr_t end_memory_size;

    // Total amount of space either wasted or contained in one of free lists
    // before the current GC.
    intptr_t start_holes_size;

    // Total amount of space either wasted or contained in one of free lists
    // after the current GC.
    intptr_t end_holes_size;

    // Size of new space objects in constructor.
    intptr_t new_space_object_size;

    // Size of survived new space objects in destructor.
    intptr_t survived_new_space_object_size;

    // Bytes marked since creation of tracer (value at start of event).
    intptr_t cumulative_incremental_marking_bytes;

    // Bytes marked since
    // - last event for SCAVENGER events
    // - last INCREMENTAL_MARK_COMPACTOR event for INCREMENTAL_MARK_COMPACTOR
    // events
    intptr_t incremental_marking_bytes;

    // Cumulative pure duration of incremental marking steps since creation of
    // tracer. (value at start of event)
    double cumulative_pure_incremental_marking_duration;

    // Duration of pure incremental marking steps since
    // - last event for SCAVENGER events
    // - last INCREMENTAL_MARK_COMPACTOR event for INCREMENTAL_MARK_COMPACTOR
    // events
    double pure_incremental_marking_duration;

    // Amounts of time spent in different scopes during GC.
    double scopes[Scope::NUMBER_OF_SCOPES];

    // Holds details for incremental marking scopes.
    IncrementalMarkingInfos
        incremental_marking_scopes[Scope::NUMBER_OF_INCREMENTAL_SCOPES];
  };

  static const int kThroughputTimeFrameMs = 5000;

  explicit GCTracer(Heap* heap);

  // Start collecting data.
  void Start(GarbageCollector collector, GarbageCollectionReason gc_reason,
             const char* collector_reason);

  // Stop collecting data and print results.
  void Stop(GarbageCollector collector);

  // Sample and accumulate bytes allocated since the last GC.
  void SampleAllocation(double current_ms, size_t new_space_counter_bytes,
                        size_t old_generation_counter_bytes);

  // Log the accumulated new space allocation bytes.
  void AddAllocation(double current_ms);

  void AddContextDisposalTime(double time);

  void AddCompactionEvent(double duration, intptr_t live_bytes_compacted);

  void AddSurvivalRatio(double survival_ratio);

  // Log an incremental marking step.
  void AddIncrementalMarkingStep(double duration, intptr_t bytes);

  // Compute the average incremental marking speed in bytes/millisecond.
  // Returns 0 if no events have been recorded.
  double IncrementalMarkingSpeedInBytesPerMillisecond() const;

  // Compute the average scavenge speed in bytes/millisecond.
  // Returns 0 if no events have been recorded.
  double ScavengeSpeedInBytesPerMillisecond(
      ScavengeSpeedMode mode = kForAllObjects) const;

  // Compute the average compaction speed in bytes/millisecond.
  // Returns 0 if not enough events have been recorded.
  double CompactionSpeedInBytesPerMillisecond() const;

  // Compute the average mark-sweep speed in bytes/millisecond.
  // Returns 0 if no events have been recorded.
  double MarkCompactSpeedInBytesPerMillisecond() const;

  // Compute the average incremental mark-sweep finalize speed in
  // bytes/millisecond.
  // Returns 0 if no events have been recorded.
  double FinalIncrementalMarkCompactSpeedInBytesPerMillisecond() const;

  // Compute the overall mark compact speed including incremental steps
  // and the final mark-compact step.
  double CombinedMarkCompactSpeedInBytesPerMillisecond();

  // Allocation throughput in the new space in bytes/millisecond.
  // Returns 0 if no allocation events have been recorded.
  double NewSpaceAllocationThroughputInBytesPerMillisecond(
      double time_ms = 0) const;

  // Allocation throughput in the old generation in bytes/millisecond in the
  // last time_ms milliseconds.
  // Returns 0 if no allocation events have been recorded.
  double OldGenerationAllocationThroughputInBytesPerMillisecond(
      double time_ms = 0) const;

  // Allocation throughput in heap in bytes/millisecond in the last time_ms
  // milliseconds.
  // Returns 0 if no allocation events have been recorded.
  double AllocationThroughputInBytesPerMillisecond(double time_ms) const;

  // Allocation throughput in heap in bytes/milliseconds in the last
  // kThroughputTimeFrameMs seconds.
  // Returns 0 if no allocation events have been recorded.
  double CurrentAllocationThroughputInBytesPerMillisecond() const;

  // Allocation throughput in old generation in bytes/milliseconds in the last
  // kThroughputTimeFrameMs seconds.
  // Returns 0 if no allocation events have been recorded.
  double CurrentOldGenerationAllocationThroughputInBytesPerMillisecond() const;

  // Computes the context disposal rate in milliseconds. It takes the time
  // frame of the first recorded context disposal to the current time and
  // divides it by the number of recorded events.
  // Returns 0 if no events have been recorded.
  double ContextDisposalRateInMilliseconds() const;

  // Computes the average survival ratio based on the last recorded survival
  // events.
  // Returns 0 if no events have been recorded.
  double AverageSurvivalRatio() const;

  // Returns true if at least one survival event was recorded.
  bool SurvivalEventsRecorded() const;

  // Discard all recorded survival events.
  void ResetSurvivalEvents();

  void NotifyIncrementalMarkingStart();

  V8_INLINE void AddScopeSample(Scope::ScopeId scope, double duration) {
    DCHECK(scope < Scope::NUMBER_OF_SCOPES);
    if (scope >= Scope::FIRST_INCREMENTAL_SCOPE &&
        scope <= Scope::LAST_INCREMENTAL_SCOPE) {
      incremental_marking_scopes_[scope].Update(duration);
    } else {
      current_.scopes[scope] += duration;
    }
  }

 private:
  FRIEND_TEST(GCTracer, AverageSpeed);
  FRIEND_TEST(GCTracerTest, AllocationThroughput);
  FRIEND_TEST(GCTracerTest, NewSpaceAllocationThroughput);
  FRIEND_TEST(GCTracerTest, NewSpaceAllocationThroughputWithProvidedTime);
  FRIEND_TEST(GCTracerTest, OldGenerationAllocationThroughputWithProvidedTime);
  FRIEND_TEST(GCTracerTest, RegularScope);
  FRIEND_TEST(GCTracerTest, IncrementalMarkingDetails);
  FRIEND_TEST(GCTracerTest, IncrementalScope);

  // Returns the average speed of the events in the buffer.
  // If the buffer is empty, the result is 0.
  // Otherwise, the result is between 1 byte/ms and 1 GB/ms.
  static double AverageSpeed(const RingBuffer<BytesAndDuration>& buffer);
  static double AverageSpeed(const RingBuffer<BytesAndDuration>& buffer,
                             const BytesAndDuration& initial, double time_ms);

  void MergeBaseline(const Event& baseline);

  void ResetForTesting();

  // Print one detailed trace line in name=value format.
  // TODO(ernstm): Move to Heap.
  void PrintNVP() const;

  // Print one trace line.
  // TODO(ernstm): Move to Heap.
  void Print() const;

  // Prints a line and also adds it to the heap's ring buffer so that
  // it can be included in later crash dumps.
  void PRINTF_FORMAT(2, 3) Output(const char* format, ...) const;

  double TotalExternalTime() const {
    return current_.scopes[Scope::EXTERNAL_WEAK_GLOBAL_HANDLES] +
           current_.scopes[Scope::MC_EXTERNAL_EPILOGUE] +
           current_.scopes[Scope::MC_EXTERNAL_PROLOGUE] +
           current_.scopes[Scope::MC_INCREMENTAL_EXTERNAL_EPILOGUE] +
           current_.scopes[Scope::MC_INCREMENTAL_EXTERNAL_PROLOGUE] +
           current_.scopes[Scope::SCAVENGER_EXTERNAL_EPILOGUE] +
           current_.scopes[Scope::SCAVENGER_EXTERNAL_PROLOGUE];
  }

  // Pointer to the heap that owns this tracer.
  Heap* heap_;

  // Current tracer event. Populated during Start/Stop cycle. Valid after Stop()
  // has returned.
  Event current_;

  // Previous tracer event.
  Event previous_;

  // Previous INCREMENTAL_MARK_COMPACTOR event.
  Event previous_incremental_mark_compactor_event_;

  // Cumulative size of incremental marking steps (in bytes) since creation of
  // tracer.
  intptr_t cumulative_incremental_marking_bytes_;

  // Cumulative duration of incremental marking steps since creation of tracer.
  double cumulative_incremental_marking_duration_;

  // Cumulative duration of pure incremental marking steps since creation of
  // tracer.
  double cumulative_pure_incremental_marking_duration_;

  // Incremental scopes carry more information than just the duration. The infos
  // here are merged back upon starting/stopping the GC tracer.
  IncrementalMarkingInfos
      incremental_marking_scopes_[Scope::NUMBER_OF_INCREMENTAL_SCOPES];

  double incremental_marking_start_time_;

  // Timestamp and allocation counter at the last sampled allocation event.
  double allocation_time_ms_;
  size_t new_space_allocation_counter_bytes_;
  size_t old_generation_allocation_counter_bytes_;

  // Accumulated duration and allocated bytes since the last GC.
  double allocation_duration_since_gc_;
  size_t new_space_allocation_in_bytes_since_gc_;
  size_t old_generation_allocation_in_bytes_since_gc_;

  double combined_mark_compact_speed_cache_;

  // Counts how many tracers were started without stopping.
  int start_counter_;

  // Separate timer used for --runtime_call_stats
  RuntimeCallTimer timer_;

  RingBuffer<BytesAndDuration> recorded_incremental_marking_steps_;
  RingBuffer<BytesAndDuration> recorded_scavenges_total_;
  RingBuffer<BytesAndDuration> recorded_scavenges_survived_;
  RingBuffer<BytesAndDuration> recorded_compactions_;
  RingBuffer<BytesAndDuration> recorded_mark_compacts_;
  RingBuffer<BytesAndDuration> recorded_incremental_mark_compacts_;
  RingBuffer<BytesAndDuration> recorded_new_generation_allocations_;
  RingBuffer<BytesAndDuration> recorded_old_generation_allocations_;
  RingBuffer<double> recorded_context_disposal_times_;
  RingBuffer<double> recorded_survival_ratios_;

  DISALLOW_COPY_AND_ASSIGN(GCTracer);
};
}  // namespace internal
}  // namespace v8

#endif  // V8_HEAP_GC_TRACER_H_
