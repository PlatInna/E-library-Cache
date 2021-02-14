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
    int memory_in_use;
    shared_ptr<IBooksUnpacker> books_unpacker_;
    const Settings& settings_;

    mutable list<BookPtr> book_ptr_list_;
    mutable unordered_map<string, list<BookPtr>::iterator> books_in_cache_;
    
    mutex m;
public:
  LruCache(
      shared_ptr<IBooksUnpacker> books_unpacker,
      const Settings& settings): 
          memory_in_use(0),
          books_unpacker_(books_unpacker),
          settings_(settings)
  { // реализуйте метод
  }

  BookPtr GetBook(const string& book_name) override {
    // реализуйте метод
      lock_guard<mutex> lock(m);
      auto book_iter = books_in_cache_.find(book_name);
      if (book_iter == books_in_cache_.end()) {
          shared_ptr book_shrd_ptr = move(books_unpacker_->UnpackBook(book_name));
          if (book_shrd_ptr->GetContent().length() > settings_.max_memory) {
              book_ptr_list_.clear();
              books_in_cache_.clear();
              memory_in_use = 0;
          } else{
              book_ptr_list_.push_front(book_shrd_ptr);
              books_in_cache_.insert({ book_name, book_ptr_list_.begin() });
              memory_in_use += book_shrd_ptr->GetContent().length();
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
          // и обнавляем данные в unordered_map
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
