#ifndef FX2QUEUE_H
#define FX2QUEUE_H

// This is an implementation of a bounded-length first-in first-out queue structured as a ring
// buffer that may be used asynchronously with a single producer and a single consumer, with
// an optimized internal state. The following is an informal proof of its correctness.
//
// To recap, the invariants for a bounded queue are:
//   1a. for the dequeue operation, that the queue is not empty (length != 0).
//   2a. for the enqueue operation, that the queue is not full  (length != capacity);
//   3a. always, 0 <= length <= capacity.
// The invariant (3a) is ensured by (1a) and (2a).
//
// First, consider a more abstract implementation of a bounded length ring buffer that consists
// of data storage, head and tail pointers. In this model, the head and tail pointers are natural
// numbers; enqueueing an element advances the head pointer, and dequeueing an element advances
// the tail pointer. The data storage is addressed modulo queue capacity, behaving as if it is
// infinitely repeated along the natural number line.
//
//      0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
//      |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|--->
//          T==========>H
//    +---+---+---+---+---+...................+..............
//    | ? | W | X | Y | Z | ? | W | X | Y | Z | ? | W | X |
//    +---+---+---+---+---+...................+..............
//
// This way, when the head pointer is incremented past the queue capacity, it rolls over to
// the start of the storage, and same for the tail pointer. The invariants now are:
//   1b. queue is not empty: !(tail == head);
//   2b. queue is not full:  !(tail + capacity == head);
//   3b. 0 <= head - tail <= capacity.
//
// Using natural numbers is impractical. Instead of a natural number N, let's consider an _epoch_
// N / capacity, and an _index_ N % capacity. This is a 1:1 transformation.
//
//      (    Epoch 0    )   (    Epoch 1    )   (    Epoch 2    )   ( ...
//      0   1   2   3   4   0   1   2   3   4   0   1   2   3   4   0
//      |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|--->
//          T==========>H
//    +---+---+---+---+---+...................+..............
//    | ? | W | X | Y | Z | ? | W | X | Y | Z | ? | W | X |
//    +---+---+---+---+---+...................+..............
//
// This way, storage accesses and invariants can be restated in terms of pointer epoch and pointer
// index. The index-th storage element is read or written before advancing the pointer to the next
// epoch and index numbers. The invariants now are:
//   1c. queue is not empty: !(index(tail) == index(head) && epoch(tail) == epoch(head));
//   2c. queue is not full:  !(index(tail) == index(head) && epoch(tail) + 1 == epoch(head));
// Since invariant (3b) follows from (1b) and (2b), we can weaken it to:
//   3c. 0 <= epoch(head) - epoch(tail) <= 1.
//
// By considering the cases permitted by the inequality (3c), we can rewrite the invariants as:
//   1d. queue is not empty: !(index(tail) == index(head) &&
//                             epoch(tail) % K == epoch(head) % K);
//   2d. queue is not full:  !(index(tail) == index(head) &&
//                             (epoch(tail) + 1) % K != epoch(head) % K).
// for any K > 1.
//
// Let's represent each pointer as a fixed point number in base 2:
//   E--EE.I--II
// where E are the epoch number bits and I are the index number bits. The index width should be
// sufficient to represent capacity:
//   width(I--II) = 1 + floor(log2(capacity - 1))
// The epoch width matches the choice of K but is otherwise arbitrary, and is chosen to pad
// the fixed point number to the most convenient machine representation:
//   width(E--EE) = log2(K)
//
// In this representation, we can rewrite the invariants as:
//   1e. queue is not empty: !(fixp(tail) == fixp(head));
//   2e. queue is not full:  !(fixp(tail) + (1 << width(I--II)) == fixp(head)).
//
// The operation of advancing the pointer becomes:
//   if(fixp(ptr) & ((1 << width(I--II)) - 1) == capacity - 1)
//   then fixp(ptr) = fixp(ptr) + 1 + ((1 << width(I--II)) - capacity)
//   else fixp(ptr) = fixp(ptr) + 1
// I.e. the fixed point representation is exploited to reset the index to zero and increment
// the epoch in a single addition. The unsigned overflow of index bits and epoch bits is invoked
// deliberately, and does not violate any invariants.
//
// An astute reader will notice that with a power-of-2 capacity, this implementation degenerates
// to the optimal power-of-two only capacity implementation in [Snellman].
//
// References:
//   [Snellman]: https://www.snellman.net/blog/archive/2016-12-13-ring-buffers/
//     The inspiration for this implementation. "Surely it must generalize to NPOT!"
//   [Cummings]: http://www.sunburst-design.com/papers/CummingsSNUG2002SJ_FIFO1.pdf
//     Unlike [Snellman], explicitly considers the most significant bit as a wraparound counter,
//     and is the inspiration for considering an "epoch" instead of using div/mod operations.
//     This work extends it by adapting to pointers that are at least as wide as needed,
//     as opposed to exactly as wide.

