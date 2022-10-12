#pragma once

#include <new>
#include <memory>
#include <cassert>
#include <cstdlib>
#include <utility>
#include <stdexcept>
#include <type_traits>

template <typename T>
class RawMemory {
public:

    // ---------------------------------------- Блок конструкторов --------------------------------------

    RawMemory() = default;

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept { 
        if (*this != other) {
            capacity_ = std::move(other.Capacity());
            buffer_ = other.GetAddress();

            other.buffer_ = nullptr;
            other.capacity_ = 0;
        }
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (*this != rhs) {
            capacity_ = std::move(rhs.Capacity());
            buffer_ = rhs.GetAddress();

            rhs.buffer_ = nullptr;
            rhs.capacity_ = 0;
        }
        return *this;
    }

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);

    }

    // ---------------------------------------- Блок получения значений элементов аллокатора ------------

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

    const T* GetAddress() const noexcept {
        return buffer_;
    }
    T* GetAddress() noexcept {
        return buffer_;
    }

    // ---------------------------------------- Блок получения параметров и прочие методы ---------------

    size_t Capacity() const {
        return capacity_;
    }
    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        if (buf != nullptr) {
            operator delete(buf);
        }
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
bool operator==(const RawMemory<T>& lhs, const RawMemory<T>& rhs) {
    return lhs.Capacity() == rhs.Capacity()
        && lhs.GetAddress() == rhs.GetAddress();
}

template <typename T>
bool operator!=(const RawMemory<T>& lhs, const RawMemory<T>& rhs) {
    return lhs.Capacity() != rhs.Capacity()
        && lhs.GetAddress() != rhs.GetAddress();
}

template <typename T>
class Vector {
public:

    using iterator = T*;
    using const_iterator = const T*;

    // ---------------------------------------- Блок конструкторов --------------------------------------

    Vector() = default;

