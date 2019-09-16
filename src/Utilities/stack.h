#ifndef LYTHON_STACK_H
#define LYTHON_STACK_H

#include <vector>

template<typename V>
class Stack{
public:
    using ReverseIterator = typename std::vector<V>::iterator;
    using Iterator = typename std::vector<V>::reverse_iterator;
    using ConstReverseIterator = typename std::vector<V>::const_iterator;
    using ConstIterator = typename std::vector<V>::const_reverse_iterator;

    void push(V const& value){
        stack.push_back(value);
        _size += 1;
    }

    V pop(){
        V v = stack[_size];
        stack.pop_back();
        _size -= 1;
        return v;
    }

    V const& peek() const{
        return stack[_size];
    }

    int size() const {
        return _size + 1;
    }

    Iterator        begin   (){ return std::rbegin(stack);  }
    Iterator        end     (){ return std::rend(stack);    }
    ReverseIterator rbegin  (){ return std::begin(stack);   }
    ReverseIterator rend    (){ return std::end(stack);     }

    ConstIterator        begin   () const { return std::rbegin(stack);  }
    ConstIterator        end     () const { return std::rend(stack);    }
    ConstReverseIterator rbegin  () const { return std::begin(stack);   }
    ConstReverseIterator rend    () const { return std::end(stack);     }

private:
    std::vector<V> stack;
    int _size = -1;
};


#endif
