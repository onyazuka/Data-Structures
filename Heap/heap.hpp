#include <vector>
#include <cmath>
#include <string>
#include <limits>
#include <algorithm>

class HeapException
{
public:
    HeapException(const std::string& descr)
        : exc{descr}{}
    const std::string& what() const {return exc;}
private:
    std::string exc;
};

//------------------------------------------HEAP

template<typename T>
class Heap
{
public:
    using Elements = std::vector<T>;
    using Index = size_t;
    T item(Index i);
    Index indexOf(T val);
    size_t size() const {return elements.size();}
protected:
    Index parent(Index i);
    Index left(Index i);
    Index right(Index i);
    Elements elements;
};

template<typename T>
T Heap<T>::item(Index i)
{
    return elements[i];
}

template<typename T>
typename Heap<T>::Index Heap<T>::indexOf(T val)
{
    // find is O(n) for vector
    auto iter = std::find(elements.begin(), elements.end(), val);
    if(iter == elements.end())
    {
        throw HeapException("Heap::indexOf(): not found");
    }
    // distance is O(1) for vector
    return std::distance(elements.begin(), iter);
}

template<typename T>
typename Heap<T>::Index Heap<T>::parent(Index i)
{
    return /*floor*/(i - 1) / 2;
}

template<typename T>
typename Heap<T>::Index Heap<T>::left(Index i)
{
    return 2 * i + 1;
}

template<typename T>
typename Heap<T>::Index Heap<T>::right(Index i)
{
    return 2 * i + 2;
}

//------------------------------------------/HEAP

//------------------------------------------MAX HEAP

template<typename T>
class MaxHeap : public Heap<T>
{
public:
    MaxHeap() : Heap<T>() {}
    using Elements = std::vector<T>;
    using Index = size_t;
    void maxHeapify(Index i);
    void insert(const T& item);
    T maximum();
    T extractMax();
    void increaseKey(Index i, T incr);

    template<typename Container>
    static MaxHeap buildMaxHeap(Container& container);
};

template<typename T>
void MaxHeap<T>::maxHeapify(Index i)
{
    Index l = this->left(i);
    Index r = this->right(i);
    Index largest;
    if(l < this->elements.size() && this->elements[l] > this->elements[i])
    {
        largest = l;
    }
    else
    {
        largest = i;
    }
    if(r < this->elements.size() && this->elements[r] > this->elements[largest])
    {
        largest = r;
    }
    if(largest != i)
    {
        std::swap(this->elements[i], this->elements[largest]);
        maxHeapify(largest);
    }
}

template<typename T>
void MaxHeap<T>::insert(const T& item)
{
    this->elements.push_back(std::numeric_limits<T>::min());
    increaseKey(this->elements.size() - 1, item);
}

template<typename T>
T MaxHeap<T>::maximum()
{
    return this->elements[0];
}

template<typename T>
T MaxHeap<T>::extractMax()
{
    if(this->elements.size() <= 0)      // heap is empty
    {
        throw HeapException{"MaxHeap::extractMax(): heap is empty"};
    }
    T heapMax = maximum();
    this->elements[0] = this->elements.back();
    this->elements.pop_back();
    maxHeapify(0);
    return heapMax;
}

template<typename T>
void MaxHeap<T>::increaseKey(Index i, T incr)
{
    if(incr < this->elements[i])
    {
        throw HeapException{"MaxHeap::increaseKey(): new key is not bigger than previous"};
    }
    this->elements[i] = incr;
    Index curIndex = i;
    // moving new key up
    while(curIndex > 0 && (this->elements[this->parent(curIndex)] < this->elements[curIndex]))
    {
        std::swap(this->elements[curIndex], this->elements[this->parent(curIndex)]);
        curIndex = this->parent(curIndex);
    }
}

template<typename T>
template<typename Container>
MaxHeap<T> MaxHeap<T>::buildMaxHeap(Container& container)
{
    MaxHeap<T> newHeap;
    newHeap.elements.resize(std::distance(container.begin(), container.end()), T());
    std::move(container.begin(), container.end(), newHeap.elements.begin());
    for(int i = floor(newHeap.elements.size() / 2) - 1; i >= 0; --i)
    {
        newHeap.maxHeapify(i);
    }
    return newHeap;
}

//------------------------------------------/MAX HEAP

//------------------------------------------MIN HEAP

template<typename T>
class MinHeap : public Heap<T>
{
public:
    using Elements = std::vector<T>;
    using Index = size_t;
    void minHeapify(Index i);
    void insert(const T& item);
    T minimum();
    T extractMin();
    void decreaseKey(Index i, T decr);

    template<typename Container>
    static MinHeap buildMinHeap(Container& container);
};

template<typename T>
void MinHeap<T>::minHeapify(Index i)
{
    Index l = this->left(i);
    Index r = this->right(i);
    Index smallest;
    if(l < this->elements.size() && this->elements[l] < this->elements[i])
    {
        smallest = l;
    }
    else
    {
        smallest = i;
    }
    if(r < this->elements.size() && this->elements[r] < this->elements[smallest])
    {
        smallest = r;
    }
    if(smallest != i)
    {
        std::swap(this->elements[i], this->elements[smallest]);
        minHeapify(smallest);
    }
}

template<typename T>
void MinHeap<T>::insert(const T& item)
{
    this->elements.push_back(std::numeric_limits<T>::max());
    decreaseKey(this->elements.size() - 1, item);
}

template<typename T>
T MinHeap<T>::minimum()
{
    return this->elements[0];
}

template<typename T>
T MinHeap<T>::extractMin()
{
    if(this->elements.size() <= 0)      // heap is empty
    {
        throw HeapException{"MaxHeap::extractMin(): heap is empty"};
    }
    T heapMin = minimum();
    this->elements[0] = this->elements.back();
    this->elements.pop_back();
    minHeapify(0);
    return heapMin;
}

template<typename T>
void MinHeap<T>::decreaseKey(Index i, T decr)
{
    if(decr > this->elements[i])
    {
        throw HeapException{"MinHeap::decreaseKey(): new key is not smaller than previous"};
    }
    this->elements[i] = decr;
    Index curIndex = i;
    // moving new key up
    while(curIndex > 0 && (this->elements[this->parent(curIndex)] > this->elements[curIndex]))
    {
        std::swap(this->elements[curIndex], this->elements[this->parent(curIndex)]);
        curIndex = this->parent(curIndex);
    }
}

template<typename T>
template<typename Container>
MinHeap<T> MinHeap<T>::buildMinHeap(Container& container)
{
    MinHeap<T> newHeap;
    newHeap.elements.resize(std::distance(container.begin(), container.end()), T());
    std::move(container.begin(), container.end(), newHeap.elements.begin());
    for(int i = floor(newHeap.elements.size() / 2) - 1; i >= 0; --i)
    {
        newHeap.minHeapify(i);
    }
    return newHeap;
}

//------------------------------------------/MIN HEAP
