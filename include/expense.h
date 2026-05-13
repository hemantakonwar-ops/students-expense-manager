#pragma once

#include <string>
#include <vector>

// ── Category ──────────────────────────────────────────────────────────────────

enum class Category {
    Food,
    Transport,
    Entertainment,
    Utilities,
    Other
};

std::string categoryToString(Category c);
Category    categoryFromString(const std::string& s);

// ── Expense ───────────────────────────────────────────────────────────────────

struct Expense {
    int         id;
    std::string date;        // "YYYY-MM-DD"
    std::string description;
    double      amount;
    Category    category;

    // Convenience: return category as display string
    std::string categoryStr() const { return categoryToString(category); }
};

// ── Free helpers ──────────────────────────────────────────────────────────────

// Print a single expense row to stdout (table format)
void printExpense(const Expense& e);

// Print a full table of expenses
void printExpenseTable(const std::vector<Expense>& expenses);

// Interactive: prompt the user and return a filled Expense (id auto-assigned)
Expense promptNewExpense(int nextId);

// Print all available categories
void printCategories();