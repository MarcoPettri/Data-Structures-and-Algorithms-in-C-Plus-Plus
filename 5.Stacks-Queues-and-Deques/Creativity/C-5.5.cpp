// File : C-5.5.cpp

/*
    C-5.5 Describe how to implement the stack ADT using two queues. What is the
    running time of the push and pop functions in this case?

    Answer :
    Keep the stack’s top at the front of queue q1, and use q2 as a temporary. Since a queue only removes from the front, we do the reordering work during push so that pop and top are trivial.

    push(x): enqueue x into the empty q2, then move everything from q1 behind it, then swap the names of q1 and q2. The new element is now at the front of q1, ahead of all the older ones.
    pop(): dequeue from the front of q1.
    top(): look at the front of q1.    

    Running times
    With this design, where n is the current number of elements:

    push: O(n). It moves all n existing elements from q1 to q2, each in O(1).
         The swap is O(1) because std::queue::swap just exchanges the underlying containers’ internals.
    pop: O(1). A single dequeue.
    top: O(1). A single front().
        
*/

#include <cstddef>
#include <iostream>
#include <optional>
#include <queue>

template <typename T>
class QueueStack {
public:
    void push(const T& x) {
        q2_.push(x);                  // new element goes in first
        while (!q1_.empty()) {        // then everything older goes behind it
            q2_.push(q1_.front());
            q1_.pop();
        }
        q1_.swap(q2_);                // q1_ now holds the stack, q2_ is empty
    }

    // Returns false if the stack is empty (no exception).
    bool pop() {
        if (q1_.empty()) return false;
        q1_.pop();
        return true;
    }

    std::optional<T> top() const {
        if (q1_.empty()) return std::nullopt;
        return q1_.front();
    }

    bool empty() const { return q1_.empty(); }
    std::size_t size() const { return q1_.size(); }

private:
    std::queue<T> q1_;   // holds the elements, top of stack at the front
    std::queue<T> q2_;   // temporary, empty between operations
};

int main(){
    QueueStack<int> s;
    s.push(1); s.push(2); s.push(3);

    std::cout << *s.top() << '\n';   // 3
    s.pop();
    std::cout << *s.top() << '\n';   // 2
}


