#pragma once

#include "expense.h"
#include <string>
#include <vector>

// ── StorageError ──────────────────────────────────────────────────────────────

struct StorageError {
    bool        failed = false;
    std::string message;
};

// ── Storage ───────────────────────────────────────────────────────────────────

class Storage {
public:
    explicit Storage(const std::string& csvPath);

    // Load all records from CSV into memory. Returns error on failure.
    StorageError load(std::vector<Expense>& out);

    // Overwrite the CSV with the current in-memory list.
    StorageError save(const std::vector<Expense>& expenses);

    // Append a single new expense (faster than full rewrite when just adding).
    StorageError append(const Expense& e);

    const std::string& path() const { return m_path; }

private:
    std::string m_path;

    // Parse one CSV line → Expense. Returns false on malformed line.
    bool parseLine(const std::string& line, Expense& out);

    // Serialize one Expense → CSV line.
    std::string serialise(const Expense& e);
};