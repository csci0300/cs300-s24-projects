#include <fstream>

#include "test_utils/test_utils.hpp"

using namespace std;

// warning: N_THREADS must be < 10 because of hash function
static constexpr size_t N_THREADS = 8;
static constexpr size_t N_KEYS_PER_THREAD = 10'000;

int main(int argc, char* argv[]) {
  std::ofstream output_file("performance-runtime.csv", std::ios::app);
  if (!output_file.is_open()) {
    std::cerr << "Failed to open output file." << std::endl;
  }
  /*
    This test evaluates the performance of your ConcurrentKVStore by asserting
    that the time it takes to service Get and Put requests *decreases* as the
    number of threads *increases*. We spawn a single thread that executes
    N_THREADS Put/Get requests sequentially, then spawn N_THREAD threads, each
    of which executes one request. If you've properly implemented bucket-based
    locking, the multithreaded operations should finish faster than the
    single-threaded one. We assert that the multithreaded operations take less
    than half the amount of time as the single-threaded operations.
  */

  // Define a KVStore for the single-threaded approach to use, including a
  // special hash function to partition keys into buckets based on last letter
  auto single_threaded_store = make_unique<ConcurrentKvStore>(
      [](const string& key) { return stoi(key.substr(key.size() - 1)); });
  // Define a KVStore for the multi-threaded approach to use, including a
  // special hash function to partition keys into buckets based on the last
  // letter
  auto multi_threaded_store = make_unique<ConcurrentKvStore>(
      [](const string& key) { return stoi(key.substr(key.size() - 1)); });

  // Generate map of random keys.
  // By appending "i" to the end of the random key string, we ensure that all of
  // the keys generated by a call to make_pseudo_rand_str(..., i) end in the
  // same letter. single_threaded_store & multi_threaded_store's hash functions
  // distribute keys to buckets based on the key's last letter. So, all of the
  // keys in a given entry (bucket_to_keys[i]) will be in the same bucket. Also,
  // since i does not repeat, each entry (bucket_to_keys[i]) will touch a
  // different bucket from another entry (bucket_to_keys[j]).
  unordered_map<size_t, vector<string>> buckets_to_keys;
  for (size_t i = 0; i < N_THREADS; i++) {
    buckets_to_keys[i] = make_pseudo_rand_str(N_KEYS_PER_THREAD, 32, i);
  }

  // Define helper that the single thread uses to sequentially Put all of the
  // keys into the single-threaded-store.
  auto insert_all = [&]() {
    for (size_t i = 0; i < N_THREADS; i++) {
      vector<string> keys = buckets_to_keys[i];
      string index = to_string(i);
      vector<string> values(keys.size(), index);
      ASSERT(put_range(*single_threaded_store, keys, values, 0,
                       N_KEYS_PER_THREAD));
    }
  };

  // Time single threaded put
  auto start = chrono::high_resolution_clock::now();
  {
    thread single_thread(insert_all);
    single_thread.join();
  }
  auto end = chrono::high_resolution_clock::now();
  auto single_threaded_time =
      chrono::duration_cast<chrono::milliseconds>(end - start);

  output_file << "single_thread_put," << single_threaded_time.count() << ","
              << to_throughput(single_threaded_time, N_THREADS,
                               N_KEYS_PER_THREAD)
              << "\n";

  // Define helper that threads use to Put the keys into the multi-threaded
  // store. Note that since each entry in buckets_to_keys touches a single
  // bucket (that's unique from the other buckets in buckets_to_keys), we are
  // guaranteed that if you have implemented bucket-based locking correctly,
  // each Put request can execute *at the same time* on N_THREADS buckets.
  auto insert_bucket = [&](size_t i) {
    vector<string> keys = buckets_to_keys[i];
    string index = to_string(i);
    // values are a vector of the index (as a string) repeated keys.size() # of
    // times (so it's the same length as keys)
    vector<string> values(keys.size(), index);
    ASSERT(
        put_range(*multi_threaded_store, keys, values, 0, N_KEYS_PER_THREAD));
  };

  // Time multithreaded put
  start = chrono::high_resolution_clock::now();
  {
    vector<thread> threads;
    for (size_t i = 0; i < N_THREADS; i++) {
      threads.emplace_back(insert_bucket, i);
    }
    for (auto& t : threads) {
      t.join();
    }
  }
  end = chrono::high_resolution_clock::now();
  auto multi_threaded_time =
      chrono::duration_cast<chrono::milliseconds>(end - start);

  output_file << "multi_thread_put," << multi_threaded_time.count() << ","
              << to_throughput(multi_threaded_time, N_THREADS,
                               N_KEYS_PER_THREAD)
              << "\n";

  // Define helper that the single thread uses to sequentially Get all of the
  // keys in the single-threaded-store.
  auto query_all = [&]() {
    for (size_t i = 0; i < N_THREADS; i++) {
      vector<string> keys = buckets_to_keys[i];
      vector<string> values(keys.size(), to_string(i));

      ASSERT(get_range(*single_threaded_store, keys, values, 0,
                       N_KEYS_PER_THREAD));
    }
  };

  // Time single threaded multiget
  start = chrono::high_resolution_clock::now();
  {
    thread single_thread(query_all);
    single_thread.join();
  }
  end = chrono::high_resolution_clock::now();
  single_threaded_time =
      chrono::duration_cast<chrono::milliseconds>(end - start);

  output_file << "single_thread_get," << single_threaded_time.count() << ","
              << to_throughput(single_threaded_time, N_THREADS,
                               N_KEYS_PER_THREAD)
              << "\n";

  // Define helper that threads use to Get the keys in the multi-threaded store.
  // Note that since each entry in buckets_to_keys touches a single bucket
  // (that's unique from the other buckets in buckets_to_keys), we are
  // guaranteed that if you have implemented bucket-based locking correctly,
  // each MultiGet request can execute *at the same time* on N_THREADS buckets.
  auto query_bucket = [&](size_t i) {
    vector<string> keys = buckets_to_keys[i];
    vector<string> values(keys.size(), to_string(i));

    ASSERT(
        get_range(*multi_threaded_store, keys, values, 0, N_KEYS_PER_THREAD));
  };

  // Time multithreaded get
  start = chrono::high_resolution_clock::now();
  {
    vector<thread> threads;
    for (size_t i = 0; i < N_THREADS; i++) {
      threads.emplace_back(query_bucket, i);
    }
    for (auto& t : threads) {
      t.join();
    }
  }
  end = chrono::high_resolution_clock::now();
  multi_threaded_time =
      chrono::duration_cast<chrono::milliseconds>(end - start);

  output_file << "multi_thread_get," << multi_threaded_time.count() << ","
              << to_throughput(multi_threaded_time, N_THREADS,
                               N_KEYS_PER_THREAD)
              << "\n";
}
