#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <type_traits>

// RAW MEMORY ---------------------------------------------------------------------------
// вспомогательный класс (упрощает работу с сырой памятью)

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

    // Операция копирования не имеет смысла для класса RawMemory,
    // так как у него нет информации о количестве находящихся в сырой памяти элементов.
    // Копирование элементов определено для класса Vector, который использует сырую память
    // для размещения элементов и знает об их количестве.
    // Поэтому конструктор копирования и копирующий оператор присваивания в классе RawMemory
    // должны быть запрещены. Это исключит возможность их случайного вызова.
    // Зато перемещающие конструктор и оператор присваивания классу RawMemory нужны:

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept {
        Init(std::move(other));
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        Init(std::move(rhs));
        return *this;
    }
    // Перемещающие конструктор и оператор присваивания не выбрасывают исключений и выполняются за O(1).

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    void Init(RawMemory&& other) noexcept {
        Deallocate(buffer_);
        buffer_ = std::exchange(other.buffer_, nullptr);
        capacity_ = std::exchange(other.capacity_, 0);
    }


    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

// VECTOR ___---------------------------------------------------------------------------

template <typename T>
class Vector {
public:
    //Чтобы задать позицию элемента, класс std::vector использует итераторы. Поступим аналогично.
    // Проще всего объявить итераторы указателями на элементы контейнера:
    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }


    // Конструктор по умолчанию.
    // Инициализирует вектор нулевого размера и вместимости.
    // Не выбрасывает исключений.
    // Алгоритмическая сложность: O(1).
    Vector() = default;

    // Конструктор, который создаёт вектор заданного размера.
    // Вместимость созданного вектора равна его размеру, а элементы
    // проинициализированы значением по умолчанию для типа T.
    // Алгоритмическая сложность: O(размер вектора).
    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    // Копирующий конструктор.
    // Создаёт копию элементов исходного вектора.
    // Имеет вместимость, равную размеру исходного вектора, то есть выделяет память без запаса.
    // Алгоритмическая сложность: O(размер исходного вектора).
    Vector(const Vector& other)
            : data_(other.size_)
            , size_(other.size_)  //
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    // Перемещающий конструктор. Выполняется за O(1)O(1)O(1) и не выбрасывает исключений.
    Vector(Vector&& other) noexcept {
        data_ = std::move(other.data_);
        size_ = std::exchange(other.size_, 0);
    }

    // Методы Allocate и Deallocate в классе Vector после этого станут не нужны —
    // выделение и освобождение памяти берёт на себя RawMemory.

    // Вызывает деструкторы n объектов массива по адресу buf
    static void DestroyN(T* buf, size_t n) noexcept {
        for (size_t i = 0; i != n; ++i) {
            Destroy(buf + i);
        }
    }

    // Создаёт копию объекта elem в сырой памяти по адресу buf
    static void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }

    // Вызывает деструктор объекта по адресу buf
    static void Destroy(T* buf) noexcept {
        buf->~T();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }
    // Метод void Reserve(size_t capacity).
    // Резервирует достаточно места, чтобы вместить количество элементов, равное capacity.
    // Если новая вместимость не превышает текущую, метод не делает ничего.
    // Алгоритмическая сложность: O(размер вектора).
    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        // constexpr оператор if будет вычислен во время компиляции
        // Используем if constexpr. Это версия оператора if, которая выполняется во время компиляции.
        // Выражение в условии должно быть константой времени компиляции. В итоге скомпилируется
        // только одна из двух веток, что и требуется:
        SafeMove(data_.GetAddress(), size_, new_data.GetAddress());
        data_.Swap(new_data);
    }

    // Деструктор. Разрушает содержащиеся в векторе элементы и освобождает занимаемую ими память.
    // Алгоритмическая сложность: O(размер вектора).
    ~Vector() {
        DestroyN(data_.GetAddress(), size_);
    }

    // Метод Swap, выполняющий обмен содержимого вектора с другим вектором.
    // Операция должна иметь сложностьO(1)O(1)O(1) и не выбрасывать исключений.
    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    // Оператор копирующего присваивания. Выполняется за O(N)O(N)O(N),
    // где N — максимум из размеров векторов, участвующих в операции.
    Vector &operator=(const Vector &rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector<T> large(rhs);
                Swap(large);
            } else {
                // Скопировать элементы из rhs, создав при необходимости новые или удалив существующие
                if (rhs.size_ > size_) {
                    /* Применить copy-and-swap */
                    for (size_t i = 0; i < size_; ++i) {
                        data_[i] = rhs[i];
                    }
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + rhs.size_ - size_,
                                              rhs.size_ - size_,
                                              data_.GetAddress() + size_);
                    size_ = rhs.size_;

                } else {
                    // Скопировать элементы из rhs, создав при необходимости новые или удалив существующие
                    for (size_t i = 0; i < rhs.size_; ++i) {
                         data_[i] = rhs[i];
                     }
                     std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                     size_ = rhs.size_;
                }
            }
        }
        return *this;
    }

    // Оператор перемещающего присваивания. Выполняется за O(1)O(1)O(1) и не выбрасывает исключений.
    Vector &operator=(Vector &&rhs) noexcept {
        if (this == &rhs) {
            return *this;
        }
        data_ = std::move(rhs.data_);
        size_ = rhs.size_;
        rhs.size_ = 0;
        return *this;
    }

    // Метод Resize должен предоставлять строгую гарантию безопасности исключений, когда выполняется любое из условий:
    //  - move-конструктор у типа T объявлен как noexcept;
    //  - тип T имеет публичный конструктор копирования.
    // Если у типа T нет конструктора копирования и move-конструктор может выбрасывать исключения, метод Resize
    // может предоставлять базовую или строгую гарантию безопасности исключений.
    // Сложность метода Resize должна линейно зависеть от разницы между текущим и новым размером вектора.
    // Если новый размер превышает текущую вместимость вектора, сложность операции может дополнительно
    // линейно зависеть от текущего размера вектора.
    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        } else if (new_size > size_) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        size_ = new_size;
    }

    // Метод PushBack должен предоставлять строгую гарантию безопасности исключений, когда выполняется любое из условий:
    //   - мove-конструктор у типа T объявлен как noexcept;
    //   - тип T имеет публичный конструктор копирования.
    // Если у типа T нет конструктора копирования и move-конструктор может выбрасывать исключения,
    // метод PushBack должен предоставлять базовую гарантию безопасности исключений.
    // Сложность метода PushBack должна быть амортизированной константой.
    void PushBack(const T& value) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            CopyConstruct(&new_data[size_], value);
            SafeMove(data_.GetAddress(), size_, new_data.GetAddress());
            data_.Swap(new_data);
        } else {
            CopyConstruct(&data_[size_], value);
        }
        ++size_;
    }

    void PushBack(T&& value) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                new(new_data + size_) T(std::move(value));
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            } else {
                try {
                    new(new_data + size_) T(std::move(value));
                } catch (...) {
                    CopyConstruct(&new_data[size_], value);
                }
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else {
            new(data_ + size_) T(std::move(value));
        }
        ++size_;
    }

    // Метод PopBack не должен выбрасывать исключений при вызове у непустого вектора.
    // При вызове PopBack у пустого вектора поведение неопределённо.
    // Метод PopBack должен иметь сложность $O(1)$.
    void PopBack() noexcept {
        assert(size_ > 0);
        std::destroy_at(data_+(size_-1));
        --size_;
    }

    // метод EmplaceBack, добавляющий новый элемент в конец вектора.
    // Созданный объект должен быть сконструирован с использованием аргументов метода EmplaceBack
    // Метод EmplaceBack должен предоставлять строгую гарантию безопасности исключений,
    // когда выполняется любое из условий:
    //  - move-конструктор у типа T объявлен как noexcept;
    //  - тип T имеет публичный конструктор копирования.
    // Если у типа T нет конструктора копирования и move-конструктор может выбрасывать исключения,
    // метод EmplaceBack должен предоставлять базовую гарантию безопасности исключений.
    // Сложность метода EmplaceBack должна быть амортизированной константой.
    template <typename... Args> T& EmplaceBack(Args&&... args) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data{size_ == 0 ? 1 : size_ * 2};
            new (new_data + size_) T(std::forward<Args>(args)...);
            SafeMove(data_.GetAddress(), size_, new_data.GetAddress());
            data_.Swap(new_data);
        } else {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return data_[size_ - 1];
    }

    template<typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        if (pos == end()) {
            assert(pos >= begin() && pos <= end());
            return &EmplaceBack(std::forward<Args>(args)...);
        }
        if (size_ == Capacity()) {
            return EmplaceWithReallocate(pos, std::forward<Args>(args)...);
        } else {
            return EmplaceWithoutReallocate(pos, std::forward<Args>(args)...);
        }

    }

    // Методы Emplace и Erase, выполняя реаллокацию, должны перемещать элементы, а не копировать,
    // если действительно любое из условий:
    //  - тип T имеет noexcept-конструктор перемещения;
    //  - тип T не имеет конструктора копирования.
    // Если при вставке происходит реаллокация, ожидается ровно Size() перемещений или копирований
    // существующих элементов. Сам вставляемый элемент должен копироваться либо перемещаться
    // в зависимости от версии метода Insert.
    // Вызов Insert и Emplace, не требующий реаллокации, должен перемещать ровно end()-pos
    // существующих элементов плюс одно перемещение элемента из временного объекта в вектор.
    // Методы Insert и Emplace при вставке элемента в конец вектора должны обеспечивать строгую
    // гарантию безопасности исключений, когда выполняется любое из условий:
    //  - шаблонный параметр T имеет конструктор копирования;
    //  - шаблонный параметр T имеет noexcept-конструктор перемещения.
    // Если ни одно из этих условий не выполняется либо элемент вставляется в середину или начало
    // вектора, методы Insert и Emplace должны обеспечивать базовую гарантию безопасности исключений.
    // Метод Erase должен вызывать деструктор ровно одного элемента, а также вызывать оператор
    // присваивания столько раз, сколько элементов находится в векторе следом за удаляемым элементом.
    // Итератор pos, который задаёт позицию удаляемого элемента, должен указывать на существующий элемент
    // вектора. Передача в метод Erase итератора end(),  невалидного итератора или итератора, полученного
    // у другого вектора, приводит к неопределённому поведению.
    iterator Insert(const_iterator pos, const T &value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T &&value) {
        return Emplace(pos, std::move(value));
    }

    iterator Erase(const_iterator pos)
    noexcept(std::is_nothrow_move_assignable_v<T>) {
        assert(pos >= begin() && pos < end());
        size_t index = static_cast<size_t>(pos - begin());
        std::move(begin() + index + 1, end(), begin() + index);
        data_[size_ - 1].~T();
        --size_;
        return begin() + index;
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    template <typename... Args>
    iterator EmplaceWithReallocate(const_iterator pos, Args&&... args) {
        size_t index = static_cast<size_t>(pos - begin());
        RawMemory<T> new_data{size_ == 0 ? 1 : size_ * 2};
        new (new_data + index) T(std::forward<Args>(args)...);
        try {
            SafeMove(data_.GetAddress(), index, new_data.GetAddress());
        }  catch (...) {
            new_data[index].~T();
            throw;
        }

        try {
            SafeMove(data_+index, size_ - index, new_data + (index+1));
        }  catch (...) {
            std::destroy_n(new_data.GetAddress(), index + 1);
            throw;
        }
        data_.Swap(new_data);

        ++size_;
        return begin() + index;
    }

    template <typename... Args>
    iterator EmplaceWithoutReallocate(const_iterator pos, Args&&... args) {
        size_t index = static_cast<size_t>(pos - begin());
        T temp(std::forward<Args>(args)...);
        // память после последнего элемента - не инициализирована, поэтому инициализируем её размещающим new
        // остальные элементы переносим на один вправо
        new (end()) T(std::move(data_[size_ - 1]));
        std::move_backward(begin() + index, end() - 1, end());
        data_[index] = std::move(temp);

        ++size_;
        return begin() + index;
    }

    void SafeMove(T *from, size_t size, T *to) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(from, size, to);
        } else {
            std::uninitialized_copy_n(from, size, to);
        }
        std::destroy_n(from, size);
    }
};