#if (__SDCC_VERSION_MAJOR == 3) && (__SDCC_VERSION_MINOR <= 9)
typedef unsigned char sig_atomic_t;
#else
#include <signal.h>
#endif

// The implementation below will only translate into efficient code if the compiler implements
// aggressive constant propagation and can transform expressions of the form (a ? b : b) into (b).
// Fortunately, that's most compilers.
//
// The data is separate from the pointers to reduce the amount of initialized data, as well as
// allow placing data and pointers in different address spaces.

// Round `v` up to next power of two, or itself if `v` is already a power of two.
#define _QUEUE_NEXT_POT_S1(v) ((v)|(v>>32))
#define _QUEUE_NEXT_POT_S2(v) ((v)|(v>>16))
#define _QUEUE_NEXT_POT_S3(v) ((v)|(v>>8))
#define _QUEUE_NEXT_POT_S4(v) ((v)|(v>>4))
#define _QUEUE_NEXT_POT_S5(v) ((v)|(v>>2))
#define _QUEUE_NEXT_POT_S6(v) ((v)|(v>>1))
#define _QUEUE_NEXT_POT(v)                                                  \
  (1+_QUEUE_NEXT_POT_S6(_QUEUE_NEXT_POT_S5(_QUEUE_NEXT_POT_S4(              \
     _QUEUE_NEXT_POT_S3(_QUEUE_NEXT_POT_S2(_QUEUE_NEXT_POT_S1((v)-1)))))))

struct _queue {
  volatile sig_atomic_t head, tail;
};
#define DECLARE_QUEUE(name, ty, cap)                                        \
  extern struct _queue name;                                                \
  extern ty name##_data[cap];
#define DEFINE_QUEUE(name, ty, cap)                                         \
  struct _queue name = {0, 0};                                              \
  ty name##_data[cap];                                                      \
  _Static_assert(cap <= (1 << (sizeof(sig_atomic_t) * 8 - 1)),              \
    "Capacity of queue " #name                                              \
    " must be less than one half of the range of sig_atomic_t");

#define _QUEUE_CAP(queue)                                                   \
  (sizeof(queue##_data)/sizeof(queue##_data[0]))
#define _QUEUE_EPOCH_LSB(queue)                                             \
  _QUEUE_NEXT_POT(_QUEUE_CAP(queue))
#define _QUEUE_INDEX_MASK(queue)                                            \
  (_QUEUE_EPOCH_LSB(queue)-1)
#define _QUEUE_NEXT(queue, ptr)                                             \
  (queue.ptr & _QUEUE_INDEX_MASK(queue)) == (_QUEUE_CAP(queue) - 1)         \
    ? queue.ptr + 1 + (_QUEUE_EPOCH_LSB(queue) - _QUEUE_CAP(queue))         \
    : queue.ptr + 1                                                         \

#define QUEUE_EMPTY(queue)                                                  \
  (queue.tail == queue.head)
#define QUEUE_FULL(queue)                                                   \
  ((sig_atomic_t)(queue.tail + _QUEUE_EPOCH_LSB(queue)) == queue.head)

#define QUEUE_PUT(queue, elem)                                              \
  do {                                                                      \
    queue##_data[queue.head & _QUEUE_INDEX_MASK(queue)] = elem;             \
    queue.head = _QUEUE_NEXT(queue, head);                                  \
  } while(0)
#define QUEUE_GET(queue, elem)                                              \
  do {                                                                      \
    elem = queue##_data[queue.tail & _QUEUE_INDEX_MASK(queue)];             \
    queue.tail = _QUEUE_NEXT(queue, tail);                                  \
  } while(0)

#endif
