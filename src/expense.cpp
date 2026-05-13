#include "expense.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

// ── Category helpers ──────────────────────────────────────────────────────────

std::string categoryToString(Category c) {
    switch (c) {
        case Category::Food:          return "Food";
        case Category::Transport:     return "Transport";
        case Category::Entertainment: return "Entertainment";
        case Category::Utilities:     return "Utilities";
        case Category::Other:
        default:                      return "Other";
    }
}

Category categoryFromString(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "food")          return Category::Food;
    if (lower == "transport")     return Category::Transport;
    if (lower == "entertainment") return Category::Entertainment;
    if (lower == "utilities")     return Category::Utilities;
    return Category::Other;
}

// ── Display helpers ───────────────────────────────────────────────────────────

void printCategories() {
    std::cout << "  1. Food\n"
              << "  2. Transport\n"
              << "  3. Entertainment\n"
              << "  4. Utilities\n"
              << "  5. Other\n";
}

void printExpense(const Expense& e) {
    std::cout << std::left
              << std::setw(5)  << e.id
              << std::setw(13) << e.date
              << std::setw(28) << e.description.substr(0, 27)
              << std::right
              << std::setw(8)  << std::fixed << std::setprecision(2) << e.amount
              << "  "
              << std::left
              << e.categoryStr()
              << "\n";
}

void printExpenseTable(const std::vector<Expense>& expenses) {
    if (expenses.empty()) {
        std::cout << "  (no expenses found)\n";
        return;
    }

    std::cout << "\n";
    std::cout << std::left
              << std::setw(5)  << "ID"
              << std::setw(13) << "DATE"
              << std::setw(28) << "DESCRIPTION"
              << std::right
              << std::setw(8)  << "AMOUNT"
              << "  "
              << std::left     << "CATEGORY"
              << "\n";
    std::cout << std::string(65, '-') << "\n";

    for (const auto& e : expenses)
        printExpense(e);

    double total = 0.0;
    for (const auto& e : expenses) total += e.amount;

    std::cout << std::string(65, '-') << "\n";
    std::cout << std::right << std::setw(46) << "TOTAL"
              << std::setw(8) << std::fixed << std::setprecision(2) << total
              << "\n\n";
}

// ── Interactive prompt ────────────────────────────────────────────────────────

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

Expense promptNewExpense(int nextId) {
    Expense e;
    e.id = nextId;

    // ── Description ──────────────────────────────────────────────────────────
    while (true) {
        std::cout << "  Description: ";
        std::getline(std::cin, e.description);
        e.description = trim(e.description);
        if (!e.description.empty()) break;
        std::cout << "  [!] Description cannot be empty.\n";
    }

    // ── Amount ────────────────────────────────────────────────────────────────
    while (true) {
        std::cout << "  Amount (₹): ";
        std::string amtStr;
        std::getline(std::cin, amtStr);
        try {
            e.amount = std::stod(trim(amtStr));
            if (e.amount > 0) break;
            std::cout << "  [!] Amount must be positive.\n";
        } catch (...) {
            std::cout << "  [!] Invalid number. Try again.\n";
        }
    }

    // ── Category ──────────────────────────────────────────────────────────────
    printCategories();
    while (true) {
        std::cout << "  Choose category (1-5 or name): ";
        std::string catIn;
        std::getline(std::cin, catIn);
        catIn = trim(catIn);
        if (catIn == "1") { e.category = Category::Food;          break; }
        if (catIn == "2") { e.category = Category::Transport;     break; }
        if (catIn == "3") { e.category = Category::Entertainment; break; }
        if (catIn == "4") { e.category = Category::Utilities;     break; }
        if (catIn == "5") { e.category = Category::Other;         break; }
        // Try name match
        Category parsed = categoryFromString(catIn);
        if (!catIn.empty()) { e.category = parsed; break; }
        std::cout << "  [!] Enter 1-5 or a category name.\n";
    }

    // ── Date (default = today) ────────────────────────────────────────────────
    std::cout << "  Date (YYYY-MM-DD, or Enter for today): ";
    std::string dateIn;
    std::getline(std::cin, dateIn);
    dateIn = trim(dateIn);
    if (dateIn.empty()) {
        // Get today's date
        time_t now = time(nullptr);
        tm*    lt  = localtime(&now);
        char   buf[11];
        strftime(buf, sizeof(buf), "%Y-%m-%d", lt);
        e.date = buf;
    } else {
        e.date = dateIn;
    }

    return e;
}