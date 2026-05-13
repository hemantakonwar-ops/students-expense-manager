#pragma once

#include "expense.h"
#include <map>
#include <string>
#include <vector>

// ── BudgetConfig ──────────────────────────────────────────────────────────────

struct BudgetConfig {
    std::map<Category, double> limits;

    // Defaults (monthly, in ₹)
    BudgetConfig() {
        limits[Category::Food]          = 500.0;
        limits[Category::Transport]     = 500.0;
        limits[Category::Entertainment] = 500.0;
        limits[Category::Utilities]     = 500.0;
        limits[Category::Other]         = 500.0;
    }

    double limitFor(Category c) const {
        auto it = limits.find(c);
        return (it != limits.end()) ? it->second : 200.0;
    }
};

// ── CategorySummary ───────────────────────────────────────────────────────────

struct CategorySummary {
    Category    category;
    double      total;
    double      budget;
    bool        overBudget;
    int         count;       // number of transactions
};

// ── Analytics ─────────────────────────────────────────────────────────────────

class Analytics {
public:
    explicit Analytics(const BudgetConfig& budget = BudgetConfig());

    // Category-wise totals for the given list
    std::vector<CategorySummary> summarise(const std::vector<Expense>& expenses) const;

    // Grand total
    double total(const std::vector<Expense>& expenses) const;

    // Filter by "YYYY-MM" month string (e.g. "2026-05")
    std::vector<Expense> filterByMonth(const std::vector<Expense>& expenses,
                                       const std::string& month) const;

    // Filter by date range [from, to] inclusive ("YYYY-MM-DD")
    std::vector<Expense> filterByRange(const std::vector<Expense>& expenses,
                                       const std::string& from,
                                       const std::string& to) const;

    // Print full analytics table + overspend alerts to stdout
    void printSummary(const std::vector<Expense>& expenses) const;

    // Export a plain-text report to the given file path
    bool exportReport(const std::vector<Expense>& expenses,
                      const std::string& filePath,
                      const std::string& aiInsight = "") const;

    // Build a compact text summary suitable for injection into an LLM prompt
    std::string buildPromptSummary(const std::vector<Expense>& expenses) const;

private:
    BudgetConfig m_budget;

    void printBar(double spent, double limit) const;
};