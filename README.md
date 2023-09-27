# __cpp-advanced-vector__
__*Финальный проект: улучшенный контейнер вектор*__
  *Собственная реализация улучшенного шаблонного класса* ```Vector<T>```. *В текущей версии контейнера используется размещающий оператор* ```new``` *и работа с сырой памятью*

---
Этот шаблонный класс инкапсулировал работу с массивом в динамической памяти, предоставляя сходный с классом ```std::vector``` набор операций.
Разработан мощный и эффективный класс ```Vector```, были освоины вариативные шаблоны и реализованы методы:
- __Конструктор по умолчанию__. Инициализирует вектор нулевого размера и вместимости. Не выбрасывает исключений. Алгоритмическая сложность: O(1).
- __Конструктор, который создаёт вектор заданного размера__. Вместимость созданного вектора равна его размеру, а элементы проинициализированы значением по умолчанию для типа T. Алгоритмическая сложность: O(размер вектора).
- __Копирующий конструктор__. Создаёт копию элементов исходного вектора. Имеет вместимость, равную размеру исходного вектора, то есть выделяет память без запаса. Алгоритмическая сложность: O(размер исходного вектора).

Конструкторы класса ```Vector``` устойчивы к возникновению исключений. Исключения не приводит к утечкам памяти или неопределённому поведению. Метод Reserve в случае возникновения исключения должен оставлять вектор в прежнем состоянии.

- __Деструктор__. Разрушает содержащиеся в векторе элементы и освобождает занимаемую ими память. Алгоритмическая сложность: O(размер вектора).
- __```Reserve```__. Резервирует достаточно места, чтобы вместить количество элементов, равное ```capacity```. Если новая вместимость не превышает текущую, метод не делает ничего. Алгоритмическая сложность: O(размер вектора). Метод устойчив к возникновению исключений. Исключения не приводит к утечкам памяти или неопределённому поведению. Метод ```Reserve``` в случае возникновения исключения должен оставлять вектор в прежнем состоянии.
- __```Swap```__, выполняет обмен содержимого вектора с другим вектором. Операция имет сложностьO(1)O(1)O(1) и не выбрасывать исключений.
- __Перемещающий конструктор__. Выполняется за O(1)O(1)O(1) и не выбрасывает исключений.
- __Оператор копирующего присваивания__. Выполняется за O(N)O(N)O(N), где N — максимум из размеров векторов, участвующих в операции.
- __Оператор перемещающего присваивания__. Выполняется за O(1)O(1)O(1) и не выбрасывает исключений.
- __```Resize```__,
- __```PushBack```__, выполняется вставка элемента вектора в конец этого же вектора.
- __```PopBack```__,
- __```EmplaceBack```__, выполняет добавление нового элемента в конец вектора. Созданный объект должен быть сконструирован с использованием аргументов метода ```EmplaceBack```. Метод предоставляет строгую гарантию безопасности исключений, когда выполняется любое из условий:
    - move-конструктор у типа ```T``` объявлен как noexcept;
    - тип ```T``` имеет публичный конструктор копирования.

Если у типа T нет конструктора копирования и move-конструктор может выбрасывать исключения, метод ```EmplaceBack``` должен предоставлять базовую гарантию безопасности исключений.
Сложность метода - амортизированная константа.
- __```Insert```__,
- __```Emplace```__,
- __```Erase```__,
Методы ```Emplace``` и ```Erase```, выполняя реаллокацию, перемещают элементы, а не копируют, если действительно любое из условий:
    - тип ```T``` имеет noexcept-конструктор перемещения;
    - тип ```T``` не имеет конструктора копирования.

Если при вставке происходит реаллокация, ожидается ровно ```Size()``` перемещений или копирований существующих элементов. Сам вставляемый элемент копируется либо перемещается в зависимости от версии метода ```Insert```.
Вызов ```Insert``` и ```Emplace```, не требующий реаллокации, должен перемещать ровно ```end()-pos``` существующих элементов плюс одно перемещение элемента из временного объекта в вектор.
Методы ```Insert``` и ```Emplace``` при вставке элемента в конец вектора обеспечивают строгую гарантию безопасности исключений, когда выполняется любое из условий:
    - шаблонный параметр ```T``` имеет конструктор копирования;
    - шаблонный параметр ```T``` имеет ```noexcept-```конструктор перемещения.

Если ни одно из этих условий не выполняется либо элемент вставляется в середину или начало вектора, методы Insert и Emplace обеспечивают базовую гарантию безопасности исключений.
Метод Erase вызывает деструктор ровно одного элемента, а также вызывает оператор присваивания столько раз, сколько элементов находится в векторе следом за удаляемым элементом. Итератор ```pos```, который задаёт позицию удаляемого элемента, указывает на существующий элемент вектора. Передача в метод ```Erase``` итератора ```end()```,  невалидного итератора или итератора, полученного у другого вектора, приводит к неопределённому поведению.
- __```begin```__ для получения итераторов на начало вектора,
- __```cbegin```__,
- __```end```__ для получения итераторов на конец вектора.
- __```cend```__ .
