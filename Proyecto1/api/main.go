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

var (
	dbHost     = "localhost"
	dbPort     = "3306"
	dbUser     = "vmuser"
	dbPassword = "1234"
	dbName     = "memory_monitor"
)

type MemoryLog struct {
	PID           int     `json:"pid"`
	ProcessName   string  `json:"process_name"`
	CallType      string  `json:"call_type"`
	MemorySize    int64   `json:"memory_size"`
	MemorySizeMB  float64 `json:"memory_size_mb"`
	MemorySizePer float64 `json:"memory_size_per"`
	Timestamp     string  `json:"timestamp"`
}

type AggregatedMemoryLog struct {
	PID            int     `json:"pid"`
	ProcessName    string  `json:"process_name"`
	TotalMemoryMB  float64 `json:"total_memory_mb"`
	TotalMemoryPer float64 `json:"total_memory_per"`
}

type CombinedMemoryLogs struct {
	IndividualLogs []MemoryLog           `json:"individual_logs"`
	AggregatedLogs []AggregatedMemoryLog `json:"aggregated_logs"`
}

func main() {
	router := mux.NewRouter()
	cors := handlers.CORS(handlers.AllowedHeaders([]string{"Content-Type"}), handlers.AllowedMethods([]string{"GET", "POST", "PUT", "DELETE"}), handlers.AllowedOrigins([]string{"http://localhost:3000"}))
	router.HandleFunc("/api/memorylogs", getMemoryLogs).Methods("GET")

	port := "8080"
	fmt.Printf("Server running on port %s...\n", port)
	log.Fatal(http.ListenAndServe(":8080", cors(router)))
}

func dbConn() (db *sql.DB) {
	db, err := sql.Open("mysql", fmt.Sprintf("%s:%s@tcp(%s:%s)/%s", dbUser, dbPassword, dbHost, dbPort, dbName))
	if err != nil {
		log.Fatal(err)
	}
	return db
}

func getMemoryLogs(w http.ResponseWriter, r *http.Request) {
	db := dbConn()
	defer db.Close()

	rows, err := db.Query("SELECT pid, process_name, call_type, memory_size, timestamp FROM memory_log ORDER BY timestamp DESC")
	if err != nil {
		log.Fatal(err)
	}
	defer rows.Close()

	var memoryLogs []MemoryLog

	for rows.Next() {
		var memLog MemoryLog
		err := rows.Scan(&memLog.PID, &memLog.ProcessName, &memLog.CallType, &memLog.MemorySize, &memLog.Timestamp)
		if err != nil {
			log.Fatal(err)
		}

		memLog.MemorySizeMB = float64(memLog.MemorySize) / (1024.0 * 1024.0)
		memLog.MemorySizeMB = math.Round(memLog.MemorySizeMB*100) / 100

		memLog.MemorySizePer = (float64(memLog.MemorySize) / 8289133824) * 100
		memLog.MemorySizePer = math.Round(memLog.MemorySizePer*100) / 100

		memoryLogs = append(memoryLogs, memLog)
	}

	aggregatedLogs := aggregateMemoryLogs(memoryLogs)

	combinedLogs := CombinedMemoryLogs{
		IndividualLogs: memoryLogs,
		AggregatedLogs: aggregatedLogs,
	}

	jsonData, err := json.Marshal(combinedLogs)
	if err != nil {
		log.Fatal(err)
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	w.Write(jsonData)
}

func aggregateMemoryLogs(logs []MemoryLog) []AggregatedMemoryLog {
	memoryMap := make(map[int]*AggregatedMemoryLog)

	for _, log := range logs {
		if _, exists := memoryMap[log.PID]; !exists {
			memoryMap[log.PID] = &AggregatedMemoryLog{
				PID:            log.PID,
				ProcessName:    log.ProcessName,
				TotalMemoryMB:  0,
				TotalMemoryPer: 0,
			}
		}

		if log.CallType == "mmap" {
			memoryMap[log.PID].TotalMemoryMB += float64(log.MemorySize) / (1024.0 * 1024.0)
		} else if log.CallType == "munmap" {
			memoryMap[log.PID].TotalMemoryMB -= float64(log.MemorySize) / (1024.0 * 1024.0)
		}

		memoryMap[log.PID].TotalMemoryMB = math.Round(memoryMap[log.PID].TotalMemoryMB*100) / 100
		memoryMap[log.PID].TotalMemoryPer = (memoryMap[log.PID].TotalMemoryMB * 1024 * 1024 / 8289133824) * 100
		memoryMap[log.PID].TotalMemoryPer = math.Round(memoryMap[log.PID].TotalMemoryPer*100) / 100
	}

	var aggregatedLogs []AggregatedMemoryLog
	for _, val := range memoryMap {
		aggregatedLogs = append(aggregatedLogs, *val)
	}

	return aggregatedLogs
}
