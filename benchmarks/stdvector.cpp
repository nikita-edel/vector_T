#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

typedef struct {
	char data[256];
} Blob256;

using Clock = std::chrono::steady_clock;

static inline double now_sec() {
	return std::chrono::duration<double>(Clock::now().time_since_epoch())
	    .count();
}

#define FENCE() __asm__ volatile("" ::: "memory")

struct BenchResult {
	const char* label;
	const char* type_name;
	uint64_t total_ops;
	double total_sec;

	double ns_per_op() const {
		return total_ops ? (total_sec * 1e9) / total_ops : 0.0;
	}
};

static std::vector<BenchResult> g_results;

template <typename Fn>
static BenchResult run_bench(const char* label, const char* type_name,
			     double max_sec, Fn fn) {
	uint64_t batch = 8, total_ops = 0;
	double elapsed = 0.0, t_start = now_sec();

	while (elapsed < max_sec) {
		total_ops += fn(batch);
		elapsed = now_sec() - t_start;

		double ns_per_op =
		    total_ops > 0 ? (elapsed * 1e9 / total_ops) : 1.0;
		uint64_t target =
		    (uint64_t)(5e6 / (ns_per_op > 0 ? ns_per_op : 1));
		batch =
		    std::max<uint64_t>(8, std::min<uint64_t>(target, 1u << 20));
	}

	return BenchResult{label, type_name, total_ops, elapsed};
}

template <typename T>
struct StdWrapper {
	// tried my best to make it aquivalent to my vector
	std::vector<T> v;

	void init() { v.reserve(4); }
	// Note: this is aquivalent to cstl. it sets data, count, and size to 0
	// swap calls the destructor and replaces it with a new stack allocated
	// vector that doesnt do an allocation
	void deinit() { std::vector<T>(0).swap(v); }

	void push_back_one(const T& val) { v.push_back(val); }
	void push_back_n(const T* arr, size_t n) {
		v.insert(v.end(), arr, arr + n);
	}

	void insert_at(const T& val, size_t pos) {
		v.insert(v.begin() + (ptrdiff_t)pos, val);
	}
	void insert_n_at(const T* arr, size_t n, size_t pos) {
		v.insert(v.begin() + (ptrdiff_t)pos, arr, arr + n);
	}

	void replace_at(const T& val, size_t pos) { v[pos] = val; }

	size_t size() const { return v.size(); }
	void clear() { v.clear(); }
};

template <typename W, typename T>
static void prefill(W& w, const T& val, size_t n) {
	for (size_t i = 0; i < n; ++i) w.push_back_one(val);
}

template <typename Std, typename T>
void run_suite(const char* type_name, double max_sec, const T& sample_val) {
	printf("[%s]\n", type_name);
	constexpr size_t BATCH = 64;
	constexpr size_t INSERT_CAP = 1024;
	constexpr size_t INSERT_FILL = INSERT_CAP / 2;

	T batch_arr[BATCH];
	for (size_t i = 0; i < BATCH; ++i) batch_arr[i] = sample_val;

	auto add = [&](BenchResult r) { g_results.push_back(r); };

	{
		Std s;
		s.init();
		add(run_bench("push_back_1", type_name, max_sec,
			      [&](uint64_t n) -> uint64_t {
				      s.clear();
				      for (uint64_t i = 0; i < n; ++i)
					      s.push_back_one(sample_val);
				      FENCE();
				      return n;
			      }));
		s.deinit();
	}

	{
		Std s;
		s.init();
		add(run_bench("push_back_64", type_name, max_sec,
			      [&](uint64_t n) -> uint64_t {
				      s.clear();
				      for (uint64_t i = 0; i < n; ++i)
					      s.push_back_n(batch_arr, BATCH);
				      FENCE();
				      return n * BATCH;
			      }));
		s.deinit();
	}

	{
		Std s;
		s.init();
		prefill(s, sample_val, INSERT_FILL);
		add(run_bench("insert_front", type_name, max_sec,
			      [&](uint64_t n) -> uint64_t {
				      for (uint64_t i = 0; i < n; ++i) {
					      if (s.size() >= INSERT_CAP) {
						      s.deinit();
						      s.init();
						      prefill(s, sample_val,
							      INSERT_FILL);
					      }
					      s.insert_at(sample_val, 0);
				      }
				      FENCE();
				      return n;
			      }));
		s.deinit();
	}

	{
		Std s;
		s.init();
		prefill(s, sample_val, INSERT_FILL);
		add(run_bench(
		    "insert_64_front", type_name, max_sec,
		    [&](uint64_t n) -> uint64_t {
			    for (uint64_t i = 0; i < n; ++i) {
				    if (s.size() + BATCH > INSERT_CAP) {
					    s.deinit();
					    s.init();
					    prefill(s, sample_val, INSERT_FILL);
				    }
				    s.insert_n_at(batch_arr, BATCH, 0);
			    }
			    FENCE();
			    return n * BATCH;
		    }));
		s.deinit();
	}

	{
		Std s;
		s.init();
		prefill(s, sample_val, INSERT_CAP);
		add(run_bench(
		    "replace", type_name, max_sec, [&](uint64_t n) -> uint64_t {
			    uint64_t pos = 0;
			    for (uint64_t i = 0; i < n; ++i) {
				    s.replace_at(sample_val, pos);
				    pos = (pos + 1) & (INSERT_CAP - 1);
			    }
			    FENCE();
			    return n;
		    }));
		s.deinit();
	}
}

static void print_results() {
	printf("\n%-20s %-10s  %12s  %14s\n", "operation", "type", "ns/op",
	       "total_ops");
	printf(
	    "----------------------------------------------------------------"
	    "\n");

	const char* last_type = nullptr;

	for (auto& r : g_results) {
		if (last_type && strcmp(last_type, r.type_name) != 0)
			printf(
			    "--------------------------------------------------"
			    "--------------\n");

		printf("%-20s %-10s  %12.2f  %14llu\n", r.label, r.type_name,
		       r.ns_per_op(), (unsigned long long)r.total_ops);

		last_type = r.type_name;
	}

	printf("\n");
}

int main(int argc, char** argv) {
	double max_sec = 2.0;
	if (argc >= 2) max_sec = atof(argv[1]);
	if (max_sec <= 0) max_sec = 2.0;

	printf("std::vector benchmark\n");
	printf("max %.1fs per test | compiler: " __VERSION__ "\n", max_sec);

	Blob256 blob;
	memset(&blob, 0xAB, sizeof blob);

	run_suite<StdWrapper<char>, char>("char", max_sec, 'x');
	run_suite<StdWrapper<int>, int>("int", max_sec, 420);
	run_suite<StdWrapper<uint64_t>, uint64_t>("uint64_t", max_sec,
						  0xDEADDEADLL);
	run_suite<StdWrapper<Blob256>, Blob256>("Blob256", max_sec, blob);

	print_results();
	return 0;
}
