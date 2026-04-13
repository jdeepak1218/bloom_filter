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
	URL         string  `json:"url"`
	Safe        bool    `json:"safe"`
	Message     string  `json:"message"`
	FPR         float64 `json:"fpr"`
	CheckTimeNs int64   `json:"checkTimeNs"`
}
type StatsResponse struct {
	TotalURLs      int     `json:"totalUrls"`
	FilterSizeKB   uint64  `json:"filterSizeKB"`
	FPR            float64 `json:"fpr"`
	FilterCapacity uint64  `json:"filterCapacity"`
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
			http.Error(w, "url parameter required", http.StatusBadRequest)
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
			URL:         url,
			Safe:        safe,
			Message:     message,
			FPR:         filter.CurrentFPR(),
			CheckTimeNs: checkTime,
		}
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	}
}
func statsHandler(filter *BloomFilter) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		response := StatsResponse{
			TotalURLs:      int(filter.count),
			FilterSizeKB:   filter.m / 8 / 1024,
			FPR:            filter.CurrentFPR(),
			FilterCapacity: filter.n,
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
		count, err := loadURLs(filter, "data/malicious_url.txt")
		if err != nil {
			log.Fatal(err)
		}
		elapsed := time.Since(start)
		fmt.Printf("Loaded %d URLs in %s\n", count, elapsed)
		err = filter.Serialize("data/malicious_filter.bin")
		if err != nil {
			log.Fatal(err)
		}
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
	port := os.Getenv("PORT")
	if port == "" {
		port = "8081"
	}
	http.ListenAndServe(":"+port, nil)
}
