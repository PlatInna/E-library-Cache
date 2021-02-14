// Solution.cpp : https://www.coursera.org/ C++ Development Fundamentals: Brown Belt, Week 4.
// Task: Implement a cache on the backend  of electronic library. The requests come in multiple threads.
//

#include "Common.h"
#include <list>
#include<unordered_map>
#include<mutex>

using namespace std;

class LruCache : public ICache {
private:
    int memory_in_use; // для контроля память кеша - чтобы общий объём считанных книг не превосходил указанного в параметре
                       // max_memory
    shared_ptr<IBooksUnpacker> books_unpacker_; // указатель на объект IBooksUnpacker
    const Settings& settings_; // настройки кэша, в нашей задаче настройки содержат всего один параметр max_memory

    mutable list<BookPtr> book_ptr_list_; // ранжирование будем обеспечивать добавляя указатели на книги в двунаправленный список. 
                                          // Максимальный ранг это начало (push_front) списка.
    mutable unordered_map<string, list<BookPtr>::iterator> books_in_cache_; // мапу пользуем для обеспечения связи итераторов списка с названиями книг
    
    mutex m; // для защиты критической секции
             // Метод GetBook() может вызываться одновременно из нескольких потоков, 
             // поэтому необходимо обеспечить ему безопасность работы в таких условиях.
public:
  LruCache( // Кэширование производится методом вытеснения давно неиспользуемых элементов (Least Recently Used, LRU). 
      shared_ptr<IBooksUnpacker> books_unpacker,
      const Settings& settings): 
          memory_in_use(0),
          books_unpacker_(books_unpacker),
          settings_(settings)
  { // реализуйте метод
  }

  BookPtr GetBook(const string& book_name) override {
    // реализуйте метод
      lock_guard<mutex> lock(m); // ставим замок на критическую секцию
      auto book_iter = books_in_cache_.find(book_name);
      if (book_iter == books_in_cache_.end()) {
          // расспаковываем книгу, расспаковщик создает объект IBook в куче, т.е. копирует текст в память. Мы фиксируем shared_ptr на этот текст.
          shared_ptr book_shrd_ptr = move(books_unpacker_->UnpackBook(book_name));
          // Если размер запрошенной книги превышает max_memory, 
          // то после вызова метода кэш остаётся пустым, то есть книга в него не добавляется.
          if (book_shrd_ptr->GetContent().length() > settings_.max_memory) {
              book_ptr_list_.clear();
              books_in_cache_.clear();
              memory_in_use = 0;
          } else{
              book_ptr_list_.push_front(book_shrd_ptr);
              books_in_cache_.insert({ book_name, book_ptr_list_.begin() });
              memory_in_use += book_shrd_ptr->GetContent().length();
              // иначе если общий размер книг превышает ограничение max_memory, из кэша удаляются книги с наименьшим рангом, пока это необходимо.
              // Возможно, пока он полностью не будет опустошён
              while (memory_in_use > settings_.max_memory) {
                  memory_in_use -= book_ptr_list_.back()->GetContent().length();
                  books_in_cache_.erase(book_ptr_list_.back()->GetName());
                  book_ptr_list_.pop_back();
              }
          }
          return book_shrd_ptr;
      }
      else {
          // если книга с таким названием уже есть в кэше, 
          // её ранг поднимается до максимального - 
          // опять вставляем указатель на книгу в начало списка (т.е. поднимаем ранг до максимума)
          // и обновляем данные в unordered_map
          auto temp_ptr = *book_iter->second; 
          auto temp_book_name = book_iter->first;
          book_ptr_list_.erase(book_iter->second);
          books_in_cache_.erase(book_iter->first);
          book_ptr_list_.push_front(temp_ptr);
          books_in_cache_.insert({ temp_book_name, book_ptr_list_.begin() });
          return *book_ptr_list_.begin();
      }
  }
};


unique_ptr<ICache> MakeCache(
    shared_ptr<IBooksUnpacker> books_unpacker,
    const ICache::Settings& settings) {
  // реализуйте функцию
    return make_unique<LruCache>(books_unpacker, settings);
}