    explicit Vector(size_t size)
        : data_(RawMemory<T>(size)), size_(size) {

        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }
    explicit Vector(const Vector& other)
        : data_(RawMemory<T>(other.Size())), size_(other.Size()) {

        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }
    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_)), size_(std::move(other.size_)) {

        other.size_ = 0;
    }
    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {

            // проверяем текущий капасити
            if (rhs.size_ > data_.Capacity()) {

                // применяем идиому Copy and Swap
                Vector temp(rhs);               // делаем копию переданного объекта
                Swap(temp);              // swap с текущим
            }
            else {

                // копируем содержимое по минимальному из двух размеров векторов
                for (size_t i = 0; i < std::min(size_, rhs.size_); ++i) {
                    data_[i] = rhs[i];
                }

                // если есть лишнее то удаляем
                if (size_ > rhs.size_) 
                {
                    std::destroy_n(data_ + rhs.size_, size_ - rhs.size_);
                }
                // если не хватает то достраиваем недостающее
                else if (size_ < rhs.size_) 
                {
                    std::uninitialized_copy_n(rhs.data_ + size_, rhs.size_ - size_, data_ + size_);
                }
            }

            // обновляем информацию о размере
            size_ = rhs.size_;
        }
        return *this;
    }
    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            data_ = std::move(rhs.data_);
            size_ = rhs.size_;
        }
        return *this;
    }

    ~Vector() {
        if (data_.GetAddress() != nullptr) {
            std::destroy_n(data_.GetAddress(), size_);
        }
    }

    // ---------------------------------------- Блок итераторов доступа ---------------------------------

    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end() noexcept {
        return data_ + size_;
    }
    const_iterator begin() const noexcept {
        return const_cast<iterator>(data_.GetAddress());
    }
    const_iterator end() const noexcept {
        return const_cast<iterator>(data_ + size_);
    }
    const_iterator cbegin() const noexcept {
        return begin();
    }
    const_iterator cend() const noexcept {
        return end();
    }

    // ---------------------------------------- Блок вставки и удаления значений -----------------------

    void PushBack(const T& value) {
        EmplaceBack(value);                                       // перенаправляем в EmplaceBack
    }
    void PushBack(T&& value) {
        EmplaceBack(std::move(value));                            // перенаправляем в EmplaceBack
    }
    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);                               // перенаправляем в Emplace
    }
    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));                    // перенаправляем в Emplace
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {

        // проверяем наличие свободного места при текущем капасити
        if (size_ == Capacity()) 
        {
            // запускаем релоцирующий EmplaceBack
            return RelocatedEmplaceBack(std::forward<Args>(args)...);
        }
        else 
        {
            // запускаем обычный EmplaceBack
            return UnusedEmplaceBack(std::forward<Args>(args)...);
        }
    }
    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {

        // проверяем наличие свободного места при текущем капасити
        if (size_ == Capacity())
        {
            // запускаем релоцирующий Emplace
            return RelocatedEmplace(pos, std::forward<Args>(args)...);
        }
        else
        {
            // запускаем обычный Emplace
            return UnusedEmplace(pos, std::forward<Args>(args)...);
        }
    }

    void PopBack() {
        std::destroy_at(&data_[--size_]);
    }
    iterator Erase(const_iterator pos) {

        if (pos == end()) {
            PopBack();                                              // если pos указывает на конец диапазона то вызываем PopBack();
            return const_cast<iterator>(end() - 1);
        }

        size_t erase_begin_align = pos - begin();                             // выравнивание удаления по началу текуещего массива

        std::move(begin() + (erase_begin_align + 1), end(), begin() + erase_begin_align);               // перемещаем элементы
        PopBack();                                                                                      // удаляем крайний элемент

        return const_cast<iterator>(begin() + erase_begin_align);

    }

    // ---------------------------------------- Блок получения параметров вектора ----------------------
    
    size_t Size() const noexcept {
        return size_;
    }
    size_t Capacity() const noexcept {
        return data_.Capacity();
    }
    const RawMemory<T>& Data() const {
        return data_;
    }

    // ---------------------------------------- Блок получения значений элементов вектора --------------

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }
    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    // ---------------------------------------- Блок вспомогательных методов ---------------------------

    void Reserve(size_t new_capacity) {

        if (new_capacity <= data_.Capacity()) 
        {
            // если новый капасити меньше или равен текущему, то ничего не делаем
            return;
        }

        RawMemory<T> new_data(new_capacity);               // создаем новый объект аллокатора с выделением требуемого места

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            // перемещаем все текущие значения в новую область памяти
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }

        std::destroy_n(data_.GetAddress(), size_);         // разрушаем старый объект памяти и вовзращаем память в кучу
        data_.Swap(new_data);                       // обмениваем содержимое временного объекта и текущего
    }
    void Resize(size_t new_size) {

        // если новый размер меньше текущего
        if (new_size < size_)
        {
            std::destroy_n(data_ + new_size, size_ - new_size);
        }

        // если новый размер больше текущего
        else
        {

            // если новый размер не укладывается в текущий capacity
            if (new_size > Capacity())
            {
                Reserve(new_size);
            }

            // инициализируем новые поля вектора стандартным конструктором типа <T>
            std::uninitialized_value_construct_n(data_ + size_, new_size - size_);
        }
        
        size_ = new_size;                    // обновляем запись о размере вектора
    }
    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

