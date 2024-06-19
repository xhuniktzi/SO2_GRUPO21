package main

import (
	"database/sql"
	"encoding/json"
	"fmt"
	"log"
	"math"
	"net/http"

	_ "github.com/go-sql-driver/mysql"
	"github.com/gorilla/handlers"
	"github.com/gorilla/mux"
)

// MySQL connection parameters
var (
	dbHost     = "localhost"
	dbPort     = "3306"
	dbUser     = "vmuser"
	dbPassword = "1234"
	dbName     = "memory_monitor" // MySQL database name
)

// MemoryLog struct for storing fetched data
type MemoryLog struct {
	PID           int     `json:"pid"`
	ProcessName   string  `json:"process_name"`
	CallType      string  `json:"call_type"`
	MemorySize    int64   `json:"memory_size"`
	MemorySizeMB  float64 `json:"memory_size_mb"`
	MemorySizePer float64 `json:"memory_size_per"`
	Timestamp     string  `json:"timestamp"`
}

func main() {
	router := mux.NewRouter()
	cors := handlers.CORS(handlers.AllowedHeaders([]string{"Content-Type"}), handlers.AllowedMethods([]string{"GET", "POST", "PUT", "DELETE"}), handlers.AllowedOrigins([]string{"http://localhost:3000"}))
	router.HandleFunc("/api/memorylogs", getMemoryLogs).Methods("GET")

	// Start HTTP server
	port := "8080" // Choose your desired port
	fmt.Printf("Server running on port %s...\n", port)
	log.Fatal(http.ListenAndServe(":8080", cors(router)))
}

// MySQL connection function
func dbConn() (db *sql.DB) {
	db, err := sql.Open("mysql", fmt.Sprintf("%s:%s@tcp(%s:%s)/%s", dbUser, dbPassword, dbHost, dbPort, dbName))
	if err != nil {
		log.Fatal(err)
	}
	return db
}

// Handler function to fetch memory logs from MySQL
func getMemoryLogs(w http.ResponseWriter, r *http.Request) {
	db := dbConn()
	defer db.Close()

	// Query to fetch memory logs
	rows, err := db.Query("SELECT pid, process_name, call_type, memory_size, timestamp FROM memory_log ORDER BY timestamp DESC LIMIT 100")
	if err != nil {
		log.Fatal(err)
	}
	defer rows.Close()

	var memoryLogs []MemoryLog

	// Iterate through query results and populate MemoryLog struct
	for rows.Next() {
		var memLog MemoryLog
		err := rows.Scan(&memLog.PID, &memLog.ProcessName, &memLog.CallType, &memLog.MemorySize, &memLog.Timestamp)
		if err != nil {
			log.Fatal(err)
		}

		// Calculate MemorySizeMB
		memLog.MemorySizeMB = float64(memLog.MemorySize) / (1024.0 * 1024.0)
		memLog.MemorySizeMB = math.Round(memLog.MemorySizeMB*100) / 100

		memLog.MemorySizePer = (float64(memLog.MemorySize) / 8289133824) * 100
		memLog.MemorySizePer = math.Round(memLog.MemorySizePer*100) / 100

		memoryLogs = append(memoryLogs, memLog)
	}

	// Convert memoryLogs slice to JSON
	jsonData, err := json.Marshal(memoryLogs)
	if err != nil {
		log.Fatal(err)
	}

	// Set JSON content type and write response
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	w.Write(jsonData)
}
