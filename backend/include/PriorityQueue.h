// PriorityQueue.h
// -----------------------------------------------------------------------------
// Purpose : The compulsory DSA requirement - a REAL priority queue, written by
//           hand as an array-backed binary max-heap.
//
// Why not just sort a vector and call it a priority queue?
//   Sorting answers "give me everything in order" once. A priority queue
//   answers "what is the most urgent item RIGHT NOW" repeatedly, while items
//   keep arriving. Inserting into a sorted vector is O(n); inserting into a
//   heap is O(log n), which is the whole point of the structure.
//
// Layout
//   The heap lives in a std::vector<T> and uses the implicit complete-binary-
//   tree indexing, so there are no pointers and no node allocation:
//
//       parent(i)      = (i - 1) / 2
//       leftChild(i)   = 2i + 1
//       rightChild(i)  = 2i + 2
//
// Complexity
//   push            O(log n)
//   pop             O(log n)
//   top  (peek)     O(1)
//   buildHeap(n)    O(n)      <- bottom-up heapify, not n inserts
//   sortedCopy      O(n log n) (this is exactly heap sort)
//   space           O(n)
//
// The comparator decides what "highest priority" means, so the same class can
// order help requests today and anything else later without being changed.
// `higherPriority(a, b)` must return true when `a` should be served BEFORE `b`.
// -----------------------------------------------------------------------------

#ifndef CC_PRIORITY_QUEUE_H
#define CC_PRIORITY_QUEUE_H

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

#include "Errors.h"

namespace cc {

template <typename T>
class PriorityQueue {
public:
    using Comparator = std::function<bool(const T&, const T&)>;

    explicit PriorityQueue(Comparator higherPriority)
        : higherPriority_(std::move(higherPriority)) {
        if (!higherPriority_) {
            throw ValidationError("A priority queue needs a comparison rule.");
        }
    }

    bool empty() const { return heap_.empty(); }
    std::size_t size() const { return heap_.size(); }
    void clear() { heap_.clear(); }

    // ---- insert: append at the end, then bubble up -------------------------
    // O(log n): the new item can only travel from a leaf towards the root.
    void push(const T& value) {
        heap_.push_back(value);
        siftUp(heap_.size() - 1);
    }

    // ---- peek: O(1), the heap property guarantees the root is the best ------
    const T& top() const {
        if (heap_.empty()) {
            throw ConflictError("The priority queue is empty.");
        }
        return heap_.front();
    }

    // ---- extract the highest priority item ---------------------------------
    // Move the last leaf into the root, shrink, then sink it down. O(log n).
    T pop() {
        if (heap_.empty()) {
            throw ConflictError("The priority queue is empty.");
        }

        T result = heap_.front();
        heap_.front() = heap_.back();
        heap_.pop_back();

        if (!heap_.empty()) {
            siftDown(0);
        }
        return result;
    }

    // ---- rebuild from a saved list -----------------------------------------
    // Bottom-up heapify: start at the last internal node and sink each one.
    // Every leaf is already a valid heap, which is why this is O(n) rather
    // than the O(n log n) you would get by pushing n items one at a time.
    // Used at startup to reconstruct the queue from requests.txt.
    void buildHeap(const std::vector<T>& items) {
        heap_ = items;

        if (heap_.size() < 2) {
            return;
        }

        // size/2 - 1 is the last node that has at least one child.
        for (std::size_t i = heap_.size() / 2; i-- > 0;) {
            siftDown(i);
        }
    }

    // ---- read everything in priority order WITHOUT modifying the queue -----
    // Works on a copy and repeatedly extracts, which is heap sort. The real
    // heap is untouched, so a display never destroys the queue.
    std::vector<T> sortedCopy() const {
        std::vector<T> working = heap_;
        std::vector<T> ordered;
        ordered.reserve(working.size());

        while (!working.empty()) {
            ordered.push_back(working.front());
            working.front() = working.back();
            working.pop_back();
            if (!working.empty()) {
                siftDownIn(working, 0);
            }
        }

        return ordered;
    }

    // Underlying array, for debugging and for the report's complexity table.
    const std::vector<T>& raw() const { return heap_; }

private:
    // Bubble an item up until its parent outranks it.
    void siftUp(std::size_t index) { siftUpIn(heap_, index); }
    void siftDown(std::size_t index) { siftDownIn(heap_, index); }

    void siftUpIn(std::vector<T>& heap, std::size_t index) const {
        while (index > 0) {
            const std::size_t parent = (index - 1) / 2;

            if (!higherPriority_(heap[index], heap[parent])) {
                break;
            }

            std::swap(heap[index], heap[parent]);
            index = parent;
        }
    }

    void siftDownIn(std::vector<T>& heap, std::size_t index) const {
        const std::size_t count = heap.size();

        while (true) {
            const std::size_t left = 2 * index + 1;
            const std::size_t right = 2 * index + 2;

            if (left >= count) {
                break; // no children: this item is a leaf
            }

            // Pick the stronger child, preferring the left on a tie so the
            // order stays deterministic.
            std::size_t best = left;
            if (right < count && higherPriority_(heap[right], heap[left])) {
                best = right;
            }

            if (!higherPriority_(heap[best], heap[index])) {
                break; // parent already outranks both children
            }

            std::swap(heap[index], heap[best]);
            index = best;
        }
    }

    std::vector<T> heap_;
    Comparator higherPriority_;
};

} // namespace cc

#endif // CC_PRIORITY_QUEUE_H
