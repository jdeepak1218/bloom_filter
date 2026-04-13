package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"
	"github.com/go-chi/cors"
)

type CheckResponse struct {
	URL       string  `json:"url"`
	Safe      bool    `json:"safe"`
	Message   string  `json:"message"`
	FPR       float64 `json:"fpr"`
	CheckTime int64   `json:"check_time"`
}
type StatsResponse struct {
	TotalURLs      int     `json:"total_urls"`
	FilterSizeKb   uint64  `json:"filter_size_kb"`
	FPR            float64 `json:"fpr"`
	FilterCapacity int     `json:"filter_capacity"`
	HashFunction   uint64  `json:"hash_function"`
}

func normalizeURL(url string) string {
	url = strings.ToLower(url)
	url = strings.TrimPrefix(url, "https://")
	url = strings.TrimPrefix(url, "http://")
	return url
}
func loadURLs(filter *BloomFilter, filename string) (int, error) {
	file, err := os.Open(filename)
	if err != nil {
		return 0, err
	}
	defer file.Close()
	scanner := bufio.NewScanner(file)
	counter := 0
	for scanner.Scan() {
		line := scanner.Text()
		if line == "" {
			continue
		}
		if line[0] == '#' {
			continue
		}
		url := normalizeURL(line)
		filter.Add(url)
		counter++
	}
	return counter, nil
}
func checkHandler(filter *BloomFilter) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		url := r.URL.Query().Get("url")
		if url == "" {
			http.Error(w, "url paramter requied", http.StatusBadRequest)
			return
		}
		url = normalizeURL(url)
		start := time.Now()
		safe := !filter.Contains(url)
		checkTime := time.Since(start).Nanoseconds()
		message := "Not found in malicious database"
		if !safe {
			message = "Found in malicious database"
		}
		response := CheckResponse{
			URL:       url,
			Safe:      safe,
			Message:   message,
			FPR:       filter.CurrentFPR(),
			CheckTime: checkTime,
		}
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	}
}
func statsHandler(filter *BloomFilter) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		response := StatsResponse{
			TotalURLs:      int(filter.count),
			FilterSizeKb:   filter.m / 8 / 1024,
			FPR:            filter.CurrentFPR(),
			FilterCapacity: filter.n,
			HashFunction:   filter.k,
		}
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	}
}
func main() {
	var filter *BloomFilter
	_, err := os.Stat("data/malicious_filter.bin")
	if err == nil {
		filter, err = Deserialize("data/malicious_filter.bin")
		if err != nil {
			log.Fatal(err)
		}
		fmt.Println("Bloom filter loaded from bin file")
	} else {
		filter = NewBloomFilter(70000, 0.01)
		start := time.Now()
		count, err := loadURLs(filter, "data/malicious_urls.txt")
		if err != nil {
			log.Fatal(err)
		}
		elapsed := time.Since(start)
		fmt.Printf("Loaded %d URLs in %s\n", count, elapsed)
		filter.Serialize("data/malicious_filter.bin")
		fmt.Println("Bloom filter cached")
	}
	r := chi.NewRouter()
	r.Use(middleware.Logger)
	r.Use(middleware.Recoverer)
	r.Use(cors.Handler(cors.Options{
		AllowedOrigins: []string{"*"},
		AllowedMethods: []string{"GET", "OPTIONS"},
		AllowedHeaders: []string{"Content-Type"},
	}))
	r.Get("/check", checkHandler(filter))
	r.Get("/stats", statsHandler(filter))
	fmt.Println("Server started on : 8080")
	http.ListenAndServe(":8080", r)
}
