#include <iomanip>
#include "../include/Storage.h"

#include <fstream>
#include <iostream>
#include <sstream>

// ── Constructor ───────────────────────────────────────────────────────────────

Storage::Storage(const std::string& csvPath)
    : m_path(csvPath) {}

// ── Load ──────────────────────────────────────────────────────────────────────

StorageError Storage::load(std::vector<Expense>& out) {
    out.clear();

    std::ifstream file(m_path);
    if (!file.is_open()) {
        // First run: file doesn't exist yet — not a hard error
        return StorageError{ false, "" };
    }

    std::string line;
    bool firstLine = true;
    int  lineNo    = 0;

    while (std::getline(file, line)) {
        ++lineNo;

        // Skip header row
        if (firstLine) { firstLine = false; continue; }

        // Skip blank lines
        if (line.empty()) continue;

        Expense e;
        if (parseLine(line, e)) {
            out.push_back(e);
        } else {
            std::cerr << "  [!] Skipping malformed CSV line " << lineNo << ": " << line << "\n";
        }
    }

    return StorageError{ false, "" };
}

// ── Save (full rewrite) ────────────────────────────────────────────────────────

StorageError Storage::save(const std::vector<Expense>& expenses) {
    std::ofstream file(m_path, std::ios::trunc);
    if (!file.is_open()) {
        return StorageError{ true, "Cannot open '" + m_path + "' for writing." };
    }

    // Header
    file << "id,date,description,amount,category\n";

    for (const auto& e : expenses) {
        file << serialise(e) << "\n";
    }

    return StorageError{ false, "" };
}

// ── Append (single record) ────────────────────────────────────────────────────

StorageError Storage::append(const Expense& e) {
    // If file is new, write header first
    std::ifstream check(m_path);
    bool needsHeader = !check.good() || check.peek() == std::ifstream::traits_type::eof();
    check.close();

    std::ofstream file(m_path, std::ios::app);
    if (!file.is_open()) {
        return StorageError{ true, "Cannot open '" + m_path + "' for appending." };
    }

    if (needsHeader) {
        file << "id,date,description,amount,category\n";
    }

    file << serialise(e) << "\n";
    return StorageError{ false, "" };
}

// ── Private: parseLine ────────────────────────────────────────────────────────
// Format: id,date,description,amount,category
// Description may contain commas if quoted — simple CSV only (no embedded newlines).

bool Storage::parseLine(const std::string& line, Expense& out) {
    // Tokenise respecting optional quoting on description field
    std::vector<std::string> fields;
    std::string field;
    bool inQuote = false;

    for (char ch : line) {
        if (ch == '"') {
            inQuote = !inQuote;
        } else if (ch == ',' && !inQuote) {
            fields.push_back(field);
            field.clear();
        } else {
            field += ch;
        }
    }
    fields.push_back(field); // last field

    if (fields.size() < 5) return false;

    try {
        out.id          = std::stoi(fields[0]);
        out.date        = fields[1];
        out.description = fields[2];
        out.amount      = std::stod(fields[3]);
        out.category    = categoryFromString(fields[4]);
    } catch (...) {
        return false;
    }

    return true;
}

// ── Private: serialise ────────────────────────────────────────────────────────

std::string Storage::serialise(const Expense& e) {
    std::ostringstream oss;
    // Quote description in case it contains commas
    oss << e.id << ","
        << e.date << ","
        << "\"" << e.description << "\","
        << std::fixed << std::setprecision(2) << e.amount << ","
        << categoryToString(e.category);
    return oss.str();
}