private:
    RawMemory<T> data_ = {};
    size_t size_ = 0;

    // Вставка элемента по указанной позиции в вектор без релокации памяти
    template <typename... Args>
    iterator UnusedEmplace(const_iterator pos, Args&&... args)
    {
        if (pos == end()) {
            // если задана позиция в конце вектора, то запускаем EmplaceBack
            UnusedEmplaceBack(std::forward<Args>(args)...);
            return end() - 1;
        }

        size_t insert_begin_align = pos - begin();                                          // выравнивание вставки по началу текуещего массива

        try
        {
            T temp_obj(std::forward<Args>(args)...);                                        // создаём временный объект
            new (end()) T(std::forward<T>(data_[size_ - 1]));                               // перемещаем в новый конец объект с позиции end() - 1
            std::move_backward(begin() + insert_begin_align, end() - 1, end());             // перемещаем все прочие объекты
            *(begin() + insert_begin_align) = std::forward<T>(temp_obj);                    // инициализируем место вставки временным объектом
        }
        catch (const std::exception&)
        {
            operator delete (end());                                                        // удаляем созданное в конце значение
            throw;                                                                          // бросаем исключение дальше
        }
        ++size_;                                                                            // увеличиваем количество элементов

        // возвращаем итератор на вставленный элемент
        return const_cast<iterator>(data_ + insert_begin_align);

    }
    // Вставка элемента по указанной позиции в вектор с релокацией памяти
    template <typename... Args>
    iterator RelocatedEmplace(const_iterator pos, Args&&... args) {

        if (pos == end()) {
            // если задана позиция в конце вектора, то запускаем EmplaceBack
            RelocatedEmplaceBack(std::forward<Args>(args)...);
            return end() - 1;
        }

        size_t insert_begin_align = pos - begin();                                          // выравнивание вставки по началу текуещего массива
        size_t insert_back_align = end() - pos;                                             // выравнивание вставки по окончанию текущего массива

        RawMemory<T> temp((size_ == 0 ? 1 : size_ * 2));                                    // создаём временный вектор того же капасити 
        new (temp.GetAddress() + insert_begin_align) T(std::forward<Args>(args)...);        // размещаем в нем переданный элемент args(value)

        try
        {
            // размещаем данные в два этапа сначала то что до позиции pos, потом то, что после
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {

                // источник данных - начало текущего вектора, количество - выравнивание по началу, приемщик - начало временного вектора
                std::uninitialized_move_n(data_.GetAddress(), insert_begin_align, temp.GetAddress());

                // источник данных - начало текущего вектора + начальное выравнивание, 
                // количество - выравнивание по окончанию, приемщик - начало временного вектора + выравнивание + 1
                std::uninitialized_move_n(data_.GetAddress() + insert_begin_align, insert_back_align, temp.GetAddress() + (insert_begin_align + 1));
            }
            else {
                // аналогично move-версии
                std::uninitialized_copy_n(data_.GetAddress(), insert_begin_align, temp.GetAddress());
                std::uninitialized_copy_n(data_.GetAddress() + insert_begin_align, insert_back_align, temp.GetAddress() + (insert_begin_align + 1));
            }
        }
        catch (const std::exception&)
        {
            std::destroy_n(temp.GetAddress(), size_ + 1);                                   // удаляем созданный буфер
            throw;                                                                          // выбрасываем исключение дальше
        }

        std::destroy_n(data_.GetAddress(), size_);                                          // удаляем старые данные
        data_.Swap(temp);                                                            // свапаем новые данные
        ++size_;                                                                            // увеличиваем количество элементов

        // возвращаем итератор на вставленный элемент
        return const_cast<iterator>(data_ + insert_begin_align);
    }

    // Вставка элемента в конец вектора без релокации памяти
    template <typename... Args>
    T& UnusedEmplaceBack(Args&&... args) 
    {
        new (data_ + size_) T(std::forward<Args>(args)...);                                 // добавляем новый элемент в конец
        return data_[size_++];                                                              // возвращаем ссылку на него и увеличиваем размер
    }
    // Вставка элемента в конец вектора с релокацией памяти
    template <typename... Args>
    T& RelocatedEmplaceBack(Args&&... args) {

        RawMemory<T> temp((size_ == 0 ? 1 : size_ * 2));                                    // создаем буфер с требуемой капасити
        new (temp.GetAddress() + size_) T(std::forward<Args>(args)...);                     // записываем в конец полученное значение args(value)

        try
        {
            // создаём увеличенную копию существующих данных
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, temp.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, temp.GetAddress());
            }
        }
        catch (const std::exception&)
        {
            std::destroy_n(temp.GetAddress(), size_ + 1);                                   // удаляем созданный буфер
            throw;                                                                          // выбрасываем исключение дальше
        }

        std::destroy_n(data_.GetAddress(), size_);                                          // удаляем старые данные
        data_.Swap(temp);                                                            // свапаем новые данные
        ++size_;                                                                            // увеличиваем количество элементов

        // возвращаем значение крайнего элемента
        return data_[size_ - 1];
    }

};

template <typename T>
bool operator==(const Vector<T>& lhs, const Vector<T>& rhs) {
    return lhs.Data() == rhs.Data()
        && lhs.Size() == rhs.Size();
}

template <typename T>
bool operator!=(const Vector<T>& lhs, const Vector<T>& rhs) {
    return lhs.Data() != rhs.Data()
        && lhs.Size() != rhs.Size();
}