#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

template <typename T>
class SortStrategy {
public:
    virtual bool compare(const T& a, const T& b) const = 0;
    virtual ~SortStrategy() = default;
};

template <typename T>
class AscendingSort : public SortStrategy<T> {
public:
    bool compare(const T& a, const T& b) const override {
        return a < b;
    }
};

template <typename T>
class DescendingSort : public SortStrategy<T> {
public:
    bool compare(const T& a, const T& b) const override {
        return a > b;
    }
};

template <typename Container>
class ContainerMemento {
private:
    Container state;

public:
    ContainerMemento(const Container& container) : state(container) {}

    Container getState() const {
        return state;
    }
};

template <typename Container>
class SortableContainer {
private:
    using ValueType = typename Container::value_type;

    Container container;
    std::unique_ptr<ContainerMemento<Container>> memento;

public:
    SortableContainer(const Container& data) : container(data) {}

    void sort(const SortStrategy<ValueType>& strategy) {
        save();

        std::sort(container.begin(), container.end(),
            [&strategy](const ValueType& a, const ValueType& b) {
                return strategy.compare(a, b);
            });
    }

    void save() {
        memento = std::make_unique<ContainerMemento<Container>>(container);
    }

    void restore() {
        if (memento) {
            container = memento->getState();
        }
    }

    void print() const {
        for (const auto& item : container) {
            std::cout << item << " ";
        }
        std::cout << std::endl;
    }

    const Container& getContainer() const {
        return container;
    }
};

int main() {
    std::vector<int> data = { 5, 1, 4, 2, 3 };

    SortableContainer<std::vector<int>> sortable(data);

    AscendingSort<int> asc;
    DescendingSort<int> desc;

    std::cout << "Original: ";
    sortable.print();

    sortable.sort(asc);
    std::cout << "Sorted ascending: ";
    sortable.print();

    sortable.restore();
    std::cout << "Restored: ";
    sortable.print();

    sortable.sort(desc);
    std::cout << "Sorted descending: ";
    sortable.print();

    sortable.restore();
    std::cout << "Restored again: ";
    sortable.print();

    return 0;
}